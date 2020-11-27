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

#ifndef STATSLIA_H
#define STATSLIA_H

#define LIA_GAME_RESULT_LOSE            0
#define LIA_GAME_RESULT_WIN             1

// NOT_IMPLEMENTED. Need for -b mode
#define LIA_GAME_RESULT_COMMAND_1_WIN   2
#define LIA_GAME_RESULT_COMMAND_2_WIN   3

//
// CStatsDOTA
//

class CDBLiAPlayer;

class CStatsLiA : public CStats
{
private:
	CDBLiAPlayer *m_Players[8];
    
	// uint32_t m_Winner;
    uint32_t m_GameResult;
	uint32_t m_Min;
	uint32_t m_Sec;

public:
	CStatsLiA( CBaseGame *nGame );
	virtual ~CStatsLiA( );

	virtual bool ProcessAction( CIncomingAction *Action );
	virtual void Save( CGHost *GHost, CGHostDB *DB, uint32_t GameID );
};

#endif
