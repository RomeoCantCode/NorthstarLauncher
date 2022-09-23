#include "pch.h"
#include "miscserverscript.h"
#include "squirrel.h"
#include "masterserver.h"
#include "serverauthentication.h"
#include "gameutils.h"
#include "dedicated.h"

// annoying helper function because i can't figure out getting players or entities from sqvm rn
// wish i didn't have to do it like this, but here we are
void* GetPlayerByIndex(int playerIndex)
{
	const int PLAYER_ARRAY_OFFSET = 0x12A53F90;
	const int PLAYER_SIZE = 0x2D728;

	void* playerArrayBase = (char*)GetModuleHandleA("engine.dll") + PLAYER_ARRAY_OFFSET;
	void* player = (char*)playerArrayBase + (playerIndex * PLAYER_SIZE);

	return player;
}

// void function NSEarlyWritePlayerIndexPersistenceForLeave( int playerIndex )
SQRESULT SQ_EarlyWritePlayerIndexPersistenceForLeave(void* sqvm)
{
	int playerIndex = ServerSq_getinteger(sqvm, 1);
	void* player = GetPlayerByIndex(playerIndex);

	if (!g_ServerAuthenticationManager->m_additionalPlayerData.count(player))
	{
		ServerSq_pusherror(sqvm, fmt::format("Invalid playerindex {}", playerIndex).c_str());
		return SQRESULT_ERROR;
	}

	g_ServerAuthenticationManager->m_additionalPlayerData[player].needPersistenceWriteOnLeave = false;
	g_ServerAuthenticationManager->WritePersistentData(player);
	return SQRESULT_NULL;
}

// bool function NSIsWritingPlayerPersistence()
SQRESULT SQ_IsWritingPlayerPersistence(void* sqvm)
{
	ServerSq_pushbool(sqvm, g_MasterServerManager->m_bSavingPersistentData);
	return SQRESULT_NOTNULL;
}

// bool function NSIsPlayerIndexLocalPlayer( int playerIndex )
SQRESULT SQ_IsPlayerIndexLocalPlayer(void* sqvm)
{
	int playerIndex = ServerSq_getinteger(sqvm, 1);
	void* player = GetPlayerByIndex(playerIndex);
	if (!g_ServerAuthenticationManager->m_additionalPlayerData.count(player))
	{
		ServerSq_pusherror(sqvm, fmt::format("Invalid playerindex {}", playerIndex).c_str());
		return SQRESULT_ERROR;
	}

	ServerSq_pushbool(sqvm, !strcmp(g_LocalPlayerUserID, (char*)player + 0xF500));
	return SQRESULT_NOTNULL;
}

// bool function NSIsDedicated()

SQRESULT SQ_IsDedicated(void* sqvm)
{
	ServerSq_pushbool(sqvm, IsDedicatedServer());
	return SQRESULT_NOTNULL;
}

// string function NSGetPlayerIP( int playerIndex )
SQRESULT SQ_GetPlayerIP(void* sqvm)
{
	int playerIndex = ServerSq_getinteger(sqvm, 1);
	void* player = GetPlayerByIndex(playerIndex);

	if (!g_ServerAuthenticationManager->m_additionalPlayerData.count(player))
	{
		ServerSq_pusherror(sqvm, fmt::format("Invalid playerindex {}", playerIndex).c_str());
		return SQRESULT_ERROR;
	}

	std::string IPstr = "";
	unsigned char *IPv6 = g_ServerAuthenticationManager->m_additionalPlayerData[player].IPv6;

	//TODO: Add check if IPv6 or IPv4, bytes 11 and 10 are FF when IPv4 is used AND check type NA_NULL, NA_LOOPBACK, NA_IP

	//TODO: Think about if putting all that stuff here is actually good or bad

	//get IPv4 bytes
	unsigned char *IP1 = IPv6+12;
	unsigned char *IP2 = IPv6+13;
	unsigned char *IP3 = IPv6+14;
	unsigned char *IP4 = IPv6+15;

	//make number a string so we can print it later in the console
	std::string IP1char = std::to_string((uint8_t)*IP1);
	std::string IP2char = std::to_string((uint8_t)*IP2);
	std::string IP3char = std::to_string((uint8_t)*IP3);
	std::string IP4char = std::to_string((uint8_t)*IP4);
	std::string dot = ".";

	//put all in one string
	IPstr = IP1char + dot + IP2char + dot + IP3char + dot + IP4char;

	//push it, requires cstring not std string
	ServerSq_pushstring(sqvm,IPstr.c_str(), -1);
	return SQRESULT_NOTNULL;
}

void InitialiseMiscServerScriptCommand(HMODULE baseAddress)
{
	g_ServerSquirrelManager->AddFuncRegistration(
		"void", "NSEarlyWritePlayerIndexPersistenceForLeave", "int playerIndex", "", SQ_EarlyWritePlayerIndexPersistenceForLeave);
	g_ServerSquirrelManager->AddFuncRegistration("bool", "NSIsWritingPlayerPersistence", "", "", SQ_IsWritingPlayerPersistence);
	g_ServerSquirrelManager->AddFuncRegistration("bool", "NSIsPlayerIndexLocalPlayer", "int playerIndex", "", SQ_IsPlayerIndexLocalPlayer);
	g_ServerSquirrelManager->AddFuncRegistration("bool", "NSIsDedicated", "", "", SQ_IsDedicated);
	g_ServerSquirrelManager->AddFuncRegistration("string", "NSGetPlayerIP", "int playerIndex", "", SQ_GetPlayerIP);
}
