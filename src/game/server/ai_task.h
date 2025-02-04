//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Sets up the tasks for default AI.
//
// $NoKeywords: $
//=============================================================================//
#ifndef AI_TASK_H
#define AI_TASK_H
#ifdef _WIN32
#pragma once
#endif

// Codes are either one of the enumerated types below, or a string (similar to Windows resource IDs)
typedef intp AI_TaskFailureCode_t;

//=========================================================
// These are the failure codes
//=========================================================
enum AI_BaseTaskFailureCodes_t
{
	NO_TASK_FAILURE,
	FAIL_NO_TARGET,
	FAIL_WEAPON_OWNED,
	FAIL_ITEM_NO_FIND,
	FAIL_NO_HINT_NODE,
	FAIL_SCHEDULE_NOT_FOUND,
	FAIL_NO_ENEMY,
	FAIL_NO_NEAR_NODE,
	FAIL_NO_BACKAWAY_NODE,
	FAIL_NO_COVER,
	FAIL_NO_FLANK,
	FAIL_NO_SHOOT,
	FAIL_NO_ROUTE,
	FAIL_NO_ROUTE_GOAL,
	FAIL_NO_ROUTE_BLOCKED,
	FAIL_NO_ROUTE_ILLEGAL,
	FAIL_NO_WALK,
	FAIL_NO_LOS,
	FAIL_ALREADY_LOCKED,
	FAIL_NO_SOUND,
	FAIL_BAD_ACTIVITY,
	FAIL_NO_GOAL,
	FAIL_NO_PLAYER,
	FAIL_NO_REACHABLE_NODE,
	FAIL_NO_AI_NETWORK,
	FAIL_NO_START_POSITION,
	FAIL_BAD_POSITION,
	FAIL_BAD_PATH_GOAL,
	FAIL_STUCK_ONTOP,
	FAIL_ITEM_TAKEN,
	FAIL_FROZEN,
	FAIL_ANIMATION_BLOCKED,
	FAIL_TIMEOUT,
	FAIL_DETOUR_SPECIFIC,

	NUM_FAIL_CODES,
};

inline bool IsPathTaskFailure(AI_TaskFailureCode_t code)
{
	return (code >= FAIL_NO_ROUTE && code <= FAIL_NO_ROUTE_ILLEGAL);
}

const char* TaskFailureToString(AI_TaskFailureCode_t code);
inline intp MakeFailCode(const char* const pszGeneralError) { return (intp)pszGeneralError; }

enum TaskStatus_e 
{
	TASKSTATUS_NEW               = 0, // Just started
	TASKSTATUS_RUN_MOVE_AND_TASK = 1, // Running task & movement
	TASKSTATUS_RUN_MOVE          = 2, // Just running movement
	TASKSTATUS_RUN_TASK          = 3, // Just running task
	TASKSTATUS_COMPLETE          = 4, // Completed, get next task
};

//=========================================================
// These are the shared tasks
//=========================================================
enum sharedtasks_e
{
	TASK_INVALID = 0,

	// Forces the activity to reset.
	TASK_RESET_ACTIVITY,

	// Waits for the specified number of seconds.
	TASK_WAIT,

	// Make announce attack sound
	TASK_ANNOUNCE_ATTACK,

	// Waits for the specified number of seconds. Will constantly turn to 
	// face the enemy while waiting.
	TASK_WAIT_WHILE_FACING_ENEMY,

	// Wait until the player enters the same PVS as this character.
	TASK_WAIT_PVS,
	TASK_NOTIFY_ENEMY_ELUDED,
	TASK_MOVE_AWAY_PATH,

	TASK_SET_PATH_CLUSTER_EXCLUDE,

	TASK_GET_PATH_AWAY_FROM_BEST_SOUND,
	TASK_GET_PATH_TO_ENEMY,
	TASK_GET_PATH_TO_ENEMY_LKP,
	TASK_GET_PATH_TO_ENEMY_LKP_LOS,
	TASK_GET_PATH_TO_ENEMY_CORPSE,
	TASK_GET_PATH_TO_SQUAD_LEADER_GOAL,
	TASK_GET_PATH_TO_SQUAD_ASSIGNED_NODE,
	TASK_GET_PATH_TO_RANGE_ENEMY_LKP_LOS,
	TASK_GET_PATH_OFF_OF_NPC,
	TASK_GET_PATH_TO_HINTNODE,
	TASK_GET_PATH_TO_NEXT_SEARCH_POINT,
	TASK_GET_PATH_TO_INITIATE_SYNCED_MELEE,

