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

#ifndef DETOURNAVMESH_H
#define DETOURNAVMESH_H

#include "Shared/Include/SharedAlloc.h"
#include "Shared/Include/SharedConst.h"
#include "Detour/Include/DetourStatus.h"

// NOTE: these are defines as we need to be able to switch between code that is
// dedicated for each version, during compile time.
#define DT_NAVMESH_SET_VERSION 8 // Public versions: 5,7,8,9.
#define DT_NAVMESH_SET_MAGIC ('M'<<24 | 'S'<<16 | 'E'<<8 | 'T')
int dtGetNavMeshVersionForSet(const int setVersion);

// Undefine (or define in a build config) the following line to use 64bit polyref.
// Generally not needed, useful for very large worlds.
// Note: tiles build using 32bit refs are not compatible with 64bit refs!
//#define DT_POLYREF64 1

//#ifdef DT_POLYREF64
// TODO: figure out a multiplatform version of uint64_t
// - maybe: https://code.google.com/p/msinttypes/
// - or: http://www.azillionmonkeys.com/qed/pstdint.h
#include <stdint.h>
//#endif

// Note: If you want to use 64-bit refs, change the types of both dtPolyRef & dtTileRef.
// It is also recommended that you change dtHashRef() to a proper 64-bit hash.

/// A handle to a polygon within a navigation mesh tile.
/// @ingroup detour
#ifdef DT_POLYREF64
static const unsigned int DT_SALT_BITS = 16;
static const unsigned int DT_TILE_BITS = 28;
static const unsigned int DT_POLY_BITS = 20;
typedef uint64_t dtPolyRef;
#else
typedef unsigned int dtPolyRef;
#endif

/// A handle to a tile within a navigation mesh.
/// @ingroup detour
#ifdef DT_POLYREF64
typedef uint64_t dtTileRef;
#else
typedef unsigned int dtTileRef;
#endif

/// A value that indicates that this tile doesn't contain any polygons with valid links
/// to the rest of the reachable area's of the navigation mesh, this tile will not be
/// added to the position lookup table.
/// @ingroup detour
static const int DT_FULL_UNLINKED_TILE_USER_ID = 1;

/// A value that indicates that this tile contains at least 1 polygon that doesn't link
/// to anything (tagged as #DT_UNLINKED_POLY_GROUP), and 1 that does link to something.
static const int DT_SEMI_UNLINKED_TILE_USER_ID = 2;

/// A value that indicates that this poly hasn't been assigned to a group yet.
static const unsigned short DT_NULL_POLY_GROUP = 0;

/// A polygon group that holds all unconnected polys (not linked to anything).
/// These are considered 'trash' by the game engine; see [r5apex_ds + CA88B2].
/// For reference, Titanfall 2 single player NavMeshes also marked everything unconnected as '1'.
static const unsigned short DT_UNLINKED_POLY_GROUP = 1;

/// The first non-reserved poly group; #DT_UNLINKED_POLY_GROUP and below are reserved.
static const unsigned short DT_FIRST_USABLE_POLY_GROUP = 2;

/// The minimum number of polygon groups (islands) before an actual table has
/// to be built and allocated. Static pathing can work without a table if we
/// only have 2 polygon groups, as all linked polygons are tagged under the
/// group #DT_FIRST_USABLE_POLY_GROUP, while the dead polygons are tagged under
/// the group #DT_UNLINKED_POLY_GROUP.
static const unsigned short DT_MIN_POLY_GROUP_COUNT = 3;

/// The maximum number of polygon groups (islands) per navmesh.
static const unsigned short DT_MAX_POLY_GROUP_COUNT = 65535;

/// The maximum number of traversal tables per navmesh that will be used for static pathing.
static const int DT_MAX_TRAVERSE_TABLES = 5;

/// A value that indicates the link doesn't require a traverse action. (Jumping, climbing, etc.)
static const unsigned char DT_NULL_TRAVERSE_TYPE = 0xff;

static const unsigned char DT_MAX_TRAVERSE_TYPES = 32;

/// A value that indicates the link doesn't contain a reverse traverse link.
static const unsigned short DT_NULL_TRAVERSE_REVERSE_LINK = 0xffff;

/// The maximum traverse distance for a traverse link. (Quantized value should not overflow #dtLink::traverseDist.)
static const float DT_TRAVERSE_DIST_MAX = 2550.0f;

/// The cached traverse link distance quantization factor.
static const float DT_TRAVERSE_DIST_QUANT_FACTOR = 0.1f;

/// A value that indicates the link doesn't contain a hint index.
static const unsigned short DT_NULL_HINT = 0xffff;

/// @{
/// @name Tile Serialization Constants
/// These constants are used to detect whether a navigation tile's data
/// and state format is compatible with the current build.
///

/// A magic number used to detect compatibility of navigation tile data.
static const int DT_NAVMESH_MAGIC = 'D'<<24 | 'N'<<16 | 'A'<<8 | 'V';

/// A version number used to detect compatibility of navigation tile data.
static const int DT_NAVMESH_VERSION = dtGetNavMeshVersionForSet(DT_NAVMESH_SET_VERSION);

/// A magic number used to detect the compatibility of navigation tile states.
static const int DT_NAVMESH_STATE_MAGIC = 'D'<<24 | 'N'<<16 | 'M'<<8 | 'S';

/// A version number used to detect compatibility of navigation tile states.
static const int DT_NAVMESH_STATE_VERSION = 1;

/// @}

/// A flag that indicates that an entity links to an external entity.
/// (E.g. A polygon edge is a portal that links to another polygon.)
static const unsigned short DT_EXT_LINK = 0x8000;

/// A value that indicates the entity does not link to anything.
static const unsigned int DT_NULL_LINK = 0xffffffff;

/// A flag that indicates that an off-mesh connection can be traversed in both directions. (Is bidirectional.)
static const unsigned int DT_OFFMESH_CON_BIDIR = 1;

/// A value that determines the offset between the start pos and the ref pos in an off-mesh connection.
static const float DT_OFFMESH_CON_REFPOS_OFFSET = 35.f;

/// A flag that indicates that the off-mesh link should be traversed from or towards the off-mesh vert.
static const unsigned char DT_OFFMESH_CON_TRAVERSE_ON_VERT = 1<<6;

/// A flag that indicates that the off-mesh link can be traversed from or towards the polygon it connects to.
static const unsigned char DT_OFFMESH_CON_TRAVERSE_ON_POLY = 1<<7;

/// A value that determines the maximum number of points describing the straight path result.
static const int DT_STRAIGHT_PATH_RESOLUTION = 5;

/// The maximum number of user defined area ids.
/// @ingroup detour
static const int DT_MAX_AREAS = 32; // <-- confirmed 32 see [r5apex_ds.exe + 0xf47dda] '-> test    [rcx+80h], ax'.

/// Tile flags used for various functions and fields.
/// For an example, see dtNavMesh::addTile().
enum dtTileFlags
{
	/// The navigation mesh owns the tile memory and is responsible for freeing it.
	DT_TILE_FREE_DATA = 0x01,

	/// The navigation mesh owns the cell memory and is responsible for freeing it.
	DT_CELL_FREE_DATA = 0x02,
};

