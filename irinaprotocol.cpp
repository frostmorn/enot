
#include "ghost.h"
#include "irinaprotocol.h"
// #include "base64.h"


CIrinaProtocol::CIrinaProtocol( )
{
}


CIrinaProtocol::~CIrinaProtocol( )
{
}

CIncomingIrinaPackage * CIrinaProtocol::IRINA_ParseJSONData( std::string jsonPayload )
{
	try
	{
		json payload = json::parse( jsonPayload );

		if ( payload["type"] == IRINA_AUTH_TOKEN )
			return IRINA_ParseAuthToken( payload["data"] );
		else if ( payload["type"] == IRINA_CREATE_GAME )
			return IRINA_ParseCreateGame( payload["data"] );
		else if ( payload["type"] == IRINA_UNHOST_GAME )
			return IRINA_ParseUnhostGame( payload["data"] );
		else if ( payload["type"] == IRINA_UPDATE_SLOT )
			return IRINA_ParseUpdateSlots( payload["data"] );
		else
			return NULL;
	}
	catch ( nlohmann::detail::parse_error e )
	{
		CONSOLE_Print( "[IRINA] AUTH_TOKEN json error." );
	}
	catch ( nlohmann::detail::type_error e )
	{
		CONSOLE_Print( "[IRINA] AUTH_TOKEN json types error." );
	}

	return NULL;
}

std::string CIrinaProtocol::IRINA_CreateAuthToken( CIncomingIrinaAuthToken * data )
{
	json result;
	result["type"] = IRINA_AUTH_TOKEN;
	result["data"]["token"] = data->GetToken( );

	return result.dump( );
}

std::string CIrinaProtocol::IRINA_CreateCreateGame( CIncomingIrinaCreateGame * data )
{
	json result;
	result["type"] = IRINA_CREATE_GAME;
	result["data"]["gameName"] = data->GetGameName( );
	
	result["data"]["hostIP"] = data->GetHostIP( );
	result["data"]["hostPort"] = data->GetHostPort( );
	result["data"]["hostCounterID"] = data->GetHostCounterID( );
	result["data"]["entryKey"] = data->GetEntryKey( );

	result["data"]["maplocalpath"] = data->GetMapLocalPath( );
	result["data"]["mapconfig"] = data->GetMapConfig( );
	
	result["data"]["players"] = data->GetPlayers( );
	result["data"]["description"] = data->GetDescription( );
	result["data"]["mapName"] = data->GetMapName( );

	result["data"]["iccup"] = data->GetIccup( );
	result["data"]["position"] = data->GetPosition( );

	std::vector<CIncomingIrinaSlot> slots = data->GetMapSlots( );

	for ( auto i = slots.begin( ); i != slots.end( ); ++i )
	{
		json slotData;
		slotData["sid"] = i->GetSID( );
		slotData["playerName"] = i->GetPlayerName( );
		slotData["playerRealm"] = i->GetPlayerName( );

		result["data"]["slots"].push_back( slotData );
	}

	return result.dump( );
}

std::string CIrinaProtocol::IRINA_CreateUnhostGame( CIncomingIrinaUnhostGame * data )
{
	json result;
	result["type"] = IRINA_UNHOST_GAME;
	result["data"]["entryKey"] = data->GetEntryKey( );

	return result.dump( );
}

std::string CIrinaProtocol::IRINA_CreateUpdateSlots( CIncomingIrinaUpdateSlots * data )
{
	json result;
	result["type"] = IRINA_UPDATE_SLOT;

	auto slots = data->GetSlots( );

	for ( auto i = slots.begin( ); i != slots.end( ); ++i )
	{
		json slotData;
		slotData["sid"] = i->GetSID( );
		slotData["playerName"] = i->GetPlayerName( );
		slotData["playerRealm"] = i->GetPlayerName( );

		result["data"]["slots"].push_back( slotData );
	}

	result["data"]["entryKey"] = data->GetEntryKey( );

	return result.dump( );
}

std::string CIrinaProtocol::IRINA_CreateMarkAsStarted( CIncomingIrinaMarkAsStarted * data )
{
	json result;
	result["type"] = IRINA_UPDATE_SLOT;
	result["data"]["entryKey"] = data->GetEntryKey( );

	return result.dump( );
}

