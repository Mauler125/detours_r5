//=============================================================================//
//
// Purpose: ServerGameDLL LiveAPI implementation
// 
// ----------------------------------------------------------------------------
// TODO:
// - Add code callback for observer target changed ( event ObserverSwitched )
// - Add code callback for player weapon switched  ( event WeaponSwitched )
//
//=============================================================================//
#include "tier1/depthcounter.h"
#include "mbedtls/include/mbedtls/sha512.h"
#include "rtech/liveapi/liveapi.h"
#include "engine/sys_utils.h"
#include "vscript/languages/squirrel_re/include/sqtable.h"
#include "vscript/languages/squirrel_re/include/sqarray.h"
#include "game/server/vscript_server.h"
#include "liveapi.h"

#pragma warning(push)
#pragma warning(disable : 4505)
#include "protoc/events.pb.h"
#pragma warning(pop) 

#define LIVEAPI_MAX_ITEM_DEPTH 128 // The total nesting depth cannot exceed this number
#define LIVEAPI_SHA512_HASH_SIZE 64


/*
	███████╗██╗   ██╗███████╗███╗   ██╗████████╗  ███╗   ███╗███████╗███████╗███████╗ █████╗  ██████╗ ███████╗███████╗
	██╔════╝██║   ██║██╔════╝████╗  ██║╚══██╔══╝  ████╗ ████║██╔════╝██╔════╝██╔════╝██╔══██╗██╔════╝ ██╔════╝██╔════╝
	█████╗  ██║   ██║█████╗  ██╔██╗ ██║   ██║     ██╔████╔██║█████╗  ███████╗███████╗███████║██║  ███╗█████╗  ███████╗
	██╔══╝  ╚██╗ ██╔╝██╔══╝  ██║╚██╗██║   ██║     ██║╚██╔╝██║██╔══╝  ╚════██║╚════██║██╔══██║██║   ██║██╔══╝  ╚════██║
	███████╗ ╚████╔╝ ███████╗██║ ╚████║   ██║     ██║ ╚═╝ ██║███████╗███████║███████║██║  ██║╚██████╔╝███████╗███████║
	╚══════╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝   ╚═╝     ╚═╝     ╚═╝╚══════╝╚══════╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚══════╝
	NOTE: messages are statically allocated to save on runtime overhead, each message is cleared after usage.
*/

static rtech::liveapi::AmmoUsed s_ammoUsed;
static rtech::liveapi::ArenasItemDeselected s_arenasItemDeselected;
static rtech::liveapi::ArenasItemSelected s_arenasItemSelected;
static rtech::liveapi::BannerCollected s_bannerCollected;
static rtech::liveapi::BlackMarketAction s_blackMarketAction;
//static rtech::liveapi::ChangeCamera s_changeCamera;
static rtech::liveapi::CharacterSelected s_characterSelected;
//static rtech::liveapi::CustomMatch_CreateLobby s_customMatch_CreateLobby;
//static rtech::liveapi::CustomMatch_GetLobbyPlayers s_customMatch_GetLobbyPlayers;
//static rtech::liveapi::CustomMatch_GetSettings s_customMatch_GetSettings;
//static rtech::liveapi::CustomMatch_JoinLobby s_customMatch_JoinLobby;
//static rtech::liveapi::CustomMatch_KickPlayer s_customMatch_KickPlayer;
//static rtech::liveapi::CustomMatch_LeaveLobby s_customMatch_LeaveLobby;
//static rtech::liveapi::CustomMatch_LobbyPlayers s_customMatch_LobbyPlayers;
//static rtech::liveapi::CustomMatch_SendChat s_customMatch_SendChat;
//static rtech::liveapi::CustomMatch_SetMatchmaking s_customMatch_SetMatchmaking;
//static rtech::liveapi::CustomMatch_SetReady s_customMatch_SetReady;
//static rtech::liveapi::CustomMatch_SetSettings s_customMatch_SetSettings;
//static rtech::liveapi::CustomMatch_SetTeam s_customMatch_SetTeam;
//static rtech::liveapi::CustomMatch_SetTeamName s_customMatch_SetTeamName;
static rtech::liveapi::CustomEvent s_customEvent;
static rtech::liveapi::GameStateChanged s_gameStateChanged;
static rtech::liveapi::GibraltarShieldAbsorbed s_gibraltarShieldAbsorbed;
static rtech::liveapi::GrenadeThrown s_grenadeThrown;
static rtech::liveapi::Init s_init;
static rtech::liveapi::InventoryDrop s_inventoryDrop;
static rtech::liveapi::InventoryPickUp s_inventoryPickUp;
static rtech::liveapi::InventoryUse s_inventoryUse;
static rtech::liveapi::LegendUpgradeSelected s_legendUpgradeSelected;
static rtech::liveapi::LiveAPIEvent s_liveAPIEvent;
static rtech::liveapi::MatchSetup s_matchSetup;
static rtech::liveapi::MatchStateEnd s_matchStateEnd;
static rtech::liveapi::ObserverAnnotation s_observerAnnotation;
static rtech::liveapi::ObserverSwitched s_observerSwitched;
//static rtech::liveapi::PauseToggle s_pauseToggle;
static rtech::liveapi::PlayerAbilityUsed s_playerAbilityUsed;
static rtech::liveapi::PlayerAssist s_playerAssist;
static rtech::liveapi::PlayerConnected s_playerConnected;
static rtech::liveapi::PlayerDamaged s_playerDamaged;
static rtech::liveapi::PlayerDisconnected s_playerDisconnected;
static rtech::liveapi::PlayerDowned s_playerDowned;
static rtech::liveapi::PlayerKilled s_playerKilled;
static rtech::liveapi::PlayerRespawnTeam s_playerRespawnTeam;
static rtech::liveapi::PlayerRevive s_playerRevive;
static rtech::liveapi::PlayerStatChanged s_playerStatChanged;
static rtech::liveapi::PlayerUpgradeTierChanged s_playerUpgradeTierChanged;
//static rtech::liveapi::Request s_request;
//static rtech::liveapi::RequestStatus s_requestStatus;
//static rtech::liveapi::Response s_response;
static rtech::liveapi::RevenantForgedShadowDamaged s_revenantForgedShadowDamaged;
static rtech::liveapi::RingFinishedClosing s_ringFinishedClosing;
static rtech::liveapi::RingStartClosing s_ringStartClosing;
static rtech::liveapi::SquadEliminated s_squadEliminated;
static rtech::liveapi::WarpGateUsed s_warpGateUsed;
static rtech::liveapi::WeaponSwitched s_weaponSwitched;
static rtech::liveapi::WraithPortal s_wraithPortal;
static rtech::liveapi::ZiplineUsed s_ziplineUsed;


/*
	███████╗███╗   ██╗██╗   ██╗███╗   ███╗███████╗██████╗  █████╗ ████████╗██╗ ██████╗ ███╗   ██╗███████╗
	██╔════╝████╗  ██║██║   ██║████╗ ████║██╔════╝██╔══██╗██╔══██╗╚══██╔══╝██║██╔═══██╗████╗  ██║██╔════╝
	█████╗  ██╔██╗ ██║██║   ██║██╔████╔██║█████╗  ██████╔╝███████║   ██║   ██║██║   ██║██╔██╗ ██║███████╗
	██╔══╝  ██║╚██╗██║██║   ██║██║╚██╔╝██║██╔══╝  ██╔══██╗██╔══██║   ██║   ██║██║   ██║██║╚██╗██║╚════██║
	███████╗██║ ╚████║╚██████╔╝██║ ╚═╝ ██║███████╗██║  ██║██║  ██║   ██║   ██║╚██████╔╝██║ ╚████║███████║
	╚══════╝╚═╝  ╚═══╝ ╚═════╝ ╚═╝     ╚═╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝
	NOTE: the values here must align with the enumeration exposed to scripts; see section "abstractions"!
*/

enum class eLiveAPI_EventTypes
{
	ammoUsed,
	arenasItemDeselected,
	arenasItemSelected,

	bannerCollected,
	blackMarketAction,
		//changeCamera,
	characterSelected,
		//checkedState,
		//clientState,

		//customMatch_CreateLobby,
		//customMatch_GetLobbyPlayers,
		//customMatch_GetSettings,
		//customMatch_JoinLobby,
		//customMatch_KickPlayer,
		//customMatch_LeaveLobby,
		//customMatch_LobbyPlayer,
		//customMatch_LobbyPlayers,
		//customMatch_SendChat,
		//customMatch_SetMatchmaking,
		//customMatch_SetReady,
		//customMatch_SetSettings,
		//customMatch_SetTeam,
		//customMatch_SetTeamName,
	customEvent,
	datacenter,
		//gameConVar,
	gameStateChanged,
	gibraltarShieldAbsorbed,
		//globalVars,
	grenadeThrown,
	init,

	inventoryDrop,
	inventoryItem,
	inventoryPickUp,
	inventoryUse,

	legendUpgradeSelected,
	liveAPIEvent,
	loadoutConfiguration,
	matchSetup,
	matchStateEnd,
	observerAnnotation,
	observerSwitched,
		//pauseToggle,

	player,
	playerAbilityUsed,
	playerAssist,
	playerConnected,
	playerDamaged,
	playerDisconnected,
	playerDowned,
	playerKilled,
	playerRespawnTeam,
	playerRevive,
	playerStatChanged,
	playerUpgradeTierChanged,