/// Vertex flags returned by dtNavMeshQuery::findStraightPath.
enum dtStraightPathFlags
{
	DT_STRAIGHTPATH_START = 0x01,				///< The vertex is the start position in the path.
	DT_STRAIGHTPATH_END = 0x02,					///< The vertex is the end position in the path.
	DT_STRAIGHTPATH_OFFMESH_CONNECTION = 0x04,	///< The vertex is the start of an off-mesh connection.
};

/// Options for dtNavMeshQuery::findStraightPath.
enum dtStraightPathOptions
{
	DT_STRAIGHTPATH_AREA_CROSSINGS = 0x01,	///< Add a vertex at every polygon edge crossing where area changes.
	DT_STRAIGHTPATH_ALL_CROSSINGS = 0x02,	///< Add a vertex at every polygon edge crossing.
};


/// Options for dtNavMeshQuery::initSlicedFindPath and updateSlicedFindPath
enum dtFindPathOptions
{
	DT_FINDPATH_ANY_ANGLE = 0x02,		///< use raycasts during pathfind to "shortcut" (raycast still consider costs)
};

/// Options for dtNavMeshQuery::raycast
enum dtRaycastOptions
{
	DT_RAYCAST_USE_COSTS = 0x01,		///< Raycast should calculate movement cost along the ray and fill RaycastHit::cost
};


/// Limit raycasting during any angle pahfinding
/// The limit is given as a multiple of the character radius
static const float DT_RAY_CAST_LIMIT_PROPORTIONS = 50.0f;

/// Flags representing the type of a navigation mesh polygon.
enum dtPolyTypes
{
	/// The polygon is a standard convex polygon that is part of the surface of the mesh.
	DT_POLYTYPE_GROUND = 0,
	/// The polygon is an off-mesh connection consisting of two vertices.
	DT_POLYTYPE_OFFMESH_CONNECTION = 1,
};

enum dtPolyAreas
{
#if DT_NAVMESH_SET_VERSION >= 9
	DT_POLYAREA_JUMP,
	DT_POLYAREA_GROUND,
#else
	DT_POLYAREA_GROUND,
	DT_POLYAREA_JUMP,
#endif
	// NOTE: not sure if anything beyond DT_POLYAREA_JUMP belongs to MSET5,
	// this needs to be confirmed, for now its been kept in for MSET5.
	DT_POLYAREA_JUMP_REVERSE,
	DT_POLYAREA_TRIGGER,
	DT_POLYAREA_WALLJUMP_LEFT,
	DT_POLYAREA_WALLJUMP_RIGHT,
	DT_POLYAREA_WALLJUMP_LEFT_REVERSE,
	DT_POLYAREA_WALLJUMP_RIGHT_REVERSE,
};

enum dtPolyFlags
{
	/// Most common polygon flags.

	/// Ability to walk (ground, grass, road).
	DT_POLYFLAGS_WALK            = 1<<0,
	/// This polygon's surface area is too small; it will be ignored during AIN script nodes generation, NavMesh_RandomPositions, dtNavMeshQuery::findLocalNeighbourhood, etc.
	DT_POLYFLAGS_TOO_SMALL       = 1<<1,
	/// This polygon is connected to a polygon on a neighbouring tile.
	DT_POLYFLAGS_HAS_NEIGHBOUR   = 1<<2,

	/// Off-mesh connection flags

	/// Ability to jump (exclusively used on off-mesh connection polygons).
	DT_POLYFLAGS_JUMP            = 1<<3,
	/// Off-mesh connections who's start and end verts link to other polygons need this flag.
	DT_POLYFLAGS_JUMP_LINKED     = 1<<4,

	/// Unknown, no use cases found yet.
	DT_POLYFLAGS_UNK2            = 1<<5,

	/// Only used along with poly area 'DT_POLYAREA_TRIGGER'.

	/// Unknown, used for small road blocks and other small but easily climbable obstacles.
	DT_POLYFLAGS_OBSTACLE        = 1<<6,
	/// Unknown, no use cases found yet.
	DT_POLYFLAGS_UNK4            = 1<<7,
	/// Used for ToggleNPCPathsForEntity. Also, see [r5apex_ds + 0xC96EA8]. Used for toggling poly's when a door closes during runtime.
	/// Also used to disable poly's in the navmesh file itself when we do happen to build navmesh on lava or other very hazardous areas.
	DT_POLYFLAGS_DISABLED        = 1<<8,
	/// see [r5apex_ds + 0xC96ED0], used for hostile objects such as electric fences.
	DT_POLYFLAGS_HAZARD          = 1<<9,
	/// See [r5apex_ds + 0xECBAE0], used for large bunker style doors (vertical and horizontal opening ones), perhaps also shooting cover hint?.
	DT_POLYFLAGS_DOOR            = 1<<10,
	/// Unknown, no use cases found yet.
	DT_POLYFLAGS_UNK8            = 1<<11,
	/// Unknown, no use cases found yet.
	DT_POLYFLAGS_UNK9            = 1<<12,
	/// Used for doors that need to be breached, such as the Explosive Holds doors.
	DT_POLYFLAGS_DOOR_BREACHABLE = 1<<13,

	/// All abilities.
	DT_POLYFLAGS_ALL             = 0xffff
};

/// Defines a polygon within a dtMeshTile object.
/// @ingroup detour
struct dtPoly
{
	/// Index to first link in linked list. (Or #DT_NULL_LINK if there is no link.)
	unsigned int firstLink;

	/// The indices of the polygon's vertices.
	/// The actual vertices are located in dtMeshTile::verts.
	unsigned short verts[RD_VERTS_PER_POLYGON];

	/// Packed data representing neighbor polygons references and flags for each edge.
	unsigned short neis[RD_VERTS_PER_POLYGON];

	/// The user defined polygon flags.
	unsigned short flags;

	/// The number of vertices in the polygon.
	unsigned char vertCount;

	/// The bit packed area id and polygon type.
	/// @note Use the structure's set and get methods to access this value.
	unsigned char areaAndtype;

	/// The poly group id determining to which island it belongs, and to which it connects.
	unsigned short groupId;

	/// The poly surface area. (Quantized by #DT_POLY_AREA_QUANT_FACTOR).
	unsigned short surfaceArea;

#if DT_NAVMESH_SET_VERSION >= 7
	// These 2 are most likely related, it needs to be reversed still.
	// No use case has been found in the executable yet, its possible these are
	// used internally in the editor. Dynamic reverse engineering required to
	// confirm this.
	unsigned short unk1;
	unsigned short unk2;
#endif

	/// The center of the polygon; see abstracted script function 'Navmesh_RandomPositions'.
	float center[3];

	/// Sets the user defined area id. [Limit: < #DT_MAX_AREAS]
	inline void setArea(unsigned char a) { areaAndtype = (areaAndtype & 0xc0) | (a & 0x3f); }

	/// Sets the polygon type. (See: #dtPolyTypes.)
	inline void setType(unsigned char t) { areaAndtype = (areaAndtype & 0x3f) | (t << 6); }

	/// Gets the user defined area id.
	inline unsigned char getArea() const { return areaAndtype & 0x3f; }

