//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#ifndef RECASTEDITOR_H
#define RECASTEDITOR_H

#include "Recast/Include/Recast.h"
#include "NavEditor/Include/EditorInterfaces.h"
#include "DebugUtils/Include/RecastDebugDraw.h"
#include "DebugUtils/Include/DetourDebugDraw.h"

#include "Detour/Include/DetourNavMeshBuilder.h"

#include "game/server/ai_navmesh.h"

struct dtMeshTile;

struct NavMeshDefaults_s
{
	const char* name;

	float radius;
	float height;
	float climbHeight;

	float cellSize;
	float cellHeight;

	int polyCellResolution;
};
extern const NavMeshDefaults_s g_navMeshDefaults[NAVMESH_COUNT];

struct TraverseType_s
{
	float minDist;
	float maxDist;
	float minElev;
	float maxElev;
	float minSlope;
	float maxSlope;
	float ovlpTrig;
	bool ovlpExcl;
};

enum TraverseType_e // todo(amos): move elsewhere
{
	TRAVERSE_UNUSED_0 = 0,

	TRAVERSE_CROSS_GAP_SMALL,
	TRAVERSE_CLIMB_OBJECT_SMALL,
	TRAVERSE_CROSS_GAP_MEDIUM,

	TRAVERSE_UNUSED_4,
	TRAVERSE_UNUSED_5,
	TRAVERSE_UNUSED_6,

	TRAVERSE_CROSS_GAP_LARGE,

	TRAVERSE_CLIMB_WALL_MEDIUM,
	TRAVERSE_CLIMB_WALL_TALL,
	TRAVERSE_CLIMB_BUILDING,

	TRAVERSE_JUMP_SHORT,
	TRAVERSE_JUMP_MEDIUM,
	TRAVERSE_JUMP_LARGE,

	TRAVERSE_UNUSED_14,
	TRAVERSE_UNUSED_15,

	TRAVERSE_UNKNOWN_16, // USED!!!
	TRAVERSE_UNKNOWN_17, // USED!!!

	TRAVERSE_UNKNOWN_18,
	TRAVERSE_UNKNOWN_19, // NOTE: does not exists in MSET5!!!

	TRAVERSE_CLIMB_TARGET_SMALL,
	TRAVERSE_CLIMB_TARGET_LARGE,

	TRAVERSE_UNUSED_22,
	TRAVERSE_UNUSED_23,

	TRAVERSE_UNKNOWN_24,

	TRAVERSE_UNUSED_25,
	TRAVERSE_UNUSED_26,
	TRAVERSE_UNUSED_27,
	TRAVERSE_UNUSED_28,
	TRAVERSE_UNUSED_29,
	TRAVERSE_UNUSED_30,
	TRAVERSE_UNUSED_31,

	// These aren't traverse type!
	NUM_TRAVERSE_TYPES,
	INVALID_TRAVERSE_TYPE = DT_NULL_TRAVERSE_TYPE
};

/// Tool types.
enum EditorToolType
{
	TOOL_NONE = 0,
	TOOL_TILE_EDIT,
	TOOL_TILE_HIGHLIGHT,
	TOOL_TEMP_OBSTACLE,
	TOOL_NAVMESH_TESTER,
	TOOL_NAVMESH_PRUNE,
	TOOL_OFFMESH_CONNECTION,
	TOOL_TRAVERSE_LINK,
	TOOL_SHAPE_VOLUME,
	TOOL_CROWD,
	MAX_TOOLS
};

/// These are just poly areas to use consistent values across the editors.
/// The use should specify these base on his needs.
//enum EditorPolyAreas // note: original poly area's for reference.
//{
//	EDITOR_POLYAREA_GROUND,
//	EDITOR_POLYAREA_JUMP,
//	EDITOR_POLYAREA_ROAD,
//	EDITOR_POLYAREA_DOOR,
//	EDITOR_POLYAREA_GRASS,
//	EDITOR_POLYAREA_WATER,
//};