		//request,
		//requestStatus,
		//response,

	revenantForgedShadowDamaged,

	ringFinishedClosing,
	ringStartClosing,

		//runCommand,
		//scriptCall,
	squadEliminated,
		//stateCheck,
		//svcMsgOverflow,
		//svcMsgRemoteScript,
	vector3,
	version,
	warpGateUsed,
	weaponSwitched,
	wraithPortal,
	ziplineUsed,
};


/*
	██╗   ██╗████████╗██╗██╗     ██╗████████╗██╗   ██╗     ██╗     ███╗   ███╗ █████╗  ██████╗██████╗  ██████╗ ███████╗
	██║   ██║╚══██╔══╝██║██║     ██║╚══██╔══╝╚██╗ ██╔╝     ██║     ████╗ ████║██╔══██╗██╔════╝██╔══██╗██╔═══██╗██╔════╝
	██║   ██║   ██║   ██║██║     ██║   ██║    ╚████╔╝   ████████╗  ██╔████╔██║███████║██║     ██████╔╝██║   ██║███████╗
	██║   ██║   ██║   ██║██║     ██║   ██║     ╚██╔╝    ██╔═██╔═╝  ██║╚██╔╝██║██╔══██║██║     ██╔══██╗██║   ██║╚════██║
	╚██████╔╝   ██║   ██║███████╗██║   ██║      ██║     ██████║    ██║ ╚═╝ ██║██║  ██║╚██████╗██║  ██║╚██████╔╝███████║
	 ╚═════╝    ╚═╝   ╚═╝╚══════╝╚═╝   ╚═╝      ╚═╝     ╚═════╝    ╚═╝     ╚═╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝
*/

static const char* LiveAPI_EventTypeToString(const eLiveAPI_EventTypes eventType)
{
	switch (eventType)
	{
	case eLiveAPI_EventTypes::ammoUsed: return "ammoUsed";
	case eLiveAPI_EventTypes::arenasItemDeselected: return "arenasItemDeselected";
	case eLiveAPI_EventTypes::arenasItemSelected: return "arenasItemSelected";
	case eLiveAPI_EventTypes::bannerCollected: return "bannerCollected";
	case eLiveAPI_EventTypes::blackMarketAction: return "blackMarketAction";
		//case eLiveAPI_EventTypes::changeCamera: return "changeCamera";
	case eLiveAPI_EventTypes::characterSelected: return "characterSelected";
		//case eLiveAPI_EventTypes::checkedState: return "checkedState";
		//case eLiveAPI_EventTypes::clientState: return "clientState";
		//case eLiveAPI_EventTypes::customMatch_CreateLobby: return "customMatch_CreateLobby";
		//case eLiveAPI_EventTypes::customMatch_GetLobbyPlayers: return "customMatch_GetLobbyPlayers";
		//case eLiveAPI_EventTypes::customMatch_GetSettings: return "customMatch_GetSettings";
		//case eLiveAPI_EventTypes::customMatch_JoinLobby: return "customMatch_JoinLobby";
		//case eLiveAPI_EventTypes::customMatch_KickPlayer: return "customMatch_KickPlayer";
		//case eLiveAPI_EventTypes::customMatch_LeaveLobby: return "customMatch_LeaveLobby";
		//case eLiveAPI_EventTypes::customMatch_LobbyPlayer: return "customMatch_LobbyPlayer";
		//case eLiveAPI_EventTypes::customMatch_LobbyPlayers: return "customMatch_LobbyPlayers";
		//case eLiveAPI_EventTypes::customMatch_SendChat: return "customMatch_SendChat";
		//case eLiveAPI_EventTypes::customMatch_SetMatchmaking: return "customMatch_SetMatchmaking";
		//case eLiveAPI_EventTypes::customMatch_SetReady: return "customMatch_SetReady";
		//case eLiveAPI_EventTypes::customMatch_SetSettings: return "customMatch_SetSettings";
		//case eLiveAPI_EventTypes::customMatch_SetTeam: return "customMatch_SetTeam";
		//case eLiveAPI_EventTypes::customMatch_SetTeamName: return "customMatch_SetTeamName";
	case eLiveAPI_EventTypes::customEvent: return "customEvent";
	case eLiveAPI_EventTypes::datacenter: return "datacenter";
		//case eLiveAPI_EventTypes::gameConVar: return "gameConVar";
	case eLiveAPI_EventTypes::gameStateChanged: return "gameStateChanged";
	case eLiveAPI_EventTypes::gibraltarShieldAbsorbed: return "gibraltarShieldAbsorbed";
		//case eLiveAPI_EventTypes::globalVars: return "globalVars";
	case eLiveAPI_EventTypes::grenadeThrown: return "grenadeThrown";
	case eLiveAPI_EventTypes::init: return "init";
	case eLiveAPI_EventTypes::inventoryDrop: return "inventoryDrop";
	case eLiveAPI_EventTypes::inventoryItem: return "inventoryItem";
	case eLiveAPI_EventTypes::inventoryPickUp: return "inventoryPickUp";
	case eLiveAPI_EventTypes::inventoryUse: return "inventoryUse";
	case eLiveAPI_EventTypes::legendUpgradeSelected: return "legendUpgradeSelected";
	case eLiveAPI_EventTypes::liveAPIEvent: return "liveAPIEvent";
	case eLiveAPI_EventTypes::loadoutConfiguration: return "loadoutConfiguration";
	case eLiveAPI_EventTypes::matchSetup: return "matchSetup";
	case eLiveAPI_EventTypes::matchStateEnd: return "matchStateEnd";
	case eLiveAPI_EventTypes::observerAnnotation: return "observerAnnotation";
	case eLiveAPI_EventTypes::observerSwitched: return "observerSwitched";
		//case eLiveAPI_EventTypes::pauseToggle: return "pauseToggle";
	case eLiveAPI_EventTypes::player: return "player";
	case eLiveAPI_EventTypes::playerAbilityUsed: return "playerAbilityUsed";
	case eLiveAPI_EventTypes::playerAssist: return "playerAssist";
	case eLiveAPI_EventTypes::playerConnected: return "playerConnected";
	case eLiveAPI_EventTypes::playerDamaged: return "playerDamaged";
	case eLiveAPI_EventTypes::playerDisconnected: return "playerDisconnected";
	case eLiveAPI_EventTypes::playerDowned: return "playerDowned";
	case eLiveAPI_EventTypes::playerKilled: return "playerKilled";
	case eLiveAPI_EventTypes::playerRespawnTeam: return "playerRespawnTeam";
	case eLiveAPI_EventTypes::playerRevive: return "playerRevive";
	case eLiveAPI_EventTypes::playerStatChanged: return "playerStatChanged";
	case eLiveAPI_EventTypes::playerUpgradeTierChanged: return "playerUpgradeTierChanged";
		//case eLiveAPI_EventTypes::request: return "request";
		//case eLiveAPI_EventTypes::requestStatus: return "requestStatus";
		//case eLiveAPI_EventTypes::response: return "response";
	case eLiveAPI_EventTypes::revenantForgedShadowDamaged: return "revenantForgedShadowDamaged";
	case eLiveAPI_EventTypes::ringFinishedClosing: return "ringFinishedClosing";
	case eLiveAPI_EventTypes::ringStartClosing: return "ringStartClosing";
		//case eLiveAPI_EventTypes::runCommand: return "runCommand";
		//case eLiveAPI_EventTypes::scriptCall: return "scriptCall";
	case eLiveAPI_EventTypes::squadEliminated: return "squadEliminated";
		//case eLiveAPI_EventTypes::stateCheck: return "stateCheck";
		//case eLiveAPI_EventTypes::svcMsgOverflow: return "svcMsgOverflow";
		//case eLiveAPI_EventTypes::svcMsgRemoteScript: return "svcMsgRemoteScript";
	case eLiveAPI_EventTypes::vector3: return "vector3";
	case eLiveAPI_EventTypes::version: return "version";
	case eLiveAPI_EventTypes::warpGateUsed: return "warpGateUsed";
	case eLiveAPI_EventTypes::weaponSwitched: return "weaponSwitched";
	case eLiveAPI_EventTypes::wraithPortal: return "wraithPortal";
	case eLiveAPI_EventTypes::ziplineUsed: return "ziplineUsed";

	default: Assert(0); return "(unknown)";
	};
}

static bool LiveAPI_EnsureType(HSQUIRRELVM const v, const SQObjectPtr& obj, const SQObjectType expectType,
	const google::protobuf::Message* const eventMsg, const SQInteger fieldNum)
{
	if (sq_type(obj) == expectType)
		return true;

	const char* const expectTypeName = IdType2Name(expectType);
	const char* const gotTypeName = IdType2Name(sq_type(obj));

	if (eventMsg)
	{
		const google::protobuf::Descriptor* const descriptor = eventMsg->GetDescriptor();

		if (fieldNum == -1)
		{
			v_SQVM_RaiseError(v, "Expected type \"%s\", got type \"%s\" for message \"%s\".", expectTypeName, gotTypeName,
				descriptor->name().c_str());
		}
		else
		{
			const google::protobuf::FieldDescriptor* const fieldDescriptor = descriptor->field(fieldNum);

			v_SQVM_RaiseError(v, "Expected type \"%s\", got type \"%s\" for field \"%s.%s\".", expectTypeName, gotTypeName,
				descriptor->name().c_str(), fieldDescriptor->name().c_str());
		}
	}
	else
	{
		v_SQVM_RaiseError(v, "Expected type \"%s\", got type \"%s\".", expectTypeName, gotTypeName);
	}

	return false;
}

