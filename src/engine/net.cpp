//=============================================================================//
//
// Purpose: Net system utilities
//
//=============================================================================//

#include "core/stdafx.h"
#include "engine/net.h"
#ifndef _TOOLS
#include "tier1/cvar.h"
#include "tier2/cryptutils.h"
#include "mathlib/color.h"
#include "net.h"
#include "net_chan.h"
#ifndef CLIENT_DLL
#include "server/server.h"
#include "client/client.h"
#endif // !CLIENT_DLL
#endif // !_TOOLS

#ifndef _TOOLS
static void NET_GetKey_f()
{
	NET_PrintKey();
}
static void NET_SetKey_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	NET_SetKey(args.Arg(1));
}
static void NET_GenerateKey_f()
{
	NET_GenerateKey();
}

void NET_UseRandomKeyChanged_f(IConVar* pConVar, const char* pOldString, float flOldValue, ChangeUserData_t pUserData)
{
	if (ConVar* pConVarRef = g_pCVar->FindVar(pConVar->GetName()))
	{
		if (strcmp(pOldString, pConVarRef->GetString()) == NULL)
			return; // Same value.

		if (pConVarRef->GetBool())
			NET_GenerateKey();
		else
			NET_SetKey(DEFAULT_NET_ENCRYPTION_KEY);
	}
}

ConVar net_useRandomKey("net_useRandomKey", "1", FCVAR_RELEASE, "Use random AES encryption key for game packets.", false, 0.f, false, 0.f, &NET_UseRandomKeyChanged_f, nullptr);

