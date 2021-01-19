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

#include "ghost.h"
#include "util.h"
#include "ghostdb.h"
#include "gameplayer.h"
#include "gameprotocol.h"
#include "game_base.h"
#include "stats.h"
#include "statslia.h"

//
// CStatsLiA
//

CStatsLiA :: CStatsLiA( CBaseGame *nGame ) : CStats( nGame )/*, m_Winner( 0 ), m_Min( 0 ), m_Sec( 0 )*/
{
	CONSOLE_Print( "[STATSLIA] using lia stats" );

	for( unsigned int i = 0; i < 8; ++i )
		m_Players[i] = NULL;
}

CStatsLiA :: ~CStatsLiA( )
{
	for( unsigned int i = 0; i < 8; ++i )
	{
		if( m_Players[i] )
			delete m_Players[i];
	}
}

bool CStatsLiA :: ProcessAction( CIncomingAction *Action )
{
	unsigned int i = 0;
	BYTEARRAY *ActionData = Action->GetAction( );
	BYTEARRAY Data;
	BYTEARRAY Key;
	BYTEARRAY Value;

	// dota actions with real time replay data start with 0x6b then the null terminated std::string "dr.x"
	// unfortunately more than one action can be sent in a single packet and the length of each action isn't explicitly represented in the packet
	// so we have to either parse all the actions and calculate the length based on the type or we can search for an identifying sequence
	// parsing the actions would be more correct but would be a lot more difficult to write for relatively little gain
	// so we take the easy route (which isn't always guaranteed to work) and search the data for the sequence "6b 64 72 2e 78 00" and hope it identifies an action

	while( ActionData->size( ) >= i + 6 )
	{
		if( (*ActionData)[i] == 0x6b &&
			(*ActionData)[i + 1] == 0x4c &&
			(*ActionData)[i + 2] == 0x69 &&
			(*ActionData)[i + 3] == 0x41 &&
			(*ActionData)[i + 4] == 0x73 &&
			(*ActionData)[i + 5] == 0x00 )
		{
			// we think we've found an action with real time replay data (but we can't be 100% sure)
			// next we parse out two null terminated strings and a 4 byte integer

			if( ActionData->size( ) >= i + 7 )
			{
				// the first null terminated std::string should either be the strings "Data" or "Global" or a player id in ASCII representation, e.g. "1" or "2"

				Data = UTIL_ExtractCString( *ActionData, i + 6 );
				auto DataDebug = UTIL_ExtractCString( *ActionData, 0 );
			
				
				if( ActionData->size( ) >= i + 8 + Data.size( ) )
				{
					// the second null terminated std::string should be the key

					Key = UTIL_ExtractCString( *ActionData, i + 7 + Data.size( ) );

					if( ActionData->size( ) >= i + 12 + Data.size( ) + Key.size( ) )
					{
						// the 4 byte integer should be the value

						Value = BYTEARRAY( ActionData->begin( ) + i + 8 + Data.size( ) + Key.size( ), ActionData->begin( ) + i + 12 + Data.size( ) + Key.size( ) );
						std::string DataString = std::string( Data.begin( ), Data.end( ) );
						std::string KeyString = std::string( Key.begin( ), Key.end( ) );
						uint32_t ValueInt = UTIL_ByteArrayToUInt32( Value, false );

						// game end on eog
						// next str represents our way to fuck onligamez of.
						// Some characters in map will be cyrillic, so, we will make their debug happy STATS[TAT] EOG[O]
						if (DataString == "STATS" || DataString == "SТАТS"){
							if (KeyString == "EOG" || KeyString == "EОG"){
								m_GameResult = ValueInt;
								if (ValueInt == 0){
									CONSOLE_Print( "[STATS]EOG, Lose" );

								}
								else 
								{
									CONSOLE_Print( "[STATS]EOG, Victory" );
								}					
								return true;
							} 
							// PTS Data
							else {
								auto index = UTIL_ToUInt32(KeyString);
								if (index < 8){
									if (m_Players[index] == NULL){
										m_Players[index] = new CDBLiAPlayer( );
									}
									m_Players[index]->SetPTS(ValueInt);
								}
							}
						}
						else if (DataString.find("DEBUG") != std::string::npos)
						{
							
							CONSOLE_Print( "[MAPDEBUG:"+m_Game->GetGameName()+"/CREATION TIMESTAMP:" + UTIL_ToString(m_Game->GetCreationTime())+"] " + DataString + ", " + KeyString + ", " + UTIL_ToString( ValueInt ) );
						}

						i += 12 + Data.size( ) + Key.size( );
					}
					else
						++i;
				}
				else
					++i;
			}
			else
				++i;
		}
		else
			++i;
	}

	return false;
}

