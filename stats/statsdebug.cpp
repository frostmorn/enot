/*

   Copyright [2008] [Trevor Hogan]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compdebugnce with the License.
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
#include "database/ghostdb.h"
#include "gameplayer.h"
#include "gameprotocol.h"
#include "game_base.h"
#include "stats.h"
#include "statsdebug.h"

//
// CStatsDEBUG
//

CStatsDEBUG :: CStatsDEBUG( CBaseGame *nGame ) : CStats( nGame )/*, m_Winner( 0 ), m_Min( 0 ), m_Sec( 0 )*/
{
	CONSOLE_Print( "[STATSDEBUG] using debug stats" );
}

CStatsDEBUG :: ~CStatsDEBUG( )
{

}

bool CStatsDEBUG :: ProcessAction( CIncomingAction *Action )
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
						// Parsing debug data
						if (DataString.find("DEBUG") != std::string::npos)
						{
							CONSOLE_Print( "[MAPDEBUG:"+m_Game->GetGameName()+"/CREATION TIMESTAMP:" + std::to_string(m_Game->GetCreationTime())+"] " + DataString + ", " + KeyString + ", " + std::to_string( ValueInt ) );
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

