//=============================================================================//
//
// Purpose: Implementation of the pylon client.
//
// $NoKeywords: $
//=============================================================================//

#include <core/stdafx.h>
#include <tier1/cvar.h>
#include <tier2/curlutils.h>
#include <tier2/jsonutils.h>
#include <networksystem/pylon.h>
#include <engine/server/server.h>

//-----------------------------------------------------------------------------
// Console variables
//-----------------------------------------------------------------------------
ConVar pylon_matchmaking_enabled("pylon_matchmaking_enabled", "1", FCVAR_RELEASE | FCVAR_ACCESSIBLE_FROM_THREADS, "Whether to use the pylon matchmaking server");
ConVar pylon_matchmaking_hostname("pylon_matchmaking_hostname", "r5r.org", FCVAR_RELEASE | FCVAR_ACCESSIBLE_FROM_THREADS, "Holds the pylon matchmaking hostname");
ConVar pylon_host_update_interval("pylon_host_update_interval", "5", FCVAR_RELEASE | FCVAR_ACCESSIBLE_FROM_THREADS, "Length of time in seconds between each status update interval to master server", true, 5.f, false, 0.f);
ConVar pylon_showdebuginfo("pylon_showdebuginfo", "0", FCVAR_RELEASE | FCVAR_ACCESSIBLE_FROM_THREADS, "Shows debug output for pylon");