	/// Gets the polygon type. (See: #dtPolyTypes)
	inline unsigned char getType() const { return areaAndtype >> 6; }
};

/// Calculates the surface area of the polygon.
///  @param[in]		poly	The polygon.
///  @param[in]		verts	The polygon vertices.
/// @return The total surface are of the polygon.
float dtCalcPolySurfaceArea(const dtPoly* poly, const float* verts);

/// Defines the location of detail sub-mesh data within a dtMeshTile.
struct dtPolyDetail
{
	unsigned int vertBase;			///< The offset of the vertices in the dtMeshTile::detailVerts array.
	unsigned int triBase;			///< The offset of the triangles in the dtMeshTile::detailTris array.
	unsigned char vertCount;		///< The number of vertices in the sub-mesh.
	unsigned char triCount;			///< The number of triangles in the sub-mesh.
};

/// Defines the vertical triangle of a wall hint.
struct dtTriangleSurface
{
	float pos[3];					///< The surface position of the triangle. [(x, y, z)]
	float minDist;					///< The minimum distance between the agent's and triangle position before they are considered close enough.
	unsigned short vertAIndex;		///< The index of the first vert connecting A->B.
	unsigned short vertBIndex;		///< The index of the second vert connecting B->C.
	unsigned short vertCIndex;		///< The index of the third vert connecting C->A.
	float surfArea;					///< The surface area of the triangle. (Note: this field was unused by the engine and therefore re-purposed).
	unsigned char edgeFlags;		///< The edge flags, use #dtGetDetailTriEdgeFlags to retrieve them.
};

/// Get flags for edge in detail triangle.
/// @param	triFlags[in]		The flags for the triangle (last component of detail vertices above).
/// @param	edgeIndex[in]		The index of the first vertex of the edge. For instance, if 0.
///								returns flags for edge AB.
inline int dtGetDetailTriEdgeFlags(unsigned char triFlags, int edgeIndex)
{
	return (triFlags >> (edgeIndex * 2)) & 0x3;
}

/// Defines a link between polygons.
/// @note This structure is rarely if ever used by the end user.
/// @see dtMeshTile
struct dtLink
{
	inline bool hasTraverseType() const { return traverseType != DT_NULL_TRAVERSE_TYPE; }
	inline unsigned char getTraverseType() const {return traverseType & (DT_MAX_TRAVERSE_TYPES-1); }

	dtPolyRef ref;					///< Neighbour reference. (The neighbor that is linked to.)
	unsigned int next;				///< Index of the next link.
	unsigned char edge;				///< Index of the polygon edge that owns this link.
	unsigned char side;				///< If a boundary link, defines on which side the link is.
	unsigned char bmin;				///< If a boundary link, defines the minimum sub-edge area.
	unsigned char bmax;				///< If a boundary link, defines the maximum sub-edge area.
	unsigned char traverseType;		///< The traverse type for this link. (Jumping, climbing, etc.)
	unsigned char traverseDist;		///< The traverse distance of this link. (Quantized by #DT_TRAVERSE_DIST_QUANT_FACTOR).
	unsigned short reverseLink;		///< The reverse traversal link for this link. (Path returns through this link.)
};

float dtCalcLinkDistance(const float* spos, const float* epos);
unsigned char dtQuantLinkDistance(const float distance);

/// Defines a cell in a tile.
/// @note This is used to prevent entities from clipping into each other.
/// @see dtMeshTile
struct dtCell
{
	inline void setOccupied() { *(int*)((uintptr_t)&occupyState & ~0x3) = -1; }

	float pos[3];					///< The position of the cell.
	unsigned int polyIndex;			///< The index of the poly this cell is on.
	unsigned char pad;				
	unsigned char occupyState[4];	///< The occupation state of this cell, -1 means not occupied. See [r5apex_ds + 0xEF86C9].

#if DT_NAVMESH_SET_VERSION >= 9
	unsigned char data[27]; // TODO: reverse this, always appears 0.
#else
	unsigned char data[52]; // TODO: reverse this, always appears 0.
#endif
};

/// Defines a hint in the navmesh.
/// @note This is used to define wall surface triangles to help with special movements such as wall running.
/// @see dtOffMeshConnection
struct dtHint
{
	float* verts;					///< The triangle vertices. [Size: (x, y, z) * dtHint::vertCount]
	dtTriangleSurface* tris;		///< The triangles. [Size: dtHint::triCount]
	char unk[24]; // Editor only.
	int vertCount;					///< The number of vertices in the hint.
	int triCount;					///< The number of triangles in the hint.
};

/// Bounding volume node.
/// @note This structure is rarely if ever used by the end user.
/// @see dtMeshTile
struct dtBVNode
{
	unsigned short bmin[3];			///< Minimum bounds of the node's AABB. [(x, y, z)]
	unsigned short bmax[3];			///< Maximum bounds of the node's AABB. [(x, y, z)]
	int i;							///< The node's index. (Negative for escape sequence.)
};

/// Defines an navigation mesh off-mesh connection within a dtMeshTile object.
/// An off-mesh connection is a user defined traversable connection made up to two vertices.
struct dtOffMeshConnection
{
	unsigned char getTraverseType() 
	{ 
#if DT_NAVMESH_SET_VERSION >= 7
		return traverseType & (DT_MAX_TRAVERSE_TYPES-1);
#else
		return traverseContext & 0xff;
#endif
	}

	unsigned char getVertLookupOrder()
	{
#if DT_NAVMESH_SET_VERSION >= 7
		return traverseType & (1<<6);
#else
		return (traverseContext >> 8) & 0xff;
#endif
	}

	void setTraverseType(unsigned char type, unsigned char order)
	{
#if DT_NAVMESH_SET_VERSION >= 7
		traverseType = type & (DT_MAX_TRAVERSE_TYPES-1);

		if (order) // Inverted, mark it.
			traverseType |= (1<<6);
#else
		traverseContext = type | (order<<8);
#endif
	}

#if DT_NAVMESH_SET_VERSION < 7
	/// The hint index of the off-mesh connection. (Or #DT_NULL_HINT if there is no hint.)
	unsigned short getHintIndex() { return traverseContext; };
	void setHintIndex(unsigned short index) { traverseContext = index; };
#endif

	/// The endpoints of the connection. [(ax, ay, az, bx, by, bz)]
	float pos[6];

	/// The radius of the endpoints. [Limit: >= 0]
	float rad;

	/// The polygon reference of the connection within the tile.
	unsigned short poly;

#if DT_NAVMESH_SET_VERSION >= 7
	/// End point side.
	unsigned char side;

	/// The traverse type.
	unsigned char traverseType;

	/// The id of the off-mesh connection. (User assigned when the navigation mesh is built.)
	unsigned short userId;

	/// The hint index.
	unsigned short hintIndex;
#else
	/// Link flags.
	/// @note These are not the connection's user defined flags. Those are assigned via the
	/// connection's dtPoly definition. These are link flags used for internal purposes.
	unsigned char flags;

	/// End point side.
	unsigned char side;

	/// The traverse type and lookup order.
	unsigned short traverseContext;

	/// The id of the off-mesh connection. (User assigned when the navigation mesh is built.)
	unsigned short userId;
#endif