//enum EditorPolyFlags // note: original poly flags for reference.
//{
//	// Most common polygon flags.
//	EDITOR_POLYFLAGS_WALK		= 1<<0,		// Ability to walk (ground, grass, road)
//	EDITOR_POLYFLAGS_JUMP		= 1<<1,		// Ability to jump.
//	EDITOR_POLYFLAGS_DOOR		= 1<<2,		// Ability to move through doors.
//	EDITOR_POLYFLAGS_SWIM		= 1<<3,		// Ability to swim (water).
//	EDITOR_POLYFLAGS_DISABLED	= 1<<4,		// Disabled polygon
//	EDITOR_POLYFLAGS_ALL		= 0xffff	// All abilities.
//};

inline static const char* const g_navMeshPolyFlagNames[] =
{
	"walk",
	"too_small",
	"has_neighbour",
	"jump",
	"jump_linked",
	"unused_8",
	"obstacle",
	"unused_128",
	"disabled",
	"hazard",
	"door",
	"unused_2048",
	"unused_4096",
	"door_breachable",
	"unused_16384",
	"unused_32768",
	"all"
};

struct TraverseLinkPolyPair
{
	TraverseLinkPolyPair(dtPolyRef p1, dtPolyRef p2)
	{
		if (p1 > p2)
			rdSwap(p1, p2);

		poly1 = p1;
		poly2 = p2;
	}

	bool operator<(const TraverseLinkPolyPair& other) const
	{
		if (poly1 < other.poly1)
			return true;
		if (poly1 > other.poly1)
			return false;

		return poly2 < other.poly2;
	}

	dtPolyRef poly1;
	dtPolyRef poly2;
};

class EditorDebugDraw : public DebugDrawGL
{
public:
	virtual unsigned int areaToFaceCol(const unsigned int area) const;
	virtual unsigned int areaToEdgeCol(const unsigned int area) const;
};

enum EditorPartitionType
{
	EDITOR_PARTITION_WATERSHED,
	EDITOR_PARTITION_MONOTONE,
	EDITOR_PARTITION_LAYERS,
};

struct EditorTool
{
	virtual ~EditorTool() {}
	virtual int type() = 0;
	virtual void init(class Editor* editor) = 0;
	virtual void reset() = 0;
	virtual void handleMenu() = 0;
	virtual void handleClick(const float* s, const float* p, const int v, bool shift) = 0;
	virtual void handleRender() = 0;
	virtual void handleRenderOverlay(double* proj, double* model, int* view) = 0;
	virtual void handleToggle() = 0;
	virtual void handleStep() = 0;
	virtual void handleUpdate(const float dt) = 0;
};

struct EditorToolState {
	virtual ~EditorToolState() {}
	virtual void init(class Editor* editor) = 0;
	virtual void reset() = 0;
	virtual void handleRender() = 0;
	virtual void handleRenderOverlay(double* proj, double* model, int* view) = 0;
	virtual void handleUpdate(const float dt) = 0;
};

class Editor
{
protected:
	class InputGeom* m_geom;
	class dtNavMesh* m_navMesh;
	class dtNavMeshQuery* m_navQuery;
	class dtCrowd* m_crowd;

	bool m_ignoreWindingOrder;
	bool m_filterLowHangingObstacles;
	bool m_filterLedgeSpans;
	bool m_filterNeighborSlopes;
	bool m_filterWalkableLowHeightSpans;
	bool m_buildTraversePortals;
	bool m_traverseRayDynamicOffset;
	bool m_traverseLinkSinglePortalPerPolyPair;
	bool m_collapseLinkedPolyGroups;
	bool m_buildBvTree;

	int m_minTileBits;
	int m_maxTileBits;
	int m_tileSize;
	float m_cellSize;
	float m_cellHeight;
	float m_agentHeight;
	float m_agentRadius;
	float m_agentMaxClimb;
	float m_agentMaxSlope;
	float m_traverseRayExtraOffset;
	float m_traverseEdgeMinOverlap;
	float m_traversePortalMaxAlign;
	int m_regionMinSize;
	int m_regionMergeSize;
	int m_edgeMaxLen;
	float m_edgeMaxError;
	int m_vertsPerPoly;
	int m_polyCellRes;
	float m_detailSampleDist;
	float m_detailSampleMaxError;
	int m_partitionType;

	float m_navMeshBMin[3];
	float m_navMeshBMax[3];

	NavMeshType_e m_selectedNavMeshType;
	NavMeshType_e m_loadedNavMeshType;
	const char* m_navmeshName;
	
	EditorTool* m_tool;
	EditorToolState* m_toolStates[MAX_TOOLS];
	
