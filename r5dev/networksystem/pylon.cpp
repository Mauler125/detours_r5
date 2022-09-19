//=====================================================================================//
//
// Purpose: Implementation of the pylon server backend.
//
// $NoKeywords: $
//=====================================================================================//

#include <core/stdafx.h>
#include <tier1/cvar.h>
#include <networksystem/pylon.h>
#ifndef CLIENT_DLL
#include <engine/server/server.h>
#endif // !CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: returns a vector of hosted servers.
//-----------------------------------------------------------------------------
vector<NetGameServer_t> CPylon::GetServerList(string& svOutMessage) const
{
    vector<NetGameServer_t> vslList{};

    nlohmann::json jsRequestBody = nlohmann::json::object();
    jsRequestBody["version"] = SDK_VERSION;

    string svRequestBody = jsRequestBody.dump(4);

    if (pylon_showdebuginfo->GetBool())
    {
        DevMsg(eDLL_T::ENGINE, "%s - Sending server list request to comp-server:\n%s\n", __FUNCTION__, svRequestBody.c_str());
    }

    httplib::Client htClient(pylon_matchmaking_hostname->GetString()); htClient.set_connection_timeout(10);
    httplib::Result htResult = htClient.Post("/servers", svRequestBody.c_str(), svRequestBody.length(), "application/json");

    if (htResult && pylon_showdebuginfo->GetBool())
    {
        DevMsg(eDLL_T::ENGINE, "%s - Comp-server replied with '%d'\n", __FUNCTION__, htResult->status);
    }

    try
    {
        if (htResult && htResult->status == 200) // STATUS_OK
        {
            nlohmann::json jsResultBody = nlohmann::json::parse(htResult->body);
            if (jsResultBody["success"].is_boolean() && jsResultBody["success"].get<bool>())
            {
                for (auto& obj : jsResultBody["servers"])
                {
                    vslList.push_back(
                        NetGameServer_t
                        {
                            obj.value("name",""),
                            obj.value("description",""),
                            obj.value("hidden","false") == "true",
                            obj.value("map",""),
                            obj.value("playlist",""),
                            obj.value("ip",""),
                            obj.value("port", ""),
                            obj.value("key",""),
                            obj.value("checksum",""),
                            obj.value("version", SDK_VERSION),
                            obj.value("playerCount", ""),
                            obj.value("maxPlayers", ""),
                            obj.value("timeStamp", 0),
                            obj.value("publicRef", ""),
                            obj.value("cachedId", ""),
                        }
                    );
                }
            }
            else
            {
                if (jsResultBody["error"].is_string())
                {
                    svOutMessage = jsResultBody["error"].get<string>();
                }
                else
                {
                    svOutMessage = string("Unknown error with status: ") + std::to_string(htResult->status);
                }
            }
        }
        else
        {
            if (htResult)
            {
                if (!htResult->body.empty())
                {
                    nlohmann::json jsResultBody = nlohmann::json::parse(htResult->body);

                    if (jsResultBody["error"].is_string())
                    {
                        svOutMessage = jsResultBody["error"].get<string>();
                    }
                    else
                    {
                        svOutMessage = string("Failed HTTP request: ") + std::to_string(htResult->status);
                    }

                    return vslList;
                }

                svOutMessage = string("Failed HTTP request: ") + std::to_string(htResult->status);
                return vslList;
            }

            svOutMessage = "Failed to reach comp-server: connection timed-out";
            return vslList;
        }
    }
    catch (const std::exception& ex)
    {
        Warning(eDLL_T::ENGINE, "%s - Exception while parsing comp-server response:\n%s\n", __FUNCTION__, ex.what());
    }

    return vslList;
}