	/// The reference position set to the start of the off-mesh connection with an offset of DT_OFFMESH_CON_REFPOS_OFFSET
	float refPos[3]; // See [r5apex_ds + F114CF], [r5apex_ds + F11B42], [r5apex_ds + F12447].
	/// The reference yaw angle set towards the end position of the off-mesh connection.
	float refYaw;    // See [r5apex_ds + F11527], [r5apex_ds + F11F90], [r5apex_ds + F12836].

#if DT_NAVMESH_SET_VERSION >= 9
	/// Off-mesh connections are always placed in pairs, in version 5 to 8, each connection for
	/// the pair was a separate instance. In newer versions, this is squashed into 1 connection.
	float secPos[6];
#endif
};

/// Calculates the yaw angle in an off-mesh connection.
/// @param	spos[in]		The start position of the off mesh connection.
/// @param	epos[in]		The end position of the off mesh connection.
///								returns the yaw angle on the XY plane in radians.
extern float dtCalcOffMeshRefYaw(const float* spos, const float* epos);
/// Calculates the ref position in an off-mesh connection.
/// @param	spos[in]		The start position of the off mesh connection.
/// @param	yawRad[in]		The yaw angle of the off-mesh connection in radians.
/// @param	offset[in]		The desired offset from the start position.
/// @param	res[in]			The output ref position.
extern void dtCalcOffMeshRefPos(const float* spos, const float yawDeg, const float* offset, float* res);

/// Provides high level information related to a dtMeshTile object.
/// @ingroup detour
struct dtMeshHeader
{
	int magic;				///< Tile magic number. (Used to identify the data format.)
	int version;			///< Tile data format version number.
	int x;					///< The x-position of the tile within the dtNavMesh tile grid. (x, y, layer)
	int y;					///< The y-position of the tile within the dtNavMesh tile grid. (x, y, layer)
	int layer;				///< The layer of the tile within the dtNavMesh tile grid. (x, y, layer)

	unsigned int userId;	///< The user defined id of the tile.
	int polyCount;			///< The number of polygons in the tile.
	int polyMapCount;
	int vertCount;			///< The number of vertices in the tile.
	int maxLinkCount;		///< The number of allocated links.

	int detailMeshCount;

	/// The number of unique vertices in the detail mesh. (In addition to the polygon vertices.)
	int detailVertCount;

	int detailTriCount;			///< The number of triangles in the detail mesh.
	int bvNodeCount;			///< The number of bounding volume nodes. (Zero if bounding volumes are disabled.)
	int offMeshConCount;		///< The number of off-mesh connections.
	int offMeshBase;			///< The index of the first polygon which is an off-mesh connection.
#if DT_NAVMESH_SET_VERSION >= 8
	int maxCellCount;			///< The number of allocated cells.
#endif

	float walkableHeight;		///< The height of the agents using the tile.
	float walkableRadius;		///< The radius of the agents using the tile.
	float walkableClimb;		///< The maximum climb height of the agents using the tile.
	float bmin[3];				///< The minimum bounds of the tile's AABB. [(x, y, z)]
	float bmax[3];				///< The maximum bounds of the tile's AABB. [(x, y, z)]

	/// The bounding volume quantization factor. 
	float bvQuantFactor;
};

/// Navigation mesh tile memory tracker base class.
/// @ingroup detour
class dtMeshTileMemoryTracker
{
public:
	virtual void destroy(const int flags) = 0;
};

/// Defines a navigation mesh tile.
/// @ingroup detour
struct dtMeshTile
{
public:
	unsigned int allocLink();
	void freeLink(unsigned int link);

	bool linkCountAvailable(const int count) const;

	void getTightBounds(float* bminOut, float* bmaxOut) const;

	unsigned int salt;					///Counter describing modifications to the tile.

	unsigned int linksFreeList;			///Index to the next free link.
	dtMeshHeader* header;				///The tile header.
	dtPoly* polys;						///The tile polygons. [Size: dtMeshHeader::polyCount]
	unsigned int* polyMap;				///TODO: needs to be reversed.
	float* verts;						///The tile vertices. [Size: dtMeshHeader::vertCount]
	dtLink* links;						///The tile links. [Size: dtMeshHeader::maxLinkCount]
	dtPolyDetail* detailMeshes;			///The tile's detail sub-meshes. [Size: dtMeshHeader::detailMeshCount]

	/// The detail mesh's unique vertices. [(x, y, z) * dtMeshHeader::detailVertCount]
	float* detailVerts;

	/// The detail mesh's triangles. [(vertA, vertB, vertC, triFlags) * dtMeshHeader::detailTriCount].
	/// See dtDetailTriEdgeFlags and dtGetDetailTriEdgeFlags.
	unsigned char* detailTris;

	/// The tile bounding volume nodes. [Size: dtMeshHeader::bvNodeCount]
	/// (Will be null if bounding volumes are disabled.)
	dtBVNode* bvTree;

	dtOffMeshConnection* offMeshCons;		///< The tile off-mesh connections. [Size: dtMeshHeader::offMeshConCount]
	dtCell* cells;							///< The tile cells. [Size: dtMeshHeader::tileCellCount]

	unsigned char* data;					///< The tile data. (Not directly accessed under normal situations.)

	int dataSize;							///< Size of the tile data.
	int flags;								///< Tile flags. (See: #dtTileFlags)
	dtMeshTile* next;						///< The next free tile, or the next tile in the spatial grid.
	dtMeshTileMemoryTracker* tracker;		///< The tiles memory tracker, called after destruction. (See [r5apex_ds + F437D9] for usage.)
private:
	dtMeshTile(const dtMeshTile&);
	dtMeshTile& operator=(const dtMeshTile&);
};

/// Configuration parameters used to create traverse links between polygon edges.
/// @ingroup detour
struct dtTraverseLinkConnectParams
{
	/// User defined callback that returns the desired traverse type based on
	/// the provided spatial and logical characteristics of this potential link.
	///  @param[in]		userData		Pointer to user defined data.
	///  @param[in]		traverseDist	The total distance in length of the traverse link. [Unit: wu]
	///  @param[in]		elevation		The elevation difference between base and land position. [Unit: wu]
	///  @param[in]		slopeAngle		The slope angle from base to land position. [Unit: Degrees]
	///  @param[in]		baseOverlaps	Whether the projection of the base edge overlaps with the land edge.
	///  @param[in]		landOverlaps	Whether the projection of the land edge overlaps with the base edge.
	/// @return The desired traverse type for provided spatial and logical characteristics.
	unsigned char(*getTraverseType)(void* userData, const float traverseDist, const float elevation,
		const float slopeAngle, const bool baseOverlaps, const bool landOverlaps);