CIncomingIrinaAuthToken * CIrinaProtocol::IRINA_ParseAuthToken( json data )
{
	try
	{
		std::string token = data["token"];

		return new CIncomingIrinaAuthToken( token );
	}
	catch ( nlohmann::detail::parse_error e )
	{
		CONSOLE_Print( "[IRINA] AUTH_TOKEN json error." );
	}
	catch ( nlohmann::detail::type_error e )
	{
		CONSOLE_Print( "[IRINA] AUTH_TOKEN json types error." );
	}

	return NULL;
}

CIncomingIrinaCreateGame * CIrinaProtocol::IRINA_ParseCreateGame( json data )
{
	try
	{
		std::string gameName = data["gameName"];
		std::string hostIP = data["hostIP"];
		uint16_t hostPort = data["hostPort"];
		uint32_t hostCounterID = data["hostCounterID"];
		uint32_t entryKey = data["entryKey"];

		std::string mapLocalPath = data["maplocalpath"];
		std::string mapConfig = data["mapconfig"];

		std::string players = data["players"];
		std::string description = data["description"];
		std::string mapName = data["mapName"];

		std::string iccup = data["iccup"];
		char position = (uint32_t)data["position"];

		std::vector<CIncomingIrinaSlot> slots;

		for ( auto i = data["slots"].items( ).begin( ); i != data["slots"].items( ).end( ); ++i )
		{
			json slotData = i.value( );

			std::string playerName = slotData["playerName"];
			std::string playerRealm = slotData["playerRealm"];
			uint32_t sid = slotData["sid"];
			uint32_t colour = slotData["colour"];

			CIncomingIrinaSlot slot( sid, playerName, playerRealm, colour );

			slots.push_back( slot );
		}

		return new CIncomingIrinaCreateGame( gameName, slots, hostIP, hostPort, hostCounterID, entryKey, mapLocalPath, mapConfig, players, description, mapName, iccup, position  );

	}
	catch ( nlohmann::detail::parse_error e )
	{
		CONSOLE_Print( "[IRINA] AUTH_TOKEN json error." );
	}
	catch ( nlohmann::detail::type_error e )
	{
		CONSOLE_Print( "[IRINA] AUTH_TOKEN json types error." );
	}

	return NULL;
}

CIncomingIrinaUnhostGame * CIrinaProtocol::IRINA_ParseUnhostGame( json data )
{
	try
	{
		uint32_t entryKey = data["entryKey"];

		return new CIncomingIrinaUnhostGame( entryKey );
	}
	catch ( nlohmann::detail::parse_error e )
	{
		CONSOLE_Print( "[IRINA] AUTH_TOKEN json error." );
	}
	catch ( nlohmann::detail::type_error e )
	{
		CONSOLE_Print( "[IRINA] AUTH_TOKEN json types error." );
	}

	return NULL;
}

CIncomingIrinaUpdateSlots * CIrinaProtocol::IRINA_ParseUpdateSlots( json data )
{
	try
	{
		std::vector<CIncomingIrinaSlot> slots;

		uint32_t entryKey = data["entryKey"];

		for ( auto i = data["slots"].items( ).begin( ); i != data["slots"].items( ).end( ); ++i )
		{
			json slotData = i.value( );

			std::string playerName = slotData["playerName"];
			std::string playerRealm = slotData["playerRealm"];
			uint32_t sid = slotData["colour"];
			uint32_t colour = slotData["colour"];

			CIncomingIrinaSlot slot( sid, playerName, playerRealm, colour );

			slots.push_back( slot );
		}

		return new CIncomingIrinaUpdateSlots( slots, entryKey );
	}
	catch ( nlohmann::detail::parse_error e )
	{
		CONSOLE_Print( "[IRINA] AUTH_TOKEN json error." );
	}
	catch ( nlohmann::detail::type_error e )
	{
		CONSOLE_Print( "[IRINA] AUTH_TOKEN json types error." );
	}

	return NULL;
}

CIncomingIrinaMarkAsStarted * CIrinaProtocol::IRINA_ParseMarkAsStarted( json data )
{
	try
	{
		std::vector<CIncomingIrinaSlot> slots;

		uint32_t entryKey = data["entryKey"];


		return new CIncomingIrinaMarkAsStarted( entryKey );
	}
	catch ( nlohmann::detail::parse_error e )
	{
		CONSOLE_Print( "[IRINA] AUTH_TOKEN json error." );
	}
	catch ( nlohmann::detail::type_error e )
	{
		CONSOLE_Print( "[IRINA] AUTH_TOKEN json types error." );
	}

	return NULL;
}