	TASK_GET_RUNAWAY_PATH_FROM_ENEMY,
	TASK_RESERVE_NEXT_SEARCH_POINT,
	TASK_INIT_SEARCH_PATH,
	TASK_STORE_POSITION_IN_SAVEPOSITION,
	TASK_STORE_BESTSOUND_IN_SAVEPOSITION,
	TASK_STORE_BESTSOUND_REACTORIGIN_IN_SAVEPOSITION,
	TASK_STORE_ENEMY_POSITION_IN_SAVEPOSITION,
	TASK_STORE_CORPSE_POSITION_IN_SAVEPOSITION,

	TASK_UNUSED_29,
	TASK_UNUSED_30,
	TASK_UNUSED_31,

	TASK_GET_PATH_TO_SAVEPOSITION,
	TASK_GET_PATH_TO_SAVEPOSITION_LOS,
	TASK_GET_PATH_TO_RANDOM_NODE,
	TASK_GET_PATH_TO_BESTSOUND,

	TASK_MOVE_PATH,
	TASK_SPRINT_PATH,
	TASK_SPRINT_RUNAWAY_PATH,
	TASK_RUN_PATH,
	TASK_WALK_PATH,
	TASK_SPRINT_PATH_TIMED,
	TASK_STRAFE_PATH,
	TASK_CLEAR_MOVE_WAIT,
	TASK_DISABLE_ARRIVAL_ONCE,
	TASK_BIG_FLINCH,
	TASK_FACE_IDEAL,
	TASK_FACE_REASONABLE,
	TASK_FACE_PLAYER,
	TASK_FACE_ENEMY,
	TASK_FACE_ENEMY_STRICT,
	TASK_FACE_ENEMY_IF_HAS_LOS,
	TASK_SET_WEAPON_BLOCKED_TIMER,
	TASK_AIM_AT_ENEMY,
	TASK_FACE_HINTNODE,
	TASK_PLAY_HINT_ACTIVITY,
	TASK_FACE_SAVEPOSITION,
	TASK_LOOKAT_ENEMY,
	TASK_LOOKAT_SAVEPOSITION,
	TASK_SET_IDEAL_YAW_TO_CURRENT,

	TASK_RANGE_ATTACK1,
	TASK_RANGE_ATTACK2,
	TASK_MELEE_ATTACK1,
	TASK_MELEE_CHARGE,
	TASK_MELEE_DIRECTIONAL_ATTACK,
	TASK_SPECIAL_ATTACK,
	TASK_MELEE_RAM,
	TASK_WAIT_BETWEEN_BURSTS,
	TASK_WAIT_UNTIL_ATTACK_READY,
	TASK_RANGE_ATTACK1_EVASIVE,
	TASK_RELOAD,
	TASK_SET_RUNNING_FROM_ENEMY,
	TASK_CLEAR_HINTNODE,
	TASK_LOCK_HINTNODE,
	TASK_SET_ACTIVITY,
	TASK_WAIT_FOR_ACTIVITY_TO_FINISH,
	TASK_SET_IDLE_ACTIVITY,
	TASK_RANDOMIZE_FRAMERATE,

	TASK_SET_SCHEDULE,
	TASK_SET_FAIL_SCHEDULE,
	TASK_SET_TOLERANCE_DISTANCE,
	TASK_SET_STOP_AT_GOAL,
	TASK_SET_ROUTE_SEARCH_TIME,
	TASK_CLEAR_FAIL_SCHEDULE,

	TASK_PLAY_SEQUENCE,
	TASK_PLAY_SEQUENCE_FACE_ENEMY,
	TASK_PLAY_REACTION_SEQUENCE,