	/// User defined callback that returns whether a traverse link based on
	/// provided spatial characteristics is clear in terms of line-of-sight.
	///  @param[in]		userData		Pointer to user defined data.
	///  @param[in]		lowerEdgeMid	The mid point of the lower edge from which the link starts. [(x, y, z)] [Unit: wu]
	///  @param[in]		higherEdgeMid	The mid point of the higher edge to which the link ends. [(x, y, z)] [Unit: wu]
	///  @param[in]		lowerEdgeNorm	The edge normal of the lower edge. [(x, y, z)] [Unit: wu]
	///  @param[in]		higherEdgeNorm	The edge normal of the higher edge. [(x, y, z)] [Unit: wu]
	///  @param[in]		walkableHeight	The walkable height defined by the tile hosting the link. [Unit: wu]
	///  @param[in]		walkableRadius	The walkable radius defined by the tile hosting the link. [Unit: wu]
	///  @param[in]		slopeAngle		The slope angle from lower to higher edge mid points. [Unit: Degrees]
	/// @return True if the link between the lower and higher edge mid points don't collide with anything.
	bool(*traverseLinkInLOS)(void* userData, const float* lowerEdgeMid, const float* higherEdgeMid, const float* lowerEdgeNorm,
		const float* higherEdgeNorm, const float walkableHeight, const float walkableRadius, const float slopeAngle);

	/// User defined callback that looks if a link between these 2 polygons
	/// have already been established. A traverse type can only be used once
	/// between 2 polygons, but the 2 polygons can have more than one link.
	///  @param[in]		userData		Pointer to user defined data.
	///  @param[in]		basePolyRef		The reference of the polygon on the base tile.
	///  @param[in]		landPolyRef		The reference of the polygon on the land tile.
	/// @return Pointer to the bit cell, null if no link was found.
	unsigned int*(*findPolyLink)(void* userData, const dtPolyRef basePolyRef, const dtPolyRef landPolyRef);

	/// User defined callback that adds a new polygon pair to the list. On
	/// subsequent lookups, the bit cell of this pair should be returned and
	/// used instead, see #findPolyLink.
	///  @param[in]		userData		Pointer to user defined data.
	///  @param[in]		basePolyRef		The reference of the polygon on the base tile.
	///  @param[in]		landPolyRef		The reference of the polygon on the land tile.
	///  @param[in]		traverseTypeBit	The traverse type bit index to be stored initially in the bit cell.
	/// @return -1 if out-of-memory, 1 if link was already present, 0 on success.
	int(*addPolyLink)(void* userData, const dtPolyRef basePolyRef, const dtPolyRef landPolyRef, const unsigned int traverseTypeBit);

	void* userData;					///< The user defined data that will be provided to all callbacks, for example: your editor's class instance.
	float minEdgeOverlap;			///< The minimum amount of projection overlap required between the 2 edges before they are considered overlapping. [Unit: wu]
	float maxPortalAlign;			///< The maximum amount of portal alignment the system will apply. [Limit: 0 >= align <= 0.5]
	bool singlePortalPerPair;		///< Whether to limit the number of portal connections between 2 polygon pairs to 1.
	bool linkToNeighbor;			///< Whether to link to polygons in neighboring tiles. Limits linkage to internal polygons if false.
};

/// Configuration parameters used to define multi-tile navigation meshes.
/// The values are used to allocate space during the initialization of a navigation mesh.
/// @see dtNavMesh::init()
/// @ingroup detour
struct dtNavMeshParams
{
	float orig[3];					///< The world space origin of the navigation mesh's tile space. [(x, y, z)]
	float tileWidth;				///< The width of each tile. (Along the x-axis.)
	float tileHeight;				///< The height of each tile. (Along the y-axis.)
	int maxTiles;					///< The maximum number of tiles the navigation mesh can contain. This and maxPolys are used to calculate how many bits are needed to identify tiles and polygons uniquely.
	int maxPolys;					///< The maximum number of polygons each tile can contain. This and maxTiles are used to calculate how many bits are needed to identify tiles and polygons uniquely.
	int polyGroupCount;				///< The total number of disjoint polygon groups.
	int traverseTableSize;			///< The total size of the static traverse table. This is computed using calcTraverseTableSize(polyGroupcount).
	int traverseTableCount;			///< The total number of traverse tables in this navmesh. Each TraverseAnimType uses its own table as their available jump links should match their behavior and abilities.
#if DT_NAVMESH_SET_VERSION >= 7
	int hintCount;					///< The total number of hints in the navmesh.
#endif
};

/// A navigation mesh based on tiles of convex polygons.
/// @ingroup detour
class dtNavMesh
{
public:
	dtNavMesh();
	~dtNavMesh();

	/// @{
	/// @name Initialization and Tile Management

	/// Initializes the navigation mesh for tiled use.
	///  @param[in]	params		Initialization parameters.
	/// @return The status flags for the operation.
	dtStatus init(const dtNavMeshParams* params);

	/// Initializes the navigation mesh for single tile use.
	///  @param[in]	data		Data of the new tile. (See: #dtCreateNavMeshData)
	///  @param[in]	dataSize	The data size of the new tile.
	///  @param[in]	tableCount	The number of traverse tables this navmesh will use.
	///  @param[in]	flags		The tile flags. (See: #dtTileFlags)
	/// @return The status flags for the operation.
	///  @see dtCreateNavMeshData
	dtStatus init(unsigned char* data, const int dataSize, const int tableCount, const int flags);

	/// The navigation mesh initialization params.
	/// @note The parameters are created automatically when the single tile
	/// initialization is performed.
	const dtNavMeshParams* getParams() const { return &m_params; }

	/// Adds a tile to the navigation mesh.
	///  @param[in]		data		Data for the new tile mesh. (See: #dtCreateNavMeshData)
	///  @param[in]		dataSize	Data size of the new tile mesh.
	///  @param[in]		flags		Tile flags. (See: #dtTileFlags)
	///  @param[in]		lastRef		The desired reference for the tile. (When reloading a tile.) [opt] [Default: 0]
	///  @param[out]	result		The tile reference. (If the tile was successfully added.) [opt]
	/// @return The status flags for the operation.
	dtStatus addTile(unsigned char* data, int dataSize, int flags, dtTileRef lastRef, dtTileRef* result);

	/// Removes the specified tile from the navigation mesh.
	///  @param[in]		ref			The reference of the tile to remove.
	///  @param[out]	data		Data associated with deleted tile.
	///  @param[out]	dataSize	Size of the data associated with deleted tile.
	/// @return The status flags for the operation.
	dtStatus removeTile(dtTileRef ref, unsigned char** data, int* dataSize);

	/// Connects the specified tile to the navigation mesh.
	///  @param[in]		ref			The reference of the tile to connect.
	/// @return The status flags for the operation.
	dtStatus connectTile(const dtTileRef tileRef);

	/// @}

	/// @{
	/// @name Query Functions

	/// Calculates the tile grid location for the specified world position.
	///  @param[in]	pos  The world position for the query. [(x, y, z)]
	///  @param[out]	tx		The tile's x-location. (x, y)
	///  @param[out]	ty		The tile's y-location. (x, y)
	void calcTileLoc(const float* pos, int* tx, int* ty) const;

	/// Gets the tile at the specified grid location.
	///  @param[in]	x		The tile's x-location. (x, y, layer)
	///  @param[in]	y		The tile's y-location. (x, y, layer)
	///  @param[in]	layer	The tile's layer. (x, y, layer)
	/// @return The tile, or null if the tile does not exist.
	const dtMeshTile* getTileAt(const int x, const int y, const int layer) const;

