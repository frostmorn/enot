#ifndef IRINA_PROTOCOL_H
#define IRINA_PROTOCOL_H

#define IRINA_AUTH_TOKEN 1
#define IRINA_CREATE_GAME 2
#define IRINA_UNHOST_GAME 3
#define IRINA_UPDATE_SLOT 4
#define IRINA_MARK_AS_STARTED 5

class CIncomingIrinaPackage;
class CIncomingIrinaAuthToken;
class CIncomingIrinaCreateGame;
class CIncomingIrinaUnhostGame;
class CIncomingIrinaUpdateSlots;
class CIncomingIrinaMarkAsStarted;

#include <nlohmann/json.hpp>

using json = nlohmann::json;

class CIrinaProtocol
{
public:
	CIrinaProtocol( );
	~CIrinaProtocol( );

	CIncomingIrinaPackage * IRINA_ParseJSONData( std::string json );

	std::string IRINA_CreateAuthToken( CIncomingIrinaAuthToken * data );
	std::string IRINA_CreateCreateGame( CIncomingIrinaCreateGame * data );
	std::string IRINA_CreateUnhostGame( CIncomingIrinaUnhostGame * data );
	std::string IRINA_CreateUpdateSlots( CIncomingIrinaUpdateSlots * data );
	std::string IRINA_CreateMarkAsStarted( CIncomingIrinaMarkAsStarted * data );
	
private:
	CIncomingIrinaAuthToken * IRINA_ParseAuthToken( json data );
	CIncomingIrinaCreateGame * IRINA_ParseCreateGame( json data );
	CIncomingIrinaUnhostGame * IRINA_ParseUnhostGame( json data );
	CIncomingIrinaUpdateSlots * IRINA_ParseUpdateSlots( json data );
	CIncomingIrinaMarkAsStarted * IRINA_ParseMarkAsStarted( json data );
};

class CIncomingIrinaPackage
{
private:
	char m_Type;

public:
	CIncomingIrinaPackage( char nType ) : m_Type( nType ) {}
	virtual ~CIncomingIrinaPackage( ) {}

	char GetType( ) { return m_Type; }
};

class CIncomingIrinaAuthToken: public CIncomingIrinaPackage
{
private:
	std::string m_Token;

public:
	CIncomingIrinaAuthToken( std::string nToken ) : m_Token( nToken ), CIncomingIrinaPackage( IRINA_AUTH_TOKEN )
	{

	}
	virtual ~CIncomingIrinaAuthToken( ) {}

	std::string GetToken( ) { return m_Token; }

	void SetToken( std::string nToken ) { m_Token = nToken; }
};

class CIncomingIrinaSlot
{
private:
	char m_SID;
	char m_Colour;
	std::string m_PlayerName;
	std::string m_PlayerRealm;


public:

	CIncomingIrinaSlot( char nSID, std::string nPlayerName, std::string nPlayerRealm, char nColour ) : m_SID( nSID ), m_PlayerName( nPlayerName ), m_PlayerRealm( nPlayerRealm ), m_Colour( nColour )
	{

	}

	virtual ~CIncomingIrinaSlot( ) {}

	char GetSID( ) { return m_SID; }
	char GetColour( ) { return m_Colour; }

	std::string GetPlayerName( ) { return m_PlayerName; }
	std::string GetPlayerRealm( ) { return m_PlayerRealm; }

	void SetSID( char nSID ) { m_SID = nSID; }
	void SetColour( char nColour ) { m_Colour = nColour; }
	void SetPlayerName( std::string nPlayerName ) { m_PlayerName = nPlayerName; }
	void SetPlayerRealm( std::string nPlayerRealm ) { m_PlayerRealm = nPlayerRealm; }

};

class CIncomingIrinaCreateGame: public CIncomingIrinaPackage
{
private:

	// For table

	std::string m_GameName;
	std::vector<CIncomingIrinaSlot> m_Slots;

	// For connector

	std::string m_HostIP;
	uint16_t m_HostPort;
	uint32_t m_HostCounterID;
	uint32_t m_EntryKey;

	std::string m_MapLocalPath;
	std::string m_MapConfig;

	std::string m_Iccup;
	char m_Position;

	// For site map prev

