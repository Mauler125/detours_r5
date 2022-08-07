//=============================================================================//
//
// Purpose: Expose native code to VScript API
// 
//-----------------------------------------------------------------------------
// 
// Create functions here under the target VM namespace. If the function has to
// be registered for 2 or more VM's, put them under the 'SHARED' namespace. 
// Ifdef them out for 'DEDICATED' / 'CLIENT_DLL' if the target VM's do not 
// include 'SERVER' / 'CLIENT'.
//
//=============================================================================//

#include "core/stdafx.h"
#include "vpc/keyvalues.h"
#ifndef CLIENT_DLL
#include "engine/server/server.h"
#endif // CLIENT_DLL
#include "engine/cmodel_bsp.h"
#include "engine/host_state.h"
#include "squirrel/sqtype.h"
#include "squirrel/sqapi.h"
#include "squirrel/sqinit.h"
#include "networksystem/pylon.h"

#ifndef DEDICATED
#include "gameui/IBrowser.h" // TODO: create dedicated class for exposing server utils to ImGui and UI VM.
#endif // !DEDICATED

namespace VSquirrel
{
    namespace SHARED
    {
        //-----------------------------------------------------------------------------
        // Purpose: SDK test and example body
        //-----------------------------------------------------------------------------
        SQRESULT SDKNativeTest(HSQUIRRELVM v)
        {
            // Function code goes here.
            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: expose SDK version to the VScript API
        //-----------------------------------------------------------------------------
        SQRESULT GetSDKVersion(HSQUIRRELVM v)
        {
            sq_pushstring(v, SDK_VERSION, -1);
            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: return all available maps
        //-----------------------------------------------------------------------------
        SQRESULT GetAvailableMaps(HSQUIRRELVM v)
        {
            if (g_vAllMaps.empty())
                return SQ_OK;

            sq_newarray(v, 0);
            for (const string& it : g_vAllMaps)
            {
                sq_pushstring(v, it.c_str(), -1);
                sq_arrayappend(v, -2);
            }

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: return all available playlists
        //-----------------------------------------------------------------------------
        SQRESULT GetAvailablePlaylists(HSQUIRRELVM v)
        {
            if (g_vAllPlaylists.empty())
                return SQ_OK;

            sq_newarray(v, 0);
            for (const string& it : g_vAllPlaylists)
            {
                sq_pushstring(v, it.c_str(), -1);
                sq_arrayappend(v, -2);
            }

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: shutdown local game (host only)
        //-----------------------------------------------------------------------------
        SQRESULT ShutdownHostGame(HSQUIRRELVM v)
        {
            if (g_pHostState->m_bActiveGame)
                g_pHostState->m_iNextState = HostStates_t::HS_GAME_SHUTDOWN;

            return SQ_OK;
        }
    }
#ifndef CLIENT_DLL
    namespace SERVER
    {
        //-----------------------------------------------------------------------------
        // Purpose: gets the number of real players on this server
        //-----------------------------------------------------------------------------
        SQRESULT GetNumHumanPlayers(HSQUIRRELVM v)
        {
            sq_pushinteger(v, g_pServer->GetNumHumanPlayers());
            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: gets the number of fake players on this server
        //-----------------------------------------------------------------------------
        SQRESULT GetNumFakeClients(HSQUIRRELVM v)
        {
            sq_pushinteger(v, g_pServer->GetNumFakeClients());
            return SQ_OK;
        }
    }
#endif // !CLIENT_DLL
#ifndef DEDICATED
    namespace CLIENT
    {
    }
    namespace UI
    {
        //-----------------------------------------------------------------------------
        // Purpose: get server's current name from serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerName(HSQUIRRELVM v)
        {
            SQInteger iServer = sq_getinteger(v, 1);
            if (iServer >= static_cast<SQInteger>(g_pBrowser->m_vServerList.size()))
            {
                v_SQVM_RaiseError(v, "Index must be less than %i.\n",
                    static_cast<SQInteger>(g_pBrowser->m_vServerList.size()));
                return SQ_ERROR;
            }

            string svServerName = g_pBrowser->m_vServerList[iServer].m_svHostName;
            sq_pushstring(v, svServerName.c_str(), -1);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get server's current description from serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerDescription(HSQUIRRELVM v)
        {
            SQInteger iServer = sq_getinteger(v, 1);
            if (iServer >= static_cast<SQInteger>(g_pBrowser->m_vServerList.size()))
            {
                v_SQVM_RaiseError(v, "Index must be less than %i.\n",
                    static_cast<SQInteger>(g_pBrowser->m_vServerList.size()));
                return SQ_ERROR;
            }

            string svServerDescription = g_pBrowser->m_vServerList[iServer].m_svDescription;
            sq_pushstring(v, svServerDescription.c_str(), -1);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get server's current map via serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerMap(HSQUIRRELVM v)
        {
            SQInteger iServer = sq_getinteger(v, 1);
            if (iServer >= static_cast<SQInteger>(g_pBrowser->m_vServerList.size()))
            {
                v_SQVM_RaiseError(v, "Index must be less than %i.\n",
                    static_cast<SQInteger>(g_pBrowser->m_vServerList.size()));
                return SQ_ERROR;
            }

            string svServerMapName = g_pBrowser->m_vServerList[iServer].m_svMapName;
            sq_pushstring(v, svServerMapName.c_str(), -1);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get server's current playlist via serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerPlaylist(HSQUIRRELVM v)
        {
            SQInteger iServer = sq_getinteger(v, 1);
            if (iServer >= static_cast<SQInteger>(g_pBrowser->m_vServerList.size()))
            {
                v_SQVM_RaiseError(v, "Index must be less than %i.\n",
                    static_cast<SQInteger>(g_pBrowser->m_vServerList.size()));
                return SQ_ERROR;
            }

            string svServerPlaylist = g_pBrowser->m_vServerList[iServer].m_svPlaylist;
            sq_pushstring(v, svServerPlaylist.c_str(), -1);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get server's current player count via serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerCurrentPlayers(HSQUIRRELVM v)
        {
            SQInteger iServer = sq_getinteger(v, 1);
            if (iServer >= static_cast<SQInteger>(g_pBrowser->m_vServerList.size()))
            {
                v_SQVM_RaiseError(v, "Index must be less than %i.\n",
                    static_cast<SQInteger>(g_pBrowser->m_vServerList.size()));
                return SQ_ERROR;
            }

            sq_pushinteger(v, strtol(g_pBrowser->m_vServerList[iServer].m_svPlayerCount.c_str(), NULL, NULL));

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get server's current player count via serverlist index
        //-----------------------------------------------------------------------------
        SQRESULT GetServerMaxPlayers(HSQUIRRELVM v)
        {
            SQInteger iServer = sq_getinteger(v, 1);
            if (iServer >= static_cast<SQInteger>(g_pBrowser->m_vServerList.size()))
            {
                v_SQVM_RaiseError(v, "Index must be less than %i.\n",
                    static_cast<SQInteger>(g_pBrowser->m_vServerList.size()));
                return SQ_ERROR;
            }

            sq_pushinteger(v, strtol(g_pBrowser->m_vServerList[iServer].m_svMaxPlayers.c_str(), NULL, NULL));

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get current server count from pylon
        //-----------------------------------------------------------------------------
        SQRESULT GetServerCount(HSQUIRRELVM v)
        {
            g_pBrowser->GetServerList(); // Refresh svListing list.
            sq_pushinteger(v, g_pBrowser->m_vServerList.size());

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get promo data for serverbrowser panels
        //-----------------------------------------------------------------------------
        SQRESULT GetPromoData(HSQUIRRELVM v)
        {
            enum class R5RPromoData : SQInteger
            {
                PromoLargeTitle,
                PromoLargeDesc,
                PromoLeftTitle,
                PromoLeftDesc,
                PromoRightTitle,
                PromoRightDesc
            };

            R5RPromoData ePromoIndex = static_cast<R5RPromoData>(sq_getinteger(v, 1));

            string svPromo;

            switch (ePromoIndex)
            {
            case R5RPromoData::PromoLargeTitle:
            {
                svPromo = "#PROMO_LARGE_TITLE";
                break;
            }
            case R5RPromoData::PromoLargeDesc:
            {
                svPromo = "#PROMO_LARGE_DESCRIPTION";
                break;
            }
            case R5RPromoData::PromoLeftTitle:
            {
                svPromo = "#PROMO_LEFT_TITLE";
                break;
            }
            case R5RPromoData::PromoLeftDesc:
            {
                svPromo = "#PROMO_LEFT_DESCRIPTION";
                break;
            }
            case R5RPromoData::PromoRightTitle:
            {
                svPromo = "#PROMO_RIGHT_TITLE";
                break;
            }
            case R5RPromoData::PromoRightDesc:
            {
                svPromo = "#PROMO_RIGHT_DESCRIPTION";
                break;
            }
            default:
            {
                svPromo = "#PROMO_SDK_ERROR";
                break;
            }
            }

            sq_pushstring(v, svPromo.c_str(), -1);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: set netchannel encryption key and connect to server
        //-----------------------------------------------------------------------------
        SQRESULT SetEncKeyAndConnect(HSQUIRRELVM v)
        {
            SQInteger iServer = sq_getinteger(v, 1);
            if (iServer >= static_cast<SQInteger>(g_pBrowser->m_vServerList.size()))
            {
                v_SQVM_RaiseError(v, "Index must be less than %i.\n",
                    static_cast<SQInteger>(g_pBrowser->m_vServerList.size()));
                return SQ_ERROR;
            }

            // !TODO: Create glue class instead.
            g_pBrowser->ConnectToServer(g_pBrowser->m_vServerList[iServer].m_svIpAddress,
                g_pBrowser->m_vServerList[iServer].m_svGamePort, 
                g_pBrowser->m_vServerList[iServer].m_svEncryptionKey);

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: create server via native serverbrowser entries
        //-----------------------------------------------------------------------------
        SQRESULT CreateServerFromMenu(HSQUIRRELVM v)
        {
            string svServerName = sq_getstring(v, 1);
            string svServerDescription = sq_getstring(v, 2);
            string svServerMapName = sq_getstring(v, 3);
            string svServerPlaylist = sq_getstring(v, 4);
            EServerVisibility eServerVisibility = static_cast<EServerVisibility>(sq_getinteger(v, 5));

            if (svServerName.empty() || svServerMapName.empty() || svServerPlaylist.empty())
                return SQ_OK;

            // Adjust browser settings.
            g_pBrowser->m_Server.m_svHostName = svServerName;
            g_pBrowser->m_Server.m_svDescription = svServerDescription;
            g_pBrowser->m_Server.m_svMapName = svServerMapName;
            g_pBrowser->m_Server.m_svPlaylist = svServerPlaylist;
            g_pBrowser->eServerVisibility = eServerVisibility;

            // Launch server.
            g_pBrowser->LaunchServer();

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: request token from pylon and join server with result.
        //-----------------------------------------------------------------------------
        SQRESULT JoinPrivateServerFromMenu(HSQUIRRELVM v)
        {
            string svHiddenServerRequestMessage;
            string svToken = sq_getstring(v, 1);

            NetGameServer_t svListing;
            bool result = g_pMasterServer->GetServerByToken(svListing, svHiddenServerRequestMessage, svToken); // Send szToken connect request.
            if (result)
            {
                g_pBrowser->ConnectToServer(svListing.m_svIpAddress, svListing.m_svGamePort, svListing.m_svEncryptionKey);
            }

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: get response from private server request
        //-----------------------------------------------------------------------------
        SQRESULT GetPrivateServerMessage(HSQUIRRELVM v)
        {
            string svHiddenServerRequestMessage;
            string svToken = sq_getstring(v, 1);

            NetGameServer_t serverListing;
            bool result = g_pMasterServer->GetServerByToken(serverListing, svHiddenServerRequestMessage, svToken); // Send token connect request.
            if (!serverListing.m_svHostName.empty())
            {
                svHiddenServerRequestMessage = "Found Server: " + serverListing.m_svHostName;
                sq_pushstring(v, svHiddenServerRequestMessage.c_str(), -1);
            }
            else
            {
                svHiddenServerRequestMessage = "Error: Server Not Found";
                sq_pushstring(v, svHiddenServerRequestMessage.c_str(), -1);
            }

            DevMsg(eDLL_T::UI, "GetPrivateServeMessage response: %s\n", svHiddenServerRequestMessage.c_str());

            return SQ_OK;
        }

        //-----------------------------------------------------------------------------
        // Purpose: connect to server from native server browser entries
        //-----------------------------------------------------------------------------
        SQRESULT ConnectToIPFromMenu(HSQUIRRELVM v)
        {
            string svIpAddr = sq_getstring(v, 1);
            string svEncKey = sq_getstring(v, 2);

            if (svIpAddr.empty() || svEncKey.empty())
                return SQ_OK;

            DevMsg(eDLL_T::UI, "Connecting to server with ip-address '%s' and encryption key '%s'\n", svIpAddr.c_str(), svEncKey.c_str());
            g_pBrowser->ConnectToServer(svIpAddr, svEncKey);

            return SQ_OK;
        }
    }
#endif // !DEDICATED
}