	/// Gets all tiles at the specified grid location. (All layers.)
	///  @param[in]		x			The tile's x-location. (x, y)
	///  @param[in]		y			The tile's y-location. (x, y)
	///  @param[out]	tiles		A pointer to an array of tiles that will hold the result.
	///  @param[in]		maxTiles	The maximum tiles the tiles parameter can hold.
	/// @return The number of tiles returned in the tiles array.
	int getTilesAt(const int x, const int y,
		dtMeshTile const** tiles, const int maxTiles) const;

	/// Gets the tile reference for the tile at specified grid location.
	///  @param[in]	x		The tile's x-location. (x, y, layer)
	///  @param[in]	y		The tile's y-location. (x, y, layer)
	///  @param[in]	layer	The tile's layer. (x, y, layer)
	/// @return The tile reference of the tile, or 0 if there is none.
	dtTileRef getTileRefAt(int x, int y, int layer) const;

	/// Gets the tile reference for the specified tile.
	///  @param[in]	tile	The tile.
	/// @return The tile reference of the tile.
	dtTileRef getTileRef(const dtMeshTile* tile) const;

	/// Gets the tile for the specified tile reference.
	///  @param[in]	ref		The tile reference of the tile to retrieve.
	/// @return The tile for the specified reference, or null if the 
	///		reference is invalid.
	const dtMeshTile* getTileByRef(dtTileRef ref) const;

	/// The maximum number of tiles supported by the navigation mesh.
	/// @return The maximum number of tiles supported by the navigation mesh.
	int getMaxTiles() const;

	/// The number of tiles added to this mesh by #addTile.
	/// @return The number of tiles added to this mesh by #addTile.
	int getTileCount() const { return m_tileCount; };

	/// Gets the tile at the specified index.
	///  @param[in]	i		The tile index. [Limit: 0 >= index < #getMaxTiles()]
	/// @return The tile at the specified index.
	const dtMeshTile* getTile(int i) const;

	/// Gets the tile and polygon for the specified polygon reference.
	///  @param[in]		ref		The reference for the a polygon.
	///  @param[out]	tile	The tile containing the polygon.
	///  @param[out]	poly	The polygon.
	/// @return The status flags for the operation.
	dtStatus getTileAndPolyByRef(const dtPolyRef ref, const dtMeshTile** tile, const dtPoly** poly) const;

	/// Returns the tile and polygon for the specified polygon reference.
	///  @param[in]		ref		A known valid reference for a polygon.
	///  @param[out]	tile	The tile containing the polygon.
	///  @param[out]	poly	The polygon.
	void getTileAndPolyByRefUnsafe(const dtPolyRef ref, const dtMeshTile** tile, const dtPoly** poly) const;

	/// Returns whether both polygons are adjacent by a shared edge.
	///  @param[in]		baseRef		The reference to the first poly.
	///  @param[in]		landRef		The reference to the second poly.
	/// @return True if both polygons are adjacent by a shared edge.
	bool arePolysAdjacent(const dtPolyRef baseRef, const dtPolyRef landRef) const;

	/// Returns whether both polygons are adjacent by a shared edge.
	///  @param[in]		basePoly	The first poly.
	///  @param[in]		baseTile	The first tile.
	///  @param[in]		landPoly	The second poly.
	///  @param[in]		landTile	The second tile.
	/// @return True if both polygons are adjacent by a shared edge.
	bool arePolysAdjacent(const dtPoly* const basePoly, const dtMeshTile* const baseTile,
		const dtPoly* const landPoly, const dtMeshTile* const landTile) const;

	/// Returns whether goal poly is reachable from start poly.
	///  @param[in]		fromRef		The reference to the start poly.
	///  @param[in]		goalRef		The reference to the goal poly.
	///  @param[in]		checkDisjointGroupsOnly	Whether to only check disjoint poly groups.
	///  @param[in]		traverseTableIndex		Traverse table to use for checking if islands are linked together.
	/// @return True if goal polygon is reachable from start polygon.
	bool isGoalPolyReachable(const dtPolyRef fromRef, const dtPolyRef goalRef,
		const bool checkDisjointGroupsOnly, const int traverseTableIndex) const;

	/// Checks the validity of a polygon reference.
	///  @param[in]	ref		The polygon reference to check.
	/// @return True if polygon reference is valid for the navigation mesh.
	bool isValidPolyRef(dtPolyRef ref) const;

	/// Gets the polygon reference for the tile's base polygon.
	///  @param[in]	tile		The tile.
	/// @return The polygon reference for the base polygon in the specified tile.
	dtPolyRef getPolyRefBase(const dtMeshTile* tile) const;

	/// Gets the endpoints for an off-mesh connection, ordered by "direction of travel".
	///  @param[in]		prevRef		The reference of the polygon before the connection.
	///  @param[in]		polyRef		The reference of the off-mesh connection polygon.
	///  @param[out]	startPos	The start position of the off-mesh connection. [(x, y, z)]
	///  @param[out]	endPos		The end position of the off-mesh connection. [(x, y, z)]
	/// @return The status flags for the operation.
	dtStatus getOffMeshConnectionPolyEndPoints(dtPolyRef prevRef, dtPolyRef polyRef, float* startPos, float* endPos) const;

	/// Gets the specified off-mesh connection.
	///  @param[in]	ref		The polygon reference of the off-mesh connection.
	/// @return The specified off-mesh connection, or null if the polygon reference is not valid.
	const dtOffMeshConnection* getOffMeshConnectionByRef(dtPolyRef ref) const;

	bool allocTraverseTables(const int count);
	void freeTraverseTables();

	/// The navigation mesh traverse tables.
	int** getTraverseTables() const { return m_traverseTables; }
	
	/// Sets the traverse table slot.
	///  @param[in]	index	The index of the traverse table.
	///  @param[in]	table	The traverse table data.
	void setTraverseTable(const int index, int* const table);

	/// Sets the number of the traverse tables.
	///  @param[in]	count	The number of the traverse tables.
	void setTraverseTableCount(const int count) { m_params.traverseTableCount = count; }

	/// Sets the size of the traverse table.
	///  @param[in]	size	The size of the traverse table.
	void setTraverseTableSize(const int size) { m_params.traverseTableSize = size; }

#if DT_NAVMESH_SET_VERSION >= 7
	void freeHints();
#endif

	/// @}

	/// @{
	/// @name State Management
	/// These functions do not effect #dtTileRef or #dtPolyRef's. 

	/// Sets the user defined flags for the specified polygon.
	///  @param[in]	ref		The polygon reference.
	///  @param[in]	flags	The new flags for the polygon.
	/// @return The status flags for the operation.
	dtStatus setPolyFlags(dtPolyRef ref, unsigned short flags);

	/// Gets the user defined flags for the specified polygon.
	///  @param[in]		ref				The polygon reference.
	///  @param[out]	resultFlags		The polygon flags.
	/// @return The status flags for the operation.
	dtStatus getPolyFlags(dtPolyRef ref, unsigned short* resultFlags) const;

	/// Sets the user defined area for the specified polygon.
	///  @param[in]	ref		The polygon reference.
	///  @param[in]	area	The new area id for the polygon. [Limit: < #DT_MAX_AREAS]
	/// @return The status flags for the operation.
	dtStatus setPolyArea(dtPolyRef ref, unsigned char area);