static bool LiveAPI_CheckSwitchType(HSQUIRRELVM const v, const SQObjectPtr& obj)
{
	if (sq_type(obj) == OT_INTEGER)
		return true;

	v_SQVM_RaiseError(v, "Cannot switch on type \"%s\".", IdType2Name(sq_type(obj)));
	return false;
}

#define LIVEAPI_ENSURE_TYPE(v, obj, expectType, eventMsg, fieldNum) { if (!LiveAPI_EnsureType(v, obj, expectType, eventMsg, fieldNum)) return false; }
#define LIVEAPI_EMPTY_TABLE_ERROR(v, eventMsg) { v_SQVM_RaiseError(v, "Empty iterable on message \"%s\".", eventMsg->GetTypeName().c_str()); return false; }
#define LIVEAPI_FIELD_ERROR(v, fieldNum, eventMsg) { v_SQVM_RaiseError(v, "Field #%d doesn't exist in message \"%s\".", fieldNum, eventMsg->GetTypeName().c_str()); return false; }
#define LIVEAPI_ONEOF_FIELD_ERROR(v, fieldNum, eventMsg) { v_SQVM_RaiseError(v, "Tried to set member #%d of oneof field in message \"%s\" while another has already been set.", fieldNum, eventMsg->GetTypeName().c_str()); return false; }
#define LIVEAPI_UNSUPPORTED_TYPE_ERROR(v, gotType, eventMsg) {v_SQVM_RaiseError(v, "Value type \"%s\" is not supported for message \"%s\".", IdType2Name(gotType), eventMsg->GetTypeName().c_str()); return false; }
#define LIVEAPI_CHECK_RECURSION_DEPTH(v, currDepth) { if (currDepth > LIVEAPI_MAX_ITEM_DEPTH) { v_SQVM_RaiseError(v, "Exceeded nesting depth limit of %i.", LIVEAPI_MAX_ITEM_DEPTH); return false; }}


/*
	██╗███╗   ██╗████████╗███████╗██████╗ ███╗   ███╗███████╗██████╗ ██╗ █████╗ ██████╗ ██╗   ██╗
	██║████╗  ██║╚══██╔══╝██╔════╝██╔══██╗████╗ ████║██╔════╝██╔══██╗██║██╔══██╗██╔══██╗╚██╗ ██╔╝
	██║██╔██╗ ██║   ██║   █████╗  ██████╔╝██╔████╔██║█████╗  ██║  ██║██║███████║██████╔╝ ╚████╔╝ 
	██║██║╚██╗██║   ██║   ██╔══╝  ██╔══██╗██║╚██╔╝██║██╔══╝  ██║  ██║██║██╔══██║██╔══██╗  ╚██╔╝  
	██║██║ ╚████║   ██║   ███████╗██║  ██║██║ ╚═╝ ██║███████╗██████╔╝██║██║  ██║██║  ██║   ██║   
	╚═╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝╚═════╝ ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝   
	Not used directly, but as part of other messages
*/

template<typename T>
static void LiveAPI_SetCommonMessageFields(T const msg, const eLiveAPI_EventTypes eventType)
{
	if (msg->timestamp() == 0)
		msg->set_timestamp(GetUnixTimeStamp());
	if (msg->category().empty())
		msg->set_category(LiveAPI_EventTypeToString(eventType));
}

static void LiveAPI_SetVector3D(rtech::liveapi::Vector3* const msgVec, const Vector3D* const dataVec)
{
	msgVec->set_x(dataVec->x);
	msgVec->set_y(dataVec->y);
	msgVec->set_z(dataVec->z);
}

static void LiveAPI_SetDataCenter(rtech::liveapi::Datacenter* const msg, const char* const name)
{
	LiveAPI_SetCommonMessageFields(msg, eLiveAPI_EventTypes::datacenter);
	msg->set_name(name);
}

static void LiveAPI_SetVersion(rtech::liveapi::Version* const msg)
{
	msg->set_major_num(LIVEAPI_MAJOR_VERSION);
	msg->set_minor_num(LIVEAPI_MINOR_VERSION);
	msg->set_build_stamp(g_SDKDll.GetNTHeaders()->FileHeader.TimeDateStamp);
	msg->set_revision(LIVEAPI_REVISION);
}

static void LiveAPI_SetNucleusHash(std::string* const msg, const SQString* const nucleusId)
{
	static const char hexChars[] = "0123456789abcdef";
	uint8_t nucleusIdHash[LIVEAPI_SHA512_HASH_SIZE];

	mbedtls_sha512(reinterpret_cast<const uint8_t*>(nucleusId->_val), nucleusId->_len, nucleusIdHash, NULL);

	const size_t hashSize = liveapi_truncate_hash_fields.GetBool()
		? LIVEAPI_SHA512_HASH_SIZE / 4
		: LIVEAPI_SHA512_HASH_SIZE;

	msg->reserve((hashSize * 2) + 1);
	msg->resize((hashSize * 2));

	for (size_t i = 0; i < hashSize; i++)
	{
		(*msg)[i * 2] = hexChars[(nucleusIdHash[i] >> 4) & 0xf];
		(*msg)[i * 2 + 1] = hexChars[nucleusIdHash[i] & 0xf];
	}
}

static bool LiveAPI_SetPlayerIdentityFields(HSQUIRRELVM const v, const SQTable* const table, rtech::liveapi::Player* const playerMsg)
{
	bool ranLoop = false;

	SQ_FOR_EACH_TABLE(table, i)
	{
		const SQTable::_HashNode& node = table->_nodes[i];

		if (sq_isnull(node.key))
			continue;

		if (!ranLoop)
			ranLoop = true;

		if (!LiveAPI_CheckSwitchType(v, node.key))
			return false;

		const SQInteger fieldNum = _integer(node.key);
		const SQObjectPtr& obj = node.val;

		switch (fieldNum)
		{
		case rtech::liveapi::Player::kNameFieldNumber:
		{
			LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, playerMsg, fieldNum);
			playerMsg->set_name(_stringval(obj));

			break;
		}
		case rtech::liveapi::Player::kTeamIdFieldNumber:
		{
			LIVEAPI_ENSURE_TYPE(v, obj, OT_INTEGER, playerMsg, fieldNum);
			playerMsg->set_teamid(_integer(obj));

			break;
		}
		case rtech::liveapi::Player::kPosFieldNumber:
		{
			LIVEAPI_ENSURE_TYPE(v, obj, OT_VECTOR, playerMsg, fieldNum);
			LiveAPI_SetVector3D(playerMsg->mutable_pos(), _vector3d(obj));

			break;
		}
		case rtech::liveapi::Player::kAnglesFieldNumber:
		{
			LIVEAPI_ENSURE_TYPE(v, obj, OT_VECTOR, playerMsg, fieldNum);
			LiveAPI_SetVector3D(playerMsg->mutable_angles(), _vector3d(obj));

			break;
		}
		case rtech::liveapi::Player::kCurrentHealthFieldNumber:
		{
			LIVEAPI_ENSURE_TYPE(v, obj, OT_INTEGER, playerMsg, fieldNum);
			playerMsg->set_currenthealth(_integer(obj));

			break;
		}
		case rtech::liveapi::Player::kMaxHealthFieldNumber:
		{
			LIVEAPI_ENSURE_TYPE(v, obj, OT_INTEGER, playerMsg, fieldNum);
			playerMsg->set_maxhealth(_integer(obj));

			break;
		}
		case rtech::liveapi::Player::kShieldHealthFieldNumber:
		{
			LIVEAPI_ENSURE_TYPE(v, obj, OT_INTEGER, playerMsg, fieldNum);
			playerMsg->set_shieldhealth(_integer(obj));

			break;
		}
		case rtech::liveapi::Player::kShieldMaxHealthFieldNumber:
		{
			LIVEAPI_ENSURE_TYPE(v, obj, OT_INTEGER, playerMsg, fieldNum);
			playerMsg->set_shieldmaxhealth(_integer(obj));

			break;
		}
		case rtech::liveapi::Player::kNucleusHashFieldNumber:
		{
			LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, playerMsg, fieldNum);
			LiveAPI_SetNucleusHash(playerMsg->mutable_nucleushash(), _string(obj));

			break;
		}
		case rtech::liveapi::Player::kHardwareNameFieldNumber:
		{
			LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, playerMsg, fieldNum);
			playerMsg->set_hardwarename(_stringval(obj));

			break;
		}
		case rtech::liveapi::Player::kTeamNameFieldNumber:
		{
			LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, playerMsg, fieldNum);
			playerMsg->set_teamname(_stringval(obj));

			break;
		}
		case rtech::liveapi::Player::kSquadIndexFieldNumber:
		{
			LIVEAPI_ENSURE_TYPE(v, obj, OT_INTEGER, playerMsg, fieldNum);
			playerMsg->set_squadindex(_integer(obj));

			break;
		}
		case rtech::liveapi::Player::kCharacterFieldNumber:
		{
			LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, playerMsg, fieldNum);
			playerMsg->set_character(_stringval(obj));

			break;
		}
		case rtech::liveapi::Player::kSkinFieldNumber:
		{
			LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, playerMsg, fieldNum);
			playerMsg->set_skin(_stringval(obj));

			break;
		}
		default:
			LIVEAPI_FIELD_ERROR(v, fieldNum, playerMsg);
		}
	}

	if (!ranLoop)
		LIVEAPI_EMPTY_TABLE_ERROR(v, playerMsg);

	return true;
}