void CStatsLiA :: Save( CGHost *GHost, CGHostDB *DB, uint32_t GameID )
{
	if( DB->Begin( ) )
	{
	// 	// since we only record the end game information it's possible we haven't recorded anything yet if the game didn't end with a tree/throne death
	// 	// this will happen if all the players leave before properly finishing the game
	// 	// the dotagame stats are always saved (with winner = 0 if the game didn't properly finish)
	// 	// the dotaplayer stats are only saved if the game is properly finished

		unsigned int Players = 0;

	// 	// save the dotagame

		GHost->m_Callables.push_back( DB->ThreadedLiAGameAdd( GameID, m_GameResult, m_Min, m_Sec ) );

	// 	// check for invalid colours and duplicates
	// 	// this can only happen if DotA sends us garbage in the "id" value but we should check anyway

		// for( unsigned int i = 0; i < 12; ++i )
		// {
		// 	if( m_Players[i] )
		// 	{
		// 		uint32_t Colour = m_Players[i]->GetColour( );

		// 		if( !( ( Colour >= 1 && Colour <= 5 ) || ( Colour >= 7 && Colour <= 11 ) ) )
		// 		{
		// 			CONSOLE_Print( "[STATSDOTA: " + m_Game->GetGameName( ) + "] discarding player data, invalid colour found" );
		// 			DB->Commit( );
		// 			return;
		// 		}

		// 		for( unsigned int j = i + 1; j < 12; ++j )
		// 		{
		// 			if( m_Players[j] && Colour == m_Players[j]->GetColour( ) )
		// 			{
		// 				CONSOLE_Print( "[STATSDOTA: " + m_Game->GetGameName( ) + "] discarding player data, duplicate colour found" );
		// 				DB->Commit( );
		// 				return;
		// 			}
		// 		}
		// 	}
		// }

	// 	// save the liaplayers

		for( unsigned int i = 0; i < 8; ++i )
		{
			if( m_Players[i] )
			{
				GHost->m_Callables.push_back( DB->ThreadedLiAPlayerAdd( GameID, m_Players[i]->GetColour(), m_Players[i]->GetPTS(),
				m_Players[i]->GetDeaths(), m_Players[i]->GetCreepKills(), m_Players[i]->GetBossKills(), 
				m_Players[i]->GetItem(0),m_Players[i]->GetItem(1),m_Players[i]->GetItem(2),m_Players[i]->GetItem(3),m_Players[i]->GetItem(4),m_Players[i]->GetItem(5),
				m_Players[i]->GetHero()));
				++Players;
			}
		}

		if( DB->Commit( ) )
			CONSOLE_Print( "[STATSLIA: " + m_Game->GetGameName( ) + "] saving " + UTIL_ToString( Players ) + " players" );
		else
			CONSOLE_Print( "[STATSLIA: " + m_Game->GetGameName( ) + "] unable to commit database transaction, data not saved" );
	}
	else
		CONSOLE_Print( "[STATSLIA: " + m_Game->GetGameName( ) + "] unable to begin database transaction, data not saved" );
}