//-----------------------------------------------------------------------------
// Purpose: Gets the server by token string.
// Input  : &slOutServer - 
//			&svOutMessage - 
//			svToken - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPylon::GetServerByToken(NetGameServer_t& slOutServer, string& svOutMessage, const string& svToken) const
{
    nlohmann::json jsRequestBody = nlohmann::json::object();
    jsRequestBody["token"] = svToken;

    string svRequestBody = jsRequestBody.dump(4);

    if (pylon_showdebuginfo->GetBool())
    {
        DevMsg(eDLL_T::ENGINE, "%s - Sending token connect request to comp-server:\n%s\n", __FUNCTION__, svRequestBody.c_str());
    }

    httplib::Client htClient(pylon_matchmaking_hostname->GetString()); htClient.set_connection_timeout(10);
    httplib::Result htResult = htClient.Post("/server/byToken", svRequestBody.c_str(), svRequestBody.length(), "application/json");

    if (pylon_showdebuginfo->GetBool())
    {
        DevMsg(eDLL_T::ENGINE, "%s - Comp-server replied with '%d'\n", __FUNCTION__, htResult->status);
    }

    try
    {
        if (htResult && htResult->status == 200) // STATUS_OK
        {
            if (!htResult->body.empty())
            {
                nlohmann::json jsResultBody = nlohmann::json::parse(htResult->body);

                if (htResult && jsResultBody["success"].is_boolean() && jsResultBody["success"])
                {
                    slOutServer = NetGameServer_t
                    {
                            jsResultBody["server"].value("name",""),
                            jsResultBody["server"].value("description",""),
                            jsResultBody["server"].value("hidden","false") == "true",
                            jsResultBody["server"].value("map",""),
                            jsResultBody["server"].value("playlist",""),
                            jsResultBody["server"].value("ip",""),
                            jsResultBody["server"].value("port", ""),
                            jsResultBody["server"].value("key",""),
                            jsResultBody["server"].value("checksum",""),
                            jsResultBody["server"].value("version", SDK_VERSION),
                            jsResultBody["server"].value("playerCount", ""),
                            jsResultBody["server"].value("maxPlayers", ""),
                            jsResultBody["server"].value("timeStamp", 0),
                            jsResultBody["server"].value("publicRef", ""),
                            jsResultBody["server"].value("cachedId", ""),
                    };
                    return true;
                }
                else
                {
                    if (jsResultBody["error"].is_string())
                    {
                        svOutMessage = jsResultBody["error"].get<string>();
                    }
                    else
                    {
                        svOutMessage = string("Unknown error with status: ") + std::to_string(htResult->status);
                    }

                    slOutServer = NetGameServer_t{};
                    return false;
                }
            }
        }
        else
        {
            if (htResult)
            {
                if (!htResult->body.empty())
                {
                    nlohmann::json jsResultBody = nlohmann::json::parse(htResult->body);

                    if (jsResultBody["error"].is_string())
                    {
                        svOutMessage = jsResultBody["error"].get<string>();
                    }
                    else
                    {
                        svOutMessage = string("Server not found: ") + std::to_string(htResult->status);
                    }

                    return false;
                }

                svOutMessage = string("Failed HTTP request: ") + std::to_string(htResult->status);
                return false;
            }

            svOutMessage = "Failed to reach comp-server: connection timed-out";
            slOutServer = NetGameServer_t{};
            return false;
        }
    }
    catch (const std::exception& ex)
    {
        Warning(eDLL_T::ENGINE, "%s - Exception while parsing comp-server response:\n%s\n", __FUNCTION__, ex.what());
    }

    return false;
}

