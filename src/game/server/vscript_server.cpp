//=============================================================================//
//
// Purpose: Expose native code to VScript API
// 
//-----------------------------------------------------------------------------
// 
// See 'game/shared/vscript_shared.cpp' for more details.
//
//=============================================================================//

#include "core/stdafx.h"
#include "engine/server/server.h"
#include "game/shared/vscript_shared.h"
#include "vscript/vscript.h"
#include "vscript/languages/squirrel_re/include/sqvm.h"

#include "liveapi/liveapi.h"
#include "vscript_server.h"
#include <engine/host_state.h>
#include "player.h"
#include <common/callback.h>

/*
=====================
SQVM_ServerScript_f

  Executes input on the
  VM in SERVER context.
=====================
*/
static void SQVM_ServerScript_f(const CCommand& args)
{
    if (args.ArgC() >= 2)
    {
        Script_Execute(args.ArgS(), SQCONTEXT::SERVER);
    }
}
static ConCommand script("script", SQVM_ServerScript_f, "Run input code as SERVER script on the VM", FCVAR_DEVELOPMENTONLY | FCVAR_GAMEDLL | FCVAR_CHEAT | FCVAR_SERVER_FRAME_THREAD);

namespace VScriptCode
{
    namespace Server
    {
        //-----------------------------------------------------------------------------
        // Purpose: sets whether the server could auto reload at this time (e.g. if
        // server admin has host_autoReloadRate AND host_autoReloadRespectGameState
        // set, and its time to auto reload, but the match hasn't finished yet, wait
        // until this is set to proceed the reload of the server
        //-----------------------------------------------------------------------------
        SQRESULT SetAutoReloadState(HSQUIRRELVM v)
        {
            SQBool state = false;
            sq_getbool(v, 2, &state);

            g_hostReloadState = state;
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: kicks a player by given name
        //-----------------------------------------------------------------------------
        SQRESULT KickPlayerByName(HSQUIRRELVM v)
        {
            const SQChar* playerName = nullptr;
            const SQChar* reason = nullptr;

            sq_getstring(v, 2, &playerName);
            sq_getstring(v, 3, &reason);

            if (!VALID_CHARSTAR(playerName))
            {
                v_SQVM_ScriptError("Empty or null player name");
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            // Discard empty strings, this will use the default message instead.
            if (!VALID_CHARSTAR(reason))
                reason = nullptr;

            g_BanSystem.KickPlayerByName(playerName, reason);
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: kicks a player by given handle or id
        //-----------------------------------------------------------------------------
        SQRESULT KickPlayerById(HSQUIRRELVM v)
        {
            const SQChar* playerHandle = nullptr;
            const SQChar* reason = nullptr;

            sq_getstring(v, 2, &playerHandle);
            sq_getstring(v, 3, &reason);

            if (!VALID_CHARSTAR(playerHandle))
            {
                v_SQVM_ScriptError("Empty or null player handle");
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            // Discard empty strings, this will use the default message instead.
            if (!VALID_CHARSTAR(reason))
                reason = nullptr;

            g_BanSystem.KickPlayerById(playerHandle, reason);
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: bans a player by given name
        //-----------------------------------------------------------------------------
        SQRESULT BanPlayerByName(HSQUIRRELVM v)
        {
            const SQChar* playerName = nullptr;
            const SQChar* reason = nullptr;

            sq_getstring(v, 2, &playerName);
            sq_getstring(v, 3, &reason);

            if (!VALID_CHARSTAR(playerName))
            {
                v_SQVM_ScriptError("Empty or null player name");
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            // Discard empty strings, this will use the default message instead.
            if (!VALID_CHARSTAR(reason))
                reason = nullptr;

            g_BanSystem.BanPlayerByName(playerName, reason);
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: bans a player by given handle or id
        //-----------------------------------------------------------------------------
        SQRESULT BanPlayerById(HSQUIRRELVM v)
        {
            const SQChar* playerHandle = nullptr;
            const SQChar* reason = nullptr;

            sq_getstring(v, 2, &playerHandle);
            sq_getstring(v, 3, &reason);

            if (!VALID_CHARSTAR(playerHandle))
            {
                v_SQVM_ScriptError("Empty or null player handle");
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            // Discard empty strings, this will use the default message instead.
            if (!VALID_CHARSTAR(reason))
                reason = nullptr;

            g_BanSystem.BanPlayerById(playerHandle, reason);
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: unbans a player by given nucleus id or ip address
        //-----------------------------------------------------------------------------
        SQRESULT UnbanPlayer(HSQUIRRELVM v)
        {
            const SQChar* szCriteria = nullptr;
            sq_getstring(v, 2, &szCriteria);

            if (!VALID_CHARSTAR(szCriteria))
            {
                v_SQVM_ScriptError("Empty or null player criteria");
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            g_BanSystem.UnbanPlayer(szCriteria);

            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: gets the number of real players on this server
        //-----------------------------------------------------------------------------
        SQRESULT GetNumHumanPlayers(HSQUIRRELVM v)
        {
            sq_pushinteger(v, g_pServer->GetNumHumanPlayers());
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: gets the number of fake players on this server
        //-----------------------------------------------------------------------------
        SQRESULT GetNumFakeClients(HSQUIRRELVM v)
        {
            sq_pushinteger(v, g_pServer->GetNumFakeClients());
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: gets the current server id
        //-----------------------------------------------------------------------------
        SQRESULT GetServerID(HSQUIRRELVM v)
        {
            sq_pushstring(v, g_LogSessionUUID.c_str(), (SQInteger)g_LogSessionUUID.length());
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }

        //-----------------------------------------------------------------------------
        // Purpose: checks whether the server is active
        //-----------------------------------------------------------------------------
        SQRESULT IsServerActive(HSQUIRRELVM v)
        {
            bool isActive = g_pServer->IsActive();
            sq_pushbool(v, isActive);

            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }
    }

    namespace PlayerEntity
    {
        //-----------------------------------------------------------------------------
        // Purpose: sets a class var on the server and each client
        //-----------------------------------------------------------------------------
        SQRESULT ScriptSetClassVar(HSQUIRRELVM v)
        {
            CPlayer* player = nullptr;

            if (!v_sq_getentity(v, (SQEntity*)&player))
                return SQ_ERROR;

            const SQChar* key = nullptr;
            sq_getstring(v, 2, &key);

            if (!VALID_CHARSTAR(key))
            {
                v_SQVM_ScriptError("Empty or null class key");
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            const SQChar* val = nullptr;
            sq_getstring(v, 3, &val);

            if (!VALID_CHARSTAR(val))
            {
                v_SQVM_ScriptError("Empty or null class var");
                SCRIPT_CHECK_AND_RETURN(v, SQ_ERROR);
            }

            CClient* const client = g_pServer->GetClient(player->GetEdict() - 1);
            SVC_SetClassVar msg(key, val);

            const bool success = client->SendNetMsgEx(&msg, false, true, false);

            if (success)
            {
                const char* pArgs[3] = {
                    "_setClassVarServer",
                    key,
                    val
                };

                const CCommand cmd((int)V_ARRAYSIZE(pArgs), pArgs, cmd_source_t::kCommandSrcCode);
                const int oldIdx = *g_nCommandClientIndex;

                *g_nCommandClientIndex = client->GetUserID();
                v__setClassVarServer_f(cmd);

                *g_nCommandClientIndex = oldIdx;
            }

            sq_pushbool(v, success);
            SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
        }
    }
}

//---------------------------------------------------------------------------------
// Purpose: registers script functions in SERVER context
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterServerFunctions(CSquirrelVM* s)
{
    Script_RegisterCommonAbstractions(s);
    Script_RegisterCoreServerFunctions(s);
    Script_RegisterAdminServerFunctions(s);

    Script_RegisterLiveAPIFunctions(s);
}

void Script_RegisterServerEnums(CSquirrelVM* const s)
{
    Script_RegisterLiveAPIEnums(s);
}

//---------------------------------------------------------------------------------
// Purpose: core server script functions
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterCoreServerFunctions(CSquirrelVM* s)
{
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, SetAutoReloadState, "Set whether we can auto-reload the server", "void", "bool");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, GetServerID, "Gets the current server ID", "string", "");
}

//---------------------------------------------------------------------------------
// Purpose: admin server script functions
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterAdminServerFunctions(CSquirrelVM* s)
{
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, GetNumHumanPlayers, "Gets the number of human players on the server", "int", "");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, GetNumFakeClients, "Gets the number of bot players on the server", "int", "");

    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, KickPlayerByName, "Kicks a player from the server by name", "void", "string, string");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, KickPlayerById, "Kicks a player from the server by handle or nucleus id", "void", "string, string");

    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, BanPlayerByName, "Bans a player from the server by name", "void", "string, string");
    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, BanPlayerById, "Bans a player from the server by handle or nucleus id", "void", "string, string");

    DEFINE_SERVER_SCRIPTFUNC_NAMED(s, UnbanPlayer, "Unbans a player from the server by nucleus id or ip address", "void", "string");
}

//---------------------------------------------------------------------------------
// Purpose: script code class function registration
//---------------------------------------------------------------------------------
static void Script_RegisterServerEntityClassFuncs()
{
    v_Script_RegisterServerEntityClassFuncs();
    static bool initialized = false;

    if (initialized)
        return;

    initialized = true;
}
//---------------------------------------------------------------------------------
static void Script_RegisterServerPlayerClassFuncs()
{
    v_Script_RegisterServerPlayerClassFuncs();
    static bool initialized = false;

    if (initialized)
        return;

    initialized = true;

    g_serverScriptPlayerStruct->AddFunction("SetClassVar",
        "ScriptSetClassVar",
        "Change a variable in the player's class settings",
        "bool",
        "string, string",
        5,
        VScriptCode::PlayerEntity::ScriptSetClassVar);
}
//---------------------------------------------------------------------------------
static void Script_RegisterServerAIClassFuncs()
{
    v_Script_RegisterServerAIClassFuncs();
    static bool initialized = false;

    if (initialized)
        return;

    initialized = true;
}
//---------------------------------------------------------------------------------
static void Script_RegisterServerWeaponClassFuncs()
{
    v_Script_RegisterServerWeaponClassFuncs();
    static bool initialized = false;

    if (initialized)
        return;

    initialized = true;
}
//---------------------------------------------------------------------------------
static void Script_RegisterServerProjectileClassFuncs()
{
    v_Script_RegisterServerProjectileClassFuncs();
    static bool initialized = false;

    if (initialized)
        return;

    initialized = true;
}
//---------------------------------------------------------------------------------
static void Script_RegisterServerTitanSoulClassFuncs()
{
    v_Script_RegisterServerTitanSoulClassFuncs();
    static bool initialized = false;

    if (initialized)
        return;

    initialized = true;
}
//---------------------------------------------------------------------------------
static void Script_RegisterServerPlayerDecoyClassFuncs()
{
    v_Script_RegisterServerPlayerDecoyClassFuncs();
    static bool initialized = false;

    if (initialized)
        return;

    initialized = true;
}
//---------------------------------------------------------------------------------
static void Script_RegisterServerSpawnpointClassFuncs()
{
    v_Script_RegisterServerSpawnpointClassFuncs();
    static bool initialized = false;

    if (initialized)
        return;

    initialized = true;
}
//---------------------------------------------------------------------------------
static void Script_RegisterServerFirstPersonProxyClassFuncs()
{
    v_Script_RegisterServerFirstPersonProxyClassFuncs();
    static bool initialized = false;

    if (initialized)
        return;

    initialized = true;
}

void VScriptServer::Detour(const bool bAttach) const
{
    DetourSetup(&v_Script_RegisterServerEntityClassFuncs, &Script_RegisterServerEntityClassFuncs, bAttach);
    DetourSetup(&v_Script_RegisterServerPlayerClassFuncs, &Script_RegisterServerPlayerClassFuncs, bAttach);
    DetourSetup(&v_Script_RegisterServerAIClassFuncs, &Script_RegisterServerAIClassFuncs, bAttach);
    DetourSetup(&v_Script_RegisterServerWeaponClassFuncs, &Script_RegisterServerWeaponClassFuncs, bAttach);
    DetourSetup(&v_Script_RegisterServerProjectileClassFuncs, &Script_RegisterServerProjectileClassFuncs, bAttach);
    DetourSetup(&v_Script_RegisterServerTitanSoulClassFuncs, &Script_RegisterServerTitanSoulClassFuncs, bAttach);
    DetourSetup(&v_Script_RegisterServerPlayerDecoyClassFuncs, &Script_RegisterServerPlayerDecoyClassFuncs, bAttach);
    DetourSetup(&v_Script_RegisterServerSpawnpointClassFuncs, &Script_RegisterServerSpawnpointClassFuncs, bAttach);
    DetourSetup(&v_Script_RegisterServerFirstPersonProxyClassFuncs, &Script_RegisterServerFirstPersonProxyClassFuncs, bAttach);
}
