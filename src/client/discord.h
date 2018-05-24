#pragma once

#include "config.h"

#ifdef USE_DISCORD

#include <discord_rpc.h>
#include "log.h"
#include "script/common/c_converter.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

#define MT_DISCORD_APP_ID "443156798510333954"

#define l_update_presence Discord::l_update_presence
#define DISCORD_API(name) API_FCT(name);

class Discord
{
public:
	Discord() {
		DiscordEventHandlers handler = {};
		handler.ready = handler_ready;
		handler.disconnected = handler_error;
		handler.errored = handler_error;
		handler.joinGame = nullptr;
		handler.spectateGame = nullptr;
		handler.joinRequest = nullptr;
		Discord_Initialize(MT_DISCORD_APP_ID, &handler, 0, NULL);
	}

	inline static int l_update_presence(lua_State *L) {
		luaL_argcheck(L, lua_istable(L, 1), 1, "update_presence expects table");

		std::string state = getstringfield_default(L, 1, "state", "Unspecified").c_str();
		std::string details = getstringfield_default(L, 1, "details", "Unspecified").c_str();

		DiscordRichPresence presence = {};
		presence.state = state.c_str();
		presence.details = details.c_str();

		Discord_UpdatePresence(&presence);

		return 0;
	}

private:
	inline static void handler_ready(const DiscordUser* request) {
		infostream << "Discord ready" << std::endl;
	}

	inline static void handler_error(int errorCode, const char* message) {
		warningstream << "Discord error [%d : %s]" << std::endl;
	}

};

#else // USE_DISCORD

#define DISCORD_API(name) ((void)0)

class Discord
{
public:
	Discord() {}
};

#endif // USE_DISCORD