static bool LiveAPI_SetInventoryItem(HSQUIRRELVM const v, const SQTable* const table, rtech::liveapi::InventoryItem* const event)
{
	bool ranLoop = false;

	SQ_FOR_EACH_TABLE(table, i)
	{
		const SQTable::_HashNode& node = table->_nodes[i];

		if (sq_isnull(node.key))
			continue;

		if (!ranLoop)
			ranLoop = true;

		if (!LiveAPI_CheckSwitchType(v, node.key))
			return false;

		const SQInteger fieldNum = _integer(node.key);
		const SQObjectPtr& obj = node.val;

		switch (fieldNum)
		{
		case rtech::liveapi::InventoryItem::kQuantityFieldNumber:
		{
			LIVEAPI_ENSURE_TYPE(v, obj, OT_INTEGER, event, fieldNum);
			event->set_quantity(_integer(obj));

			break;
		}
		case rtech::liveapi::InventoryItem::kItemFieldNumber:
		{
			LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, event, fieldNum);
			event->set_item(_stringval(obj));

			break;
		}
		case rtech::liveapi::InventoryItem::kExtraDataFieldNumber:
		{
			LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, event, fieldNum);
			event->set_extradata(_stringval(obj));

			break;
		}
		default:
			LIVEAPI_FIELD_ERROR(v, fieldNum, event);
		}
	}

	if (!ranLoop)
		LIVEAPI_EMPTY_TABLE_ERROR(v, event);

	return true;
}

static bool LiveAPI_SetLoadoutConfiguration(HSQUIRRELVM const v, rtech::liveapi::LoadoutConfiguration* const msg, const SQTable* const table)
{
	bool ranOuterLoop = false;
	bool ranInnerLoop = false;

	SQ_FOR_EACH_TABLE(table, i)
	{
		const SQTable::_HashNode& node = table->_nodes[i];

		if (sq_isnull(node.key))
			continue;

		if (!ranOuterLoop)
			ranOuterLoop = true;

		if (!LiveAPI_CheckSwitchType(v, node.key))
			return false;

		const SQInteger fieldNum = _integer(node.key);
		const SQObjectPtr& obj = node.val;

		rtech::liveapi::InventoryItem* (rtech::liveapi::LoadoutConfiguration:: * addFunctor)();

		switch (fieldNum)
		{
		case rtech::liveapi::LoadoutConfiguration::kWeaponsFieldNumber:
		{
			addFunctor = &rtech::liveapi::LoadoutConfiguration::add_weapons;
			break;
		}
		case rtech::liveapi::LoadoutConfiguration::kEquipmentFieldNumber:
		{
			addFunctor = &rtech::liveapi::LoadoutConfiguration::add_equipment;
			break;
		}
		default:
			LIVEAPI_FIELD_ERROR(v, fieldNum, msg);
		}

		LIVEAPI_ENSURE_TYPE(v, obj, OT_ARRAY, msg, fieldNum);
		const SQArray* const fieldArray = _array(obj);

		for (SQInteger j = 0; j < fieldArray->Size(); j++)
		{
			const SQObject& fieldObj = fieldArray->_values[j];

			if (sq_isnull(fieldObj))
				continue;

			if (!ranInnerLoop)
				ranInnerLoop = true;

			LIVEAPI_ENSURE_TYPE(v, fieldObj, OT_TABLE, msg, fieldNum);

			if (!LiveAPI_SetInventoryItem(v, _table(fieldObj), (msg->*addFunctor)()))
				return false;
		}
	}

	if (!ranOuterLoop || !ranInnerLoop)
		LIVEAPI_EMPTY_TABLE_ERROR(v, msg);

	return true;
}


/*
	███████╗██╗   ██╗███████╗███╗   ██╗████████╗  ██╗  ██╗ █████╗ ███╗   ██╗██████╗ ██╗     ███████╗██████╗ ███████╗
	██╔════╝██║   ██║██╔════╝████╗  ██║╚══██╔══╝  ██║  ██║██╔══██╗████╗  ██║██╔══██╗██║     ██╔════╝██╔══██╗██╔════╝
	█████╗  ██║   ██║█████╗  ██╔██╗ ██║   ██║     ███████║███████║██╔██╗ ██║██║  ██║██║     █████╗  ██████╔╝███████╗
	██╔══╝  ╚██╗ ██╔╝██╔══╝  ██║╚██╗██║   ██║     ██╔══██║██╔══██║██║╚██╗██║██║  ██║██║     ██╔══╝  ██╔══██╗╚════██║
	███████╗ ╚████╔╝ ███████╗██║ ╚████║   ██║     ██║  ██║██║  ██║██║ ╚████║██████╔╝███████╗███████╗██║  ██║███████║
	╚══════╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝   ╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═════╝ ╚══════╝╚══════╝╚═╝  ╚═╝╚══════╝
	NOTE: one handler may be used for multiple events to cut down on boilerplate code, this is possible if a message
	has the same structure layout as another message; the actual field numbers don't matter, but the field names do!
*/

static bool LiveAPI_HandleInitEvent(rtech::liveapi::Init* const event, const eLiveAPI_EventTypes eventType)
{
	LiveAPI_SetCommonMessageFields(event, eventType);
	event->set_gameversion(Sys_GetBuildString());
	LiveAPI_SetVersion(event->mutable_apiversion());
	event->set_platform(Sys_GetPlatformString());
	event->set_name(liveapi_session_name.GetString());

	return true;
}

static bool LiveAPI_HandleMatchSetup(HSQUIRRELVM const v, const SQObject& obj, rtech::liveapi::MatchSetup* const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case rtech::liveapi::MatchSetup::kMapFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, event, fieldNum);
		event->set_map(_stringval(obj));

		break;
	}
	case rtech::liveapi::MatchSetup::kPlaylistNameFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, event, fieldNum);
		event->set_playlistname(_stringval(obj));

		break;
	}
	case rtech::liveapi::MatchSetup::kPlaylistDescFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, event, fieldNum);
		event->set_playlistdesc(_stringval(obj));

		break;
	}
	case rtech::liveapi::MatchSetup::kDatacenterFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, event, fieldNum);
		LiveAPI_SetDataCenter(event->mutable_datacenter(), _stringval(obj));

		break;
	}
	case rtech::liveapi::MatchSetup::kAimAssistOnFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_BOOL, event, fieldNum);
		event->set_aimassiston(_bool(obj));

		break;
	}
	case rtech::liveapi::MatchSetup::kAnonymousModeFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_BOOL, event, fieldNum);
		event->set_anonymousmode(_bool(obj));

		break;
	}
	case rtech::liveapi::MatchSetup::kServerIdFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, event, fieldNum);
		event->set_serverid(_stringval(obj));

		break;
	}
	case rtech::liveapi::MatchSetup::kStartingLoadoutFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);

		if (!LiveAPI_SetLoadoutConfiguration(v, event->mutable_startingloadout(), _table(obj)))
			return false;

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

static bool LiveAPI_HandleAmmoUsed(HSQUIRRELVM const v, const SQObject& obj, rtech::liveapi::AmmoUsed* const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case rtech::liveapi::AmmoUsed::kPlayerFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_player()))
			return false;

		break;
	}
	case rtech::liveapi::AmmoUsed::kAmmoTypeFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, event, fieldNum);
		event->set_ammotype(_stringval(obj));

		break;
	}
	case rtech::liveapi::AmmoUsed::kAmountUsedFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_INTEGER, event, fieldNum);
		event->set_amountused(_integer(obj));

		break;
	}
	case rtech::liveapi::AmmoUsed::kOldAmmoCountFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_INTEGER, event, fieldNum);
		event->set_oldammocount(_integer(obj));

		break;
	}
	case rtech::liveapi::AmmoUsed::kNewAmmoCountFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_INTEGER, event, fieldNum);
		event->set_newammocount(_integer(obj));

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

template<typename T>
static bool LiveAPI_HandleInventoryChange(HSQUIRRELVM const v, const SQObject& obj, T const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case event->kPlayerFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_player()))
			return false;

		break;
	}
	case event->kItemFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, event, fieldNum);
		event->set_item(_stringval(obj));

		break;
	}
	case event->kQuantityFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_INTEGER, event, fieldNum);
		event->set_quantity(_integer(obj));

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

static bool LiveAPI_HandleInventoryDrop(HSQUIRRELVM const v, const SQObject& obj, rtech::liveapi::InventoryDrop* const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case rtech::liveapi::InventoryDrop::kPlayerFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_player()))
			return false;

		break;
	}
	case rtech::liveapi::InventoryDrop::kItemFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, event, fieldNum);
		event->set_item(_stringval(obj));

		break;
	}
	case rtech::liveapi::InventoryDrop::kQuantityFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_INTEGER, event, fieldNum);
		event->set_quantity(_integer(obj));

		break;
	}
	case rtech::liveapi::InventoryDrop::kExtraDataFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_ARRAY, event, fieldNum);
		const SQArray* const fieldArray = _array(obj);

		for (SQInteger j = 0; j < fieldArray->Size(); j++)
		{
			const SQObject& fieldObj = fieldArray->_values[j];

			if (sq_isnull(fieldObj))
				continue;

			LIVEAPI_ENSURE_TYPE(v, fieldObj, OT_STRING, event, fieldNum);
			event->add_extradata(_stringval(fieldObj));
		}

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