	/// Gets the user defined area for the specified polygon.
	///  @param[in]		ref			The polygon reference.
	///  @param[out]	resultArea	The area id for the polygon.
	/// @return The status flags for the operation.
	dtStatus getPolyArea(dtPolyRef ref, unsigned char* resultArea) const;

	/// Gets the polygon group count.
	/// @return The total number of polygon groups.
	int getPolyGroupCount() const { return m_params.polyGroupCount; }

	/// Sets the polygon group count.
	///  @param[in]		count		The polygon group count.
	void setPolyGroupCount(const int count) { m_params.polyGroupCount = count; }

	/// Gets the size of the buffer required by #storeTileState to store the specified tile's state.
	///  @param[in]	tile	The tile.
	/// @return The size of the buffer required to store the state.
	int getTileStateSize(const dtMeshTile* tile) const;

	/// Stores the non-structural state of the tile in the specified buffer. (Flags, area ids, etc.)
	///  @param[in]		tile			The tile.
	///  @param[out]	data			The buffer to store the tile's state in.
	///  @param[in]		maxDataSize		The size of the data buffer. [Limit: >= #getTileStateSize]
	/// @return The status flags for the operation.
	dtStatus storeTileState(const dtMeshTile* tile, unsigned char* data, const int maxDataSize) const;

	/// Restores the state of the tile.
	///  @param[in]	tile			The tile.
	///  @param[in]	data			The new state. (Obtained from #storeTileState.)
	///  @param[in]	maxDataSize		The size of the state within the data buffer.
	/// @return The status flags for the operation.
	dtStatus restoreTileState(dtMeshTile* tile, const unsigned char* data, const int maxDataSize);

	/// @}

	/// @{
	/// @name Encoding and Decoding
	/// These functions are generally meant for internal use only.

	/// Derives a standard polygon reference.
	///  @note This function is generally meant for internal use only.
	///  @param[in]	salt	The tile's salt value.
	///  @param[in]	it		The index of the tile.
	///  @param[in]	ip		The index of the polygon within the tile.
	inline dtPolyRef encodePolyId(unsigned int salt, unsigned int it, unsigned int ip) const
	{
#ifdef DT_POLYREF64
		return ((dtPolyRef)salt << (DT_POLY_BITS+DT_TILE_BITS)) | ((dtPolyRef)it << DT_POLY_BITS) | (dtPolyRef)ip;
#else
		return ((dtPolyRef)salt << (m_polyBits+m_tileBits)) | ((dtPolyRef)it << m_polyBits) | (dtPolyRef)ip;
#endif
	}

	/// Decodes a standard polygon reference.
	///  @note This function is generally meant for internal use only.
	///  @param[in]	ref   The polygon reference to decode.
	///  @param[out]	salt	The tile's salt value.
	///  @param[out]	it		The index of the tile.
	///  @param[out]	ip		The index of the polygon within the tile.
	///  @see #encodePolyId
	inline void decodePolyId(dtPolyRef ref, unsigned int& salt, unsigned int& it, unsigned int& ip) const
	{
#ifdef DT_POLYREF64
		const dtPolyRef saltMask = ((dtPolyRef)1<<DT_SALT_BITS)-1;
		const dtPolyRef tileMask = ((dtPolyRef)1<<DT_TILE_BITS)-1;
		const dtPolyRef polyMask = ((dtPolyRef)1<<DT_POLY_BITS)-1;
		salt = (unsigned int)((ref >> (DT_POLY_BITS+DT_TILE_BITS)) & saltMask);
		it = (unsigned int)((ref >> DT_POLY_BITS) & tileMask);
		ip = (unsigned int)(ref & polyMask);
#else
		const dtPolyRef saltMask = ((dtPolyRef)1<<m_saltBits)-1;
		const dtPolyRef tileMask = ((dtPolyRef)1<<m_tileBits)-1;
		const dtPolyRef polyMask = ((dtPolyRef)1<<m_polyBits)-1;
		salt = (unsigned int)((ref >> (m_polyBits+m_tileBits)) & saltMask);
		it = (unsigned int)((ref >> m_polyBits) & tileMask);
		ip = (unsigned int)(ref & polyMask);
#endif
	}

	/// Extracts a tile's salt value from the specified polygon reference.
	///  @note This function is generally meant for internal use only.
	///  @param[in]	ref		The polygon reference.
	///  @see #encodePolyId
	inline unsigned int decodePolyIdSalt(dtPolyRef ref) const
	{
#ifdef DT_POLYREF64
		const dtPolyRef saltMask = ((dtPolyRef)1<<DT_SALT_BITS)-1;
		return (unsigned int)((ref >> (DT_POLY_BITS+DT_TILE_BITS)) & saltMask);
#else
		const dtPolyRef saltMask = ((dtPolyRef)1<<m_saltBits)-1;
		return (unsigned int)((ref >> (m_polyBits+m_tileBits)) & saltMask);
#endif
	}

	/// Extracts the tile's index from the specified polygon reference.
	///  @note This function is generally meant for internal use only.
	///  @param[in]	ref		The polygon reference.
	///  @see #encodePolyId
	inline unsigned int decodePolyIdTile(dtPolyRef ref) const
	{
#ifdef DT_POLYREF64
		const dtPolyRef tileMask = ((dtPolyRef)1<<DT_TILE_BITS)-1;
		return (unsigned int)((ref >> DT_POLY_BITS) & tileMask);
#else
		const dtPolyRef tileMask = ((dtPolyRef)1<<m_tileBits)-1;
		return (unsigned int)((ref >> m_polyBits) & tileMask);
#endif
	}

	/// Extracts the polygon's index (within its tile) from the specified polygon reference.
	///  @note This function is generally meant for internal use only.
	///  @param[in]	ref		The polygon reference.
	///  @see #encodePolyId
	inline unsigned int decodePolyIdPoly(dtPolyRef ref) const
	{
#ifdef DT_POLYREF64
		const dtPolyRef polyMask = ((dtPolyRef)1<<DT_POLY_BITS)-1;
		return (unsigned int)(ref & polyMask);
#else
		const dtPolyRef polyMask = ((dtPolyRef)1<<m_polyBits)-1;
		return (unsigned int)(ref & polyMask);
#endif
	}

	/// @}
	/// Returns pointer to tile in the tile array.
	dtMeshTile* getTile(int i);
private:
	// Explicitly disabled copy constructor and copy assignment operator.
	dtNavMesh(const dtNavMesh&);
	dtNavMesh& operator=(const dtNavMesh&);


public:
	/// Returns tile based on position.
	int getTilesAt(const int x, const int y,
		dtMeshTile** tiles, const int maxTiles) const;

	/// Returns neighbour tile based on side.
	int getNeighbourTilesAt(const int x, const int y, const int side,
		dtMeshTile** tiles, const int maxTiles) const;

	/// Builds external polygon links for a tile.
	dtStatus connectTraverseLinks(const dtTileRef tileRef, const dtTraverseLinkConnectParams& params);
	/// Builds external polygon links for a tile.
	dtStatus connectOffMeshLinks(const dtTileRef tileRef);

