/*

   Copyright [2008] [Trevor Hogan]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   CODE PORTED FROM THE ORIGINAL GHOST PROJECT: http://ghost.pwner.org/

*/
#include "lia.h"
#include "ghost.h"
#include "util.h"
#include "config.h"
#include "language.h"
#include "socket.h"
#include "database/ghostdb.h"
#include "bnet.h"
#include "map.h"
#include "packed.h"
#include "savegame.h"
#include "gameplayer.h"
#include "gameprotocol.h"
#include "game_base.h"
#include "game.h"
#include "stats/stats.h"
#include "stats/statslia.h"
#include "stats/statsdota.h"
#include "stats/statsw3mmd.h"
#include "stats/statsdebug.h"

#include <cmath>
#include <string.h>
#include <time.h>

#include "discord.h"
//
// sorting classes
//
/*
#include "exprtk.hpp"
double calculate(const std::string expression_string ="3+4" ){
	typedef exprtk::expression<double>          expression_t;
	typedef exprtk::parser<double>                  parser_t;
	parser_t parser;
	expression_t expression;
	parser.compile(expression_string,expression);
	double result = 0;
	exprtk::compute(expression_string, result);

	return result;
}
*/
class CGamePlayerSortAscByPing
{
public:
	bool operator( ) ( CGamePlayer *Player1, CGamePlayer *Player2 ) const
	{
		return Player1->GetPing( false ) < Player2->GetPing( false );
	}
};

class CGamePlayerSortDescByPing
{
public:
	bool operator( ) ( CGamePlayer *Player1, CGamePlayer *Player2 ) const
	{
		return Player1->GetPing( false ) > Player2->GetPing( false );
	}
};

//
// CGame
//

CGame :: CGame( CGHost *nGHost, CMap *nMap, CSaveGame *nSaveGame, uint16_t nHostPort, unsigned char nGameState, std::string nGameName, std::string nOwnerName, std::string nCreatorName, std::string nCreatorServer ) : CBaseGame( nGHost, nMap, nSaveGame, nHostPort, nGameState, nGameName, nOwnerName, nCreatorName, nCreatorServer ), m_DBBanLast( NULL ), m_Stats( NULL ) ,m_CallableGameAdd( NULL )
{
	m_DBGame = new CDBGame( 0, std::string( ), m_Map->GetMapPath( ), std::string( ), std::string( ), std::string( ), 0 );

	if( m_Map->GetMapType( ) == "w3mmd" )
		m_Stats = new CStatsW3MMD( this, m_Map->GetMapStatsW3MMDCategory( ) );
	else if( m_Map->GetMapType( ) == "dota" )
		m_Stats = new CStatsDOTA( this );
	else if ( m_Map->GetMapType( ) == "debug" )
		m_Stats = new CStatsDEBUG(this);
#ifdef STATS_LIA_GHOST
	else if(m_Map->GetMapType() == "lia")
		m_Stats = new CStatsLiA(this);
#endif
}

CGame :: ~CGame( )
{
 	m_GHost->m_CallablesMutex.lock();
	if( m_CallableGameAdd && m_CallableGameAdd->GetReady( ) )
	{
		if( m_CallableGameAdd->GetResult( ) > 0 )
		{
			CONSOLE_Print( "[GAME: " + m_GameName + "] saving player/stats data to database" );

			// store the CDBGamePlayers in the database

			for( std::vector<CDBGamePlayer *> :: iterator i = m_DBGamePlayers.begin( ); i != m_DBGamePlayers.end( ); ++i )
				m_GHost->m_Callables.push_back( m_GHost->m_DB->ThreadedGamePlayerAdd( m_CallableGameAdd->GetResult( ), (*i)->GetName( ), (*i)->GetIP( ), (*i)->GetSpoofed( ), (*i)->GetSpoofedRealm( ), (*i)->GetReserved( ), (*i)->GetLoadingTime( ), (*i)->GetLeft( ), (*i)->GetLeftReason( ), (*i)->GetTeam( ), (*i)->GetColour( ) ) );

			// store the stats in the database

			if( m_Stats )
				m_Stats->Save( m_GHost, m_GHost->m_DB, m_CallableGameAdd->GetResult( ) );
		}
		else
			CONSOLE_Print( "[GAME: " + m_GameName + "] unable to save player/stats data to database" );

		m_GHost->m_DB->RecoverCallable( m_CallableGameAdd );
		delete m_CallableGameAdd;
		m_CallableGameAdd = NULL;
	}

	for( std::vector<PairedBanCheck> :: iterator i = m_PairedBanChecks.begin( ); i != m_PairedBanChecks.end( ); ++i )
		m_GHost->m_Callables.push_back( i->second );

	for( std::vector<PairedBanAdd> :: iterator i = m_PairedBanAdds.begin( ); i != m_PairedBanAdds.end( ); ++i )
		m_GHost->m_Callables.push_back( i->second );

	for( std::vector<PairedGPSCheck> :: iterator i = m_PairedGPSChecks.begin( ); i != m_PairedGPSChecks.end( ); ++i )
		m_GHost->m_Callables.push_back( i->second );

	for( std::vector<PairedDPSCheck> :: iterator i = m_PairedDPSChecks.begin( ); i != m_PairedDPSChecks.end( ); ++i )
		m_GHost->m_Callables.push_back( i->second );
#ifdef STATS_LIA_GHOST
	for( std::vector<PairedLPSCheck> :: iterator i = m_PairedLPSChecks.begin( ); i != m_PairedLPSChecks.end( ); ++i )
		m_GHost->m_Callables.push_back( i->second );
#endif
	m_GHost->m_CallablesMutex.unlock();

	for( std::vector<CDBBan *> :: iterator i = m_DBBans.begin( ); i != m_DBBans.end( ); ++i )
		delete *i;

	delete m_DBGame;

	for( std::vector<CDBGamePlayer *> :: iterator i = m_DBGamePlayers.begin( ); i != m_DBGamePlayers.end( ); ++i )
		delete *i;

	delete m_Stats;

	// it's a "bad thing" if m_CallableGameAdd is non NULL here
	// it means the game is being deleted after m_CallableGameAdd was created (the first step to saving the game data) but before the associated thread terminated
	// rather than failing horribly we choose to allow the thread to complete in the orphaned callables list but step 2 will never be completed
	// so this will create a game entry in the database without any gameplayers and/or DotA stats

	if( m_CallableGameAdd )
	{
		CONSOLE_Print( "[GAME: " + m_GameName + "] game is being deleted before all game data was saved, game data has been lost" );
		m_GHost->m_CallablesMutex.lock();
		m_GHost->m_Callables.push_back( m_CallableGameAdd );
		m_GHost->m_CallablesMutex.unlock();
	}
}