static bool LiveAPI_HandleGameStateChanged(HSQUIRRELVM const v, const SQObject& obj, rtech::liveapi::GameStateChanged* const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case rtech::liveapi::GameStateChanged::kStateFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, event, fieldNum);
		event->set_state(_stringval(obj));

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

static bool LiveAPI_HandleMatchStateEnd(HSQUIRRELVM const v, const SQObject& obj, rtech::liveapi::MatchStateEnd* const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case rtech::liveapi::MatchStateEnd::kStateFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, event, fieldNum);
		event->set_state(_stringval(obj));

		break;
	}
	case rtech::liveapi::MatchStateEnd::kWinnersFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_ARRAY, event, fieldNum);
		const SQArray* const fieldArray = _array(obj);

		for (SQInteger j = 0; j < fieldArray->Size(); j++)
		{
			const SQObject& fieldObj = fieldArray->_values[j];

			if (sq_isnull(fieldObj))
				continue;

			LIVEAPI_ENSURE_TYPE(v, fieldObj, OT_TABLE, event, fieldNum);

			if (!LiveAPI_SetPlayerIdentityFields(v, _table(fieldObj), event->add_winners()))
				return false; // failure
		}

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

static bool LiveAPI_HandleObserverSwitched(HSQUIRRELVM const v, const SQObject& obj, rtech::liveapi::ObserverSwitched* const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case rtech::liveapi::ObserverSwitched::kObserverFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);

		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_observer()))
			return false;

		break;
	}
	case rtech::liveapi::ObserverSwitched::kTargetFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);

		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_target()))
			return false;

		break;
	}
	case rtech::liveapi::ObserverSwitched::kTargetTeamFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_ARRAY, event, fieldNum);
		const SQArray* const fieldArray = _array(obj);

		for (SQInteger j = 0; j < fieldArray->Size(); j++)
		{
			const SQObject& fieldObj = fieldArray->_values[j];

			if (sq_isnull(fieldObj))
				continue;

			LIVEAPI_ENSURE_TYPE(v, fieldObj, OT_TABLE, event, fieldNum);

			if (!LiveAPI_SetPlayerIdentityFields(v, _table(fieldObj), event->add_targetteam()))
				return false; // failure
		}

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

static bool LiveAPI_HandleObserverAnnotation(HSQUIRRELVM const v, const SQObject& obj, rtech::liveapi::ObserverAnnotation* const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case rtech::liveapi::ObserverAnnotation::kAnnotationSerialFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_INTEGER, event, fieldNum);
		event->set_annotationserial(_integer(obj));

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

static bool LiveAPI_HandleBannerCollected(HSQUIRRELVM const v, const SQObject& obj, rtech::liveapi::BannerCollected* const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case rtech::liveapi::BannerCollected::kPlayerFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_player()))
			return false;

		break;
	}
	case rtech::liveapi::BannerCollected::kCollectedFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_collected()))
			return false;

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

static bool LiveAPI_HandlePlayerRevive(HSQUIRRELVM const v, const SQObject& obj, rtech::liveapi::PlayerRevive* const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case rtech::liveapi::PlayerRevive::kPlayerFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_player()))
			return false;

		break;
	}
	case rtech::liveapi::PlayerRevive::kRevivedFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_revived()))
			return false;

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

static bool LiveAPI_HandlePlayerDisconnected(HSQUIRRELVM const v, const SQObject& obj, rtech::liveapi::PlayerDisconnected* const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case rtech::liveapi::PlayerDisconnected::kPlayerFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_player()))
			return false;

		break;
	}
	case rtech::liveapi::PlayerDisconnected::kCanReconnectFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_BOOL, event, fieldNum);
		event->set_canreconnect(_bool(obj));

		break;
	}
	case rtech::liveapi::PlayerDisconnected::kIsAliveFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_BOOL, event, fieldNum);
		event->set_isalive(_bool(obj));

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

template<typename T>
static bool LiveAPI_HandlePlayerAttackCommon(HSQUIRRELVM const v, const SQObject& obj, T const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case event->kAttackerFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_attacker()))
			return false;

		break;
	}
	case event->kVictimFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_victim()))
			return false;

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

static bool LiveAPI_HandlePlayerDowned(HSQUIRRELVM const v, const SQObject& obj, rtech::liveapi::PlayerDowned* const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case rtech::liveapi::PlayerDowned::kAttackerFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_attacker()))
			return false;

		break;
	}
	case rtech::liveapi::PlayerDowned::kVictimFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_victim()))
			return false;

		break;
	}
	case rtech::liveapi::PlayerDowned::kWeaponFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, event, fieldNum);
		event->set_weapon(_stringval(obj));

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

static bool LiveAPI_HandlePlayerKilled(HSQUIRRELVM const v, const SQObject& obj, rtech::liveapi::PlayerKilled* const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case rtech::liveapi::PlayerKilled::kAttackerFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_attacker()))
			return false;

		break;
	}
	case rtech::liveapi::PlayerKilled::kVictimFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_victim()))
			return false;

		break;
	}
	case rtech::liveapi::PlayerKilled::kAwardedToFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_awardedto()))
			return false;

		break;
	}
	case rtech::liveapi::PlayerKilled::kWeaponFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, event, fieldNum);
		event->set_weapon(_stringval(obj));

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

template<typename T>
static bool LiveAPI_HandleAbilityDamaged(HSQUIRRELVM const v, const SQObject& obj, T const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case event->kAttackerFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_attacker()))
			return false;

		break;
	}
	case event->kVictimFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_victim()))
			return false;

		break;
	}
	case event->kDamageInflictedFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_INTEGER, event, fieldNum);
		event->set_damageinflicted(_integer(obj));

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

static bool LiveAPI_HandlePlayerDamaged(HSQUIRRELVM const v, const SQObject& obj, rtech::liveapi::PlayerDamaged* const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case rtech::liveapi::PlayerDamaged::kAttackerFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_attacker()))
			return false;

		break;
	}
	case rtech::liveapi::PlayerDamaged::kVictimFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_victim()))
			return false;

		break;
	}
	case rtech::liveapi::PlayerDamaged::kWeaponFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, event, fieldNum);
		event->set_weapon(_stringval(obj));

		break;
	}
	case rtech::liveapi::PlayerDamaged::kDamageInflictedFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_INTEGER, event, fieldNum);
		event->set_damageinflicted(_integer(obj));

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

static bool LiveAPI_HandlePlayerAssist(HSQUIRRELVM const v, const SQObject& obj, rtech::liveapi::PlayerAssist* const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case rtech::liveapi::PlayerAssist::kAssistantFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_assistant()))
			return false;

		break;
	}
	case rtech::liveapi::PlayerAssist::kVictimFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_victim()))
			return false;

		break;
	}
	case rtech::liveapi::PlayerAssist::kWeaponFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, event, fieldNum);
		event->set_weapon(_stringval(obj));

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

// CharacterSelected
// PlayerConnected
// WraithPortal
// WarpGateUsed
template<typename T>
static bool LiveAPI_HandleSimplePlayerMessage(HSQUIRRELVM const v, const SQObject& obj, T const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case event->kPlayerFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_player()))
			return false;

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

static bool LiveAPI_HandleRingFinishedClosing(HSQUIRRELVM const v, const SQObject& obj, rtech::liveapi::RingFinishedClosing* const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case event->kStageFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_INTEGER, event, fieldNum);
		event->set_stage(_integer(obj));

		break;
	}
	case event->kCenterFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_VECTOR, event, fieldNum);
		LiveAPI_SetVector3D(event->mutable_center(), _vector3d(obj));

		break;
	}
	case event->kCurrentRadiusFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_FLOAT, event, fieldNum);
		event->set_currentradius(_float(obj));

		break;
	}
	case event->kShrinkDurationFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_FLOAT, event, fieldNum);
		event->set_shrinkduration(_float(obj));

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

static bool LiveAPI_HandleDeathFieldStartClosing(HSQUIRRELVM const v, const SQObject& obj, rtech::liveapi::RingStartClosing* const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case rtech::liveapi::RingStartClosing::kStageFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_INTEGER, event, fieldNum);
		event->set_stage(_integer(obj));

		break;
	}
	case rtech::liveapi::RingStartClosing::kCenterFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_VECTOR, event, fieldNum);
		LiveAPI_SetVector3D(event->mutable_center(), _vector3d(obj));

		break;
	}
	case rtech::liveapi::RingStartClosing::kCurrentRadiusFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_FLOAT, event, fieldNum);
		event->set_currentradius(_float(obj));

		break;
	}
	case rtech::liveapi::RingStartClosing::kEndRadiusFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_FLOAT, event, fieldNum);
		event->set_endradius(_float(obj));

		break;
	}
	case rtech::liveapi::RingStartClosing::kShrinkDurationFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_FLOAT, event, fieldNum);
		event->set_shrinkduration(_float(obj));

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