//-----------------------------------------------------------------------------
// Purpose: checks if server listing fields are valid, and sets outGameServer
// Input  : &value - 
//          &outGameServer - 
// Output : true on success, false on failure.
//-----------------------------------------------------------------------------
static bool GetServerListingFromJSON(const rapidjson::Value& value, NetGameServer_t& outGameServer)
{
    if (JSON_GetValue(value, "name",        outGameServer.name)        &&
        JSON_GetValue(value, "description", outGameServer.description) &&
        JSON_GetValue(value, "hidden",      outGameServer.hidden)      &&
        JSON_GetValue(value, "map",         outGameServer.map)         &&
        JSON_GetValue(value, "playlist",    outGameServer.playlist)    &&
        JSON_GetValue(value, "ip",          outGameServer.address)     &&
        JSON_GetValue(value, "port",        outGameServer.port)        &&
        JSON_GetValue(value, "key",         outGameServer.netKey)      &&
        JSON_GetValue(value, "checksum",    outGameServer.checksum)    &&
        JSON_GetValue(value, "numPlayers",  outGameServer.numPlayers)  &&
        JSON_GetValue(value, "maxPlayers",  outGameServer.maxPlayers))
    {
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
// Purpose: gets a vector of hosted servers.
// Input  : &outMessage - 
// Output : true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPylon::GetServerList(vector<NetGameServer_t>& outServerList, string& outMessage) const
{
    if (!IsEnabled())
    {
        SetDisabledMessage(outMessage);
        return false;
    }

    rapidjson::Document requestJson;
    requestJson.SetObject();
    requestJson.AddMember("version", SDK_VERSION, requestJson.GetAllocator());

    rapidjson::StringBuffer stringBuffer;
    JSON_DocumentToBufferDeserialize(requestJson, stringBuffer);

    rapidjson::Document responseJson;
    CURLINFO status;

    if (!SendRequest("/servers", requestJson, responseJson,
        outMessage, status, "server list error"))
    {
        return false;
    }

    rapidjson::Document::ConstMemberIterator serversIt;

    if (!JSON_GetIterator(responseJson, "servers", JSONFieldType_e::kArray, serversIt))
    {
        outMessage = Format("Invalid response with status: %d", int(status));
        return false;
    }

    const rapidjson::Value::ConstArray serverArray = serversIt->value.GetArray();

    for (const rapidjson::Value& obj : serverArray)
    {
        NetGameServer_t gameServer;

        if (!GetServerListingFromJSON(obj, gameServer))
        {
            // Missing details; skip this server listing.
            continue;
        }

        outServerList.push_back(gameServer);
    }

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Gets the server by token string.
// Input  : &outGameServer - 
//			&outMessage - 
//			&token - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPylon::GetServerByToken(NetGameServer_t& outGameServer,
    string& outMessage, const string& token) const
{
    if (!IsEnabled())
    {
        SetDisabledMessage(outMessage);
        return false;
    }

    rapidjson::Document requestJson;
    requestJson.SetObject();

    rapidjson::Document::AllocatorType& allocator = requestJson.GetAllocator();
    requestJson.AddMember("version", rapidjson::Value(SDK_VERSION, requestJson.GetAllocator()), allocator);
    requestJson.AddMember("token", rapidjson::Value(token.c_str(), requestJson.GetAllocator()), allocator);

    rapidjson::Document responseJson;
    CURLINFO status;

    if (!SendRequest("/server/byToken", requestJson, responseJson,
        outMessage, status, "server not found"))
    {
        return false;
    }

    rapidjson::Document::ConstMemberIterator serversIt;

    if (!JSON_GetIterator(responseJson, "server", JSONFieldType_e::kObject, serversIt))
    {
        outMessage = Format("Invalid response with status: %d", int(status));
        return false;
    }

    const rapidjson::Value& serverJson = serversIt->value;

    if (!GetServerListingFromJSON(serverJson, outGameServer))
    {
        outMessage = Format("Invalid server listing data!");
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Sends host server POST request.
// Input  : &outMessage - 
//			&outToken - 
//			&netGameServer - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPylon::PostServerHost(string& outMessage, string& outToken, string& outHostIp, const NetGameServer_t& netGameServer) const
{
    if (!IsEnabled())
    {
        SetDisabledMessage(outMessage);
        return false;
    }

    rapidjson::Document requestJson;
    requestJson.SetObject();

    rapidjson::Document::AllocatorType& allocator = requestJson.GetAllocator();

    requestJson.AddMember("name",        rapidjson::Value(netGameServer.name.c_str(),        allocator), allocator);
    requestJson.AddMember("description", rapidjson::Value(netGameServer.description.c_str(), allocator), allocator);
    requestJson.AddMember("hidden",      netGameServer.hidden,                               allocator);
    requestJson.AddMember("map",         rapidjson::Value(netGameServer.map.c_str(),         allocator), allocator);
    requestJson.AddMember("playlist",    rapidjson::Value(netGameServer.playlist.c_str(),    allocator), allocator);
    requestJson.AddMember("ip",          rapidjson::Value(netGameServer.address.c_str(),     allocator), allocator);
    requestJson.AddMember("port",        netGameServer.port,                                 allocator);
    requestJson.AddMember("key",         rapidjson::Value(netGameServer.netKey.c_str(),      allocator), allocator);
    requestJson.AddMember("checksum",    netGameServer.checksum,                             allocator);
    requestJson.AddMember("version",     rapidjson::Value(netGameServer.versionId.c_str(),   allocator), allocator);
    requestJson.AddMember("numPlayers",  netGameServer.numPlayers,                           allocator);
    requestJson.AddMember("maxPlayers",  netGameServer.maxPlayers,                           allocator);
    requestJson.AddMember("timeStamp",   netGameServer.timeStamp,                            allocator);

    rapidjson::Document responseJson;
    CURLINFO status;

    if (!SendRequest("/servers/add", requestJson, responseJson, outMessage, status, "server host error"))
    {
        return false;
    }

    if (netGameServer.hidden)
    {
        const char* token = nullptr;

        if (!JSON_GetValue(responseJson, "token", token))
        {
            outMessage = Format("Invalid response with status: %d", int(status));
            outToken.clear();
            return false;
        }

        outToken = token;
    }

    const char* ip = nullptr;
    int port = 0;

    if (JSON_GetValue(responseJson, "ip", ip) &&
        JSON_GetValue(responseJson, "port", port))
    {
        outHostIp = Format("[%s]:%i", ip, port);
    }

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Checks a list of clients for their banned status.
// Input  : &inBannedVec - 
//			**outBannedVec  - allocated; caller is responsible for freeing it
// Output : True on success, false otherwise.
//-----------------------------------------------------------------------------
bool CPylon::GetBannedList(const CBanSystem::BannedList_t& inBannedVec, CBanSystem::BannedList_t** outBannedVec) const
{
    if (!IsEnabled())
        return false;

    rapidjson::Document requestJson;
    requestJson.SetObject();

    rapidjson::Value playersArray(rapidjson::kArrayType);

    rapidjson::Document::AllocatorType& allocator = requestJson.GetAllocator();

    FOR_EACH_VEC(inBannedVec, i)
    {
        const CBanSystem::Banned_t& banned = inBannedVec[i];
        rapidjson::Value player(rapidjson::kObjectType);

        player.AddMember("id", banned.m_NucleusID, allocator);
        player.AddMember("ip", rapidjson::Value(banned.m_Address.String(), allocator), allocator);

        playersArray.PushBack(player, allocator);
    }

    requestJson.AddMember("players", playersArray, allocator);

    rapidjson::Document responseJson;

    string outMessage;
    CURLINFO status;

    if (!SendRequest("/banlist/bulkCheck", requestJson, responseJson, outMessage, status, "banned bulk check error"))
    {
        return false;
    }

    rapidjson::Value::ConstMemberIterator bannedPlayersIt;

    if (!JSON_GetIterator(responseJson, "bannedPlayers", JSONFieldType_e::kArray, bannedPlayersIt))
    {
        outMessage = Format("Invalid response with status: %d", int(status));
        return false;
    }

    const rapidjson::Value::ConstArray bannedPlayers = bannedPlayersIt->value.GetArray();

    if (bannedPlayers.Empty())
        return false;

    *outBannedVec = new CBanSystem::BannedList_t();
    Assert(*outBannedVec);

    for (const rapidjson::Value& obj : bannedPlayers)
    {
        const char* reason = nullptr;
        JSON_GetValue(obj, "reason", reason);

        NucleusID_t nuc = NULL;
        JSON_GetValue(obj, "id", nuc);

        CBanSystem::Banned_t banned(reason ? reason : "#DISCONNECT_BANNED", nuc);
        (*outBannedVec)->AddToTail(banned);
    }

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if client is banned on the comp server.
// Input  : &ipAddress - 
//			nucleusId  - 
//			&outReason - <- contains banned reason if any.
// Output : True if banned, false if not banned.
//-----------------------------------------------------------------------------
bool CPylon::CheckForBan(const string& ipAddress, const uint64_t nucleusId, const string& personaName, string& outReason) const
{
    if (!IsEnabled())
    {
        SetDisabledMessage(outReason);
        return false;
    }

    rapidjson::Document requestJson;
    requestJson.SetObject();

    rapidjson::Document::AllocatorType& allocator = requestJson.GetAllocator();

    requestJson.AddMember("name", rapidjson::Value(personaName.c_str(), allocator), allocator);
    requestJson.AddMember("id", nucleusId, allocator);
    requestJson.AddMember("ip", rapidjson::Value(ipAddress.c_str(), allocator), allocator);

    rapidjson::Document responseJson;
    string outMessage;
    CURLINFO status;

    if (!SendRequest("/banlist/isBanned", requestJson, responseJson, outMessage, status, "banned check error"))
    {
        return false;
    }

    bool isBanned = false;

    if (JSON_GetValue(responseJson, "banned", isBanned))
    {
        if (isBanned)
        {
            const char* reason = nullptr;

            outReason = JSON_GetValue(responseJson, "reason", reason)
                ? reason
                : "#DISCONNECT_BANNED";

            return true;
        }
    }

    return false;
}

//-----------------------------------------------------------------------------
// Purpose: authenticate for 'this' particular connection.
// Input  : nucleusId   - 
//          *ipAddress  - 
//          *authCode   - 
//          &outToken   - 
//          &outMessage - 
// Output : true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPylon::AuthForConnection(const uint64_t nucleusId, const char* ipAddress,
    const char* authCode, string& outToken, string& outMessage) const
{
    if (!IsEnabled())
    {
        SetDisabledMessage(outMessage);
        return false;
    }

    rapidjson::Document requestJson;
    requestJson.SetObject();

    rapidjson::Document::AllocatorType& allocator = requestJson.GetAllocator();

    requestJson.AddMember("id", nucleusId, allocator);
    requestJson.AddMember("ip", rapidjson::Value(ipAddress, allocator), allocator);
    requestJson.AddMember("code", rapidjson::Value(authCode, allocator), allocator);

    rapidjson::Document responseJson;

    CURLINFO status;

    if (!SendRequest("/client/authenticate", requestJson, responseJson, outMessage, status, "origin auth error"))
    {
        return false;
    }

    const char* token = nullptr;

    if (JSON_GetValue(responseJson, "token", token))
    {
        outToken = token;
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
// Purpose: Gets the EULA from master server.
// Input  : &outData    -
//          &outMessage - 
// Output : True on success, false on failure.
//-----------------------------------------------------------------------------
bool CPylon::GetEULA(MSEulaData_t& outData, string& outMessage) const
{
    if (!IsEnabled())
    {
        SetDisabledMessage(outMessage);
        return false;
    }

    rapidjson::Document requestJson;
    requestJson.SetObject();

    rapidjson::Document responseJson;
    CURLINFO status;

    if (!SendRequest("/eula", requestJson, responseJson, outMessage, status, "eula fetch error", false))
    {
        return false;
    }

    rapidjson::Document::ConstMemberIterator serversIt;

    if (!JSON_GetIterator(responseJson, "data", JSONFieldType_e::kObject, serversIt))
    {
        outMessage = "missing or invalid data";
        return false;
    }

    const rapidjson::Value& data = serversIt->value;

    int version = 0;
    const char* language = nullptr;
    const char* contents = nullptr;

    // check if the EULA response fields are valid.
    if (!JSON_GetValue(data, "version", version) ||
        !JSON_GetValue(data, "language", language) ||
        !JSON_GetValue(data, "contents", contents))
    {
        outMessage = "schema is invalid";
        return false;
    }

    outData.version = version;
    outData.language = language;
    outData.contents = contents;

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Sends request to Pylon Master Server.
// Input  : *endpoint -
//			&requestJson -
//			&responseJson -
//			&outMessage -
//			&status -
//			checkEula - 
// Output : True on success, false on failure.
//-----------------------------------------------------------------------------
bool CPylon::SendRequest(const char* endpoint, const rapidjson::Document& requestJson,
    rapidjson::Document& responseJson, string& outMessage, CURLINFO& status,
    const char* errorText, const bool checkEula) const
{
    if (!IsDedicated() && !IsEULAUpToDate() && checkEula)
    {
        outMessage = "EULA not accepted";
        return false;
    }

    rapidjson::StringBuffer stringBuffer;
    JSON_DocumentToBufferDeserialize(requestJson, stringBuffer);

    string responseBody;
    if (!QueryServer(endpoint, stringBuffer.GetString(), responseBody, outMessage, status))
    {
        return false;
    }

    if (status == 200) // STATUS_OK
    {
        responseJson.Parse(responseBody.c_str());

        if (responseJson.HasParseError())
        {
            Warning(eDLL_T::ENGINE, "%s: JSON parse error at position %zu: %s\n", __FUNCTION__,
                responseJson.GetErrorOffset(), rapidjson::GetParseError_En(responseJson.GetParseError()));

            return false;
        }

        if (!responseJson.IsObject())
        {
            Warning(eDLL_T::ENGINE, "%s: JSON root was not an object\n", __FUNCTION__);
            return false;
        }

        if (pylon_showdebuginfo.GetBool())
        {
            LogBody(responseJson);
        }

        bool success = false;

        if (JSON_GetValue(responseJson, "success", success)
            && success)
        {
            return true;
        }
        else
        {
            ExtractError(responseJson, outMessage, status);
            return false;
        }
    }
    else
    {
        ExtractError(responseBody, outMessage, status, errorText);
        return false;
    }
}

//-----------------------------------------------------------------------------
// Purpose: Sends query to master server.
// Input  : *endpoint    - 
//          *request     - 
//          &outResponse - 
//          &outMessage  - <- contains an error message on failure.
//          &outStatus   - 
// Output : True on success, false on failure.
//-----------------------------------------------------------------------------
bool CPylon::QueryServer(const char* endpoint, const char* request,
    string& outResponse, string& outMessage, CURLINFO& outStatus) const
{
    const bool showDebug = pylon_showdebuginfo.GetBool();
    const char* hostName = pylon_matchmaking_hostname.GetString();

    if (showDebug)
    {
        Msg(eDLL_T::ENGINE, "Sending request to '%s' with endpoint '%s':\n%s\n",
            hostName, endpoint, request);
    }

    string finalUrl;
    CURLFormatUrl(finalUrl, hostName, endpoint);
    finalUrl += Format("?language=%s", this->GetLanguage().c_str());

    CURLParams params;

    params.writeFunction = CURLWriteStringCallback;
    params.timeout = curl_timeout.GetInt();
    params.verifyPeer = ssl_verify_peer.GetBool();
    params.verbose = curl_debug.GetBool();

    curl_slist* sList = nullptr;
    CURL* curl = CURLInitRequest(finalUrl.c_str(), request, outResponse, sList, params);
    if (!curl)
    {
        return false;
    }

    CURLcode res = CURLSubmitRequest(curl, sList);
    if (!CURLHandleError(curl, res, outMessage,
        !IsDedicated(/* Errors are already shown for dedicated! */)))
    {
        return false;
    }

    outStatus = CURLRetrieveInfo(curl);

    if (showDebug)
    {
        Msg(eDLL_T::ENGINE, "Host '%s' replied with status: '%d'\n",
            hostName, outStatus);
    }

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Extracts the error from the result json.
// Input  : &resultJson - 
//          &outMessage - 
//          status      - 
//          *errorText  - 
//-----------------------------------------------------------------------------
void CPylon::ExtractError(const rapidjson::Document& resultJson, string& outMessage,
    CURLINFO status, const char* errorText) const
{
    const char* error = nullptr;

    if (resultJson.IsObject() && 
        JSON_GetValue(resultJson, "error", error))
    {
        outMessage = error;
    }
    else
    {
        if (!errorText)
        {
            errorText = "unknown error";
        }

        outMessage = Format("Failed with status: %d (%s)",
            int(status), errorText);
    }
}

//-----------------------------------------------------------------------------
// Purpose: Extracts the error from the response buffer.
// Input  : &response   - 
//          &outMessage - 
//          status      - 
//          *errorText  - 
//-----------------------------------------------------------------------------
void CPylon::ExtractError(const string& response, string& outMessage,
    CURLINFO status, const char* errorText) const
{
    if (!response.empty())
    {
        rapidjson::Document resultBody;
        resultBody.Parse(response.c_str());

        ExtractError(resultBody, outMessage, status, errorText);
    }
    else if (status)
    {
        outMessage = Format("Failed server query: %d", int(status));
    }
    else
    {
        outMessage = Format("Failed to reach server: %s",
            "connection timed out");
    }
}

//-----------------------------------------------------------------------------
// Purpose: Logs the response body if debug is enabled.
// Input  : &responseJson -
//-----------------------------------------------------------------------------
void CPylon::LogBody(const rapidjson::Document& responseJson) const
{
    rapidjson::StringBuffer stringBuffer;

    JSON_DocumentToBufferDeserialize(responseJson, stringBuffer);
    Msg(eDLL_T::ENGINE, "\n%s\n", stringBuffer.GetString());
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether the pylon matchmaking system is enabled.
//-----------------------------------------------------------------------------
bool CPylon::IsEnabled() const
{
    return pylon_matchmaking_enabled.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: If the system is disabled, this will be the default reason message.
//-----------------------------------------------------------------------------
void CPylon::SetDisabledMessage(string& outMsg) const
{
    outMsg = "matchmaking disabled";
}

///////////////////////////////////////////////////////////////////////////////
CPylon g_MasterServer;