bool CGame :: Update( void *fd, void *send_fd )
{
	// update callables

	for( std::vector<PairedBanCheck> :: iterator i = m_PairedBanChecks.begin( ); i != m_PairedBanChecks.end( ); )
	{
		if( i->second->GetReady( ) )
		{
			CDBBan *Ban = i->second->GetResult( );

			if( Ban )
				SendAllChat( m_GHost->m_Language->UserWasBannedOnByBecause( i->second->GetServer( ), i->second->GetUser( ), Ban->GetDate( ), Ban->GetAdmin( ), Ban->GetReason( ) ) );
			else
				SendAllChat( m_GHost->m_Language->UserIsNotBanned( i->second->GetServer( ), i->second->GetUser( ) ) );

			m_GHost->m_DB->RecoverCallable( i->second );
			delete i->second;
			i = m_PairedBanChecks.erase( i );
		}
		else
			++i;
	}

	for( std::vector<PairedBanAdd> :: iterator i = m_PairedBanAdds.begin( ); i != m_PairedBanAdds.end( ); )
	{
		if( i->second->GetReady( ) )
		{
			if( i->second->GetResult( ) )
			{
				for( std::vector<CBNET *> :: iterator j = m_GHost->m_BNETs.begin( ); j != m_GHost->m_BNETs.end( ); ++j )
				{
					if( (*j)->GetServer( ) == i->second->GetServer( ) )
						(*j)->AddBan( i->second->GetUser( ), i->second->GetIP( ), i->second->GetGameName( ), i->second->GetAdmin( ), i->second->GetReason( ) );
				}

				SendAllChat( m_GHost->m_Language->PlayerWasBannedByPlayer( i->second->GetServer( ), i->second->GetUser( ), i->first ) );
			}

			m_GHost->m_DB->RecoverCallable( i->second );
			delete i->second;
			i = m_PairedBanAdds.erase( i );
		}
		else
			++i;
	}

	for( std::vector<PairedGPSCheck> :: iterator i = m_PairedGPSChecks.begin( ); i != m_PairedGPSChecks.end( ); )
	{
		if( i->second->GetReady( ) )
		{
			CDBGamePlayerSummary *GamePlayerSummary = i->second->GetResult( );

			if( GamePlayerSummary )
			{
				if( i->first.empty( ) )
					SendAllChat( m_GHost->m_Language->HasPlayedGamesWithThisBot( i->second->GetName( ), GamePlayerSummary->GetFirstGameDateTime( ), GamePlayerSummary->GetLastGameDateTime( ), std::to_string( GamePlayerSummary->GetTotalGames( ) ), std::to_string( (float)GamePlayerSummary->GetAvgLoadingTime( ) / 1000), std::to_string( GamePlayerSummary->GetAvgLeftPercent( ) ) ) );
				else
				{
					CGamePlayer *Player = GetPlayerFromName( i->first, true );

					if( Player )
						SendChat( Player, m_GHost->m_Language->HasPlayedGamesWithThisBot( i->second->GetName( ), GamePlayerSummary->GetFirstGameDateTime( ), GamePlayerSummary->GetLastGameDateTime( ), std::to_string( GamePlayerSummary->GetTotalGames( ) ), std::to_string( (float)GamePlayerSummary->GetAvgLoadingTime( ) / 1000), std::to_string( GamePlayerSummary->GetAvgLeftPercent( ) ) ) );
				}
			}
			else
			{
				if( i->first.empty( ) )
					SendAllChat( m_GHost->m_Language->HasntPlayedGamesWithThisBot( i->second->GetName( ) ) );
				else
				{
					CGamePlayer *Player = GetPlayerFromName( i->first, true );

					if( Player )
						SendChat( Player, m_GHost->m_Language->HasntPlayedGamesWithThisBot( i->second->GetName( ) ) );
				}
			}

			m_GHost->m_DB->RecoverCallable( i->second );
			delete i->second;
			i = m_PairedGPSChecks.erase( i );
		}
		else
			++i;
	}

	for( std::vector<PairedDPSCheck> :: iterator i = m_PairedDPSChecks.begin( ); i != m_PairedDPSChecks.end( ); )
	{
		if( i->second->GetReady( ) )
		{
			CDBDotAPlayerSummary *DotAPlayerSummary = i->second->GetResult( );

			if( DotAPlayerSummary )
			{
				std::string Summary = m_GHost->m_Language->HasPlayedDotAGamesWithThisBot(	i->second->GetName( ),
					std::to_string( DotAPlayerSummary->GetTotalGames( ) ),
					std::to_string( DotAPlayerSummary->GetTotalWins( ) ),
					std::to_string( DotAPlayerSummary->GetTotalLosses( ) ),
					std::to_string( DotAPlayerSummary->GetTotalKills( ) ),
					std::to_string( DotAPlayerSummary->GetTotalDeaths( ) ),
					std::to_string( DotAPlayerSummary->GetTotalCreepKills( ) ),
					std::to_string( DotAPlayerSummary->GetTotalCreepDenies( ) ),
					std::to_string( DotAPlayerSummary->GetTotalAssists( ) ),
					std::to_string( DotAPlayerSummary->GetTotalNeutralKills( ) ),
					std::to_string( DotAPlayerSummary->GetTotalTowerKills( ) ),
					std::to_string( DotAPlayerSummary->GetTotalRaxKills( ) ),
					std::to_string( DotAPlayerSummary->GetTotalCourierKills( ) ),
					std::to_string( DotAPlayerSummary->GetAvgKills( ) ),
					std::to_string( DotAPlayerSummary->GetAvgDeaths( ) ),
					std::to_string( DotAPlayerSummary->GetAvgCreepKills( ) ),
					std::to_string( DotAPlayerSummary->GetAvgCreepDenies( ) ),
					std::to_string( DotAPlayerSummary->GetAvgAssists( ) ),
					std::to_string( DotAPlayerSummary->GetAvgNeutralKills( ) ),
					std::to_string( DotAPlayerSummary->GetAvgTowerKills( ) ),
					std::to_string( DotAPlayerSummary->GetAvgRaxKills( ) ),
					std::to_string( DotAPlayerSummary->GetAvgCourierKills( ) ) );

				if( i->first.empty( ) )
					SendAllChat( Summary );
				else
				{
					CGamePlayer *Player = GetPlayerFromName( i->first, true );

					if( Player )
						SendChat( Player, Summary );
				}
			}
			else
			{
				if( i->first.empty( ) )
					SendAllChat( m_GHost->m_Language->HasntPlayedDotAGamesWithThisBot( i->second->GetName( ) ) );
				else
				{
					CGamePlayer *Player = GetPlayerFromName( i->first, true );

					if( Player )
						SendChat( Player, m_GHost->m_Language->HasntPlayedDotAGamesWithThisBot( i->second->GetName( ) ) );
				}
			}

			m_GHost->m_DB->RecoverCallable( i->second );
			delete i->second;
			i = m_PairedDPSChecks.erase( i );
		}
		else
			++i;
	}
#ifdef STATS_LIA_GHOST
		for( std::vector<PairedLPSCheck> :: iterator i = m_PairedLPSChecks.begin( ); i != m_PairedLPSChecks.end( ); )
	{
		if( i->second->GetReady( ) )
		{
			CDBLiAPlayerSummary *LiAPlayerSummary = i->second->GetResult( );

			if( LiAPlayerSummary )
			{
				std::string Summary = m_GHost->m_Language->HasPlayedLiAGamesWithThisBot(	i->second->GetName( ),
					std::to_string( LiAPlayerSummary->GetTotalGames( ) ),
					std::to_string( LiAPlayerSummary->GetTotalWins( ) ),
					std::to_string( LiAPlayerSummary->GetTotalLosses( ) ),
					std::to_string( LiAPlayerSummary->GetTotalDeaths( ) ),
					std::to_string( LiAPlayerSummary->GetTotalPTS( ) ),
					std::to_string( LiAPlayerSummary->GetTotalCreepKills( ) ),
					std::to_string( LiAPlayerSummary->GetTotalBossKills( ) ),
					std::to_string( LiAPlayerSummary->GetAvgDeaths( )),
					std::to_string( LiAPlayerSummary->GetAvgCreepKills( )),
					std::to_string( LiAPlayerSummary->GetAvgBossKills( )),
					std::to_string( LiAPlayerSummary->GetAvgPTS( )));

				if( i->first.empty( ) )
					SendAllChat( Summary );
				else
				{
					CGamePlayer *Player = GetPlayerFromName( i->first, true );

					if( Player )
						SendChat( Player, Summary );
				}
			}
			else
			{
				if( i->first.empty( ) )
					SendAllChat( m_GHost->m_Language->HasntPlayedLiAGamesWithThisBot( i->second->GetName( ) ) );
				else
				{
					CGamePlayer *Player = GetPlayerFromName( i->first, true );

					if( Player )
						SendChat( Player, m_GHost->m_Language->HasntPlayedLiAGamesWithThisBot( i->second->GetName( ) ) );
				}
			}

			m_GHost->m_DB->RecoverCallable( i->second );
			delete i->second;
			i = m_PairedLPSChecks.erase( i );
		}
		else
			++i;
	}
#endif





	return CBaseGame :: Update( fd, send_fd );
}

void CGame :: EventPlayerDeleted( CGamePlayer *player )
{
	CBaseGame :: EventPlayerDeleted( player );

	// record everything we need to know about the player for storing in the database later
	// since we haven't stored the game yet (it's not over yet!) we can't link the gameplayer to the game
	// see the destructor for where these CDBGamePlayers are stored in the database
	// we could have inserted an incomplete record on creation and updated it later but this makes for a cleaner interface

	if( m_GameLoading || m_GameLoaded )
	{
		// todotodo: since we store players that crash during loading it's possible that the stats classes could have no information on them
		// that could result in a DBGamePlayer without a corresponding DBDotAPlayer - just be aware of the possibility

		unsigned char SID = GetSIDFromPID( player->GetPID( ) );
		unsigned char Team = 255;
		unsigned char Colour = 255;

		if( SID < m_Slots.size( ) )
		{
			Team = m_Slots[SID].GetTeam( );
			Colour = m_Slots[SID].GetColour( );
		}

		m_DBGamePlayers.push_back( new CDBGamePlayer( 0, 0, player->GetName( ), player->GetExternalIPString( ), player->GetSpoofed( ) ? 1 : 0, player->GetSpoofedRealm( ), player->GetReserved( ) ? 1 : 0, player->GetFinishedLoading( ) ? player->GetFinishedLoadingTicks( ) - m_StartedLoadingTicks : 0, m_GameTicks / 1000, player->GetLeftReason( ), Team, Colour ) );

		// also keep track of the last player to leave for the !banlast command

		for( std::vector<CDBBan *> :: iterator i = m_DBBans.begin( ); i != m_DBBans.end( ); ++i )
		{
			if( (*i)->GetName( ) == player->GetName( ) )
				m_DBBanLast = *i;
		}
	}
}

bool CGame :: EventPlayerAction( CGamePlayer *player, CIncomingAction *action )
{
	bool success = CBaseGame :: EventPlayerAction( player, action );

	// give the stats class a chance to process the action

	if( success && m_Stats && m_Stats->ProcessAction( action ) && m_GameOverTime == 0 )
	{
		CONSOLE_Print( "[GAME: " + m_GameName + "] gameover timer started (stats class reported game over)" );
		SendEndMessage( );
		m_GameOverTime = GetTime( );
	}
	
	return success;
}