static bool LiveAPI_HandleSquadEliminated(HSQUIRRELVM const v, const SQObject& obj, rtech::liveapi::SquadEliminated* const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case rtech::liveapi::SquadEliminated::kPlayersFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_ARRAY, event, fieldNum);
		const SQArray* const fieldArray = _array(obj);

		for (SQInteger j = 0; j < fieldArray->Size(); j++)
		{
			const SQObject& fieldObj = fieldArray->_values[j];

			if (sq_isnull(fieldObj))
				continue;

			LIVEAPI_ENSURE_TYPE(v, fieldObj, OT_TABLE, event, fieldNum);
			rtech::liveapi::Player* const player = event->add_players();

			if (!LiveAPI_SetPlayerIdentityFields(v, _table(fieldObj), player))
				return false; // failure
		}

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

template<typename T>
static bool LiveAPI_HandleLinkedEntityEvent(HSQUIRRELVM const v, const SQObject& obj, T const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case event->kPlayerFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_player()))
			return false;

		break;
	}
	case event->kLinkedEntityFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, event, fieldNum);
		event->set_linkedentity(_stringval(obj));

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

static bool LiveAPI_HandleWeaponSwitchedEvent(HSQUIRRELVM const v, const SQObject& obj, rtech::liveapi::WeaponSwitched* const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case rtech::liveapi::WeaponSwitched::kPlayerFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_player()))
			return false;

		break;
	}
	case rtech::liveapi::WeaponSwitched::kOldWeaponFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, event, fieldNum);
		event->set_oldweapon(_stringval(obj));

		break;
	}
	case rtech::liveapi::WeaponSwitched::kNewWeaponFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, event, fieldNum);
		event->set_newweapon(_stringval(obj));

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

static bool LiveAPI_HandleBlackMarketActionEvent(HSQUIRRELVM const v, const SQObject& obj, rtech::liveapi::BlackMarketAction* const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case rtech::liveapi::BlackMarketAction::kPlayerFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_player()))
			return false;

		break;
	}
	case rtech::liveapi::BlackMarketAction::kItemFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, event, fieldNum);
		event->set_item(_stringval(obj));

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

static bool LiveAPI_HandlePlayerUpgradeTierChanged(HSQUIRRELVM const v, const SQObject& obj, rtech::liveapi::PlayerUpgradeTierChanged* const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case rtech::liveapi::PlayerUpgradeTierChanged::kPlayerFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_player()))
			return false;

		break;
	}
	case rtech::liveapi::PlayerUpgradeTierChanged::kLevelFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_INTEGER, event, fieldNum);
		event->set_level(_integer(obj));

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

static bool LiveAPI_HandleLegendUpgradeSelected(HSQUIRRELVM const v, const SQObject& obj, rtech::liveapi::LegendUpgradeSelected* const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case rtech::liveapi::LegendUpgradeSelected::kPlayerFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_player()))
			return false;

		break;
	}
	case rtech::liveapi::LegendUpgradeSelected::kUpgradeNameFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, event, fieldNum);
		event->set_upgradename(_stringval(obj));

		break;
	}
	case rtech::liveapi::LegendUpgradeSelected::kUpgradeDescFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, event, fieldNum);
		event->set_upgradedesc(_stringval(obj));

		break;
	}
	case rtech::liveapi::LegendUpgradeSelected::kLevelFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_INTEGER, event, fieldNum);
		event->set_level(_integer(obj));

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

static bool LiveAPI_HandlePlayerRespawnTeam(HSQUIRRELVM const v, const SQObject& obj, rtech::liveapi::PlayerRespawnTeam* const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case rtech::liveapi::PlayerRespawnTeam::kPlayerFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_player()))
			return false;

		break;
	}
	case rtech::liveapi::PlayerRespawnTeam::kRespawnedFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_ARRAY, event, fieldNum);
		const SQArray* const fieldArray = _array(obj);

		for (SQInteger j = 0; j < fieldArray->Size(); j++)
		{
			const SQObject& fieldObj = fieldArray->_values[j];

			if (sq_isnull(fieldObj))
				continue;

			LIVEAPI_ENSURE_TYPE(v, fieldObj, OT_TABLE, event, fieldNum);
			rtech::liveapi::Player* const player = event->add_respawned();

			if (!LiveAPI_SetPlayerIdentityFields(v, _table(fieldObj), player))
				return false; // failure
		}

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

static bool LiveAPI_HandlePlayerStatChanged(HSQUIRRELVM const v, const SQObject& obj, rtech::liveapi::PlayerStatChanged* const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case  rtech::liveapi::PlayerStatChanged::kPlayerFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetPlayerIdentityFields(v, _table(obj), event->mutable_player()))
			return false;

		break;
	}
	case  rtech::liveapi::PlayerStatChanged::kStatNameFieldNumber:
	{
		LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, event, fieldNum);
		event->set_statname(_stringval(obj));

		break;
	}
	case rtech::liveapi::PlayerStatChanged::kIntValueFieldNumber:
	{
		if (event->newValue_case() != rtech::liveapi::PlayerStatChanged::NEWVALUE_NOT_SET)
			LIVEAPI_ONEOF_FIELD_ERROR(v, fieldNum, event);

		LIVEAPI_ENSURE_TYPE(v, obj, OT_INTEGER, event, fieldNum);
		event->set_intvalue(_integer(obj));

		break;
	}
	case rtech::liveapi::PlayerStatChanged::kFloatValueFieldNumber:
	{
		if (event->newValue_case() != rtech::liveapi::PlayerStatChanged::NEWVALUE_NOT_SET)
			LIVEAPI_ONEOF_FIELD_ERROR(v, fieldNum, event);

		LIVEAPI_ENSURE_TYPE(v, obj, OT_FLOAT, event, fieldNum);
		event->set_floatvalue(_float(obj));

		break;
	}
	case rtech::liveapi::PlayerStatChanged::kBoolValueFieldNumber:
	{
		if (event->newValue_case() != rtech::liveapi::PlayerStatChanged::NEWVALUE_NOT_SET)
			LIVEAPI_ONEOF_FIELD_ERROR(v, fieldNum, event);

		LIVEAPI_ENSURE_TYPE(v, obj, OT_BOOL, event, fieldNum);
		event->set_boolvalue(_bool(obj));

		break;
	}
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

static void LiveAPI_SetCustomVectorField(google::protobuf::Struct* const structData, const Vector3D* const vecData)
{
	google::protobuf::Map<std::string, google::protobuf::Value>* const fieldData = structData->mutable_fields();

	(*fieldData)["x"].set_number_value(vecData->x);
	(*fieldData)["y"].set_number_value(vecData->y);
	(*fieldData)["z"].set_number_value(vecData->z);
}

static bool LiveAPI_SetCustomTableFields(HSQUIRRELVM const v, google::protobuf::Struct* const structData, const SQTable* const tableData);
static bool LiveAPI_SetCustomArrayFields(HSQUIRRELVM const v, google::protobuf::ListValue* const listData, const SQArray* const arrayData);

static int s_currentDepth = 0;

static bool LiveAPI_SetCustomArrayFields(HSQUIRRELVM const v, google::protobuf::ListValue* const listData, const SQArray* const arrayData)
{
	CDepthCounter<int> counter(s_currentDepth);
	bool ranLoop = false;

	for (SQInteger i = 0; i < arrayData->Size(); i++)
	{
		const SQObject& valueObj = arrayData->_values[i];

		if (sq_isnull(valueObj))
			continue;

		if (!ranLoop)
			ranLoop = true;

		const SQObjectType valueType = sq_type(valueObj);

		switch (valueType)
		{
		case OT_BOOL:
			listData->add_values()->set_bool_value(_bool(valueObj));
			break;
		case OT_INTEGER:
			listData->add_values()->set_number_value(_integer(valueObj));
			break;
		case OT_FLOAT:
			listData->add_values()->set_number_value(_float(valueObj));
			break;
		case OT_STRING:
			listData->add_values()->set_string_value(_stringval(valueObj));
			break;
		case OT_VECTOR:
			LiveAPI_SetCustomVectorField(listData->add_values()->mutable_struct_value(), _vector3d(valueObj));
			break;
		case OT_ARRAY:
			LIVEAPI_CHECK_RECURSION_DEPTH(v, counter.Get());

			if (arrayData == _array(valueObj))
			{
				v_SQVM_RaiseError(v, "Attempted to nest array at depth \"%i\" into itself at index \"%i\".", counter.Get(), i);
				return false;
			}

			if (!LiveAPI_SetCustomArrayFields(v, listData->add_values()->mutable_list_value(), _array(valueObj)))
				return false;

			break;
		case OT_TABLE:
			LIVEAPI_CHECK_RECURSION_DEPTH(v, counter.Get());

			if (!LiveAPI_SetCustomTableFields(v, listData->add_values()->mutable_struct_value(), _table(valueObj)))
				return false;

			break;
		default:
			LIVEAPI_UNSUPPORTED_TYPE_ERROR(v, valueType, listData);
		}
	}

	if (!ranLoop)
		LIVEAPI_EMPTY_TABLE_ERROR(v, listData);

	return true;
}