	std::string m_Players;
	std::string m_Description;
	std::string m_MapName;

public:
	CIncomingIrinaCreateGame( std::string nGameName, std::vector<CIncomingIrinaSlot> nSlots, std::string nHostIP, uint16_t nHostPort, uint32_t nHostCounterID, uint32_t nEntryKey, std::string nMapLocalPath, std::string nMapConfig, std::string nPlayers, std::string nDescription, std::string nMapName, std::string nIccup, char nPosition ) : CIncomingIrinaPackage( IRINA_CREATE_GAME ), m_GameName( nGameName ), m_Slots( nSlots ), m_HostIP( nHostIP ), m_HostPort( nHostPort ), m_HostCounterID( nHostCounterID ), m_EntryKey( nEntryKey ), m_Players( nPlayers ), m_Description( nDescription ), m_MapName( nMapName ), m_MapLocalPath( nMapLocalPath ), m_MapConfig( nMapConfig ), m_Iccup( nIccup ), m_Position( nPosition )
	{

	}

	virtual ~CIncomingIrinaCreateGame( ) {}

	std::vector<CIncomingIrinaSlot> GetMapSlots( ) { return m_Slots; }
	std::string GetGameName( ) { return m_GameName; }
	std::string GetHostIP( ) { return m_HostIP; }
	uint16_t GetHostPort( ) { return m_HostPort; }
	uint32_t GetHostCounterID( ) { return m_HostCounterID; }
	uint32_t GetEntryKey( ) { return m_EntryKey; }

	std::string GetPlayers( ) { return m_Players; }
	std::string GetDescription( ) { return m_Description; }
	std::string GetMapName( ) { return m_MapName; }

	std::string GetMapLocalPath( ) { return m_MapLocalPath; }
	std::string GetMapConfig( ) { return m_MapConfig; }

	std::string GetIccup( ) { return m_Iccup; }
	char GetPosition( ) { return m_Position; }

	// �������� ������� ����

	void SetGameName( std::string nGameName ) { m_GameName = nGameName; }
	void SetHostIP( std::string nHostIP ) { m_HostIP = nHostIP; }
	void SetHostPort( uint16_t nHostPort ) { m_HostPort = nHostPort; }
	void SetHostCounterID( uint32_t nHostCounterID ) { m_HostCounterID = nHostCounterID; }
	void SetEntryKey( uint32_t nEntryKey ) { m_EntryKey = nEntryKey; }

	void SetPlayers( std::string nPlayers ) { m_Players = nPlayers; }
	void SetDescription( std::string nDescription ) { m_Description = nDescription; }
	void SetMapName( std::string nMapName ) { m_MapName = nMapName; }

	void SetMapLocalPath( std::string nMapLocalPath ) { m_MapLocalPath = nMapLocalPath; }
	void SertMapConfig( std::string nMapConfig ) { m_MapConfig = nMapConfig; }
};

class CIncomingIrinaUnhostGame: public CIncomingIrinaPackage
{
public:

	CIncomingIrinaUnhostGame( uint32_t nEntryKey ) : CIncomingIrinaPackage( IRINA_UNHOST_GAME ), m_EntryKey( nEntryKey )
	{

	}

	uint32_t GetEntryKey( ) { return m_EntryKey; }
	void SetEntryKey( uint32_t nEntryKey ) { m_EntryKey = nEntryKey; }

public:
	uint32_t m_EntryKey;
};

class CIncomingIrinaUpdateSlots: public CIncomingIrinaPackage
{
private:
	std::vector<CIncomingIrinaSlot> m_Slots;
	uint32_t m_EntryKey;

public:

	CIncomingIrinaUpdateSlots( std::vector<CIncomingIrinaSlot> nSlots, uint32_t nEntryKey ) : CIncomingIrinaPackage( IRINA_UPDATE_SLOT ), m_Slots( nSlots ), m_EntryKey( nEntryKey )
	{

	}

	virtual ~CIncomingIrinaUpdateSlots( ) {}

	std::vector<CIncomingIrinaSlot> GetSlots( ) { return m_Slots; }
	uint32_t GetEntryKey( ) { return m_EntryKey; }

	void SetSlots( std::vector<CIncomingIrinaSlot> nSlot ) { m_Slots = nSlot; }
	void SetEntryKey( uint32_t nEntryKey ) { m_EntryKey = nEntryKey; }
};

class CIncomingIrinaMarkAsStarted: public CIncomingIrinaPackage
{
private:
	uint32_t m_EntryKey;

public:

	CIncomingIrinaMarkAsStarted( uint32_t nEntryKey ) : CIncomingIrinaPackage( IRINA_MARK_AS_STARTED ), m_EntryKey( nEntryKey )
	{

	}

	virtual ~CIncomingIrinaMarkAsStarted( ) {}

	uint32_t GetEntryKey( ) { return m_EntryKey; }

	void SetEntryKey( uint32_t nEntryKey ) { m_EntryKey = nEntryKey; }
};
#endif