//-----------------------------------------------------------------------------
// Purpose: Sends host server POST request.
// Input  : &svOutMessage - 
//			&svOutToken - 
//			&slServerListing - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPylon::PostServerHost(string& svOutMessage, string& svOutToken, const NetGameServer_t& slServerListing) const
{
    nlohmann::json jsRequestBody = nlohmann::json::object();
    jsRequestBody["name"] = slServerListing.m_svHostName;
    jsRequestBody["description"] = slServerListing.m_svDescription;
    jsRequestBody["hidden"] = slServerListing.m_bHidden;
    jsRequestBody["map"] = slServerListing.m_svHostMap;
    jsRequestBody["playlist"] = slServerListing.m_svPlaylist;
    jsRequestBody["ip"] = slServerListing.m_svIpAddress;
    jsRequestBody["port"] = slServerListing.m_svGamePort;
    jsRequestBody["key"] = slServerListing.m_svEncryptionKey;
    jsRequestBody["checksum"] = slServerListing.m_svRemoteChecksum;
    jsRequestBody["version"] = slServerListing.m_svSDKVersion;
    jsRequestBody["playerCount"] = slServerListing.m_svPlayerCount;
    jsRequestBody["maxPlayers"] = slServerListing.m_svMaxPlayers;
    jsRequestBody["timeStamp"] = slServerListing.m_nTimeStamp;
    jsRequestBody["publicRef"] = slServerListing.m_svPublicRef;
    jsRequestBody["cachedId"] = slServerListing.m_svCachedId;

    string svRequestBody = jsRequestBody.dump(4);
    if (pylon_showdebuginfo->GetBool())
    {
        DevMsg(eDLL_T::ENGINE, "%s - Sending post host request to comp-server:\n%s\n", __FUNCTION__, svRequestBody.c_str());
    }

    httplib::Client htClient(pylon_matchmaking_hostname->GetString()); htClient.set_connection_timeout(10);
    httplib::Result htResult = htClient.Post("/servers/add", svRequestBody.c_str(), svRequestBody.length(), "application/json");

    if (htResult && pylon_showdebuginfo->GetBool())
    {
        DevMsg(eDLL_T::ENGINE, "%s - Comp-server replied with '%d'\n", __FUNCTION__, htResult->status);
    }

    try
    {
        if (htResult && htResult->status == 200) // STATUS_OK
        {
            nlohmann::json jsResultBody = nlohmann::json::parse(htResult->body);
            if (jsResultBody["success"].is_boolean() && jsResultBody["success"].get<bool>())
            {
                if (jsResultBody["token"].is_string())
                {
                    svOutToken = jsResultBody["token"].get<string>();
                }
                else
                {
                    svOutToken = string();
                }

                return true;
            }
            else
            {
                if (jsResultBody["error"].is_string())
                {
                    svOutMessage = jsResultBody["error"].get<string>();
                }
                else
                {
                    svOutMessage = string("Unknown error with status: ") + std::to_string(htResult->status);
                }
                return false;
            }
        }
        else
        {
            if (htResult)
            {
                if (!htResult->body.empty())
                {
                    nlohmann::json jsResultBody = nlohmann::json::parse(htResult->body);

                    if (jsResultBody["error"].is_string())
                    {
                        svOutMessage = jsResultBody["error"].get<string>();
                    }
                    else
                    {
                        svOutMessage = string("Failed HTTP request: ") + std::to_string(htResult->status);
                    }

                    svOutToken = string();
                    return false;
                }

                svOutToken = string();
                svOutMessage = string("Failed HTTP request: ") + std::to_string(htResult->status);
                return false;
            }

            svOutToken = string();
            svOutMessage = "Failed to reach comp-server: connection timed-out";
            return false;
        }
    }
    catch (const std::exception& ex)
    {
        Warning(eDLL_T::ENGINE, "%s - Exception while parsing comp-server response:\n%s\n", __FUNCTION__, ex.what());
    }

    return false;
}

//-----------------------------------------------------------------------------
// Purpose: Send keep alive request to Pylon Master Server.
// Input  : &netGameServer - 
// Output : Returns true on success, false otherwise.
//-----------------------------------------------------------------------------
bool CPylon::KeepAlive(const NetGameServer_t& netGameServer) const
{
#ifndef CLIENT_DLL
    if (g_pServer->IsActive() && sv_pylonVisibility->GetBool()) // Check for active game.
    {
        string m_szHostToken;
        string m_szHostRequestMessage;

        bool result = g_pMasterServer->PostServerHost(m_szHostRequestMessage, m_szHostToken, netGameServer);
        return result;
    }
#endif // !CLIENT_DLL
    return false;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if client is banned on the comp server.
// Input  : svIpAddress - 
//			nNucleusID - 
//			&svOutReason - 
// Output : Returns true if banned, false if not banned.
//-----------------------------------------------------------------------------
bool CPylon::CheckForBan(const string& svIpAddress, const uint64_t nNucleusID, string& svOutReason) const
{
    nlohmann::json jsRequestBody = nlohmann::json::object();
    jsRequestBody["id"] = nNucleusID;
    jsRequestBody["ip"] = svIpAddress;

    httplib::Client htClient(pylon_matchmaking_hostname->GetString()); htClient.set_connection_timeout(10);
    httplib::Result htResult = htClient.Post("/banlist/isBanned", jsRequestBody.dump(4).c_str(), jsRequestBody.dump(4).length(), "application/json");

    try
    {
        if (htResult && htResult->status == 200)
        {
            nlohmann::json jsResultBody = nlohmann::json::parse(htResult->body);
            if (jsResultBody["success"].is_boolean() && jsResultBody["success"].get<bool>())
            {
                if (jsResultBody["banned"].is_boolean() && jsResultBody["banned"].get<bool>())
                {
                    svOutReason = jsResultBody.value("reason", "#DISCONNECT_BANNED");
                    return true;
                }
            }
        }
    }
    catch (const std::exception& ex)
    {
        Warning(eDLL_T::ENGINE, "%s - Exception while parsing comp-server response:\n%s\n", __FUNCTION__, ex.what());
    }
    return false;
}
///////////////////////////////////////////////////////////////////////////////
CPylon* g_pMasterServer(new CPylon());