static bool LiveAPI_SetCustomTableFields(HSQUIRRELVM const v, google::protobuf::Struct* const structData, const SQTable* const tableData)
{
	CDepthCounter<int> counter(s_currentDepth);
	bool ranLoop = false;

	SQ_FOR_EACH_TABLE(tableData, i)
	{
		const SQTable::_HashNode& node = tableData->_nodes[i];

		if (sq_isnull(node.key))
			continue;

		if (!ranLoop)
			ranLoop = true;

		const SQObjectType keyType = sq_type(node.key);

		if (keyType != OT_STRING)
		{
			v_SQVM_RaiseError(v, "Key in table must be a \"%s\", got \"%s\" for message \"%s\" at depth \"%i\" at index \"%i\".",
				IdType2Name(OT_STRING), IdType2Name(keyType), structData->GetTypeName().c_str(), counter.Get(), i);

			return false;
		}

		const SQObjectType valueType = sq_type(node.val);

		switch (valueType)
		{
		case OT_BOOL:
			(*structData->mutable_fields())[_stringval(node.key)].set_bool_value(_bool(node.val));
			break;
		case OT_INTEGER:
			(*structData->mutable_fields())[_stringval(node.key)].set_number_value(_integer(node.val));
			break;
		case OT_FLOAT:
			(*structData->mutable_fields())[_stringval(node.key)].set_number_value(_float(node.val));
			break;
		case OT_STRING:
			(*structData->mutable_fields())[_stringval(node.key)].set_string_value(_stringval(node.val));
			break;
		case OT_VECTOR:
			LiveAPI_SetCustomVectorField((*structData->mutable_fields())[_stringval(node.key)].mutable_struct_value(), _vector3d(node.val));
			break;
		case OT_ARRAY:
			LIVEAPI_CHECK_RECURSION_DEPTH(v, counter.Get());

			if (!LiveAPI_SetCustomArrayFields(v, (*structData->mutable_fields())[_stringval(node.key)].mutable_list_value(), _array(node.val)))
				return false;

			break;
		case OT_TABLE:
			LIVEAPI_CHECK_RECURSION_DEPTH(v, counter.Get());

			if (tableData == _table(node.val))
			{
				v_SQVM_RaiseError(v, "Attempted to nest table at depth \"%i\" into itself at index \"%i\".", counter.Get(), i);
				return false;
			}

			if (!LiveAPI_SetCustomTableFields(v, (*structData->mutable_fields())[_stringval(node.key)].mutable_struct_value(), _table(node.val)))
				return false;

			break;
		default:
			LIVEAPI_UNSUPPORTED_TYPE_ERROR(v, valueType, structData);
		}
	}

	if (!ranLoop)
		LIVEAPI_EMPTY_TABLE_ERROR(v, structData);

	return true;
}

static bool LiveAPI_HandleCustomEvent(HSQUIRRELVM const v, const SQObject& obj, rtech::liveapi::CustomEvent* const event,
	const eLiveAPI_EventTypes eventType, const SQInteger fieldNum)
{
	LiveAPI_SetCommonMessageFields(event, eventType);

	switch (fieldNum)
	{
	case rtech::liveapi::CustomEvent::kNameFieldNumber:
		LIVEAPI_ENSURE_TYPE(v, obj, OT_STRING, event, fieldNum);
		event->set_name(_stringval(obj));

		break;
	case rtech::liveapi::CustomEvent::kDataFieldNumber:
		LIVEAPI_ENSURE_TYPE(v, obj, OT_TABLE, event, fieldNum);
		if (!LiveAPI_SetCustomTableFields(v, event->mutable_data(), _table(obj)))
			return false;

		break;
	default:
		LIVEAPI_FIELD_ERROR(v, fieldNum, event);
	}

	return true;
}

/*
	███████╗██╗   ██╗███████╗███╗   ██╗████████╗  ██████╗ ██╗███████╗██████╗  █████╗ ████████╗ ██████╗██╗  ██╗███████╗██████╗ 
	██╔════╝██║   ██║██╔════╝████╗  ██║╚══██╔══╝  ██╔══██╗██║██╔════╝██╔══██╗██╔══██╗╚══██╔══╝██╔════╝██║  ██║██╔════╝██╔══██╗
	█████╗  ██║   ██║█████╗  ██╔██╗ ██║   ██║     ██║  ██║██║███████╗██████╔╝███████║   ██║   ██║     ███████║█████╗  ██████╔╝
	██╔══╝  ╚██╗ ██╔╝██╔══╝  ██║╚██╗██║   ██║     ██║  ██║██║╚════██║██╔═══╝ ██╔══██║   ██║   ██║     ██╔══██║██╔══╝  ██╔══██╗
	███████╗ ╚████╔╝ ███████╗██║ ╚████║   ██║     ██████╔╝██║███████║██║     ██║  ██║   ██║   ╚██████╗██║  ██║███████╗██║  ██║
	╚══════╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝   ╚═╝     ╚═════╝ ╚═╝╚══════╝╚═╝     ╚═╝  ╚═╝   ╚═╝    ╚═════╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝
*/

static void LiveAPI_SendEvent(const google::protobuf::Message* const msg)
{
	s_liveAPIEvent.set_event_size(msg->ByteSize());
	s_liveAPIEvent.mutable_gamemessage()->PackFrom(*msg);

	LiveAPISystem()->LogEvent(&s_liveAPIEvent, &s_liveAPIEvent.gamemessage());
	s_liveAPIEvent.Clear();
}