	dtPolyRef clampOffMeshVertToPoly(dtOffMeshConnection* con, dtMeshTile* conTile, const dtMeshTile* lookupTile, const bool start);

private:
	/// Returns all polygons in neighbour tile based on portal defined by the segment.
	int findConnectingPolys(const float* va, const float* vb,
		const dtMeshTile* tile, int side,
		dtPolyRef* con, float* conarea, int maxcon) const;

	/// Builds internal polygons links for a tile.
	void connectIntLinks(dtMeshTile* tile);

	/// Builds external polygon links for a tile.
	void connectExtLinks(dtMeshTile* tile, dtMeshTile* target, int side);

	/// Removes external links at specified side.
	void unconnectLinks(dtMeshTile* tile, dtMeshTile* target);


	// TODO: These methods are duplicates from dtNavMeshQuery, but are needed for off-mesh connection finding.

	/// Queries polygons within a tile.
	int queryPolygonsInTile(const dtMeshTile* tile, const float* qmin, const float* qmax,
		dtPolyRef* polys, const int maxPolys) const;
	/// Find nearest polygon within a tile.
	dtPolyRef findNearestPolyInTile(const dtMeshTile* tile, const float* center,
		const float* halfExtents, float* nearestPt) const;
	/// Returns whether position is over the poly and the height at the position if so.
	bool getPolyHeight(const dtMeshTile* tile, const dtPoly* poly, const float* pos, float* height) const;
	/// Returns closest point on polygon.
	void closestPointOnPoly(dtPolyRef ref, const float* pos, float* closest, bool* posOverPoly) const;

	dtMeshTile** m_posLookup;			///< Tile hash lookup.
	dtMeshTile* m_nextFree;				///< Freelist of tiles.
	dtMeshTile* m_tiles;				///< List of tiles.
	int** m_traverseTables;				///< Array of traverse tables.
	dtHint* m_hints;					///< List of hints.

	int m_unused0;
	int m_unused1;

	dtNavMeshParams m_params;			///< Current initialization params. TODO: do not store this info twice.
	float m_orig[3];					///< Origin of the tile (0,0)
	float m_tileWidth, m_tileHeight;	///< Dimensions of each tile.
	int m_tileCount;					///< Number of tiles in the mesh.
	int m_maxTiles;						///< Max number of tiles.
	int m_tileLutSize;					///< Tile hash lookup size (must be pot).
	int m_tileLutMask;					///< Tile hash lookup mask.

#ifndef DT_POLYREF64
	unsigned int m_saltBits;			///< Number of salt bits in the tile ID.
	unsigned int m_tileBits;			///< Number of tile bits in the tile ID.
	unsigned int m_polyBits;			///< Number of poly bits in the tile ID.
#endif
	friend class dtNavMeshQuery;
};

/// Returns the cell index for the static traverse table.
///  @param[in]	numPolyGroups	The total number of poly groups.
///  @param[in]	polyGroup1		The poly group ID of the first island.
///  @param[in]	polyGroup2		The poly group ID of the second island.
///  @return The cell index for the static traverse table.
///  @ingroup detour
int dtCalcTraverseTableCellIndex(const int numPolyGroups,
	const unsigned short polyGroup1, const unsigned short polyGroup2);

/// Returns the total size needed for the static traverse table.
///  @param[in]	numPolyGroups	The total number of poly groups.
///  @return the total size needed for the static traverse table.
///  @ingroup detour
int dtCalcTraverseTableSize(const int numPolyGroups);

/// Defines a navigation mesh tile data block.
/// @ingroup detour
struct dtNavMeshTileHeader
{
	dtTileRef tileRef;					///< The tile reference for this tile.
	int dataSize;						///< The total size of this tile.
};

/// Defines a navigation mesh set data block.
/// @ingroup detour
struct dtNavMeshSetHeader
{
	int magic;							///< Set magic number. (Used to identify the data format.)
	int version;						///< Set data format version number.
	int numTiles;						///< The total number of tiles in this set.
	dtNavMeshParams params;				///< The initialization parameters for this set.
};

/// Allocates a navigation mesh object using the Detour allocator.
/// @return A navigation mesh that is ready for initialization, or null on failure.
///  @ingroup detour
dtNavMesh* dtAllocNavMesh();

/// Frees the specified navigation mesh object using the Detour allocator.
///  @param[in]	navmesh		A navigation mesh allocated using #dtAllocNavMesh
///  @ingroup detour
void dtFreeNavMesh(dtNavMesh* navmesh);

#endif // DETOURNAVMESH_H

///////////////////////////////////////////////////////////////////////////

// This section contains detailed documentation for members that don't have
// a source file. It reduces clutter in the main section of the header.

/**

@typedef dtPolyRef
@par

Polygon references are subject to the same invalidate/preserve/restore
rules that apply to #dtTileRef's.  If the #dtTileRef for the polygon's
tile changes, the polygon reference becomes invalid.

Changing a polygon's flags, area id, etc. does not impact its polygon
reference.

@typedef dtTileRef
@par

The following changes will invalidate a tile reference:

- The referenced tile has been removed from the navigation mesh.
- The navigation mesh has been initialized using a different set
  of #dtNavMeshParams.

A tile reference is preserved/restored if the tile is added to a navigation
mesh initialized with the original #dtNavMeshParams and is added at the
original reference location. (E.g. The lastRef parameter is used with
dtNavMesh::addTile.)

Basically, if the storage structure of a tile changes, its associated
tile reference changes.


@var unsigned short dtPoly::neis[DT_VERTS_PER_POLYGON]
@par

Each entry represents data for the edge starting at the vertex of the same index.
E.g. The entry at index n represents the edge data for vertex[n] to vertex[n+1].

A value of zero indicates the edge has no polygon connection. (It makes up the
border of the navigation mesh.)

The information can be extracted as follows:
@code
neighborRef = neis[n] & 0xff; // Get the neighbor polygon reference.

if (neis[n] & #DT_EX_LINK)
{
	// The edge is an external (portal) edge.
}
@endcode

@var float dtMeshHeader::bvQuantFactor
@par

This value is used for converting between world and bounding volume coordinates.
For example:
@code
const float cs = 1.0f / tile->header->bvQuantFactor;
const dtBVNode* n = &tile->bvTree[i];
if (n->i >= 0)
{
	// This is a leaf node.
	float worldMinX = tile->header->bmin[0] + n->bmin[0]*cs;
	float worldMinY = tile->header->bmin[0] + n->bmin[1]*cs;
	// Etc...
}
@endcode

@struct dtMeshTile
@par

Tiles generally only exist within the context of a dtNavMesh object.

Some tile content is optional.  For example, a tile may not contain any
off-mesh connections.  In this case the associated pointer will be null.

If a detail mesh exists it will share vertices with the base polygon mesh.
Only the vertices unique to the detail mesh will be stored in #detailVerts.

@warning Tiles returned by a dtNavMesh object are not guaranteed to be populated.
For example: The tile at a location might not have been loaded yet, or may have been removed.
In this case, pointers will be null.  So if in doubt, check the polygon count in the
tile's header to determine if a tile has polygons defined.

@var float dtOffMeshConnection::pos[6]
@par

For a properly built navigation mesh, vertex A will always be within the bounds of the mesh.
Vertex B is not required to be within the bounds of the mesh.

*/