	BuildContext* m_ctx;
	dtDisjointSet m_djs[DT_MAX_TRAVERSE_TABLES];
	std::map<TraverseLinkPolyPair, unsigned int> m_traverseLinkPolyMap;

	EditorDebugDraw m_dd;
	unsigned int m_navMeshDrawFlags;
	duDrawTraverseLinkParams m_traverseLinkDrawParams;
	float m_recastDrawOffset[3];
	float m_detourDrawOffset[3];

public:
	std::string m_modelName;

	Editor();
	virtual ~Editor();

	bool loadAll(std::string path, const bool fullPath = false);
	void saveAll(std::string path, const dtNavMesh* mesh);

	bool loadNavMesh(const char* path, const bool fullPath = false);
	
	void setContext(BuildContext* ctx) { m_ctx = ctx; }
	
	void setTool(EditorTool* tool);
	EditorToolState* getToolState(int type) { return m_toolStates[type]; }
	void setToolState(int type, EditorToolState* s) { m_toolStates[type] = s; }

	EditorDebugDraw& getDebugDraw() { return m_dd; }
	const float* getRecastDrawOffset() const { return m_recastDrawOffset; }
	const float* getDetourDrawOffset() const { return m_detourDrawOffset; }

	virtual void handleSettings();
	virtual void handleTools();
	virtual void handleDebugMode();
	virtual void handleClick(const float* s, const float* p, const int v, bool shift);
	virtual void handleToggle();
	virtual void handleStep();
	virtual void handleRender();
	virtual void handleRenderOverlay(double* proj, double* model, int* view);
	virtual void handleMeshChanged(class InputGeom* geom);
	virtual bool handleBuild();
	virtual void handleUpdate(const float dt);
	virtual void collectSettings(struct BuildSettings& settings);

	virtual class InputGeom* getInputGeom() { return m_geom; }
	virtual class dtNavMesh* getNavMesh() { return m_navMesh; }
	virtual class dtNavMeshQuery* getNavMeshQuery() { return m_navQuery; }
	virtual class dtCrowd* getCrowd() { return m_crowd; }
	virtual float getAgentRadius() { return m_agentRadius; }
	virtual float getAgentHeight() { return m_agentHeight; }
	virtual float getAgentClimb() { return m_agentMaxClimb; }
	virtual float getAgentSlope() { return m_agentMaxSlope; }

	inline float getCellHeight() const { return m_cellHeight; }
	
	inline unsigned int getNavMeshDrawFlags() const { return m_navMeshDrawFlags; }
	inline void setNavMeshDrawFlags(unsigned int flags) { m_navMeshDrawFlags = flags; }

	inline void toggleNavMeshDrawFlag(unsigned int flag) { m_navMeshDrawFlags ^= flag; }

	inline NavMeshType_e getSelectedNavMeshType() const { return m_selectedNavMeshType; }
	inline NavMeshType_e getLoadedNavMeshType() const { return m_loadedNavMeshType; }

	inline bool useDynamicTraverseRayOffset() const { return m_traverseRayDynamicOffset; }
	inline float getTraverseRayExtraOffset() const { return m_traverseRayExtraOffset; }

	inline std::map<TraverseLinkPolyPair, unsigned int>& getTraverseLinkPolyMap() { return m_traverseLinkPolyMap; }

	inline const char* getModelName() const { return m_modelName.c_str(); }

	void updateToolStates(const float dt);
	void initToolStates(Editor* editor);
	void resetToolStates();
	void renderToolStates();
	void renderOverlayToolStates(double* proj, double* model, int* view);

	void renderMeshOffsetOptions();
	void renderDetourDebugMenu();
	void renderTraverseTableFineTuners();
	void renderIntermediateTileMeshOptions();

	void selectNavMeshType(const NavMeshType_e navMeshType);

	void resetCommonSettings();
	void handleCommonSettings();

	void updateTraverseLinkRenderParams();

	void createTraverseLinkParams(dtTraverseLinkConnectParams& params);

	bool createTraverseLinks();
	void connectOffMeshLinks();

	bool createStaticPathingData();

private:
	// Explicitly disabled copy constructor and copy assignment operator.
	Editor(const Editor&);
	Editor& operator=(const Editor&);
};


#endif // RECASTEDITOR_H