static bool LiveAPI_HandleEventByCategory(HSQUIRRELVM const v, const SQTable* const table, const eLiveAPI_EventTypes eventType)
{
	google::protobuf::Message* msg = nullptr;

	SQ_FOR_EACH_TABLE(table, i)
	{
		const SQTable::_HashNode& node = table->_nodes[i];

		if (sq_isnull(node.key))
			continue;

		if (!LiveAPI_CheckSwitchType(v, node.key))
			return false;

		const SQInteger fieldNum = _integer(node.key);
		const SQObjectPtr& obj = node.val;

		bool ret = false;

		switch (eventType)
		{
		case eLiveAPI_EventTypes::init:
			msg = &s_init;
			ret = LiveAPI_HandleInitEvent(&s_init, eventType);
			break;
		case eLiveAPI_EventTypes::matchSetup:
			msg = &s_matchSetup;
			ret = LiveAPI_HandleMatchSetup(v, obj, &s_matchSetup, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::ammoUsed:
			msg = &s_ammoUsed;
			ret = LiveAPI_HandleAmmoUsed(v, obj, &s_ammoUsed, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::arenasItemDeselected:
			msg = &s_arenasItemDeselected;
			ret = LiveAPI_HandleInventoryChange(v, obj, &s_arenasItemDeselected, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::arenasItemSelected:
			msg = &s_arenasItemSelected;
			ret = LiveAPI_HandleInventoryChange(v, obj, &s_arenasItemSelected, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::bannerCollected:
			msg = &s_bannerCollected;
			ret = LiveAPI_HandleBannerCollected(v, obj, &s_bannerCollected, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::customEvent:
			msg = &s_customEvent;
			ret = LiveAPI_HandleCustomEvent(v, obj, &s_customEvent, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::inventoryPickUp:
			msg = &s_inventoryPickUp;
			ret = LiveAPI_HandleInventoryChange(v, obj, &s_inventoryPickUp, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::inventoryDrop:
			msg = &s_inventoryDrop;
			ret = LiveAPI_HandleInventoryDrop(v, obj, &s_inventoryDrop, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::inventoryUse:
			msg = &s_inventoryUse;
			ret = LiveAPI_HandleInventoryChange(v, obj, &s_inventoryUse, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::gameStateChanged:
			msg = &s_gameStateChanged;
			ret = LiveAPI_HandleGameStateChanged(v, obj, &s_gameStateChanged, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::matchStateEnd:
			msg = &s_matchStateEnd;
			ret = LiveAPI_HandleMatchStateEnd(v, obj, &s_matchStateEnd, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::characterSelected:
			msg = &s_characterSelected;
			ret = LiveAPI_HandleSimplePlayerMessage(v, obj, &s_characterSelected, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::warpGateUsed:
			msg = &s_warpGateUsed;
			ret = LiveAPI_HandleSimplePlayerMessage(v, obj, &s_warpGateUsed, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::wraithPortal:
			msg = &s_wraithPortal;
			ret = LiveAPI_HandleSimplePlayerMessage(v, obj, &s_wraithPortal, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::playerConnected:
			msg = &s_playerConnected;
			ret = LiveAPI_HandleSimplePlayerMessage(v, obj, &s_playerConnected, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::playerRevive:
			msg = &s_playerRevive;
			ret = LiveAPI_HandlePlayerRevive(v, obj, &s_playerRevive, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::playerDisconnected:
			msg = &s_playerDisconnected;
			ret = LiveAPI_HandlePlayerDisconnected(v, obj, &s_playerDisconnected, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::playerDamaged:
			msg = &s_playerDamaged;
			ret = LiveAPI_HandlePlayerDamaged(v, obj, &s_playerDamaged, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::playerDowned:
			msg = &s_playerDowned;
			ret = LiveAPI_HandlePlayerDowned(v, obj, &s_playerDowned, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::playerKilled:
			msg = &s_playerKilled;
			ret = LiveAPI_HandlePlayerKilled(v, obj, &s_playerKilled, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::playerAssist:
			msg = &s_playerAssist;
			ret = LiveAPI_HandlePlayerAssist(v, obj, &s_playerAssist, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::playerRespawnTeam:
			msg = &s_playerRespawnTeam;
			ret = LiveAPI_HandlePlayerRespawnTeam(v, obj, &s_playerRespawnTeam, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::playerStatChanged:
			msg = &s_playerStatChanged;
			ret = LiveAPI_HandlePlayerStatChanged(v, obj, &s_playerStatChanged, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::playerUpgradeTierChanged:
			msg = &s_playerUpgradeTierChanged;
			ret = LiveAPI_HandlePlayerUpgradeTierChanged(v, obj, &s_playerUpgradeTierChanged, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::legendUpgradeSelected:
			msg = &s_legendUpgradeSelected;
			ret = LiveAPI_HandleLegendUpgradeSelected(v, obj, &s_legendUpgradeSelected, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::gibraltarShieldAbsorbed:
			msg = &s_gibraltarShieldAbsorbed;
			ret = LiveAPI_HandleAbilityDamaged(v, obj, &s_gibraltarShieldAbsorbed, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::revenantForgedShadowDamaged:
			msg = &s_revenantForgedShadowDamaged;
			ret = LiveAPI_HandleAbilityDamaged(v, obj, &s_revenantForgedShadowDamaged, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::ringStartClosing:
			msg = &s_ringStartClosing;
			ret = LiveAPI_HandleDeathFieldStartClosing(v, obj, &s_ringStartClosing, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::ringFinishedClosing:
			msg = &s_ringFinishedClosing;
			ret = LiveAPI_HandleRingFinishedClosing(v, obj, &s_ringFinishedClosing, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::squadEliminated:
			msg = &s_squadEliminated;
			ret = LiveAPI_HandleSquadEliminated(v, obj, &s_squadEliminated, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::ziplineUsed:
			msg = &s_ziplineUsed;
			ret = LiveAPI_HandleLinkedEntityEvent(v, obj, &s_ziplineUsed, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::grenadeThrown:
			msg = &s_grenadeThrown;
			ret = LiveAPI_HandleLinkedEntityEvent(v, obj, &s_grenadeThrown, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::playerAbilityUsed:
			msg = &s_playerAbilityUsed;
			ret = LiveAPI_HandleLinkedEntityEvent(v, obj, &s_playerAbilityUsed, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::weaponSwitched:
			msg = &s_weaponSwitched;
			ret = LiveAPI_HandleWeaponSwitchedEvent(v, obj, &s_weaponSwitched, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::blackMarketAction:
			msg = &s_blackMarketAction;
			ret = LiveAPI_HandleBlackMarketActionEvent(v, obj, &s_blackMarketAction, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::observerSwitched:
			msg = &s_observerSwitched;
			ret = LiveAPI_HandleObserverSwitched(v, obj, &s_observerSwitched, eventType, fieldNum);
			break;
		case eLiveAPI_EventTypes::observerAnnotation:
			msg = &s_observerAnnotation;
			ret = LiveAPI_HandleObserverAnnotation(v, obj, &s_observerAnnotation, eventType, fieldNum);
			break;
		default:
			v_SQVM_RaiseError(v, "Event type \"%d\" not found.", eventType);
			return false;
		}

		if (!ret)
		{
			Assert(msg);

			msg->Clear();
			return false;
		}
	}

	if (!msg) // Script bug, e.g. giving an empty table (either completely empty or filled with null)
	{
		v_SQVM_RaiseError(v, "Empty table on event type \"%d\".", eventType);
		return false;
	}

	LiveAPI_SendEvent(msg);
	msg->Clear();

	return true;
}

/*
	 █████╗ ██████╗ ███████╗████████╗██████╗  █████╗  ██████╗████████╗██╗ ██████╗ ███╗   ██╗███████╗
	██╔══██╗██╔══██╗██╔════╝╚══██╔══╝██╔══██╗██╔══██╗██╔════╝╚══██╔══╝██║██╔═══██╗████╗  ██║██╔════╝
	███████║██████╔╝███████╗   ██║   ██████╔╝███████║██║        ██║   ██║██║   ██║██╔██╗ ██║███████╗
	██╔══██║██╔══██╗╚════██║   ██║   ██╔══██╗██╔══██║██║        ██║   ██║██║   ██║██║╚██╗██║╚════██║
	██║  ██║██████╔╝███████║   ██║   ██║  ██║██║  ██║╚██████╗   ██║   ██║╚██████╔╝██║ ╚████║███████║
	╚═╝  ╚═╝╚═════╝ ╚══════╝   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝
	Code exposed to scripts
*/

namespace VScriptCode
{
	namespace Server
	{
		SQRESULT LiveAPI_IsValidToRun(HSQUIRRELVM v);
		SQRESULT LiveAPI_LogRaw(HSQUIRRELVM v);

		SQRESULT LiveAPI_StartLogging(HSQUIRRELVM v);
		SQRESULT LiveAPI_StopLogging(HSQUIRRELVM v);
	}
}

SQRESULT VScriptCode::Server::LiveAPI_IsValidToRun(HSQUIRRELVM v)
{
	sq_pushbool(v, LiveAPISystem()->IsValidToRun());
	SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
}

SQRESULT VScriptCode::Server::LiveAPI_LogRaw(HSQUIRRELVM v)
{
	if (!LiveAPISystem()->IsEnabled())
		SCRIPT_CHECK_AND_RETURN(v, SQ_OK);

	SQRESULT result = SQ_OK;
	const SQObjectPtr& object = v->GetUp(-2);

	if (sq_istable(object))
	{
		const SQTable* const table = _table(object);
		SQInteger eventType = 0;

		if (SQ_FAILED(sq_getinteger(v, 3, &eventType)))
		{
			v_SQVM_ScriptError("Second argument must be an integer.");
			result = SQ_FAIL;
		}
		else if (!LiveAPI_HandleEventByCategory(v, table, eLiveAPI_EventTypes(eventType)))
			result = SQ_ERROR;
	}
	else
	{
		v_SQVM_ScriptError("First argument must be a table.");
		result = SQ_FAIL;
	}

	SCRIPT_CHECK_AND_RETURN(v, result);
}

SQRESULT VScriptCode::Server::LiveAPI_StartLogging(HSQUIRRELVM v)
{
	LiveAPISystem()->CreateLogger();
	SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
}

SQRESULT VScriptCode::Server::LiveAPI_StopLogging(HSQUIRRELVM v)
{
	LiveAPISystem()->DestroyLogger();
	SCRIPT_CHECK_AND_RETURN(v, SQ_OK);
}

void Script_RegisterLiveAPIFunctions(CSquirrelVM* const s)
{
	DEFINE_SERVER_SCRIPTFUNC_NAMED(s, LiveAPI_IsValidToRun, "Whether the LiveAPI system is enabled and able to run", "bool", "");
	DEFINE_SERVER_SCRIPTFUNC_NAMED(s, LiveAPI_LogRaw, "VM bridge to the LiveAPI logger from scripts", "void", "table< int, var > data, int eventType");

	DEFINE_SERVER_SCRIPTFUNC_NAMED(s, LiveAPI_StartLogging, "Start the LiveAPI session logger", "void", "");
	DEFINE_SERVER_SCRIPTFUNC_NAMED(s, LiveAPI_StopLogging, "Stop the LiveAPI session logger", "void", "");
}

void Script_RegisterLiveAPIEnums(CSquirrelVM* const s)
{
	DEFINE_SCRIPTENUM_NAMED(s, "eLiveAPI_EventTypes", 0,
		"ammoUsed",
		"arenasItemDeselected",
		"arenasItemSelected",
		"bannerCollected",
		"blackMarketAction",
			//"changeCamera",
		"characterSelected",
			//"checkedState",
			//"clientState",
			//"customMatch_CreateLobby",
			//"customMatch_GetLobbyPlayers",
			//"customMatch_GetSettings",
			//"customMatch_JoinLobby",
			//"customMatch_KickPlayer",
			//"customMatch_LeaveLobby",
			//"customMatch_LobbyPlayer",
			//"customMatch_LobbyPlayers",
			//"customMatch_SendChat",
			//"customMatch_SetMatchmaking",
			//"customMatch_SetReady",
			//"customMatch_SetSettings",
			//"customMatch_SetTeam",
			//"customMatch_SetTeamName",
		"customEvent",
		"datacenter",
			//"gameConVar",
		"gameStateChanged",
		"gibraltarShieldAbsorbed",
			//"globalVars",
		"grenadeThrown",
		"init",
		"inventoryDrop",
		"inventoryItem",
		"inventoryPickUp",
		"inventoryUse",
		"legendUpgradeSelected",
		"liveAPIEvent",
		"loadoutConfiguration",
		"matchSetup",
		"matchStateEnd",
		"observerAnnotation",
		"observerSwitched",
			//"pauseToggle",
		"player",
		"playerAbilityUsed",
		"playerAssist",
		"playerConnected",
		"playerDamaged",
		"playerDisconnected",
		"playerDowned",
		"playerKilled",
		"playerRespawnTeam",
		"playerRevive",
		"playerStatChanged",
		"playerUpgradeTierChanged",
			//"request",
			//"requestStatus",
			//"response",
		"revenantForgedShadowDamaged",
		"ringFinishedClosing",
		"ringStartClosing",
			//"runCommand",
			//"scriptCall",
		"squadEliminated",
			//"stateCheck",
			//"svcMsgOverflow",
			//"svcMsgRemoteScript",
		"vector3",
		"version",
		"warpGateUsed",
		"weaponSwitched",
		"wraithPortal",
		"ziplineUsed"
	);
}