	TASK_FIND_COVER_FROM_BEST_SOUND,
	TASK_FIND_COVER_FROM_ENEMY,
	TASK_FIND_COVER_FROM_ENEMY_WITHIN_RADIUS,
	TASK_FIND_COVER_FROM_ENEMY_LIMITLESS,
	TASK_FIND_LATERAL_COVER_FROM_ENEMY,
	TASK_FIND_BACKAWAY_FROM_SAVEPOSITION,
	TASK_FIND_SAFE_HINT_FROM_ENEMY,
	TASK_FIND_HIGH_GROUND_NODE_FROM_ORIGIN,

	TASK_DIE,

	TASK_WAIT_FOR_SCRIPT_ANIM_END,
	TASK_WAIT_FOR_SYNCED_MELEE_ANIM_END,

	TASK_UNUSED_98,

	TASK_WAIT_RANDOM,
	TASK_WAIT_INDEFINITE,

	TASK_STOP_MOVING,
	TASK_REMEMBER,
	TASK_FORGET,
	TASK_ARRIVAL_ANIMATION,
	TASK_SET_ADJUST_MOVE_SPEED_TO_SQUAD,

	TASK_WAIT_FOR_MOVEMENT,
	TASK_WAIT_FOR_MOVEMENT_CHASE,
	TASK_WAIT_FOR_MOVEMENT_STEP,
	TASK_WAIT_FOR_MOVEMENT_FACE_TARGET_AT_END,

	TASK_FALL_TO_GROUND,
	TASK_WANDER,
	TASK_GATHER_CONDITIONS,
	TASK_ADD_HEALTH,

	TASK_UNUSED_114,
	TASK_UNUSED_115,

	// First task of all schedules for playing back scripted sequences
	TASK_PRE_SCRIPT,
	TASK_DISPLACE_FROM_DANGEROUS_AREA,

	TASK_DODGE,
	TASK_DODGE_PATH,

	TASK_STRAFE_DODGE,
	TASK_STRAFE_SLIDE,
	TASK_CIRCLE_STRAFE_PATH,
	TASK_ATTACK_RUN_PATH,
	TASK_REFRESH_REACT_TO_SOUND,
	TASK_SCRIPTED_DIALOGUE,
	TASK_SET_NPC_FLAG,
	TASK_CLEAR_NPC_FLAG,
	TASK_GET_PATH_TO_THROWABLE_OBJECT,
	TASK_GET_PATH_TO_SHOOTING_COVER,
	TASK_ALIGN_WITH_SHOOTING_COVER_IDEAL_YAW,
	TASK_SET_SHOOTING_COVER_ACTIVITY,
	TASK_SET_ADVANCE_FROM_SHOOTING_COVER_ACTIVITY,
	TASK_SET_RETREAT_TO_SHOOTING_COVER_ACTIVITY,
	TASK_BEGIN_SHOOTING_COVER_BURST_FIRE,
	TASK_GET_PATH_TO_BATTLE_LOCATION,
	TASK_FACE_BLOCKING_PHYS_ENT,
	TASK_KNOCK_AWAY_PHYS_ENTS,
	TASK_BLOCK_DAMAGE,
	TASK_WAIT_FOR_BLOCK,
	TASK_CANT_FOLLOW,
	TASK_FACE_FOLLOW_TARGET,
	TASK_MOVE_TO_FOLLOW_POSITION,
	TASK_GET_PATH_TO_FOLLOW_POSITION,
	TASK_SET_FOLLOW_TARGET_MARK,
	TASK_FOLLOWER_FACE_TACTICAL,
	TASK_SET_FOLLOW_DELAY,
	TASK_GET_PATH_TO_FOLLOW_POINT,
	TASK_ARRIVE_AT_FOLLOW_POINT,
	TASK_SET_FOLLOW_POINT_STAND_SCHEDULE,
	TASK_BEGIN_STAND_AT_WAIT_POINT,
	TASK_GET_PATH_TO_ASSAULT_POINT,
	TASK_FACE_ASSAULT_POINT,

	// ======================================
	// IMPORTANT: This must be the last enum
	// ======================================
	LAST_SHARED_TASK
};

// an array of tasks is a task list
// an array of schedules is a schedule list
struct Task_t
{
	int     iTask;
	float   flTaskData;
};

#endif // AI_TASK_H