bool CGame :: EventPlayerBotCommand( CGamePlayer *player, std::string command, std::string payload )
{
	bool HideCommand = CBaseGame :: EventPlayerBotCommand( player, command, payload );

	// todotodo: don't be lazy

	std::string User = player->GetName( );
	std::string Command = command;
	std::string Payload = payload;

	bool AdminCheck = false;

	for( std::vector<CBNET *> :: iterator i = m_GHost->m_BNETs.begin( ); i != m_GHost->m_BNETs.end( ); ++i )
	{
		if( (*i)->GetServer( ) == player->GetSpoofedRealm( ) && (*i)->IsAdmin( User ) )
		{
			AdminCheck = true;
			break;
		}
	}

	bool RootAdminCheck = false;

	for( std::vector<CBNET *> :: iterator i = m_GHost->m_BNETs.begin( ); i != m_GHost->m_BNETs.end( ); ++i )
	{
		if( (*i)->GetServer( ) == player->GetSpoofedRealm( ) && (*i)->IsRootAdmin( User ) )
		{
			RootAdminCheck = true;
			break;
		}
	}

	if( player->GetSpoofed( ) && ( AdminCheck || RootAdminCheck || IsOwner( User ) ) )
	{
		CONSOLE_Print( "[GAME: " + m_GameName + "] admin [" + User + "] sent command [" + Command + "] with payload [" + Payload + "]" );

		if( !m_Locked || RootAdminCheck || IsOwner( User ) )
		{
			/*****************
			* ADMIN COMMANDS *
			******************/

			//
			// !ABORT (abort countdown)
			// !A
			//

			// we use "!a" as an alias for abort because you don't have much time to abort the countdown so it's useful for the abort command to be easy to type

			if( ( Command == "abort" || Command == "a" ) && m_CountDownStarted && !m_GameLoading && !m_GameLoaded )
			{
				SendAllChat( m_GHost->m_Language->CountDownAborted( ) );
				m_CountDownStarted = false;
			}

			//
			// !ADDBAN
			// !BAN
			//

			else if( ( Command == "addban" || Command == "ban" ) && !Payload.empty( ) && !m_GHost->m_BNETs.empty( ) )
			{
				// extract the victim and the reason
				// e.g. "Varlock leaver after dying" -> victim: "Varlock", reason: "leaver after dying"

				std::string Victim;
				std::string Reason;
				std::stringstream SS;
				SS << Payload;
				SS >> Victim;

				if( !SS.eof( ) )
				{
					getline( SS, Reason );
					std::string :: size_type Start = Reason.find_first_not_of( " " );

					if( Start != std::string :: npos )
						Reason = Reason.substr( Start );
				}

				if( m_GameLoaded )
				{
					std::string VictimLower = Victim;
					transform( VictimLower.begin( ), VictimLower.end( ), VictimLower.begin( ), (int(*)(int))tolower );
					uint32_t Matches = 0;
					CDBBan *LastMatch = NULL;

					// try to match each player with the passed std::string (e.g. "Varlock" would be matched with "lock")
					// we use the m_DBBans vector for this in case the player already left and thus isn't in the m_Players vector anymore

					for( std::vector<CDBBan *> :: iterator i = m_DBBans.begin( ); i != m_DBBans.end( ); ++i )
					{
						std::string TestName = (*i)->GetName( );
						transform( TestName.begin( ), TestName.end( ), TestName.begin( ), (int(*)(int))tolower );

						if( TestName.find( VictimLower ) != std::string :: npos )
						{
							Matches++;
							LastMatch = *i;

							// if the name matches exactly stop any further matching

							if( TestName == VictimLower )
							{
								Matches = 1;
								break;
							}
						}
					}

					if( Matches == 0 )
						SendAllChat( m_GHost->m_Language->UnableToBanNoMatchesFound( Victim ) );
					else if( Matches == 1 )
						m_PairedBanAdds.push_back( PairedBanAdd( User, m_GHost->m_DB->ThreadedBanAdd( LastMatch->GetServer( ), LastMatch->GetName( ), LastMatch->GetIP( ), m_GameName, User, Reason ) ) );
					else
						SendAllChat( m_GHost->m_Language->UnableToBanFoundMoreThanOneMatch( Victim ) );
				}
				else
				{
					CGamePlayer *LastMatch = NULL;
					uint32_t Matches = GetPlayerFromNamePartial( Victim, &LastMatch );

					if( Matches == 0 )
						SendAllChat( m_GHost->m_Language->UnableToBanNoMatchesFound( Victim ) );
					else if( Matches == 1 )
						m_PairedBanAdds.push_back( PairedBanAdd( User, m_GHost->m_DB->ThreadedBanAdd( LastMatch->GetJoinedRealm( ), LastMatch->GetName( ), LastMatch->GetExternalIPString( ), m_GameName, User, Reason ) ) );
					else
						SendAllChat( m_GHost->m_Language->UnableToBanFoundMoreThanOneMatch( Victim ) );
				}
			}

			//
			// !ANNOUNCE
			//

			else if( Command == "announce" && !m_CountDownStarted )
			{
				if( Payload.empty( ) || Payload == "off" )
				{
					SendAllChat( m_GHost->m_Language->AnnounceMessageDisabled( ) );
					SetAnnounce( 0, std::string( ) );
				}
				else
				{
					// extract the interval and the message
					// e.g. "30 hello everyone" -> interval: "30", message: "hello everyone"

					uint32_t Interval;
					std::string Message;
					std::stringstream SS;
					SS << Payload;
					SS >> Interval;

					if( SS.fail( ) || Interval == 0 )
						CONSOLE_Print( "[GAME: " + m_GameName + "] bad input #1 to announce command" );
					else
					{
						if( SS.eof( ) )
							CONSOLE_Print( "[GAME: " + m_GameName + "] missing input #2 to announce command" );
						else
						{
							getline( SS, Message );
							std::string :: size_type Start = Message.find_first_not_of( " " );

							if( Start != std::string :: npos )
								Message = Message.substr( Start );

							SendAllChat( m_GHost->m_Language->AnnounceMessageEnabled( ) );
							SetAnnounce( Interval, Message );
						}
					}
				}
			}

			//
			// !AUTOSAVE
			//

			else if( Command == "autosave" )
			{
				if( Payload == "on" )
				{
					SendAllChat( m_GHost->m_Language->AutoSaveEnabled( ) );
					m_AutoSave = true;
				}
				else if( Payload == "off" )
				{
					SendAllChat( m_GHost->m_Language->AutoSaveDisabled( ) );
					m_AutoSave = false;
				}
			}

			//
			// !AUTOSTART
			//

			else if( Command == "autostart" && !m_CountDownStarted )
			{
				if( Payload.empty( ) || Payload == "off" )
				{
					SendAllChat( m_GHost->m_Language->AutoStartDisabled( ) );
					m_AutoStartPlayers = 0;
				}
				else
				{
					uint32_t AutoStartPlayers = UTIL_ToUInt32( Payload );

					if( AutoStartPlayers != 0 )
					{
						SendAllChat( m_GHost->m_Language->AutoStartEnabled( std::to_string( AutoStartPlayers ) ) );
						m_AutoStartPlayers = AutoStartPlayers;
					}
				}
			}

			//
			// !BANLAST
			//

			else if( Command == "banlast" && m_GameLoaded && !m_GHost->m_BNETs.empty( ) && m_DBBanLast )
				m_PairedBanAdds.push_back( PairedBanAdd( User, m_GHost->m_DB->ThreadedBanAdd( m_DBBanLast->GetServer( ), m_DBBanLast->GetName( ), m_DBBanLast->GetIP( ), m_GameName, User, Payload ) ) );

			//
			// !CHECK
			//

			else if( Command == "check" )
			{
				if( !Payload.empty( ) )
				{
					CGamePlayer *LastMatch = NULL;
					uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

					if( Matches == 0 )
						SendAllChat( m_GHost->m_Language->UnableToCheckPlayerNoMatchesFound( Payload ) );
					else if( Matches == 1 )
					{
						bool LastMatchAdminCheck = false;

						for( std::vector<CBNET *> :: iterator i = m_GHost->m_BNETs.begin( ); i != m_GHost->m_BNETs.end( ); ++i )
						{
							if( (*i)->GetServer( ) == LastMatch->GetSpoofedRealm( ) && (*i)->IsAdmin( LastMatch->GetName( ) ) )
							{
								LastMatchAdminCheck = true;
								break;
							}
						}

						bool LastMatchRootAdminCheck = false;

						for( std::vector<CBNET *> :: iterator i = m_GHost->m_BNETs.begin( ); i != m_GHost->m_BNETs.end( ); ++i )
						{
							if( (*i)->GetServer( ) == LastMatch->GetSpoofedRealm( ) && (*i)->IsRootAdmin( LastMatch->GetName( ) ) )
							{
								LastMatchRootAdminCheck = true;
								break;
							}
						}

						SendAllChat( m_GHost->m_Language->CheckedPlayer( LastMatch->GetName( ), LastMatch->GetNumPings( ) > 0 ? std::to_string( LastMatch->GetPing( m_GHost->m_LCPings ) ) + "ms" : "N/A", LastMatch->GetCountry() , LastMatchAdminCheck || LastMatchRootAdminCheck ? "Yes" : "No", IsOwner( LastMatch->GetName( ) ) ? "Yes" : "No", LastMatch->GetSpoofed( ) ? "Yes" : "No", LastMatch->GetSpoofedRealm( ).empty( ) ? "N/A" : LastMatch->GetSpoofedRealm( ), LastMatch->GetReserved( ) ? "Yes" : "No" ) );
					}
					else
						SendAllChat( m_GHost->m_Language->UnableToCheckPlayerFoundMoreThanOneMatch( Payload ) );
				}
				else
					SendAllChat( m_GHost->m_Language->CheckedPlayer( User, player->GetNumPings( ) > 0 ? std::to_string( player->GetPing( m_GHost->m_LCPings ) ) + "ms" : "N/A", player->GetCountry(), AdminCheck || RootAdminCheck ? "Yes" : "No", IsOwner( User ) ? "Yes" : "No", player->GetSpoofed( ) ? "Yes" : "No", player->GetSpoofedRealm( ).empty( ) ? "N/A" : player->GetSpoofedRealm( ), player->GetReserved( ) ? "Yes" : "No" ) );
			}

			//
			// !CHECKBAN
			//

			else if( Command == "checkban" && !Payload.empty( ) && !m_GHost->m_BNETs.empty( ) )
			{
				for( std::vector<CBNET *> :: iterator i = m_GHost->m_BNETs.begin( ); i != m_GHost->m_BNETs.end( ); ++i )
					m_PairedBanChecks.push_back( PairedBanCheck( User, m_GHost->m_DB->ThreadedBanCheck( (*i)->GetServer( ), Payload, std::string( ) ) ) );
			}

			//
			// !CLEARHCL
			//

			else if( Command == "clearhcl" && !m_CountDownStarted )
			{
				m_HCLCommandString.clear( );
				SendAllChat( m_GHost->m_Language->ClearingHCL( ) );
				
				
				
				SendAllChat("Switching to map mode: " + detect_lia_mode(m_HCLCommandString));
			}

			//
			// !CLOSE (close slot)
			//

			else if( (Command == "close"||Command == "c") && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded )
			{
				// close as many slots as specified, e.g. "5 10" closes slots 5 and 10

				std::stringstream SS;
				SS << Payload;

				while( !SS.eof( ) )
				{
					uint32_t SID;
					SS >> SID;

					if( SS.fail( ) )
					{
						CONSOLE_Print( "[GAME: " + m_GameName + "] bad input to close command" );
						break;
					}
					else
						CloseSlot( (unsigned char)( SID - 1 ), true );
				}
			}

			//
			// !CLOSEALL
			//

			else if( (Command == "closeall" || Command == "ca") && !m_GameLoading && !m_GameLoaded )
				CloseAllSlots( );

			//
			// !COMP (computer slot)
			//

			else if( Command == "comp" && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded && !m_SaveGame )
			{
				// extract the slot and the skill
				// e.g. "1 2" -> slot: "1", skill: "2"

				uint32_t Slot;
				uint32_t Skill = 1;
				std::stringstream SS;
				SS << Payload;
				SS >> Slot;

				if( SS.fail( ) )
					CONSOLE_Print( "[GAME: " + m_GameName + "] bad input #1 to comp command" );
				else
				{
					if( !SS.eof( ) )
						SS >> Skill;

					if( SS.fail( ) )
						CONSOLE_Print( "[GAME: " + m_GameName + "] bad input #2 to comp command" );
					else
						ComputerSlot( (unsigned char)( Slot - 1 ), (unsigned char)Skill, true );
				}
			}

			//
			// !COMPCOLOUR (computer colour change)
			//

			else if( Command == "compcolour" && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded && !m_SaveGame )
			{
				// extract the slot and the colour
				// e.g. "1 2" -> slot: "1", colour: "2"

				uint32_t Slot;
				uint32_t Colour;
				std::stringstream SS;
				SS << Payload;
				SS >> Slot;

				if( SS.fail( ) )
					CONSOLE_Print( "[GAME: " + m_GameName + "] bad input #1 to compcolour command" );
				else
				{
					if( SS.eof( ) )
						CONSOLE_Print( "[GAME: " + m_GameName + "] missing input #2 to compcolour command" );
					else
					{
						SS >> Colour;

						if( SS.fail( ) )
							CONSOLE_Print( "[GAME: " + m_GameName + "] bad input #2 to compcolour command" );
						else
						{
							unsigned char SID = (unsigned char)( Slot - 1 );

							if( !( m_Map->GetMapOptions( ) & MAPOPT_FIXEDPLAYERSETTINGS ) && Colour < 12 && SID < m_Slots.size( ) )
							{
								if( m_Slots[SID].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[SID].GetComputer( ) == 1 )
									ColourSlot( SID, Colour );
							}
						}
					}
				}
			}

			//
			// !COMPHANDICAP (computer handicap change)
			//

			else if( Command == "comphandicap" && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded && !m_SaveGame )
			{
				// extract the slot and the handicap
				// e.g. "1 50" -> slot: "1", handicap: "50"

				uint32_t Slot;
				uint32_t Handicap;
				std::stringstream SS;
				SS << Payload;
				SS >> Slot;

				if( SS.fail( ) )
					CONSOLE_Print( "[GAME: " + m_GameName + "] bad input #1 to comphandicap command" );
				else
				{
					if( SS.eof( ) )
						CONSOLE_Print( "[GAME: " + m_GameName + "] missing input #2 to comphandicap command" );
					else
					{
						SS >> Handicap;

						if( SS.fail( ) )
							CONSOLE_Print( "[GAME: " + m_GameName + "] bad input #2 to comphandicap command" );
						else
						{
							unsigned char SID = (unsigned char)( Slot - 1 );

							if( !( m_Map->GetMapOptions( ) & MAPOPT_FIXEDPLAYERSETTINGS ) && ( Handicap == 50 || Handicap == 60 || Handicap == 70 || Handicap == 80 || Handicap == 90 || Handicap == 100 ) && SID < m_Slots.size( ) )
							{
								if( m_Slots[SID].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[SID].GetComputer( ) == 1 )
								{
									m_Slots[SID].SetHandicap( (unsigned char)Handicap );
									SendAllSlotInfo( );
								}
							}
						}
					}
				}
			}

			//
			// !COMPRACE (computer race change)
			//

			else if( Command == "comprace" && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded && !m_SaveGame )
			{
				// extract the slot and the race
				// e.g. "1 human" -> slot: "1", race: "human"

				uint32_t Slot;
				std::string Race;
				std::stringstream SS;
				SS << Payload;
				SS >> Slot;

				if( SS.fail( ) )
					CONSOLE_Print( "[GAME: " + m_GameName + "] bad input #1 to comprace command" );
				else
				{
					if( SS.eof( ) )
						CONSOLE_Print( "[GAME: " + m_GameName + "] missing input #2 to comprace command" );
					else
					{
						getline( SS, Race );
						std::string :: size_type Start = Race.find_first_not_of( " " );

						if( Start != std::string :: npos )
							Race = Race.substr( Start );

						transform( Race.begin( ), Race.end( ), Race.begin( ), (int(*)(int))tolower );
						unsigned char SID = (unsigned char)( Slot - 1 );

						if( !( m_Map->GetMapOptions( ) & MAPOPT_FIXEDPLAYERSETTINGS ) && !( m_Map->GetMapFlags( ) & MAPFLAG_RANDOMRACES ) && SID < m_Slots.size( ) )
						{
							if( m_Slots[SID].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[SID].GetComputer( ) == 1 )
							{
								if( Race == "human" )
								{
									m_Slots[SID].SetRace( SLOTRACE_HUMAN | SLOTRACE_SELECTABLE );
									SendAllSlotInfo( );
								}
								else if( Race == "orc" )
								{
									m_Slots[SID].SetRace( SLOTRACE_ORC | SLOTRACE_SELECTABLE );
									SendAllSlotInfo( );
								}
								else if( Race == "night elf" )
								{
									m_Slots[SID].SetRace( SLOTRACE_NIGHTELF | SLOTRACE_SELECTABLE );
									SendAllSlotInfo( );
								}
								else if( Race == "undead" )
								{
									m_Slots[SID].SetRace( SLOTRACE_UNDEAD | SLOTRACE_SELECTABLE );
									SendAllSlotInfo( );
								}
								else if( Race == "random" )
								{
									m_Slots[SID].SetRace( SLOTRACE_RANDOM | SLOTRACE_SELECTABLE );
									SendAllSlotInfo( );
								}
								else
									CONSOLE_Print( "[GAME: " + m_GameName + "] unknown race [" + Race + "] sent to comprace command" );
							}
						}
					}
				}
			}

			//
			// !COMPTEAM (computer team change)
			//

			else if( Command == "compteam" && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded && !m_SaveGame )
			{
				// extract the slot and the team
				// e.g. "1 2" -> slot: "1", team: "2"

				uint32_t Slot;
				uint32_t Team;
				std::stringstream SS;
				SS << Payload;
				SS >> Slot;

				if( SS.fail( ) )
					CONSOLE_Print( "[GAME: " + m_GameName + "] bad input #1 to compteam command" );
				else
				{
					if( SS.eof( ) )
						CONSOLE_Print( "[GAME: " + m_GameName + "] missing input #2 to compteam command" );
					else
					{
						SS >> Team;

						if( SS.fail( ) )
							CONSOLE_Print( "[GAME: " + m_GameName + "] bad input #2 to compteam command" );
						else
						{
							unsigned char SID = (unsigned char)( Slot - 1 );

							if( !( m_Map->GetMapOptions( ) & MAPOPT_FIXEDPLAYERSETTINGS ) && Team < 12 && SID < m_Slots.size( ) )
							{
								if( m_Slots[SID].GetSlotStatus( ) == SLOTSTATUS_OCCUPIED && m_Slots[SID].GetComputer( ) == 1 )
								{
									m_Slots[SID].SetTeam( (unsigned char)( Team - 1 ) );
									SendAllSlotInfo( );
								}
							}
						}
					}
				}
			}

			//
			// !DBSTATUS
			//

			else if( Command == "dbstatus" )
				SendAllChat( m_GHost->m_DB->GetStatus( ) );

			//
			// !DOWNLOAD
			// !DL
			//

			else if( ( Command == "download" || Command == "dl" ) && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded )
			{
				CGamePlayer *LastMatch = NULL;
				uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

				if( Matches == 0 )
					SendAllChat( m_GHost->m_Language->UnableToStartDownloadNoMatchesFound( Payload ) );
				else if( Matches == 1 )
				{
					if( !LastMatch->GetDownloadStarted( ) && !LastMatch->GetDownloadFinished( ) )
					{
						unsigned char SID = GetSIDFromPID( LastMatch->GetPID( ) );

						if( SID < m_Slots.size( ) && m_Slots[SID].GetDownloadStatus( ) != 100 )
						{
							// inform the client that we are willing to send the map

							CONSOLE_Print( "[GAME: " + m_GameName + "] map download started for player [" + LastMatch->GetName( ) + "]" );
							Send( LastMatch, m_Protocol->SEND_W3GS_STARTDOWNLOAD( GetHostPID( ) ) );
							LastMatch->SetDownloadAllowed( true );
							LastMatch->SetDownloadStarted( true );
							LastMatch->SetStartedDownloadingTicks( GetTicks( ) );
						}
					}
				}
				else
					SendAllChat( m_GHost->m_Language->UnableToStartDownloadFoundMoreThanOneMatch( Payload ) );
			}

			//
			// !DROP
			//

			else if( Command == "drop" && m_GameLoaded )
				StopLaggers( "lagged out (dropped by admin)" );

			//
			// !END
			//

			else if( Command == "end" && m_GameLoaded )
			{
				CONSOLE_Print( "[GAME: " + m_GameName + "] is over (admin ended game)" );
				StopPlayers( "was disconnected (admin ended game)" );
			}

			//
			// !FAKEPLAYER
			//

			else if( Command == "fakeplayer" && !m_CountDownStarted )
			{
				if( m_FakePlayerPID == 255 )
					CreateFakePlayer( );
				else
					DeleteFakePlayer( );
			}

			//
			// !FPPAUSE
			//

			else if( (Command == "fppause" || Command == "fp") && m_FakePlayerPID != 255 && m_GameLoaded )
			{
				BYTEARRAY CRC;
				BYTEARRAY Action;
				Action.push_back( 1 );
				m_Actions.push( new CIncomingAction( m_FakePlayerPID, CRC, Action ) );
			}

			//
			// !FPRESUME
			//

			else if( (Command == "fpresume" || Command == "fr") && m_FakePlayerPID != 255 && m_GameLoaded )
			{
				BYTEARRAY CRC;
				BYTEARRAY Action;
				Action.push_back( 2 );
				m_Actions.push( new CIncomingAction( m_FakePlayerPID, CRC, Action ) );
			}
			//
			// !FROM
			//

			else if( Command == "from" )
			{
				std::string Froms;

				for( std::vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
				{
					// we reverse the byte order on the IP because it's stored in network byte order

					Froms += (*i)->GetNameTerminated( );
					Froms += ": (";
					Froms += (*i)->GetCountry( );
					Froms += ")";

					if( i != m_Players.end( ) - 1 )
						Froms += ", ";

					if( ( m_GameLoading || m_GameLoaded ) && Froms.size( ) > 100 )
					{
						// cut the text into multiple lines ingame

						SendAllChat( Froms );
						Froms.clear( );
					}
				}

				if( !Froms.empty( ) )
					SendAllChat( Froms );
			}

			//
			// !HCL
			//

			else if( Command == "hcl" && !m_CountDownStarted )
			{
				if( !Payload.empty( ) )
				{
					if( Payload.size( ) <= m_Slots.size( ) )
					{
						std::string HCLChars = "abcdefghijklmnopqrstuvwxyz0123456789 -=,.";

						if( Payload.find_first_not_of( HCLChars ) == std::string :: npos )
						{
							m_HCLCommandString = Payload;
							SendAllChat( m_GHost->m_Language->SettingHCL( m_HCLCommandString ) );

							SendAllChat("Switching to "+detect_lia_mode(m_HCLCommandString));
						}
						else
							SendAllChat( m_GHost->m_Language->UnableToSetHCLInvalid( ) );
					}
					else
						SendAllChat( m_GHost->m_Language->UnableToSetHCLTooLong( ) );
				}
				else
					SendAllChat( m_GHost->m_Language->TheHCLIs( m_HCLCommandString ) );
			}

			//
			// !HOLD (hold a slot for someone)
			//

			else if( (Command == "hold" || Command == "h")  && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded )
			{
				// hold as many players as specified, e.g. "Varlock Kilranin" holds players "Varlock" and "Kilranin"

				std::stringstream SS;
				SS << Payload;

				while( !SS.eof( ) )
				{
					std::string HoldName;
					SS >> HoldName;

					if( SS.fail( ) )
					{
						CONSOLE_Print( "[GAME: " + m_GameName + "] bad input to hold command" );
						break;
					}
					else
					{
						SendAllChat( m_GHost->m_Language->AddedPlayerToTheHoldList( HoldName ) );
						AddToReserved( HoldName );
					}
				}
			}

			//
			// !KICK (kick a player)
			//

			else if( (Command == "kick"  || Command == "k") && !Payload.empty( ) )
			{
				CGamePlayer *LastMatch = NULL;
				uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

				if( Matches == 0 )
					SendAllChat( m_GHost->m_Language->UnableToKickNoMatchesFound( Payload ) );
				else if( Matches == 1 )
				{
					LastMatch->SetDeleteMe( true );
					LastMatch->SetLeftReason( m_GHost->m_Language->WasKickedByPlayer( User ) );

					if( !m_GameLoading && !m_GameLoaded )
						LastMatch->SetLeftCode( PLAYERLEAVE_LOBBY );
					else
						LastMatch->SetLeftCode( PLAYERLEAVE_LOST );

					if( !m_GameLoading && !m_GameLoaded )
						OpenSlot( GetSIDFromPID( LastMatch->GetPID( ) ), false );
				}
				else
					SendAllChat( m_GHost->m_Language->UnableToKickFoundMoreThanOneMatch( Payload ) );
			}

			//
			// !LATENCY (set game latency)
			//

			else if( Command == "latency"  || Command == "l" )
			{
				if( Payload.empty( ) )
					SendAllChat( m_GHost->m_Language->LatencyIs( std::to_string( m_Latency ) ) );
				else
				{
					m_Latency = UTIL_ToUInt32( Payload );

					if( m_Latency <= 5 )
					{
						m_Latency = 5;
						SendAllChat( m_GHost->m_Language->SettingLatencyToMinimum( "1" ) );
					}
					else if( m_Latency >= 100 )
					{
						m_Latency = 100;
						SendAllChat( m_GHost->m_Language->SettingLatencyToMaximum( "100" ) );
					}
					else
						SendAllChat( m_GHost->m_Language->SettingLatencyTo( std::to_string( m_Latency ) ) );
				}
			}

			//
			// !LOCK
			//

			else if( Command == "lock" && ( RootAdminCheck || IsOwner( User ) ) )
			{
				SendAllChat( m_GHost->m_Language->GameLocked( ) );
				m_Locked = true;
			}

			//
			// !MESSAGES
			//

			else if( Command == "messages" )
			{
				if( Payload == "on" )
				{
					SendAllChat( m_GHost->m_Language->LocalAdminMessagesEnabled( ) );
					m_LocalAdminMessages = true;
				}
				else if( Payload == "off" )
				{
					SendAllChat( m_GHost->m_Language->LocalAdminMessagesDisabled( ) );
					m_LocalAdminMessages = false;
				}
			}

			//
			// !MUTE
			//
			#ifdef GHOST_MUTE
			else if( Command == "mute"  || Command == "m" )
			{
				CGamePlayer *LastMatch = NULL;
				uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

				if( Matches == 0 )
					SendAllChat( m_GHost->m_Language->UnableToMuteNoMatchesFound( Payload ) );
				else if( Matches == 1 )
				{
					SendAllChat( m_GHost->m_Language->MutedPlayer( LastMatch->GetName( ), User ) );
					LastMatch->SetMuted( true );
				}
				else
					SendAllChat( m_GHost->m_Language->UnableToMuteFoundMoreThanOneMatch( Payload ) );
			}

			//
			// !MUTEALL
			//

			else if( (Command == "muteall" || Command == "ma") && m_GameLoaded )
			{
				SendAllChat( m_GHost->m_Language->GlobalChatMuted( ) );
				m_MuteAll = true;
			}
			#endif
			//
			// !OPEN (open slot)
			//

			else if( (Command == "open" || Command == "o")  && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded )
			{
				// open as many slots as specified, e.g. "5 10" opens slots 5 and 10

				std::stringstream SS;
				SS << Payload;

				while( !SS.eof( ) )
				{
					uint32_t SID;
					SS >> SID;

					if( SS.fail( ) )
					{
						CONSOLE_Print( "[GAME: " + m_GameName + "] bad input to open command" );
						break;
					}
					else
						OpenSlot( (unsigned char)( SID - 1 ), true );
				}
			}

			//
			// !OPENALL
			//

			else if( (Command == "openall" || Command == "oa")  && !m_GameLoading && !m_GameLoaded )
				OpenAllSlots( );

			//
			// !OWNER (set game owner)
			//

			else if( Command == "owner" )
			{
				if( RootAdminCheck || IsOwner( User ) || !GetPlayerFromName( m_OwnerName, false ) )
				{
					if( !Payload.empty( ) )
					{
						SendAllChat( m_GHost->m_Language->SettingGameOwnerTo( Payload ) );
						m_OwnerName = Payload;
					}
					else
					{
						SendAllChat( m_GHost->m_Language->SettingGameOwnerTo( User ) );
						m_OwnerName = User;
					}
				}
				else
					SendAllChat( m_GHost->m_Language->UnableToSetGameOwner( m_OwnerName ) );
			}
			
			//
			// !PING
			//

			else if( Command == "ping" || Command == "p" )
			{
				// kick players with ping higher than payload if payload isn't empty
				// we only do this if the game hasn't started since we don't want to kick players from a game in progress

				uint32_t Kicked = 0;
				uint32_t KickPing = 0;

				if( !m_GameLoading && !m_GameLoaded && !Payload.empty( ) )
					KickPing = UTIL_ToUInt32( Payload );

				// copy the m_Players vector so we can sort by descending ping so it's easier to find players with high pings

				std::vector<CGamePlayer *> SortedPlayers = m_Players;
				sort( SortedPlayers.begin( ), SortedPlayers.end( ), CGamePlayerSortDescByPing( ) );
				std::string Pings;

				for( auto Player : SortedPlayers)
				{
					Pings += "["+Player->GetNameTerminated( )+"]";
					Pings += ": Current => ";

					if( Player->GetNumPings( ) > 0 )
					{
						Pings += std::to_string( Player->GetPing( m_GHost->m_LCPings ) );

						Pings += "ms Average => ";
						Pings += std::to_string( Player->GetAveragePing( m_GHost->m_LCPings ) );
						Pings += "ms ";
					}
					else
						Pings += "N/A";

					if( Player != SortedPlayers.back() )
						Pings += ", ";

					if( m_GameLoading || m_GameLoaded )
					{
						// cut the text into multiple lines ingame

						SendAllChat( Pings );
						Pings.clear( );
					}
				}

				if( !Pings.empty( ) )
					SendAllChat( Pings );
			}


			//
			// !PRIV (rehost as private game)
			//

			else if( Command == "priv" && !Payload.empty( ) && !m_CountDownStarted && !m_SaveGame )
			{
				if( Payload.length() < 31 )
				{
					// CONSOLE_Print( "[GAME: " + m_GameName + "] trying to rehost as private game [" + Payload + "]" );
					// SendAllChat( m_GHost->m_Language->TryingToRehostAsPrivateGame( Payload ) );
					m_GameState = GAME_PRIVATE;
					m_LastGameName = m_GameName;
					m_GameName = Payload;
					m_HostCounter = m_GHost->m_HostCounter++;
					m_RefreshError = false;
					m_RefreshRehosted = true;

					for( std::vector<CBNET *> :: iterator i = m_GHost->m_BNETs.begin( ); i != m_GHost->m_BNETs.end( ); ++i )
					{
						// unqueue any existing game refreshes because we're going to assume the next successful game refresh indicates that the rehost worked
						// this ignores the fact that it's possible a game refresh was just sent and no response has been received yet
						// we assume this won't happen very often since the only downside is a potential false positive

						(*i)->UnqueueGameRefreshes( );
						(*i)->QueueGameUncreate( );
						(*i)->QueueEnterChat( );

						// we need to send the game creation message now because private games are not refreshed

						(*i)->QueueGameCreate( m_GameState, m_GameName, std::string( ), m_Map, NULL, m_HostCounter );

						if( (*i)->GetPasswordHashType( ) != "pvpgn" )
							(*i)->QueueEnterChat( );
					}

					m_CreationTime = GetTime( );
					m_LastRefreshTime = GetTime( );
				}
				else
					SendAllChat( m_GHost->m_Language->UnableToCreateGameNameTooLong( Payload ) );
			}

			//
			// !PUB (rehost as public game)
			//

			else if( Command == "pub" && !Payload.empty( ) && !m_CountDownStarted && !m_SaveGame )
			{
				if( Payload.length() < 31 )
				{
					// CONSOLE_Print( "[GAME: " + m_GameName + "] trying to rehost as public game [" + Payload + "]" );
					// SendAllChat( m_GHost->m_Language->TryingToRehostAsPublicGame( Payload ) );
					m_GameState = GAME_PUBLIC;
					m_LastGameName = m_GameName;
					m_GameName = Payload;
					m_HostCounter = m_GHost->m_HostCounter++;
					m_RefreshError = false;
					m_RefreshRehosted = true;

					for( std::vector<CBNET *> :: iterator i = m_GHost->m_BNETs.begin( ); i != m_GHost->m_BNETs.end( ); ++i )
					{
						// unqueue any existing game refreshes because we're going to assume the next successful game refresh indicates that the rehost worked
						// this ignores the fact that it's possible a game refresh was just sent and no response has been received yet
						// we assume this won't happen very often since the only downside is a potential false positive

						(*i)->UnqueueGameRefreshes( );
						(*i)->QueueGameUncreate( );
						(*i)->QueueEnterChat( );

						// the game creation message will be sent on the next refresh
					}

					m_CreationTime = GetTime( );
					m_LastRefreshTime = GetTime( );
				}
				else
					SendAllChat( m_GHost->m_Language->UnableToCreateGameNameTooLong( Payload ) );
			}
			//
			// !REFRESH (turn on or off refresh messages)
			//

			else if( Command == "refresh" && !m_CountDownStarted )
			{
				if( Payload == "on" )
				{
					SendAllChat( m_GHost->m_Language->RefreshMessagesEnabled( ) );
					m_RefreshMessages = true;
				}
				else if( Payload == "off" )
				{
					SendAllChat( m_GHost->m_Language->RefreshMessagesDisabled( ) );
					m_RefreshMessages = false;
				}
			}

			//
			// !SAY
			//

			else if( Command == "say" && !Payload.empty( ) )
			{
				for( std::vector<CBNET *> :: iterator i = m_GHost->m_BNETs.begin( ); i != m_GHost->m_BNETs.end( ); ++i )
					(*i)->QueueChatCommand( Payload );

				HideCommand = true;
			}

			//
			// !SENDLAN
			//

			else if( Command == "sendlan" && !Payload.empty( ) && !m_CountDownStarted )
			{
				// extract the ip and the port
				// e.g. "1.2.3.4 6112" -> ip: "1.2.3.4", port: "6112"

				std::string IP;
				uint32_t Port = 6112;
				std::stringstream SS;
				SS << Payload;
				SS >> IP;

				if( !SS.eof( ) )
					SS >> Port;

				if( SS.fail( ) )
					CONSOLE_Print( "[GAME: " + m_GameName + "] bad inputs to sendlan command" );
				else
				{
					// construct a fixed host counter which will be used to identify players from this "realm" (i.e. LAN)
					// the fixed host counter's 4 most significant bits will contain a 4 bit ID (0-15)
					// the rest of the fixed host counter will contain the 28 least significant bits of the actual host counter
					// since we're destroying 4 bits of information here the actual host counter should not be greater than 2^28 which is a reasonable assumption
					// when a player joins a game we can obtain the ID from the received host counter
					// note: LAN broadcasts use an ID of 0, battle.net refreshes use an ID of 1-10, the rest are unused

					uint32_t FixedHostCounter = m_HostCounter & 0x0FFFFFFF;

					// we send 12 for SlotsTotal because this determines how many PID's Warcraft 3 allocates
					// we need to make sure Warcraft 3 allocates at least SlotsTotal + 1 but at most 12 PID's
					// this is because we need an extra PID for the virtual host player (but we always delete the virtual host player when the 12th person joins)
					// however, we can't send 13 for SlotsTotal because this causes Warcraft 3 to crash when sharing control of units
					// nor can we send SlotsTotal because then Warcraft 3 crashes when playing maps with less than 12 PID's (because of the virtual host player taking an extra PID)
					// we also send 12 for SlotsOpen because Warcraft 3 assumes there's always at least one player in the game (the host)
					// so if we try to send accurate numbers it'll always be off by one and results in Warcraft 3 assuming the game is full when it still needs one more player
					// the easiest solution is to simply send 12 for both so the game will always show up as (1/12) players

					if( m_SaveGame )
					{
						// note: the PrivateGame flag is not set when broadcasting to LAN (as you might expect)

						uint32_t MapGameType = MAPGAMETYPE_SAVEDGAME;
						BYTEARRAY MapWidth;
						MapWidth.push_back( 0 );
						MapWidth.push_back( 0 );
						BYTEARRAY MapHeight;
						MapHeight.push_back( 0 );
						MapHeight.push_back( 0 );
						m_GHost->m_UDPSocket->SendTo( IP, Port, m_Protocol->SEND_W3GS_GAMEINFO( m_GHost->m_TFT, m_GHost->m_LANWar3Version, UTIL_CreateByteArray( MapGameType, false ), m_Map->GetMapGameFlags( ), MapWidth, MapHeight, m_GameName, "|cff00ff00EN0T", GetTime( ) - m_CreationTime, "Save\\Multiplayer\\" + m_SaveGame->GetFileNameNoPath( ), m_SaveGame->GetMagicNumber( ), 12, 12, m_HostPort, FixedHostCounter, m_EntryKey ) );
					}
					else
					{
						// note: the PrivateGame flag is not set when broadcasting to LAN (as you might expect)
						// note: we do not use m_Map->GetMapGameType because none of the filters are set when broadcasting to LAN (also as you might expect)

						uint32_t MapGameType = MAPGAMETYPE_UNKNOWN0;
						m_GHost->m_UDPSocket->SendTo( IP, Port, m_Protocol->SEND_W3GS_GAMEINFO( m_GHost->m_TFT, m_GHost->m_LANWar3Version, UTIL_CreateByteArray( MapGameType, false ), m_Map->GetMapGameFlags( ), m_Map->GetMapWidth( ), m_Map->GetMapHeight( ), m_GameName, "|cff00ff00EN0T", GetTime( ) - m_CreationTime, m_Map->GetMapPath( ), m_Map->GetMapCRC( ), 12, 12, m_HostPort, FixedHostCounter, m_EntryKey ) );
					}
				}
			}

			//
			// !SP
			//

			else if( Command == "sp" && !m_CountDownStarted )
			{
				SendAllChat( m_GHost->m_Language->ShufflingPlayers( ) );
				ShuffleSlots( );
			}

			//
			// !START
			//

			else if( Command == "start" && !m_CountDownStarted )
			{
				// if the player sent "!start force" skip the checks and start the countdown
				// otherwise check that the game is ready to start

				if( Payload == "force" )
					StartCountDown( true );
				else
				{
					if( GetTicks( ) - m_LastPlayerLeaveTicks >= 2000 )
						StartCountDown( false );
					else
						SendAllChat( m_GHost->m_Language->CountDownAbortedSomeoneLeftRecently( ) );
				}
			}

			//
			// !SWAP (swap slots)
			//

			else if( Command == "swap" && !Payload.empty( ) && !m_GameLoading && !m_GameLoaded )
			{
				uint32_t SID1;
				uint32_t SID2;
				std::stringstream SS;
				SS << Payload;
				SS >> SID1;

				if( SS.fail( ) )
					CONSOLE_Print( "[GAME: " + m_GameName + "] bad input #1 to swap command" );
				else
				{
					if( SS.eof( ) )
						CONSOLE_Print( "[GAME: " + m_GameName + "] missing input #2 to swap command" );
					else
					{
						SS >> SID2;

						if( SS.fail( ) )
							CONSOLE_Print( "[GAME: " + m_GameName + "] bad input #2 to swap command" );
						else
							SwapSlots( (unsigned char)( SID1 - 1 ), (unsigned char)( SID2 - 1 ) );
					}
				}
			}

			//
			// !SYNCLIMIT
			//

			else if( Command == "synclimit" )
			{
				if( Payload.empty( ) )
					SendAllChat( m_GHost->m_Language->SyncLimitIs( std::to_string( m_SyncLimit ) ) );
				else
				{
					m_SyncLimit = UTIL_ToUInt32( Payload );

					if( m_SyncLimit <= 10 )
					{
						m_SyncLimit = 10;
						SendAllChat( m_GHost->m_Language->SettingSyncLimitToMinimum( "10" ) );
					}
					else if( m_SyncLimit >= 10000 )
					{
						m_SyncLimit = 10000;
						SendAllChat( m_GHost->m_Language->SettingSyncLimitToMaximum( "10000" ) );
					}
					else
						SendAllChat( m_GHost->m_Language->SettingSyncLimitTo( std::to_string( m_SyncLimit ) ) );
				}
			}

			//
			// !UNHOST
			//

			else if( Command == "unhost" && !m_CountDownStarted )
				m_Exiting = true;

			//
			// !UNLOCK
			//

			else if( Command == "unlock" && ( RootAdminCheck || IsOwner( User ) ) )
			{
				SendAllChat( m_GHost->m_Language->GameUnlocked( ) );
				m_Locked = false;
			}

			//
			// !UNMUTE
			//
			#ifdef GHOST_MUTE
			else if( Command == "unmute"  || Command == "u" )
			{
				CGamePlayer *LastMatch = NULL;
				uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

				if( Matches == 0 )
					SendAllChat( m_GHost->m_Language->UnableToMuteNoMatchesFound( Payload ) );
				else if( Matches == 1 )
				{
					SendAllChat( m_GHost->m_Language->UnmutedPlayer( LastMatch->GetName( ), User ) );
					LastMatch->SetMuted( false );
				}
				else
					SendAllChat( m_GHost->m_Language->UnableToMuteFoundMoreThanOneMatch( Payload ) );
			}

			//
			// !UNMUTEALL
			//

			else if( (Command == "unmuteall" || Command == "ua")  && m_GameLoaded )
			{
				SendAllChat( m_GHost->m_Language->GlobalChatUnmuted( ) );
				m_MuteAll = false;
			}
			#endif
			//
			// !VIRTUALHOST
			//

			else if( Command == "virtualhost" && !Payload.empty( ) && Payload.size( ) <= 15 && !m_CountDownStarted )
			{
				DeleteVirtualHost( );
				m_VirtualHostName = Payload;
			}

			//
			// !VOTECANCEL
			//

			else if( Command == "votecancel" && !m_KickVotePlayer.empty( ) )
			{
				SendAllChat( m_GHost->m_Language->VoteKickCancelled( m_KickVotePlayer ) );
				m_KickVotePlayer.clear( );
				m_StartedKickVoteTime = 0;
			}

			//
			// !W
			//

			else if( Command == "w" && !Payload.empty( ) )
			{
				// extract the name and the message
				// e.g. "Varlock hello there!" -> name: "Varlock", message: "hello there!"

				std::string Name;
				std::string Message;
				std::string :: size_type MessageStart = Payload.find( " " );

				if( MessageStart != std::string :: npos )
				{
					Name = Payload.substr( 0, MessageStart );
					Message = Payload.substr( MessageStart + 1 );

					for( std::vector<CBNET *> :: iterator i = m_GHost->m_BNETs.begin( ); i != m_GHost->m_BNETs.end( ); ++i )
						(*i)->QueueChatCommand( Message, Name, true );
				}

				HideCommand = true;
			}
		}
		else
		{
			CONSOLE_Print( "[GAME: " + m_GameName + "] admin command ignored, the game is locked" );
			SendChat( player, m_GHost->m_Language->TheGameIsLocked( ) );
		}
	}
	else
	{
		if( !player->GetSpoofed( ) )
			CONSOLE_Print( "[GAME: " + m_GameName + "] non-spoofchecked user [" + User + "] sent command [" + Command + "] with payload [" + Payload + "]" );
		else
			CONSOLE_Print( "[GAME: " + m_GameName + "] non-admin [" + User + "] sent command [" + Command + "] with payload [" + Payload + "]" );
	}

	/*********************
	* NON ADMIN COMMANDS *
	*********************/

	//
	// !CHECKME
	//

	if( Command == "checkme" )
		SendChat( player, m_GHost->m_Language->CheckedPlayer( User, player->GetNumPings( ) > 0 ? std::to_string( player->GetPing( m_GHost->m_LCPings ) ) + "ms" : "N/A", player->GetCountry(), AdminCheck || RootAdminCheck ? "Yes" : "No", IsOwner( User ) ? "Yes" : "No", player->GetSpoofed( ) ? "Yes" : "No", player->GetSpoofedRealm( ).empty( ) ? "N/A" : player->GetSpoofedRealm( ), player->GetReserved( ) ? "Yes" : "No" ) );

		
	//
	// !!IGNORE
	//
/*
	else if( Command == "ignore"  || Command == "i" )
	{
		CGamePlayer *LastMatch = NULL;
		uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

		if( Matches == 0 )
			SendChat(player, "Невозможно добавить пользователя в игнор лист. Пользователь не обнаружен" );
		else if( Matches == 1 )
		{
			if (player == LastMatch){
				SendChat(player, "Вы не можете добавить себя в игнор-лист" );
			}
			else{
				SendChat(player, "Вы добавили "+LastMatch->GetName()+" в игнор-лист" );
				LastMatch->SetIgnoredBy(player);
			}
		}
		else
			SendChat(player, "Невозможно точно определить пользователя. Пожалуйста уточните...");
	}
	
	//
	// !!IGNOREALL
	//

	else if( Command == "ignoreall"  || Command == "ia" )
	{
		for (auto ignoredPlayer:m_Players){
			if (ignoredPlayer != player)
				ignoredPlayer->SetIgnoredBy(player);
		}
		SendChat(player, "Все пользователи добавлены в игнор-лист");
	}
	//
	// !UNIGNORE
	//
	else if( Command == "unignore" || Command == "un")
	{
		CGamePlayer *LastMatch = NULL;
		uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

		if( Matches == 0 )
			SendChat(player, "Невозможно удалить пользователя из игнор листа. Пользователь не обнаружен" );
		else if( Matches == 1 )
		{
			SendChat(player, "Вы удалили "+LastMatch->GetName()+" из игнор-листа" );
			LastMatch->DelIgnoredBy(player);
		}
		else
			SendChat(player, "Невозможно точно определить пользователя. Пожалуйста уточните...");
	}
	
	//
	// !UNIGNOREALL
	//
	else if( Command == "unignoreall" || Command == "una")
	{
		for (auto probablyIgnoredPlayer:m_Players){
			probablyIgnoredPlayer->DelIgnoredBy(player);	
		}
		SendChat(player, "Игнор-лист очищен");
	}
	*/

	// Non Admin ping
	// !PING
	//

	else if( (!(AdminCheck ||RootAdminCheck)) && (Command == "ping" || Command == "p"))
	{
		std::string Pings;
		if( player->GetNumPings( ) > 0 )
		{
			Pings += std::to_string( player->GetPing( m_GHost->m_LCPings ) );

			Pings += "ms Average => ";
			Pings += std::to_string( player->GetAveragePing( m_GHost->m_LCPings ) );
			Pings += "ms ";
		}
		else
			Pings += "N/A";
		
		SendChat(player, Pings );
		Pings.clear( );
	
	}


	//
	// !CALCULATE
	//

/*	else if( Command == "calculate"|| Command == "calc" || Command == "c" )
	{
		if( Payload.empty( ) ){

			SendChat(player, "Calculates value of expression");
			SendChat(player, "calc usage:");
			SendChat(player, "calc <expression>");
		}
		else
		{
						
			std::ostringstream str_stream;
			str_stream <<  calculate(Payload);
			std::string value_str = str_stream.str();

			SendChat(player, "|CFF00FF00"+ value_str+"|r");				
		}
	}
*/	
	//
	// !STATS
	//
	else if( Command == "stats" && GetTime( ) - player->GetStatsSentTime( ) >= 5 )
	{
		std::string StatsUser = User;

		if( !Payload.empty( ) )
			StatsUser = Payload;

		if( player->GetSpoofed( ) && ( AdminCheck || RootAdminCheck || IsOwner( User ) ) )
			m_PairedGPSChecks.push_back( PairedGPSCheck( std::string( ), m_GHost->m_DB->ThreadedGamePlayerSummaryCheck( StatsUser ) ) );
		else
			m_PairedGPSChecks.push_back( PairedGPSCheck( User, m_GHost->m_DB->ThreadedGamePlayerSummaryCheck( StatsUser ) ) );

		player->SetStatsSentTime( GetTime( ) );
	}

	//
	// !STATSDOTA
	// !SD
	//

	else if( (Command == "statsdota" || Command == "sd") && GetTime( ) - player->GetStatsDotASentTime( ) >= 5 )
	{
		std::string StatsUser = User;

		if( !Payload.empty( ) )
			StatsUser = Payload;

		if( player->GetSpoofed( ) && ( AdminCheck || RootAdminCheck || IsOwner( User ) ) )
			m_PairedDPSChecks.push_back( PairedDPSCheck( std::string( ), m_GHost->m_DB->ThreadedDotAPlayerSummaryCheck( StatsUser ) ) );
		else
			m_PairedDPSChecks.push_back( PairedDPSCheck( User, m_GHost->m_DB->ThreadedDotAPlayerSummaryCheck( StatsUser ) ) );

		player->SetStatsDotASentTime( GetTime( ) );
	}

	//
	// !STATSLIA
	// !SL
	//
#ifdef STATS_LIA_GHOST
	else if( (Command == "statslia" || Command == "sl") && GetTime( ) - player->GetStatsLiASentTime( ) >= 5 )
	{
		std::string StatsUser = User;

		if( !Payload.empty( ) )
			StatsUser = Payload;

		if( player->GetSpoofed( ) && ( AdminCheck || RootAdminCheck || IsOwner( User ) ) )
			m_PairedLPSChecks.push_back( PairedLPSCheck( std::string( ), m_GHost->m_DB->ThreadedLiAPlayerSummaryCheck( StatsUser ) ) );
		else
			m_PairedLPSChecks.push_back( PairedLPSCheck( User, m_GHost->m_DB->ThreadedLiAPlayerSummaryCheck( StatsUser ) ) );

		player->SetStatsLiASentTime( GetTime( ) );
	}
#endif

	// !VOTESTART
    //
   
    else if( Command == "votestart" || Command == "go"|| Command == "го") 
    {
        bool votestartAuth = player->GetSpoofed( ) && ( AdminCheck || RootAdminCheck || IsOwner( User ) );
        bool votestartAutohost = m_GameState == GAME_PUBLIC && !m_GHost->m_AutoHostGameName.empty( ) && m_GHost->m_AutoHostMaximumGames != 0 && m_GHost->m_AutoHostAutoStartPlayers != 0 && m_AutoStartPlayers != 0;
 
        if( m_GHost->m_VoteStartAllowed && !m_CountDownStarted && (votestartAuth || votestartAutohost || !m_GHost->m_VoteStartAutohostOnly) )
        {
            if( m_GHost->m_CurrentGame->GetLocked( ) )
            {
                SendChat( player, " Игра заблокирована. Запуск игры голосованием невозможен. Владелец " + m_OwnerName );
                return HideCommand;
            }
 
 
            if(m_StartedVoteStartTime == 0)
            { //need >minplayers or admin to START a votestart
                if (GetNumHumanPlayers() < m_GHost->m_VoteStartMinPlayers && !votestartAuth)
                { //need at least eight players to votestart
			SendChat( player, "Для голосования нужно → [" + std::to_string(m_GHost->m_VoteStartMinPlayers) + "] или более игроков!" );
			return HideCommand;
                }
 
                for( std::vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
                    (*i)->SetStartVote( false );
 
                m_StartedVoteStartTime = GetTime();
           
                CONSOLE_Print( "[Игра: " + m_GameName + "] голосование создано игроком [" + User + "]" );
            }
 
            player->SetStartVote(true);
               
            uint32_t VotesNeeded;
            uint32_t Votes = 0;
 
 
            for( std::vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
            {
                if( (*i)->GetStartVote( ) )
                    ++Votes;
            }
 
            
			if( m_GHost->m_VoteStartPercentalVoting)
			{
                VotesNeeded = ((uint32_t) (GetNumHumanPlayers() *  (m_GHost->m_VoteStartPercent - 1) / 100)) + 1;
            }
            else
            {
                VotesNeeded = m_GHost->m_VoteStartMinPlayers;
            }
       
            if( Votes < VotesNeeded )
            {
            //    std::string uname = m_NoName ? GetNoName( player->GetPID() ) : User;
                SendAllChat("[ " + player->GetName( ) + " ]  Проголосовал " + std::to_string( Votes )+"/"+ std::to_string(VotesNeeded)+ " необходимо для начала, регистрировать голос " + std::string( 1, m_GHost->m_CommandTrigger )+"го или " + std::string( 1, m_GHost->m_CommandTrigger )+ "go.");
            }
            else
            {
                StartCountDown( true );
            }
 
        }
    }
 	//
	// !BUG (Added by Kira_o_O)
	//
#ifdef GHOST_DISCORD

	else if( Command == "bug"){
		if (Payload.empty( )){
			SendChat(player, "Error! Bug message not provided. Usage : !bug <message>" );
		}
		else {
			discord_bug_message(m_GHost->m_discord_bug_webhook_url, GetGameName(), player->GetName(), Payload);
			SendChat(player, "Bug reported! Thnx :)" );			
		}

	}
#endif
 	//
	// !VOTEKICK
	//
	else if( (Command == "votekick"||Command == "vk") && m_GHost->m_VoteKickAllowed && !Payload.empty( ) )
	{
		if( !m_KickVotePlayer.empty( ) )
			SendChat( player, m_GHost->m_Language->UnableToVoteKickAlreadyInProgress( ) );
		else if( m_Players.size( ) == 2 )
			SendChat( player, m_GHost->m_Language->UnableToVoteKickNotEnoughPlayers( ) );
		else
		{
			CGamePlayer *LastMatch = NULL;
			uint32_t Matches = GetPlayerFromNamePartial( Payload, &LastMatch );

			if( Matches == 0 )
				SendChat( player, m_GHost->m_Language->UnableToVoteKickNoMatchesFound( Payload ) );
			else if( Matches == 1 )
			{
				if( LastMatch->GetReserved( ) )
					SendChat( player, m_GHost->m_Language->UnableToVoteKickPlayerIsReserved( LastMatch->GetName( ) ) );
				else
				{
					m_KickVotePlayer = LastMatch->GetName( );
					m_StartedKickVoteTime = GetTime( );

					for( std::vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
						(*i)->SetKickVote( false );

					player->SetKickVote( true );
					CONSOLE_Print( "[GAME: " + m_GameName + "] votekick against player [" + m_KickVotePlayer + "] started by player [" + User + "]" );
					SendAllChat( m_GHost->m_Language->StartedVoteKick( LastMatch->GetName( ), User, std::to_string( (uint32_t)ceil( ( GetNumHumanPlayers( ) - 1 ) * (float)m_GHost->m_VoteKickPercentage / 100 ) - 1 ) ) );
					SendAllChat( m_GHost->m_Language->TypeYesToVote( std::string( 1, m_GHost->m_CommandTrigger ) ) );
				}
			}
			else
				SendChat( player, m_GHost->m_Language->UnableToVoteKickFoundMoreThanOneMatch( Payload ) );
		}
	}

	//
	// !YES
	//

	else if( Command == "yes" && !m_KickVotePlayer.empty( ) && player->GetName( ) != m_KickVotePlayer && !player->GetKickVote( ) )
	{
		player->SetKickVote( true );
		uint32_t VotesNeeded = (uint32_t)ceil( ( GetNumHumanPlayers( ) - 1 ) * (float)m_GHost->m_VoteKickPercentage / 100 );
		uint32_t Votes = 0;

		for( std::vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
		{
			if( (*i)->GetKickVote( ) )
				++Votes;
		}

		if( Votes >= VotesNeeded )
		{
			CGamePlayer *Victim = GetPlayerFromName( m_KickVotePlayer, true );

			if( Victim )
			{
				Victim->SetDeleteMe( true );
				Victim->SetLeftReason( m_GHost->m_Language->WasKickedByVote( ) );

				if( !m_GameLoading && !m_GameLoaded )
					Victim->SetLeftCode( PLAYERLEAVE_LOBBY );
				else
					Victim->SetLeftCode( PLAYERLEAVE_LOST );

				if( !m_GameLoading && !m_GameLoaded )
					OpenSlot( GetSIDFromPID( Victim->GetPID( ) ), false );

				CONSOLE_Print( "[GAME: " + m_GameName + "] votekick against player [" + m_KickVotePlayer + "] passed with " + std::to_string( Votes ) + "/" + std::to_string( GetNumHumanPlayers( ) ) + " votes" );
				SendAllChat( m_GHost->m_Language->VoteKickPassed( m_KickVotePlayer ) );
			}
			else
				SendAllChat( m_GHost->m_Language->ErrorVoteKickingPlayer( m_KickVotePlayer ) );

			m_KickVotePlayer.clear( );
			m_StartedKickVoteTime = 0;
		}
		else
			SendAllChat( m_GHost->m_Language->VoteKickAcceptedNeedMoreVotes( m_KickVotePlayer, User, std::to_string( VotesNeeded - Votes ) ) );
	}

	return HideCommand;
}

void CGame :: EventGameStarted( )
{
	CBaseGame :: EventGameStarted( );

	// record everything we need to ban each player in case we decide to do so later
	// this is because when a player leaves the game an admin might want to ban that player
	// but since the player has already left the game we don't have access to their information anymore
	// so we create a "potential ban" for each player and only store it in the database if requested to by an admin

	for( std::vector<CGamePlayer *> :: iterator i = m_Players.begin( ); i != m_Players.end( ); ++i )
		m_DBBans.push_back( new CDBBan( (*i)->GetJoinedRealm( ), (*i)->GetName( ), (*i)->GetExternalIPString( ), std::string( ), std::string( ), std::string( ), std::string( ) ) );
}

bool CGame :: IsGameDataSaved( )
{
	return m_CallableGameAdd && m_CallableGameAdd->GetReady( );
}

void CGame :: SaveGameData( )
{
	CONSOLE_Print( "[GAME: " + m_GameName + "] saving game data to database" );
	m_CallableGameAdd = m_GHost->m_DB->ThreadedGameAdd( m_GHost->m_BNETs.size( ) == 1 ? m_GHost->m_BNETs[0]->GetServer( ) : std::string( ), m_DBGame->GetMap( ), m_GameName, m_OwnerName, m_GameTicks / 1000, m_GameState, m_CreatorName, m_CreatorServer );
}