static ConVar net_tracePayload("net_tracePayload", "0", FCVAR_DEVELOPMENTONLY, "Log the payload of the send/recv datagram to a file on the disk.");
static ConVar net_encryptionEnable("net_encryptionEnable", "1", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Use AES encryption on game packets.");

static ConCommand net_getkey("net_getkey", NET_GetKey_f, "Gets the installed base64 net key", FCVAR_RELEASE);
static ConCommand net_setkey("net_setkey", NET_SetKey_f, "Sets user specified base64 net key", FCVAR_RELEASE);
static ConCommand net_generatekey("net_generatekey", NET_GenerateKey_f, "Generates and sets a random base64 net key", FCVAR_RELEASE);

//-----------------------------------------------------------------------------
// Purpose: hook and log the receive datagram
// Input  : iSocket - 
//			*pInpacket - 
//			bEncrypted - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool NET_ReceiveDatagram(int iSocket, netpacket_s* pInpacket, bool bEncrypted)
{
	const bool decryptPacket = (bEncrypted && net_encryptionEnable.GetBool());
	const bool result = v_NET_ReceiveDatagram(iSocket, pInpacket, decryptPacket);

	if (result && net_tracePayload.GetBool())
	{
		// Log received packet data.
		HexDump("[+] NET_ReceiveDatagram ", "net_trace",
			pInpacket->pData, size_t(pInpacket->wiresize));
	}

	return result;
}

//-----------------------------------------------------------------------------
// Purpose: hook and log the send datagram
// Input  : s - 
//			*pPayload - 
//			iLenght - 
//			*pAdr - 
//			bEncrypt - 
// Output : outgoing sequence number for this packet
//-----------------------------------------------------------------------------
int NET_SendDatagram(SOCKET s, void* pPayload, int iLenght, netadr_t* pAdr, bool bEncrypt)
{
	const bool encryptPacket = (bEncrypt && net_encryptionEnable.GetBool());
	const int result = v_NET_SendDatagram(s, pPayload, iLenght, pAdr, encryptPacket);

	if (result && net_tracePayload.GetBool())
	{
		// Log transmitted packet data.
		HexDump("[+] NET_SendDatagram ", "net_trace", pPayload, size_t(iLenght));
	}

	return result;
}

//-----------------------------------------------------------------------------
// Purpose: compresses the input buffer into the output buffer
// Input  : *dest - 
//			*destLen - 
//			*source - 
//			sourceLen - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool NET_BufferToBufferCompress(uint8_t* const dest, size_t* const destLen, uint8_t* const source, const size_t sourceLen)
{
	CLZSS lzss;
	uint32_t compLen = (uint32_t)sourceLen;

	if (!lzss.CompressNoAlloc(source, (uint32_t)sourceLen, dest, &compLen))
	{
		memcpy(dest, source, sourceLen);

		*destLen = sourceLen;
		return false;
	}

	*destLen = compLen;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: decompresses the input buffer into the output buffer
// Input  : *source - 
//			&sourceLen - 
//			*dest - 
//			destLen - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
unsigned int NET_BufferToBufferDecompress(uint8_t* const source, size_t& sourceLen, uint8_t* const dest, const size_t destLen)
{
	Assert(source);
	Assert(sourceLen);

	CLZSS lzss;

	if (lzss.IsCompressed(source))
	{
		return lzss.SafeUncompress(source, dest, (unsigned int)destLen);
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: safely decompresses the input buffer into the output buffer
// Input  : *lzss - 
//			*pInput - 
//			*pOutput - 
//			unBufSize - 
// Output : total decompressed bytes
//-----------------------------------------------------------------------------
unsigned int NET_BufferToBufferDecompress_LZSS(CLZSS* lzss, unsigned char* pInput, unsigned char* pOutput, unsigned int unBufSize)
{
	return lzss->SafeUncompress(pInput, pOutput, unBufSize);
}

//-----------------------------------------------------------------------------
// Purpose: configures the network system
//-----------------------------------------------------------------------------
void NET_Config()
{
	v_NET_Config();
	g_pNetAdr->SetPort(htons(u_short(hostport->GetInt())));
}

//-----------------------------------------------------------------------------
// Purpose: prints the currently installed encryption key
//-----------------------------------------------------------------------------
void NET_PrintKey()
{
	Msg(eDLL_T::ENGINE, "Installed NetKey: %s'%s%s%s'\n",
		g_svReset.c_str(), g_svGreyB.c_str(), g_pNetKey->GetBase64NetKey(), g_svReset.c_str());
}

//-----------------------------------------------------------------------------
// Purpose: sets the user specified encryption key
// Input  : svNetKey - 
//-----------------------------------------------------------------------------
void NET_SetKey(const string& svNetKey)
{
	string svTokenizedKey;

	if (svNetKey.size() == AES_128_B64_ENCODED_SIZE &&
		IsValidBase64(svNetKey, &svTokenizedKey)) // Results are tokenized by 'IsValidBase64()'.
	{
		v_NET_SetKey(g_pNetKey, svTokenizedKey.c_str());
		NET_PrintKey();
	}
	else
	{
		Error(eDLL_T::ENGINE, NO_ERROR, "AES-128 key not encoded or invalid\n");
	}
}

//-----------------------------------------------------------------------------
// Purpose: calculates and sets the encryption key
//-----------------------------------------------------------------------------
void NET_GenerateKey()
{
	if (!net_useRandomKey.GetBool())
	{
		net_useRandomKey.SetValue(1);
		return; // Change callback will handle this.
	}

	uint8_t keyBuf[AES_128_KEY_SIZE];
	const char* errorMsg = nullptr;

	if (!Plat_GenerateRandom(keyBuf, sizeof(keyBuf), errorMsg))
	{
		Error(eDLL_T::ENGINE, NO_ERROR, "%s\n", errorMsg);
		return;
	}

	NET_SetKey(Base64Encode(string(reinterpret_cast<char*>(&keyBuf), AES_128_KEY_SIZE)));
}

//-----------------------------------------------------------------------------
// Purpose: hook and log the client's signonstate to the console
// Input  : *fmt - 
//			... - 
//-----------------------------------------------------------------------------
void NET_PrintFunc(const char* fmt, ...)
{
#ifndef DEDICATED
	const static eDLL_T context = eDLL_T::CLIENT;
#else // !DEDICATED
	const static eDLL_T context = eDLL_T::SERVER;
#endif

	string result;

	va_list args;
	va_start(args, fmt);
	result = FormatV(fmt, args);
	va_end(args);

	if (result.back() != '\n')
	{
		result.push_back('\n');
	}

	Msg(context, "%s", result.c_str());
}

//-----------------------------------------------------------------------------
// Purpose: disconnect the client and shutdown netchannel
// Input  : *pClient - 
//			nIndex - 
//			*szReason - 
//			bBadRep - 
//			bRemoveNow - 
//-----------------------------------------------------------------------------
void NET_RemoveChannel(CClient* pClient, int nIndex, const char* szReason, uint8_t bBadRep, bool bRemoveNow)
{
#ifndef CLIENT_DLL
	if (!pClient || std::strlen(szReason) == NULL || !pClient->GetNetChan())
	{
		return;
	}

	pClient->GetNetChan()->Shutdown(szReason, bBadRep, bRemoveNow); // Shutdown NetChannel.
	pClient->Clear();                                               // Reset CClient slot.
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: reads the net message type from buffer
// Input  : &outType - 
//			&buffer - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool NET_ReadMessageType(int* outType, bf_read* buffer)
{
	*outType = buffer->ReadUBitLong(NETMSG_TYPE_BITS);
	return !buffer->IsOverflowed();
}

//-----------------------------------------------------------------------------
// Purpose: checks whether the provided address is the local server.
// Input  : &netAdr - 
// Output : true if equal, false otherwise
//-----------------------------------------------------------------------------
bool NET_IsRemoteLocal(const CNetAdr& netAdr)
{
	return (g_pNetAdr->ComparePort(netAdr) && g_pNetAdr->CompareAdr(netAdr));
}

#endif // !_TOOLS

//-----------------------------------------------------------------------------
// Purpose: returns the WSA error code
//-----------------------------------------------------------------------------
const char* NET_ErrorString(int iCode)
{
	switch (iCode)
	{
		case WSAEINTR                   : return "WSAEINTR";
		case WSAEBADF                   : return "WSAEBADF";
		case WSAEACCES                  : return "WSAEACCES";
		case WSAEFAULT                  : return "WSAEFAULT";
		case WSAEINVAL                  : return "WSAEINVAL";
		case WSAEMFILE                  : return "WSAEMFILE";
		case WSAEWOULDBLOCK             : return "WSAEWOULDBLOCK";
		case WSAEINPROGRESS             : return "WSAEINPROGRESS";
		case WSAEALREADY                : return "WSAEALREADY";
		case WSAENOTSOCK                : return "WSAENOTSOCK";
		case WSAEDESTADDRREQ            : return "WSAEDESTADDRREQ";
		case WSAEMSGSIZE                : return "WSAEMSGSIZE";
		case WSAEPROTOTYPE              : return "WSAEPROTOTYPE";
		case WSAENOPROTOOPT             : return "WSAENOPROTOOPT";
		case WSAEPROTONOSUPPORT         : return "WSAEPROTONOSUPPORT";
		case WSAESOCKTNOSUPPORT         : return "WSAESOCKTNOSUPPORT";
		case WSAEOPNOTSUPP              : return "WSAEOPNOTSUPP";
		case WSAEPFNOSUPPORT            : return "WSAEPFNOSUPPORT";
		case WSAEAFNOSUPPORT            : return "WSAEAFNOSUPPORT";
		case WSAEADDRINUSE              : return "WSAEADDRINUSE";
		case WSAEADDRNOTAVAIL           : return "WSAEADDRNOTAVAIL";
		case WSAENETDOWN                : return "WSAENETDOWN";
		case WSAENETUNREACH             : return "WSAENETUNREACH";
		case WSAENETRESET               : return "WSAENETRESET";
		case WSAECONNABORTED            : return "WSAECONNABORTED";
		case WSAECONNRESET              : return "WSAECONNRESET";
		case WSAENOBUFS                 : return "WSAENOBUFS";
		case WSAEISCONN                 : return "WSAEISCONN";
		case WSAENOTCONN                : return "WSAENOTCONN";
		case WSAESHUTDOWN               : return "WSAESHUTDOWN";
		case WSAETOOMANYREFS            : return "WSAETOOMANYREFS";
		case WSAETIMEDOUT               : return "WSAETIMEDOUT";
		case WSAECONNREFUSED            : return "WSAECONNREFUSED";
		case WSAELOOP                   : return "WSAELOOP";
		case WSAENAMETOOLONG            : return "WSAENAMETOOLONG";
		case WSAEHOSTDOWN               : return "WSAEHOSTDOWN";
		case WSAEHOSTUNREACH            : return "WSAEHOSTUNREACH";
		case WSAENOTEMPTY               : return "WSAENOTEMPTY";
		case WSAEPROCLIM                : return "WSAEPROCLIM";
		case WSAEUSERS                  : return "WSAEUSERS";
		case WSAEDQUOT                  : return "WSAEDQUOT";
		case WSAESTALE                  : return "WSAESTALE";
		case WSAEREMOTE                 : return "WSAEREMOTE";
		case WSASYSNOTREADY             : return "WSASYSNOTREADY";
		case WSAVERNOTSUPPORTED         : return "WSAVERNOTSUPPORTED";
		case WSANOTINITIALISED          : return "WSANOTINITIALISED";
		case WSAEDISCON                 : return "WSAEDISCON";
		case WSAENOMORE                 : return "WSAENOMORE";
		case WSAECANCELLED              : return "WSAECANCELLED";
		case WSAEINVALIDPROCTABLE       : return "WSAEINVALIDPROCTABLE";
		case WSAEINVALIDPROVIDER        : return "WSAEINVALIDPROVIDER";
		case WSAEPROVIDERFAILEDINIT     : return "WSAEPROVIDERFAILEDINIT";
		case WSASYSCALLFAILURE          : return "WSASYSCALLFAILURE";
		case WSASERVICE_NOT_FOUND       : return "WSASERVICE_NOT_FOUND";
		case WSATYPE_NOT_FOUND          : return "WSATYPE_NOT_FOUND";
		case WSA_E_NO_MORE              : return "WSA_E_NO_MORE";
		case WSA_E_CANCELLED            : return "WSA_E_CANCELLED";
		case WSAEREFUSED                : return "WSAEREFUSED";
		case WSAHOST_NOT_FOUND          : return "WSAHOST_NOT_FOUND";
		case WSATRY_AGAIN               : return "WSATRY_AGAIN";
		case WSANO_RECOVERY             : return "WSANO_RECOVERY";
		case WSANO_DATA                 : return "WSANO_DATA";
		case WSA_QOS_RECEIVERS          : return "WSA_QOS_RECEIVERS";
		case WSA_QOS_SENDERS            : return "WSA_QOS_SENDERS";
		case WSA_QOS_NO_SENDERS         : return "WSA_QOS_NO_SENDERS";
		case WSA_QOS_NO_RECEIVERS       : return "WSA_QOS_NO_RECEIVERS";
		case WSA_QOS_REQUEST_CONFIRMED  : return "WSA_QOS_REQUEST_CONFIRMED";
		case WSA_QOS_ADMISSION_FAILURE  : return "WSA_QOS_ADMISSION_FAILURE";
		case WSA_QOS_POLICY_FAILURE     : return "WSA_QOS_POLICY_FAILURE";
		case WSA_QOS_BAD_STYLE          : return "WSA_QOS_BAD_STYLE";
		case WSA_QOS_BAD_OBJECT         : return "WSA_QOS_BAD_OBJECT";
		case WSA_QOS_TRAFFIC_CTRL_ERROR : return "WSA_QOS_TRAFFIC_CTRL_ERROR";
		case WSA_QOS_GENERIC_ERROR      : return "WSA_QOS_GENERIC_ERROR";
		case WSA_QOS_ESERVICETYPE       : return "WSA_QOS_ESERVICETYPE";
		case WSA_QOS_EFLOWSPEC          : return "WSA_QOS_EFLOWSPEC";
		case WSA_QOS_EPROVSPECBUF       : return "WSA_QOS_EPROVSPECBUF";
		case WSA_QOS_EFILTERSTYLE       : return "WSA_QOS_EFILTERSTYLE";
		case WSA_QOS_EFILTERTYPE        : return "WSA_QOS_EFILTERTYPE";
		case WSA_QOS_EFILTERCOUNT       : return "WSA_QOS_EFILTERCOUNT";
		case WSA_QOS_EOBJLENGTH         : return "WSA_QOS_EOBJLENGTH";
		case WSA_QOS_EFLOWCOUNT         : return "WSA_QOS_EFLOWCOUNT";
		case WSA_QOS_EUNKOWNPSOBJ       : return "WSA_QOS_EUNKOWNPSOBJ";
		case WSA_QOS_EPOLICYOBJ         : return "WSA_QOS_EPOLICYOBJ";
		case WSA_QOS_EFLOWDESC          : return "WSA_QOS_EFLOWDESC";
		case WSA_QOS_EPSFLOWSPEC        : return "WSA_QOS_EPSFLOWSPEC";
		case WSA_QOS_EPSFILTERSPEC      : return "WSA_QOS_EPSFILTERSPEC";
		case WSA_QOS_ESDMODEOBJ         : return "WSA_QOS_ESDMODEOBJ";
		case WSA_QOS_ESHAPERATEOBJ      : return "WSA_QOS_ESHAPERATEOBJ";
		case WSA_QOS_RESERVED_PETYPE    : return "WSA_QOS_RESERVED_PETYPE";
		case WSA_SECURE_HOST_NOT_FOUND  : return "WSA_SECURE_HOST_NOT_FOUND";
		case WSA_IPSEC_NAME_POLICY_ERROR: return "WSA_IPSEC_NAME_POLICY_ERROR";
	default                    : return "UNKNOWN_ERROR";
	}
}

#ifndef _TOOLS
///////////////////////////////////////////////////////////////////////////////
void VNet::Detour(const bool bAttach) const
{
	DetourSetup(&v_NET_Config, &NET_Config, bAttach);
	DetourSetup(&v_NET_ReceiveDatagram, &NET_ReceiveDatagram, bAttach);
	DetourSetup(&v_NET_SendDatagram, &NET_SendDatagram, bAttach);

	DetourSetup(&v_NET_BufferToBufferCompress, &NET_BufferToBufferCompress, bAttach);
	DetourSetup(&v_NET_BufferToBufferDecompress_LZSS, &NET_BufferToBufferDecompress_LZSS, bAttach);
	DetourSetup(&v_NET_PrintFunc, &NET_PrintFunc, bAttach);
}

///////////////////////////////////////////////////////////////////////////////
netadr_t* g_pNetAdr = nullptr;
netkey_t* g_pNetKey = nullptr;

double* g_pNetTime = nullptr;
#endif // !_TOOLS
