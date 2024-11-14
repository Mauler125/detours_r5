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

#include "Shared/Include/SharedMath.h"
#include "Shared/Include/SharedCommon.h"
#include "Shared/Include/SharedAlloc.h"
#include "Shared/Include/SharedAssert.h"
#include "Detour/Include/DetourNavMesh.h"
#include "Detour/Include/DetourNavMeshBuilder.h"


struct BVItem
{
	unsigned short bmin[3];
	unsigned short bmax[3];
	int i;
};

static int compareItemX(const void* va, const void* vb)
{
	const BVItem* a = (const BVItem*)va;
	const BVItem* b = (const BVItem*)vb;
	if (a->bmin[0] < b->bmin[0])
		return -1;
	if (a->bmin[0] > b->bmin[0])
		return 1;
	return 0;
}

static int compareItemY(const void* va, const void* vb)
{
	const BVItem* a = (const BVItem*)va;
	const BVItem* b = (const BVItem*)vb;
	if (a->bmin[1] < b->bmin[1])
		return -1;
	if (a->bmin[1] > b->bmin[1])
		return 1;
	return 0;
}

static int compareItemZ(const void* va, const void* vb)
{
	const BVItem* a = (const BVItem*)va;
	const BVItem* b = (const BVItem*)vb;
	if (a->bmin[2] < b->bmin[2])
		return -1;
	if (a->bmin[2] > b->bmin[2])
		return 1;
	return 0;
}

static void calcExtends(BVItem* items, const int /*nitems*/, const int imin, const int imax,
						unsigned short* bmin, unsigned short* bmax)
{
	bmin[0] = items[imin].bmin[0];
	bmin[1] = items[imin].bmin[1];
	bmin[2] = items[imin].bmin[2];
	
	bmax[0] = items[imin].bmax[0];
	bmax[1] = items[imin].bmax[1];
	bmax[2] = items[imin].bmax[2];
	
	for (int i = imin+1; i < imax; ++i)
	{
		const BVItem& it = items[i];
		if (it.bmin[0] < bmin[0]) bmin[0] = it.bmin[0];
		if (it.bmin[1] < bmin[1]) bmin[1] = it.bmin[1];
		if (it.bmin[2] < bmin[2]) bmin[2] = it.bmin[2];
		
		if (it.bmax[0] > bmax[0]) bmax[0] = it.bmax[0];
		if (it.bmax[1] > bmax[1]) bmax[1] = it.bmax[1];
		if (it.bmax[2] > bmax[2]) bmax[2] = it.bmax[2];
	}
}

inline int longestAxis(unsigned short x, unsigned short y, unsigned short z)
{
	int	axis = 0;
	unsigned short maxVal = x;
	if (y > maxVal)
	{
		axis = 1;
		maxVal = y;
	}
	if (z > maxVal)
	{
		axis = 2;
	}
	return axis;
}

static void subdivide(BVItem* items, int nitems, int imin, int imax, rdTempVector<BVItem>& nodes)
{
	const int inum = imax - imin;
	const int icur = (int)nodes.size();

	nodes.resize(icur+1);
	BVItem& node = nodes[icur];
	
	if (inum == 1)
	{
		// Leaf
		node.bmin[0] = items[imin].bmin[0];
		node.bmin[1] = items[imin].bmin[1];
		node.bmin[2] = items[imin].bmin[2];
		
		node.bmax[0] = items[imin].bmax[0];
		node.bmax[1] = items[imin].bmax[1];
		node.bmax[2] = items[imin].bmax[2];
		
		node.i = items[imin].i;
	}
	else
	{
		// Split
		calcExtends(items, nitems, imin, imax, node.bmin, node.bmax);
		
		const int	axis = longestAxis(node.bmax[0] - node.bmin[0],
							   node.bmax[1] - node.bmin[1],
							   node.bmax[2] - node.bmin[2]);
		
		if (axis == 0)
		{
			// Sort along x-axis
			qsort(items+imin, inum, sizeof(BVItem), compareItemX);
		}
		else if (axis == 1)
		{
			// Sort along y-axis
			qsort(items+imin, inum, sizeof(BVItem), compareItemY);
		}
		else
		{
			// Sort along z-axis
			qsort(items+imin, inum, sizeof(BVItem), compareItemZ);
		}
		
		const int isplit = imin+inum/2;
		
		// Left
		subdivide(items, nitems, imin, isplit, nodes);
		// Right
		subdivide(items, nitems, isplit, imax, nodes);
		
		int iescape = (int)nodes.size() - icur;
		// Negative index means escape.
		node.i = -iescape;
	}
}

inline static void quantItem(BVItem& it, const float* tileBmin, const float* tileBmax, 
	const float* polyBmin, const float* polyBmax, const float quantFactor)
{
	it.bmin[0] = (unsigned short)rdClamp((int)rdMathFloorf((tileBmax[0] - polyBmax[0])*quantFactor), 0, 0xffff);
	it.bmin[1] = (unsigned short)rdClamp((int)rdMathFloorf((polyBmin[1] - tileBmin[1])*quantFactor), 0, 0xffff);
	it.bmin[2] = (unsigned short)rdClamp((int)rdMathFloorf((polyBmin[2] - tileBmin[2])*quantFactor), 0, 0xffff);

	it.bmax[0] = (unsigned short)rdClamp((int)rdMathCeilf((tileBmax[0] - polyBmin[0])*quantFactor), 0, 0xffff);
	it.bmax[1] = (unsigned short)rdClamp((int)rdMathCeilf((polyBmax[1] - tileBmin[1])*quantFactor), 0, 0xffff);
	it.bmax[2] = (unsigned short)rdClamp((int)rdMathCeilf((polyBmax[2] - tileBmin[2])*quantFactor), 0, 0xffff);
}

static bool createBVTree(const dtNavMeshCreateParams* params, rdTempVector<BVItem>& nodes)
{
	BVItem* items = (BVItem*)rdAlloc(sizeof(BVItem)*params->polyCount, RD_ALLOC_TEMP);

	if (!items)
		return false;

	// note(amos): reserve enough memory here to avoid reallocation during subdivisions.
	if (!nodes.reserve(params->polyCount*2))
		return false;

	// Build tree
	float quantFactor = 1 / params->cs;
	for (int i = 0; i < params->polyCount; i++)
	{
		BVItem& it = items[i];
		it.i = i;

		float polyVerts[RD_VERTS_PER_POLYGON*3];

		const float* targetVert;
		int vertCount;

		// Calc polygon bounds. Use detail meshes if available.
		if (params->detailMeshes)
		{
			const int vb = (int)params->detailMeshes[i*4+0];

			vertCount = (int)params->detailMeshes[i*4+1];
			targetVert = &params->detailVerts[vb*3];
		}
		else
		{
			const int nvp = params->nvp;
			const unsigned short* p = &params->polys[i*nvp * 2];

			vertCount = rdCountPolyVerts(p, nvp);

			for (int j = 0; j < vertCount; ++j)
			{
				const unsigned short* polyVert = &params->verts[p[j] * 3];
				float* flPolyVert = &polyVerts[j * 3];

				flPolyVert[0] = params->bmin[0]+polyVert[0]*params->cs;
				flPolyVert[1] = params->bmin[1]+polyVert[1]*params->cs;
				flPolyVert[2] = params->bmin[2]+polyVert[2]*params->ch;
			}

			targetVert = polyVerts;
		}

		float bmin[3];
		float bmax[3];

		rdVcopy(bmin, targetVert);
		rdVcopy(bmax, targetVert);

		for (int j = 1; j < vertCount; j++)
		{
			rdVmin(bmin, &targetVert[j * 3]);
			rdVmax(bmax, &targetVert[j * 3]);
		}

		// BV-tree uses cs for all dimensions
		quantItem(it, params->bmin, params->bmax, bmin, bmax, quantFactor);
	}

	subdivide(items, params->polyCount, 0, params->polyCount, nodes);
	rdFree(items);

	return true;
}

static bool rebuildBVTree(dtMeshTile* tile, const unsigned short* oldPolyIndices, const int polyCount, rdTempVector<BVItem>& nodes)
{
	BVItem* items = (BVItem*)rdAlloc(sizeof(BVItem)*polyCount, RD_ALLOC_TEMP);

	if (!items)
		return false;

	// note(amos): reserve enough memory here to avoid reallocation during subdivisions.
	if (!nodes.reserve(polyCount * 2))
		return false;

	const dtMeshHeader* header = tile->header;
	const float quantFactor = header->bvQuantFactor;

	for (int i = 0; i < polyCount; i++)
	{
		BVItem& it = items[i];
		it.i = i;

		const int oldPolyIndex = oldPolyIndices[i];
		const dtPoly& poly = tile->polys[oldPolyIndex];

		rdAssert(poly.getType() != DT_POLYTYPE_OFFMESH_CONNECTION);
		const dtPolyDetail& detail = tile->detailMeshes[oldPolyIndex];

		float bmin[3];
		float bmax[3];

		rdVset(bmin, FLT_MAX, FLT_MAX, FLT_MAX);
		rdVset(bmax, -FLT_MAX, -FLT_MAX, -FLT_MAX);

		for (int j = 0; j < detail.triCount; ++j)
		{
			const unsigned char* t = &tile->detailTris[(detail.triBase + j) * 4];
			float triVerts[3][3];

			for (int k = 0; k < 3; ++k)
			{
				if (t[k] < poly.vertCount)
					rdVcopy(triVerts[k], &tile->verts[poly.verts[t[k]] * 3]);
				else
					rdVcopy(triVerts[k], &tile->detailVerts[(detail.vertBase + t[k] - poly.vertCount) * 3]);

				rdVmin(bmin, triVerts[k]);
				rdVmax(bmax, triVerts[k]);
			}
		}

		// BV-tree uses cs for all dimensions
		quantItem(it, header->bmin, header->bmax, bmin, bmax, quantFactor);
	}

	subdivide(items, polyCount, 0, polyCount, nodes);
	rdFree(items);

	return true;
}

static void setPolyGroupsTraversalReachability(int* const tableData, const int numPolyGroups,
	const unsigned short polyGroup1, const unsigned short polyGroup2, const bool isReachable)
{
	const int index = dtCalcTraverseTableCellIndex(numPolyGroups, polyGroup1, polyGroup2);
	const int value = 1<<(polyGroup2 & 31);

	if (isReachable)
		tableData[index] |= value;
}

static void unionTraverseLinkedPolyGroups(const dtTraverseTableCreateParams* params, const int tableIndex)
{
	dtDisjointSet& set = params->sets[tableIndex];

	if (!set.getSetCount())
		return;

	dtNavMesh* nav = params->nav;
	const int maxTiles = nav->getMaxTiles();

	// Handle traverse linked poly's.
	for (int i = 0; i < maxTiles; ++i)
	{
		dtMeshTile* tile = nav->getTile(i);
		const dtMeshHeader* header = tile->header;

		if (!header)
			continue;

		const int pcount = header->polyCount;
		for (int j = 0; j < pcount; j++)
		{
			dtPoly& poly = tile->polys[j];

			for (unsigned int k = poly.firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
			{
				const dtLink* link = &tile->links[k];

				const dtMeshTile* landTile;
				const dtPoly* landPoly;

				nav->getTileAndPolyByRefUnsafe(link->ref, &landTile, &landPoly);
				rdAssert(landPoly->groupId != DT_UNLINKED_POLY_GROUP);

				if (poly.groupId != landPoly->groupId && params->canTraverse(params, link, tableIndex))
					set.setUnion(poly.groupId, landPoly->groupId);
			}
		}
	}
}

static bool floodPolygonIsland(dtNavMesh* nav, dtDisjointSet& set, const dtPolyRef startRef)
{
	std::set<dtPolyRef> visitedPolys;
	rdPermVector<dtPolyRef> openList;

	openList.push_back(startRef);

	while (!openList.empty())
	{
		dtPolyRef polyRef = openList.back();
		openList.pop_back();

		// Skip already visited polygons.
		if (visitedPolys.find(polyRef) != visitedPolys.end())
			continue;

		visitedPolys.insert(polyRef);
		unsigned int salt, it, ip;

		nav->decodePolyId(polyRef, salt, it, ip);

		dtMeshTile* currentTile = nav->getTile(it);
		dtPoly* poly = &currentTile->polys[ip];

		if (poly->groupId == DT_NULL_POLY_GROUP)
		{
			const int newGroup = set.insertNew();

			// Overflow, too many polygon islands.
			if (newGroup == -1)
				return false;

			poly->groupId = (unsigned short)newGroup;
		}

		for (unsigned int i = poly->firstLink; i != DT_NULL_LINK; i = currentTile->links[i].next)
		{
			const dtLink& link = currentTile->links[i];

			// Skip traverse links as these can join separate islands together.
			if (link.hasTraverseType())
				continue;

			const dtPolyRef neiRef = link.ref;

			if (visitedPolys.find(neiRef) == visitedPolys.end())
			{
				nav->decodePolyId(neiRef, salt, it, ip);

				dtMeshTile* neiTile = nav->getTile(it);
				dtPoly* neiPoly = &neiTile->polys[ip];

				if (neiPoly->groupId != DT_NULL_POLY_GROUP)
					continue;

				if (neiPoly->getType() != DT_POLYTYPE_OFFMESH_CONNECTION)
				{
					neiPoly->groupId = poly->groupId;
					openList.push_back(neiRef);
				}
			}
		}
	}

	return true;
}

static void copyBaseDisjointSets(const dtTraverseTableCreateParams* params)
{
	dtDisjointSet& set = params->sets[0];

	// Copy base disjoint set results to sets for each traverse table.
	for (int i = 0; i < params->tableCount; i++)
	{
		dtDisjointSet& targetSet = params->sets[i];

		if (i > 0) // Don't copy the base into itself.
			set.copy(targetSet);

		if (!params->collapseGroups)
			unionTraverseLinkedPolyGroups(params, i);
	}
}

bool dtCreateDisjointPolyGroups(const dtTraverseTableCreateParams* params)
{
	dtNavMesh* nav = params->nav;
	dtDisjointSet& set = params->sets[0];

	rdAssert(nav);

	// Reserve the first poly groups
	// 0 = DT_NULL_POLY_GROUP.
	// 1 = DT_UNLINKED_POLY_GROUP.
	set.init(DT_FIRST_USABLE_POLY_GROUP, DT_MAX_POLY_GROUP_COUNT);

	const int maxTiles = nav->getMaxTiles();

	// Clear all labels.
	for (int i = 0; i < maxTiles; ++i)
	{
		dtMeshTile* tile = nav->getTile(i);
		const dtMeshHeader* header = tile->header;
		if (!header)
			continue;

		const int pcount = header->polyCount;
		for (int j = 0; j < pcount; j++)
		{
			dtPoly& poly = tile->polys[j];

			poly.groupId = DT_NULL_POLY_GROUP;
#if DT_NAVMESH_SET_VERSION >= 7
			// NOTE: these fields are unknown and need to be reversed.
			// It is possible these are used internally only.
			poly.unk1 = (unsigned short)-1;
			poly.unk2 = (unsigned short)-1;
#endif
		}
	}

	// True if we have more than DT_MAX_POLY_GROUPS polygon islands.
	bool failure = false;

	// Mark polygon islands and unlinked polygons.
	for (int i = 0; i < maxTiles; ++i)
	{
		dtMeshTile* tile = nav->getTile(i);
		const dtMeshHeader* header = tile->header;

		if (!header)
			continue;

		const int pcount = header->polyCount;
		for (int j = 0; j < pcount; ++j)
		{
			dtPoly& poly = tile->polys[j];

			// Skip if the polygon is already part of an island.
			if (poly.groupId != DT_NULL_POLY_GROUP)
				continue;

			if (poly.firstLink == DT_NULL_LINK)
			{
				poly.groupId = DT_UNLINKED_POLY_GROUP;
				continue;
			}

			if (failure)
				continue;

			if (params->collapseGroups)
			{
				poly.groupId = DT_FIRST_USABLE_POLY_GROUP;
				continue;
			}

			// Off-mesh connections need their own ID's, skip the assignment
			// here since else we will be marking 2 (or more) poly islands
			// under the same group id.
			if (poly.getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
			{
				const int newId = set.insertNew();

				if (newId == -1)
				{
					failure = true;
					continue;
				}

				poly.groupId = (unsigned short)newId;
				continue;
			}

			const dtPolyRef polyRefBase = nav->getPolyRefBase(tile);

			if (!floodPolygonIsland(nav, set, polyRefBase | j))
				failure = true;
		}
	}

	return !failure;
}

bool dtCreateTraverseTableData(const dtTraverseTableCreateParams* params)
{
	dtNavMesh* nav = params->nav;

	nav->freeTraverseTables();
	const int tableCount = params->tableCount;

	if (!nav->allocTraverseTables(tableCount))
		return false;

	nav->setTraverseTableCount(tableCount);
	copyBaseDisjointSets(params);

	const dtDisjointSet& baseSet = params->sets[0];
	const int polyGroupCount = baseSet.getSetCount();

	const int tableSize = dtCalcTraverseTableSize(polyGroupCount);
	nav->setTraverseTableSize(tableSize);

	for (int i = 0; i < tableCount; i++)
	{
		const rdSizeType bufferSize = sizeof(int)*tableSize;
		int* const traverseTable = (int*)rdAlloc(bufferSize, RD_ALLOC_PERM);

		if (!traverseTable)
			return false;

		memset(traverseTable, 0, bufferSize);
		nav->setTraverseTable(i, traverseTable);

		const dtDisjointSet& set = params->sets[i];

		for (unsigned short j = 0; j < polyGroupCount; j++)
		{
			for (unsigned short k = 0; k < polyGroupCount; k++)
			{
				// Only reachable if its the same poly group or if they are linked!
				const bool isReachable = j == k || set.find(j) == set.find(k);
				setPolyGroupsTraversalReachability(traverseTable, polyGroupCount, j, k, isReachable);
			}
		}
	}

	nav->setPolyGroupCount(baseSet.getSetCount());
	return true;
}

#if DT_NAVMESH_SET_VERSION >= 8
struct CellItem
{
	float pos[3];
	int polyIndex;
};

static bool createPolyMeshCells(const dtNavMeshCreateParams* params, rdTempVector<CellItem>& cellItems)
{
	const int nvp = params->nvp;
	const int resolution = params->cellResolution;
	const float stepX = (params->bmax[0]-params->bmin[0]) / resolution;
	const float stepY = (params->bmax[1]-params->bmin[1]) / resolution;

	for (int i = 0; i < params->polyCount; ++i)
	{
		const unsigned short* p = &params->polys[i*2*nvp];
		const int nv = rdCountPolyVerts(p, nvp);

		if (nv < 3) // Don't generate cells for off-mesh connections.
			continue;

		const unsigned int vb = params->detailMeshes[i*4+0];
		const unsigned int tb = params->detailMeshes[i*4+2];
		const unsigned int tc = params->detailMeshes[i*4+3];

		float polyVerts[RD_VERTS_PER_POLYGON*3];

		for (int j = 0; j < nv; ++j)
		{
			const unsigned short* polyVert = &params->verts[p[j]*3];
			float* flPolyVert = &polyVerts[j*3];

			flPolyVert[0] = params->bmin[0]+polyVert[0]*params->cs;
			flPolyVert[1] = params->bmin[1]+polyVert[1]*params->cs;
			flPolyVert[2] = params->bmin[2]+polyVert[2]*params->ch;
		}

		for (int j = 0; j <= resolution; j++)
		{
			for (int k = 0; k <= resolution; k++)
			{
				const float offsetX = (k % 2 == 0) ? 0.0f : stepX / 2.0f;

				float targetCellPos[3];
				targetCellPos[0] = params->bmin[0]+j*stepX+offsetX;
				targetCellPos[1] = params->bmin[1]+k*stepY;
				targetCellPos[2] = params->bmax[2];

				bool heightPointSet = false;

				if (!rdPointInPolygon(targetCellPos, polyVerts, nv))
					continue;

				for (unsigned int l = 0; l < tc; ++l)
				{
					const unsigned char* tris = &params->detailTris[(tb+l)*4];
					float storage[3][3];
					const float* v[3];

					for (int m = 0; m < 3; ++m)
					{
						if (tris[m] < nv)
						{
							for (int n = 0; n < 3; ++n)
							{
								storage[m][n] = params->bmin[n] + params->verts[p[tris[m]]*3+n] * (n == 2 ? params->ch : params->cs);
							}
							v[m] = storage[m];
						}
						else
						{
							v[m] = &params->detailVerts[(vb+tris[m])*3];
						}
					}

					if (rdClosestHeightPointTriangle(targetCellPos, v[0], v[1], v[2], targetCellPos[2]))
					{
						heightPointSet = true;
						break;
					}
				}

				// NOTE: this is very rare, but does happen when for example, our target pos
				// is right on a poly edge; see call to closestPointOnDetailEdges in
				// dtNavMesh::getPolyHeight. This is largely based on the implementation of
				// closestPointOnDetailEdges.
				if (!heightPointSet)
				{
					bool onlyBoundary = false;

					float storage[3][3];
					const float* v[3];

					float dmin = FLT_MAX;
					float tmin = 0;
					const float* pmin = 0;
					const float* pmax = 0;

					for (unsigned int l = 0; l < tc; l++)
					{
						const unsigned char* tris = &params->detailTris[(tb+l)*4];

						const int ANY_BOUNDARY_EDGE =
							(RD_DETAIL_EDGE_BOUNDARY << 0) |
							(RD_DETAIL_EDGE_BOUNDARY << 2) |
							(RD_DETAIL_EDGE_BOUNDARY << 4);

						if (onlyBoundary && (tris[3] & ANY_BOUNDARY_EDGE) == 0)
							continue;

						for (int m = 0; m < 3; ++m)
						{
							if (tris[m] < nv)
							{
								for (int n = 0; n < 3; ++n)
								{
									storage[m][n] = params->bmin[n] + params->verts[p[tris[m]]*3+n] * (n == 2 ? params->ch : params->cs);
								}
								v[m] = storage[m];
							}
							else
							{
								v[m] = &params->detailVerts[(vb+tris[m])*3];
							}
						}

						for (int m = 0, n = 2; m < 3; n = m++)
						{
							if ((dtGetDetailTriEdgeFlags(tris[3], n) & RD_DETAIL_EDGE_BOUNDARY) == 0 &&
								(onlyBoundary || tris[n] < tris[m]))
							{
								// Only looking at boundary edges and this is internal, or
								// this is an inner edge that we will see again or have already seen.
								continue;
							}

							float t;
							float d = rdDistancePtSegSqr2D(targetCellPos, v[n], v[m], t);
							if (d < dmin)
							{
								dmin = d;
								tmin = t;
								pmin = v[n];
								pmax = v[m];
							}
						}
					}

					float closest[3];
					rdVlerp(closest, pmin, pmax, tmin);

					targetCellPos[2] = closest[2];
				}

				const rdSizeType newCount = cellItems.size()+1;

				if (!cellItems.reserve(newCount))
					return false;

				cellItems.resize(newCount);
				CellItem& cell = cellItems[newCount-1];

				rdVcopy(cell.pos, targetCellPos);
				cell.polyIndex = i;
			}
		}
	}

	return true;
}
#endif // DT_NAVMESH_SET_VERSION >= 8

// TODO: Better error handling.

/// @par
/// 
/// The output data array is allocated using the detour allocator (rdAlloc()).  The method
/// used to free the memory will be determined by how the tile is added to the navigation
/// mesh.
///
/// @see dtNavMesh, dtNavMesh::addTile()
bool dtCreateNavMeshData(dtNavMeshCreateParams* params, unsigned char** outData, int* outDataSize)
{
	if (params->nvp > RD_VERTS_PER_POLYGON)
		return false;
	if (params->vertCount >= 0xffff)
		return false;
	if (!params->vertCount || !params->verts)
		return false;
	if (!params->polyCount || !params->polys)
		return false;

	const int nvp = params->nvp;
	
	// Classify off-mesh connection points. We store only the connections
	// whose start point is inside the tile.
	unsigned char* offMeshConClass = 0;
	int storedOffMeshConCount = 0;
	int baseOffMeshConLinkCount = 0;
	int landOffMeshConLinkCount = 0;
	
	if (params->offMeshConCount > 0)
	{
		offMeshConClass = (unsigned char*)rdAlloc(sizeof(unsigned char)*params->offMeshConCount*2, RD_ALLOC_TEMP);
		if (!offMeshConClass)
			return false;

		// Find tight height bounds, used for culling out off-mesh start locations.
		float hmin = FLT_MAX;
		float hmax = -FLT_MAX;
		
		if (params->detailVerts && params->detailVertsCount)
		{
			for (int i = 0; i < params->detailVertsCount; ++i)
			{
				const float h = params->detailVerts[i*3+2];
				hmin = rdMin(hmin,h);
				hmax = rdMax(hmax,h);
			}
		}
		else
		{
			for (int i = 0; i < params->vertCount; ++i)
			{
				const unsigned short* iv = &params->verts[i*3];
				const float h = params->bmin[2] + iv[2] * params->ch;
				hmin = rdMin(hmin,h);
				hmax = rdMax(hmax,h);
			}
		}
		hmin -= params->walkableClimb;
		hmax += params->walkableClimb;
		float bmin[3], bmax[3];
		rdVcopy(bmin, params->bmin);
		rdVcopy(bmax, params->bmax);
		bmin[2] = hmin;
		bmax[2] = hmax;

		for (int i = 0; i < params->offMeshConCount; ++i)
		{
			const float* p0 = &params->offMeshConVerts[(i*2+0)*3];
			const float* p1 = &params->offMeshConVerts[(i*2+1)*3];
			offMeshConClass[i*2+0] = rdClassifyPointOutsideBounds(p0, bmin, bmax);
			offMeshConClass[i*2+1] = rdClassifyPointOutsideBounds(p1, bmin, bmax);

			// Zero out off-mesh start positions which are not even potentially touching the mesh.
			if (offMeshConClass[i*2+0] == 0xff)
			{
				if (p0[2] < bmin[2] || p0[2] > bmax[2])
					offMeshConClass[i*2+0] = 0;
			}

			// Count how many links should be allocated for off-mesh connections.
			if (offMeshConClass[i*2+0] == 0xff)
				baseOffMeshConLinkCount++;
			if (offMeshConClass[i*2+1] == 0xff && params->offMeshConDir[i])
				landOffMeshConLinkCount++;

			if (offMeshConClass[i*2+0] == 0xff)
				storedOffMeshConCount++;
		}
	}
	
	// Off-mesh connections are stored as polygons, adjust values.
	const int totPolyCount = params->polyCount + storedOffMeshConCount;
	const int totVertCount = params->vertCount + storedOffMeshConCount*2;
	
	// Find portal edges which are at tile borders.
	int edgeCount = 0;
	int portalCount = 0;
	int detailBoundCount = 0;
	for (int i = 0; i < params->polyCount; ++i)
	{
		const unsigned short* p = &params->polys[i*2*nvp];
		int numPolyBounds = 0;

		for (int j = 0; j < nvp; ++j)
		{
			if (p[j] == RD_MESH_NULL_IDX) break;
			edgeCount++;
			
			if (!p[nvp+j])
				numPolyBounds++;
			else if (p[nvp+j] & 0x8000)
			{
				unsigned short dir = p[nvp+j] & 0xf;
				if (dir != 0xf)
					portalCount++;
			}
		}

		int numDetailBounds = 0;

		if (params->detailMeshes)
		{
			const unsigned int tb = params->detailMeshes[i*4+2];
			const unsigned int tc = params->detailMeshes[i*4+3];

			for (unsigned int j = 0; j < tc; j++)
			{
				const unsigned char* tris = &params->detailTris[(tb+j)*4];

				const int ANY_BOUNDARY_EDGE =
					(RD_DETAIL_EDGE_BOUNDARY << 0) |
					(RD_DETAIL_EDGE_BOUNDARY << 2) |
					(RD_DETAIL_EDGE_BOUNDARY << 4);

				if ((tris[3] & ANY_BOUNDARY_EDGE))
					numDetailBounds++;
			}
		}

		// Subtract as polygon bounds are already counted in edgeCount.
		detailBoundCount += (numDetailBounds-numPolyBounds);
	}

	const int maxLinkCount = edgeCount + detailBoundCount*32 + portalCount*2 + baseOffMeshConLinkCount*3 + landOffMeshConLinkCount;
	
	// Find unique detail vertices.
	int uniqueDetailVertCount = 0;
	int detailTriCount = 0;
	if (params->detailMeshes)
	{
		// Has detail mesh, count unique detail vertex count and use input detail tri count.
		detailTriCount = params->detailTriCount;
		for (int i = 0; i < params->polyCount; ++i)
		{
			const unsigned short* p = &params->polys[i*nvp*2];
			int ndv = params->detailMeshes[i*4+1];
			int nv = 0;
			for (int j = 0; j < nvp; ++j)
			{
				if (p[j] == RD_MESH_NULL_IDX) break;
				nv++;
			}
			ndv -= nv;
			uniqueDetailVertCount += ndv;
		}
	}
	else
	{
		// No input detail mesh, build detail mesh from nav polys.
		uniqueDetailVertCount = 0; // No extra detail verts.
		detailTriCount = 0;
		for (int i = 0; i < params->polyCount; ++i)
		{
			const unsigned short* p = &params->polys[i*nvp*2];
			int nv = 0;
			for (int j = 0; j < nvp; ++j)
			{
				if (p[j] == RD_MESH_NULL_IDX) break;
				nv++;
			}
			detailTriCount += nv-2;
		}
	}

	// Create BVtree.
	rdTempVector<BVItem> treeItems;
	if (params->buildBvTree)
	{
		if (!createBVTree(params, treeItems))
			return false;
	}

#if DT_NAVMESH_SET_VERSION >= 8
	rdTempVector<CellItem> cellItems;
	if (params->detailMeshes)
	{
		if (!createPolyMeshCells(params, cellItems))
			return false;
	}
#endif

	// Calculate data size
	const int headerSize = rdAlign4(sizeof(dtMeshHeader));
	const int vertsSize = rdAlign4(sizeof(float)*3*totVertCount);
	const int polysSize = rdAlign4(sizeof(dtPoly)*totPolyCount);
	const int linksSize = rdAlign4(sizeof(dtLink)*maxLinkCount);
	const int detailMeshesSize = rdAlign4(sizeof(dtPolyDetail)*params->polyCount);
	const int detailVertsSize = rdAlign4(sizeof(float)*3*uniqueDetailVertCount);
	const int detailTrisSize = rdAlign4(sizeof(unsigned char)*4*detailTriCount);
	const int bvTreeSize = rdAlign4(sizeof(dtBVNode)*(int)treeItems.size());
	const int offMeshConsSize = rdAlign4(sizeof(dtOffMeshConnection)*storedOffMeshConCount);
#if DT_NAVMESH_SET_VERSION >= 8
	const int cellsSize = rdAlign4(sizeof(dtCell)*(int)cellItems.size());
#endif
	
	int polyMapCount = 0; // TODO: this data has to be reversed still from the NavMesh!
	const int polyMapSize = polyMapCount * totPolyCount;

	const int dataSize = headerSize + vertsSize + polysSize + polyMapSize + linksSize +
		detailMeshesSize + detailVertsSize + detailTrisSize +
		bvTreeSize + offMeshConsSize
#if DT_NAVMESH_SET_VERSION >= 8
		+ cellsSize;
#else
		;
#endif

	//printf("%i %i %i %i(%i links) %i %i %i %i %i\n", headerSize, vertsSize, polysSize, linksSize, maxLinkCount, detailMeshesSize, detailVertsSize, detailTrisSize, bvTreeSize, offMeshConsSize);
	//printf("%i\n", dataSize);

	unsigned char* data = (unsigned char*)rdAlloc(sizeof(unsigned char)*dataSize, RD_ALLOC_PERM);
	if (!data)
	{
		rdFree(offMeshConClass);
		return false;
	}
	memset(data, 0, dataSize);
	
	unsigned char* d = data;
	
	dtMeshHeader* header = rdGetThenAdvanceBufferPointer<dtMeshHeader>(d, headerSize);
	float* navVerts = rdGetThenAdvanceBufferPointer<float>(d, vertsSize);
	dtPoly* navPolys = rdGetThenAdvanceBufferPointer<dtPoly>(d, polysSize);
	unsigned int* polyMap = rdGetThenAdvanceBufferPointer<unsigned int>(d, polyMapSize);
	d += linksSize; // Ignore links; just leave enough space for them. They'll be created on load.
	//dtLink* links = rdGetThenAdvanceBufferPointer<dtLink>(d, linksSize);
	dtPolyDetail* navDMeshes = rdGetThenAdvanceBufferPointer<dtPolyDetail>(d, detailMeshesSize);
	float* navDVerts = rdGetThenAdvanceBufferPointer<float>(d, detailVertsSize);
	unsigned char* navDTris = rdGetThenAdvanceBufferPointer<unsigned char>(d, detailTrisSize);
	dtBVNode* navBvtree = rdGetThenAdvanceBufferPointer<dtBVNode>(d, bvTreeSize);
	dtOffMeshConnection* offMeshCons = rdGetThenAdvanceBufferPointer<dtOffMeshConnection>(d, offMeshConsSize);
#if DT_NAVMESH_SET_VERSION >= 8
	dtCell* navCells = rdGetThenAdvanceBufferPointer<dtCell>(d, cellsSize);
#endif

	rdIgnoreUnused(polyMap);
	//for(int i=0;i<polyMapCount*totPolyCount;i++)
	//	polyMap[i] = rand();

	// Store header
	header->magic = DT_NAVMESH_MAGIC;
	header->version = DT_NAVMESH_VERSION;
	header->x = params->tileX;
	header->y = params->tileY;
	header->layer = params->tileLayer;
	header->userId = params->userId;
	header->polyCount = totPolyCount;
	header->polyMapCount = polyMapCount;
	header->vertCount = totVertCount;
	header->maxLinkCount = maxLinkCount;
	rdVcopy(header->bmin, params->bmin);
	rdVcopy(header->bmax, params->bmax);
	header->detailMeshCount = params->polyCount;
	header->detailVertCount = uniqueDetailVertCount;
	header->detailTriCount = detailTriCount;
	header->bvQuantFactor = 1.0f / params->cs;
	header->offMeshBase = params->polyCount;
#if DT_NAVMESH_SET_VERSION >= 8
	header->maxCellCount = (int)cellItems.size();
#endif
	header->walkableHeight = params->walkableHeight;
	header->walkableRadius = params->walkableRadius;
	header->walkableClimb = params->walkableClimb;
	header->offMeshConCount = storedOffMeshConCount;
	header->bvNodeCount = (int)treeItems.size();

	const int offMeshVertsBase = params->vertCount;
	const int offMeshPolyBase = params->polyCount;
	
	// Store vertices
	// Mesh vertices
	for (int i = 0; i < params->vertCount; ++i)
	{
		const unsigned short* iv = &params->verts[i*3];
		float* v = &navVerts[i*3];
		v[0] = params->bmin[0] + iv[0] * params->cs;
		v[1] = params->bmin[1] + iv[1] * params->cs;
		v[2] = params->bmin[2] + iv[2] * params->ch;
	}
	// Off-mesh link vertices.
	int n = 0;
	for (int i = 0; i < params->offMeshConCount; ++i)
	{
		// Only store connections which start from this tile.
		if (offMeshConClass[i*2+0] == 0xff)
		{
			const float* linkv = &params->offMeshConVerts[i*2*3];
			float* v = &navVerts[(offMeshVertsBase + n*2)*3];
			rdVcopy(&v[0], &linkv[0]);
			rdVcopy(&v[3], &linkv[3]);
			n++;
		}
	}
	
	// Store polygons
	// Mesh polys
	const unsigned short* src = params->polys;
	for (int i = 0; i < params->polyCount; ++i)
	{
		dtPoly* p = &navPolys[i];
		p->flags = params->polyFlags[i];
		p->vertCount = 0;
		p->setArea(params->polyAreas[i]);
		p->setType(DT_POLYTYPE_GROUND);
		p->surfaceArea = params->surfAreas[i];
		for (int j = 0; j < nvp; ++j)
		{
			if (src[j] == RD_MESH_NULL_IDX) break;
			p->verts[j] = src[j];
			if (src[nvp+j] & 0x8000)
			{
				// Border or portal edge.
				unsigned short dir = src[nvp+j] & 0xf;
				if (dir == 0xf) // Border
					p->neis[j] = 0;
				else if (dir == 0) // Portal x-
					p->neis[j] = DT_EXT_LINK | 4;
				else if (dir == 1) // Portal y+
					p->neis[j] = DT_EXT_LINK | 2;
				else if (dir == 2) // Portal x+
					p->neis[j] = DT_EXT_LINK | 0;
				else if (dir == 3) // Portal y-
					p->neis[j] = DT_EXT_LINK | 6;
			}
			else
			{
				// Normal connection
				p->neis[j] = src[nvp+j]+1;
			}
			rdVadd(p->center, p->center, &navVerts[p->verts[j] * 3]);
			p->vertCount++;
		}
		rdVscale(p->center, p->center, 1 / (float)(p->vertCount));
		src += nvp*2;
	}

	// Off-mesh connection vertices.
	n = 0;
	for (int i = 0; i < params->offMeshConCount; ++i)
	{
		// Only store connections which start from this tile.
		if (offMeshConClass[i*2+0] == 0xff)
		{
			dtPoly* p = &navPolys[offMeshPolyBase+n];
			p->vertCount = 2;
			p->verts[0] = (unsigned short)(offMeshVertsBase + n*2+0);
			p->verts[1] = (unsigned short)(offMeshVertsBase + n*2+1);
			p->flags = params->offMeshConFlags[i];
			p->setArea(params->offMeshConAreas[i]);
			p->setType(DT_POLYTYPE_OFFMESH_CONNECTION);
			n++;
		}
	}

	// Store detail meshes and vertices.
	// The nav polygon vertices are stored as the first vertices on each mesh.
	// We compress the mesh data by skipping them and using the navmesh coordinates.
	if (params->detailMeshes)
	{
		unsigned short vbase = 0;
		for (int i = 0; i < params->polyCount; ++i)
		{
			dtPolyDetail& dtl = navDMeshes[i];
			const int vb = (int)params->detailMeshes[i*4+0];
			const int ndv = (int)params->detailMeshes[i*4+1];
			const int nv = navPolys[i].vertCount;
			dtl.vertBase = (unsigned int)vbase;
			dtl.vertCount = (unsigned char)(ndv-nv);
			dtl.triBase = (unsigned int)params->detailMeshes[i*4+2];
			dtl.triCount = (unsigned char)params->detailMeshes[i*4+3];
			// Copy vertices except the first 'nv' verts which are equal to nav poly verts.
			if (ndv-nv)
			{
				memcpy(&navDVerts[vbase*3], &params->detailVerts[(vb+nv)*3], sizeof(float)*3*(ndv-nv));
				vbase += (unsigned short)(ndv-nv);
			}
		}
		// Store triangles.
		memcpy(navDTris, params->detailTris, sizeof(unsigned char)*4*params->detailTriCount);
	}
	else
	{
		// Create dummy detail mesh by triangulating polys.
		int tbase = 0;
		for (int i = 0; i < params->polyCount; ++i)
		{
			dtPolyDetail& dtl = navDMeshes[i];
			const int nv = navPolys[i].vertCount;
			dtl.vertBase = 0;
			dtl.vertCount = 0;
			dtl.triBase = (unsigned int)tbase;
			dtl.triCount = (unsigned char)(nv-2);
			// Triangulate polygon (local indices).
			for (int j = 2; j < nv; ++j)
			{
				unsigned char* t = &navDTris[tbase*4];
				t[0] = 0;
				t[1] = (unsigned char)(j-1);
				t[2] = (unsigned char)(j);
				// Bit for each edge that belongs to poly boundary.
				t[3] = (1<<2);
				if (j == 2) t[3] |= (1<<0);
				if (j == nv-1) t[3] |= (1<<4);
				tbase++;
			}
		}
	}

	// Store BVTree.
	for (int i = 0; i < (int)treeItems.size(); i++)
	{
		dtBVNode& node = navBvtree[i];
		const BVItem& item = treeItems[i];

		node.bmin[0] = item.bmin[0];
		node.bmin[1] = item.bmin[1];
		node.bmin[2] = item.bmin[2];
		node.bmax[0] = item.bmax[0];
		node.bmax[1] = item.bmax[1];
		node.bmax[2] = item.bmax[2];
		node.i = item.i;
	}
	
	// Store Off-Mesh connections.
	n = 0;
	for (int i = 0; i < params->offMeshConCount; ++i)
	{
		// Only store connections which start from this tile.
		if (offMeshConClass[i*2+0] == 0xff)
		{
			dtOffMeshConnection* con = &offMeshCons[n];
			// Copy connection end-points.
			const float* endPts = &params->offMeshConVerts[i*2*3];
			const float* refPos = &params->offMeshConRefPos[i*3];
			rdVcopy(&con->pos[0], &endPts[0]);
			rdVcopy(&con->pos[3], &endPts[3]);
			con->rad = params->offMeshConRad[i];
			con->poly = (unsigned short)(offMeshPolyBase + n);
			con->side = offMeshConClass[i*2+1];
			con->setTraverseType(params->offMeshConJumps[i], params->offMeshConOrders[i]);
			con->userId = params->offMeshConUserID[i];
#if DT_NAVMESH_SET_VERSION >= 7
			con->hintIndex = DT_NULL_HINT;
#else
			con->flags = params->offMeshConDir[i] ? DT_OFFMESH_CON_BIDIR : 0;
#endif
			rdVcopy(&con->refPos[0], &refPos[0]);
			con->refYaw = params->offMeshConRefYaw[i];
			n++;
		}
	}

#if DT_NAVMESH_SET_VERSION >= 8
	// Store polygon cells.
	for (int i = 0; i < (int)cellItems.size(); i++)
	{
		const CellItem& cellItem = cellItems[i];
		dtCell& cell = navCells[i];

		rdVcopy(cell.pos, cellItem.pos);
		cell.polyIndex = cellItem.polyIndex;
		cell.setOccupied();
	}
#endif

	rdFree(offMeshConClass);

	*outData = data;
	*outDataSize = dataSize;
	
	return true;
}

bool dtNavMeshHeaderSwapEndian(unsigned char* data, const int /*dataSize*/)
{
	dtMeshHeader* header = (dtMeshHeader*)data;
	
	int swappedMagic = DT_NAVMESH_MAGIC;
	int swappedVersion = DT_NAVMESH_VERSION;
	rdSwapEndian(&swappedMagic);
	rdSwapEndian(&swappedVersion);
	
	if ((header->magic != DT_NAVMESH_MAGIC || header->version != DT_NAVMESH_VERSION) &&
		(header->magic != swappedMagic || header->version != swappedVersion))
	{
		return false;
	}
		
	rdSwapEndian(&header->magic);
	rdSwapEndian(&header->version);
	rdSwapEndian(&header->x);
	rdSwapEndian(&header->y);
	rdSwapEndian(&header->layer);
	rdSwapEndian(&header->userId);
	rdSwapEndian(&header->polyCount);
	rdSwapEndian(&header->vertCount);
	rdSwapEndian(&header->maxLinkCount);
	rdSwapEndian(&header->detailMeshCount);
	rdSwapEndian(&header->detailVertCount);
	rdSwapEndian(&header->detailTriCount);
	rdSwapEndian(&header->bvNodeCount);
	rdSwapEndian(&header->offMeshConCount);
	rdSwapEndian(&header->offMeshBase);
#if DT_NAVMESH_SET_VERSION >= 8
	rdSwapEndian(&header->maxCellCount);
#endif
	rdSwapEndian(&header->walkableHeight);
	rdSwapEndian(&header->walkableRadius);
	rdSwapEndian(&header->walkableClimb);
	rdSwapEndian(&header->bmin[0]);
	rdSwapEndian(&header->bmin[1]);
	rdSwapEndian(&header->bmin[2]);
	rdSwapEndian(&header->bmax[0]);
	rdSwapEndian(&header->bmax[1]);
	rdSwapEndian(&header->bmax[2]);
	rdSwapEndian(&header->bvQuantFactor);

	// Freelist index and pointers are updated when tile is added, no need to swap.

	return true;
}

/// @par
/// 
/// This function will remove all disabled polygons marked #DT_UNLINKED_POLY_GROUP
/// from the tile. Its associated data, such as the detail polygons, links, cells,
/// etc will also be removed. The BVTree is the only data that needs to be rebuilt
/// as we have to re-subdivide the bounding volumes with only the polygons that remain
/// to exist. Off-mesh connections that lack the poly flag #DT_POLYFLAGS_JUMP_LINKED
/// will also be removed.
bool dtUpdateNavMeshData(dtNavMesh* nav, const unsigned int tileIndex)
{
	dtMeshTile* tile = nav->getTile(tileIndex);
	const dtMeshHeader* header = tile->header;

	// Remove the tile instead of updating it!
	rdAssert(header->userId != DT_FULL_UNLINKED_TILE_USER_ID);

	rdScopedDelete<unsigned short> oldPolyIdMap((unsigned short*)rdAlloc(sizeof(unsigned short)*header->polyCount, RD_ALLOC_TEMP));
	rdScopedDelete<unsigned short> newPolyIdMap((unsigned short*)rdAlloc(sizeof(unsigned short)*header->polyCount, RD_ALLOC_TEMP));

	rdScopedDelete<unsigned short> oldVertIdMap((unsigned short*)rdAlloc(sizeof(unsigned short)*header->vertCount, RD_ALLOC_TEMP));
	rdScopedDelete<unsigned short> newVertIdMap((unsigned short*)rdAlloc(sizeof(unsigned short)*header->vertCount, RD_ALLOC_TEMP));

	memset(newVertIdMap, 0xff, sizeof(unsigned short)*header->vertCount);

	rdScopedDelete<int> oldOffMeshConnIdMap((int*)rdAlloc(sizeof(int)*header->offMeshConCount, RD_ALLOC_TEMP));
	rdScopedDelete<int> newOffMeshConnIdMap((int*)rdAlloc(sizeof(int)*header->offMeshConCount, RD_ALLOC_TEMP));

	rdScopedDelete<unsigned int> oldLinkIdMap((unsigned int*)rdAlloc(sizeof(unsigned int)*header->maxLinkCount, RD_ALLOC_TEMP));
	rdScopedDelete<unsigned int> newLinkIdMap((unsigned int*)rdAlloc(sizeof(unsigned int)*header->maxLinkCount, RD_ALLOC_TEMP));

	int totPolyCount = 0, offMeshConCount = 0, detailTriCount = 0, portalCount = 0, detailVertCount = 0, vertCount = 0, maxLinkCount = 0;

	// Iterate through this tile's polys, indexing them by their new poly ids
	for (int i = 0; i < header->polyCount; i++)
	{
		const dtPoly& poly = tile->polys[i];

		// Unlinked polygon, drop it.
		if (poly.groupId == DT_UNLINKED_POLY_GROUP && (poly.flags & DT_POLYFLAGS_DISABLED))
			continue;

		const bool isOffMeshConn = poly.getType() == DT_POLYTYPE_OFFMESH_CONNECTION;

		if (isOffMeshConn)
		{
			// Unlinked off-mesh connection, drop it.
			if (!(poly.flags & DT_POLYFLAGS_JUMP_LINKED))
			{
				if (poly.firstLink == DT_NULL_LINK)
					continue; // Off-mesh link also doesn't have a base.

				// Find and remove the link connecting the poly 
				// the off-mesh connection is basing from. This
				// is necessary as otherwise we would still keep
				// this link while we drop the off-mesh connection.
				const dtLink& link = tile->links[poly.firstLink];

				unsigned int salt, it, ip;
				nav->decodePolyId(link.ref, salt, it, ip);

				if (it != tileIndex)
					continue;

				dtPoly& basePoly = tile->polys[ip];

				unsigned int j = basePoly.firstLink;
				unsigned int pj = DT_NULL_LINK;
				while (j != DT_NULL_LINK)
				{
					if (nav->decodePolyIdPoly(tile->links[j].ref) == (unsigned int)i)
					{
						// Remove link.
						unsigned int nk = tile->links[j].next;
						if (pj == DT_NULL_LINK)
							basePoly.firstLink = nk;
						else
							tile->links[pj].next = nk;
						tile->freeLink(j);
						j = nk;

						break;
					}
					else
					{
						// Advance
						pj = j;
						j = tile->links[j].next;
					}
				}

				continue;
			}

			const int conIdx = i-header->offMeshBase;

			const dtOffMeshConnection& conn = tile->offMeshCons[conIdx];
			rdAssert(conn.poly == i);

			oldOffMeshConnIdMap[offMeshConCount] = conIdx;
			newOffMeshConnIdMap[conIdx] = offMeshConCount++;
		}
		else
		{
			for (int j = 0; j < poly.vertCount; j++)
			{
				if (poly.neis[j] == RD_MESH_NULL_IDX)
					continue;

				if (poly.neis[j] & DT_EXT_LINK)
					portalCount++;
			}
		}

		oldPolyIdMap[totPolyCount] = (unsigned short)i;
		newPolyIdMap[i] = (unsigned short)totPolyCount++;

		// Flag this poly's vertices so we can throw out those that end up being
		// eliminated. Skip vertices which have already been identified as this
		// will also prevent us from creating duplicates (and will remove them if
		// there are any in the input).
		for (unsigned int v = 0; v < poly.vertCount; v++)
		{
			if (newVertIdMap[poly.verts[v]] != 0xffff)
				continue;

			oldVertIdMap[vertCount] = poly.verts[v];
			newVertIdMap[poly.verts[v]] = (unsigned short)vertCount++;
		}

		// Off-mesh links don't have detail meshes.
		if (!isOffMeshConn)
		{
			detailVertCount += tile->detailMeshes[i].vertCount;
			detailTriCount += tile->detailMeshes[i].triCount;
		}
	}

	if (!totPolyCount)
	{
		// This happens when all polygons in a tile are marked unreachable. The
		// tile itself has to be removed entirely.
		rdAssert(0);
		return false;
	}

	// Flag links that need to be kept, this has to run separately from the
	// first loop, since we typically process off-mesh connection polygons
	// after normal ones, and dropping off-mesh connection links will happen
	// after we flagged links from some polygons, meaning that if we process
	// this during the first iteration, dead off-mesh connection links will
	// still make it into the rebuilt tile.
	for (int i = 0; i < header->polyCount; i++)
	{
		const dtPoly& poly = tile->polys[i];

		// Flag all links connected to this polygon.
		for (unsigned int j = poly.firstLink; j != DT_NULL_LINK; j = tile->links[j].next)
		{
			oldLinkIdMap[maxLinkCount] = j;
			newLinkIdMap[j] = maxLinkCount++;
		}
	}

	// Count without off-mesh link polygons.
	const int polyCount = totPolyCount - offMeshConCount;

	rdTempVector<BVItem> treeItems;
	if (header->bvNodeCount)
	{
		if (!rebuildBVTree(tile, oldPolyIdMap, polyCount, treeItems))
		{
			rdAssert(0);
			return false;
		}
	}

#if DT_NAVMESH_SET_VERSION >= 8
	rdTempVector<CellItem> cellItems(header->maxCellCount);
	int numCellsKept = 0;

	for (int i = 0; i < header->maxCellCount; i++)
	{
		const dtCell& cell = tile->cells[i];
		const dtPoly& poly = tile->polys[cell.polyIndex];

		// Don't copy cells residing on dead polygons.
		if (poly.groupId == DT_UNLINKED_POLY_GROUP && (poly.flags & DT_POLYFLAGS_DISABLED))
			continue;

		CellItem& newCell = cellItems[numCellsKept++];

		rdVcopy(newCell.pos, cell.pos);
		newCell.polyIndex = newPolyIdMap[cell.polyIndex];
	}
#endif
	const int polyMapCount = header->polyMapCount;

	const int headerSize = rdAlign4(sizeof(dtMeshHeader));
	const int vertsSize = rdAlign4(sizeof(float)*3*vertCount);
	const int polysSize = rdAlign4(sizeof(dtPoly)*totPolyCount);
	const int polyMapSize = rdAlign4(sizeof(int)*(polyMapCount*totPolyCount));
	const int linksSize = rdAlign4(sizeof(dtLink)*maxLinkCount);
	const int detailMeshesSize = rdAlign4(sizeof(dtPolyDetail)*polyCount);
	const int detailVertsSize = rdAlign4(sizeof(float)*3*detailVertCount);
	const int detailTrisSize = rdAlign4(sizeof(unsigned char)*4*detailTriCount);
	const int bvTreeSize = rdAlign4(sizeof(dtBVNode)*(int)treeItems.size());
	const int offMeshConsSize = rdAlign4(sizeof(dtOffMeshConnection)*offMeshConCount);
#if DT_NAVMESH_SET_VERSION >= 8
	const int cellsSize = rdAlign4(sizeof(dtCell)*numCellsKept);
#endif

	const unsigned int dataSize = headerSize + vertsSize + polysSize + polyMapSize + linksSize +
		detailMeshesSize + detailVertsSize + detailTrisSize + 
		bvTreeSize + offMeshConsSize
#if DT_NAVMESH_SET_VERSION >= 8
		+ cellsSize
#endif
		;

	unsigned char* data = new unsigned char[dataSize];

	if (!data)
		return false;

	memset(data, 0, dataSize);
	unsigned char* d = data;

	dtMeshHeader* newHeader = rdGetThenAdvanceBufferPointer<dtMeshHeader>(d, headerSize);
	float* navVerts = rdGetThenAdvanceBufferPointer<float>(d, vertsSize);
	dtPoly* navPolys = rdGetThenAdvanceBufferPointer<dtPoly>(d, polysSize);
	unsigned int* polyMap = rdGetThenAdvanceBufferPointer<unsigned int>(d, polyMapSize);
	dtLink* links = rdGetThenAdvanceBufferPointer<dtLink>(d, linksSize);
	dtPolyDetail* navDMeshes = rdGetThenAdvanceBufferPointer<dtPolyDetail>(d, detailMeshesSize);
	float* navDVerts = rdGetThenAdvanceBufferPointer<float>(d, detailVertsSize);
	unsigned char* navDTris = rdGetThenAdvanceBufferPointer<unsigned char>(d, detailTrisSize);
	dtBVNode* navBvtree = rdGetThenAdvanceBufferPointer<dtBVNode>(d, bvTreeSize);
	dtOffMeshConnection* offMeshCons = rdGetThenAdvanceBufferPointer<dtOffMeshConnection>(d, offMeshConsSize);
#if DT_NAVMESH_SET_VERSION >= 8
	dtCell* navCells = rdGetThenAdvanceBufferPointer<dtCell>(d, cellsSize);
#endif

	// Store header
	newHeader->magic = DT_NAVMESH_MAGIC;
	newHeader->version = DT_NAVMESH_VERSION;
	newHeader->x = header->x;
	newHeader->y = header->y;
	newHeader->layer = header->layer;
	newHeader->userId = 0;
	newHeader->polyCount = totPolyCount;
	newHeader->polyMapCount = polyMapCount;
	newHeader->vertCount = vertCount;
	newHeader->maxLinkCount = maxLinkCount;
	newHeader->detailMeshCount = polyCount;
	newHeader->detailVertCount = detailVertCount;
	newHeader->detailTriCount = detailTriCount;
	newHeader->bvNodeCount = (int)treeItems.size();
	newHeader->offMeshConCount = offMeshConCount;
	newHeader->offMeshBase = polyCount;
#if DT_NAVMESH_SET_VERSION >= 8
	newHeader->maxCellCount = numCellsKept;
#endif
	newHeader->walkableHeight = header->walkableHeight;
	newHeader->walkableRadius = header->walkableRadius;
	newHeader->walkableClimb = header->walkableClimb;
	rdVcopy(newHeader->bmin, header->bmin);
	rdVcopy(newHeader->bmax, header->bmax);
	newHeader->bvQuantFactor = header->bvQuantFactor;

	// Store vertices.
	for (int i = 0; i < vertCount; i++)
		rdVcopy(&navVerts[i*3], &tile->verts[oldVertIdMap[i]*3]);

	// Store polygons.
	for (int i = 0; i < totPolyCount; i++)
	{
		const dtPoly& ip = tile->polys[oldPolyIdMap[i]];
		dtPoly& p = navPolys[i];

		// Unlinked and disabled polygons should not reach this stage.
		rdAssert(!(ip.groupId == DT_UNLINKED_POLY_GROUP && (ip.flags & DT_POLYFLAGS_DISABLED)));
		const bool nullLink = ip.firstLink == DT_NULL_LINK;

		p.firstLink = nullLink ? DT_NULL_LINK : newLinkIdMap[ip.firstLink];
		p.flags = ip.flags;
		p.vertCount = ip.vertCount;
		p.areaAndtype = ip.areaAndtype;
		p.groupId = ip.groupId;
		p.surfaceArea = ip.surfaceArea;
#if DT_NAVMESH_SET_VERSION >= 7
		p.unk1 = ip.unk1;
		p.unk2 = ip.unk2;
#endif
		rdVcopy(p.center, ip.center);

		for (int v = 0; v < p.vertCount; v++)
			p.verts[v] = newVertIdMap[ip.verts[v]];

		for (int n = 0; n < RD_VERTS_PER_POLYGON; n++)
		{
			// if this is a portal, leave these values unchanged
			if (ip.neis[n] & DT_EXT_LINK || !ip.neis[n])
				p.neis[n] = ip.neis[n];
			else
				p.neis[n] = newPolyIdMap[ip.neis[n]-1]+1;
		}
	}

	// Store polymap.
	for (int i = 0; i < polyMapCount; i++)
	{
		unsigned int* oldPolyMapBase = &tile->polyMap[i*header->polyCount];
		unsigned int* newPolyMapBase = &polyMap[i*totPolyCount];

		for (int j = 0; j < totPolyCount; j++)
			newPolyMapBase[j] = oldPolyMapBase[oldPolyIdMap[j]];
	}

	// Fix up internal references and store links.
	const dtPolyRef polyRefBase = nav->getPolyRefBase(tile);

	for (int i = 0; i < maxLinkCount; i++)
	{
		const dtLink& oldLink = tile->links[oldLinkIdMap[i]];
		dtLink& newLink = links[i];

		unsigned int salt, it, ip;
		nav->decodePolyId(oldLink.ref, salt, it, ip);

		const bool sameTile = it == tileIndex;

		const dtPolyRef newRef = sameTile
			? (polyRefBase | (dtPolyRef)newPolyIdMap[ip])
			: oldLink.ref;

		const bool nullLink = oldLink.next == DT_NULL_LINK;

		const unsigned int newNext = nullLink
			? DT_NULL_LINK
			: newLinkIdMap[oldLink.next];

		const unsigned short newReverseLink = !sameTile
			? oldLink.reverseLink
			: oldLink.reverseLink == DT_NULL_TRAVERSE_REVERSE_LINK
			? DT_NULL_TRAVERSE_REVERSE_LINK
			: (unsigned short)newLinkIdMap[oldLink.reverseLink];

		newLink.ref = newRef;
		newLink.next = newNext;
		newLink.edge = oldLink.edge;
		newLink.side = oldLink.side;
		newLink.bmin = oldLink.bmin;
		newLink.bmax = oldLink.bmax;
		newLink.traverseType = oldLink.traverseType;
		newLink.traverseDist = oldLink.traverseDist;
		newLink.reverseLink = newReverseLink;
	}

	// note(amos): tiles we already processed should not be reprocessed, doing
	// so will cause the polygon indices to be shifted again.
	std::set<const dtMeshTile*> processedExtTiles;

	// Fix up external reverences from neighboring tiles.
	static const int MAX_NEIS = 32;
	dtMeshTile* neis[MAX_NEIS];

	for (int i = 0; i < 8; ++i)
	{
		const int nneis = nav->getNeighbourTilesAt(header->x, header->y, i, neis, MAX_NEIS);
		for (int j = 0; j < nneis; ++j)
		{
			const dtMeshTile* neiTile = neis[j];
			processedExtTiles.insert(neiTile);

			const dtMeshHeader* neiHdr = neiTile->header;

			for (int k = 0; k < neiHdr->polyCount; k++)
			{
				const dtPoly& neiPoly = neiTile->polys[k];

				for (unsigned int l = neiPoly.firstLink; l != DT_NULL_LINK; l = neiTile->links[l].next)
				{
					dtLink& neiLink = neiTile->links[l];

					unsigned int salt, it, ip;
					nav->decodePolyId(neiLink.ref, salt, it, ip);

					if (it != tileIndex)
						continue;

					// Polygons that are not part of the selection should not
					// have their refs fixed, these links will be removed once
					// that tile gets processed.
					if (salt != tile->salt || ip >= (unsigned int)header->polyCount)
						continue;

					const dtPolyRef newRef = (polyRefBase | (dtPolyRef)newPolyIdMap[ip]);
					neiLink.ref = newRef;

					if (neiLink.reverseLink != DT_NULL_TRAVERSE_REVERSE_LINK)
						neiLink.reverseLink = (unsigned short)newLinkIdMap[neiLink.reverseLink];
				}
			}
		}
	}

	// Fix up external references from off-mesh links originating from our tile.
	for (int i = 0; i < offMeshConCount; i++)
	{
		// note(amos): we only have to fix external references, nothing has to
		// be removed. Off-mesh connections only get their land-side position
		// linked if it can be based on a polygon within its own tile. The only
		// reason for an off-mesh link to be removed is if it can't be based, or
		// can't be linked to the land-side tile.
		const dtOffMeshConnection& con = tile->offMeshCons[oldOffMeshConnIdMap[i]];
		const dtPoly& offMeshPoly = tile->polys[con.poly];

		rdAssert((offMeshPoly.flags & DT_POLYFLAGS_JUMP_LINKED));

		// Find tiles the query touches.
		int tx, ty;
		nav->calcTileLoc(&con.pos[3], &tx, &ty);

		const dtMeshTile* landTile = nav->getTileAt(tx, ty, header->layer);

		if (landTile == tile)
			continue; // Already dealt with when fixing up internal links.

		if (processedExtTiles.find(landTile) != processedExtTiles.end())
			continue;

		processedExtTiles.insert(landTile);

		const dtMeshHeader* landHdr = landTile->header;

		for (int j = 0; j < landHdr->polyCount; j++)
		{
			const dtPoly& landPoly = landTile->polys[j];

			for (unsigned int k = landPoly.firstLink; k != DT_NULL_LINK; k = landTile->links[k].next)
			{
				dtLink& landLink = landTile->links[k];

				unsigned int salt, it, ip;
				nav->decodePolyId(landLink.ref, salt, it, ip);

				if (it != tileIndex)
					continue;

				if (salt != tile->salt || ip >= (unsigned int)header->polyCount)
					continue;

				const dtPolyRef newRef = (polyRefBase | (dtPolyRef)newPolyIdMap[ip]);
				landLink.ref = newRef;
			}
		}
	}

	// Fix up external references from off-mesh links originating from other tiles.
	for (int i = 0; i < nav->getMaxTiles(); ++i)
	{
		dtMeshTile* offTile = nav->getTile(i);

		if (offTile == tile)
			continue; // Already dealt with when fixing up internal links.

		const dtMeshHeader* offHeader = offTile->header;

		if (!offHeader)
			continue;

		const int conCount = offHeader->offMeshConCount;

		if (!conCount)
			continue;

		if (processedExtTiles.find(offTile) != processedExtTiles.end())
			continue;

		processedExtTiles.insert(offTile);

		for (int j = 0; j < conCount; j++)
		{
			const dtOffMeshConnection& con = offTile->offMeshCons[j];
			const dtPoly& offMeshPoly = offTile->polys[con.poly];

			// This off-mesh link is dead and will be removed when its origin
			// tile gets processed.
			if (!(offMeshPoly.flags & DT_POLYFLAGS_JUMP_LINKED))
				continue;

			dtLink& offMeshLink = offTile->links[offMeshPoly.firstLink];

			unsigned int salt, it, ip;
			nav->decodePolyId(offMeshLink.ref, salt, it, ip);

			if (it != tileIndex)
				continue;

			if (salt != tile->salt || ip >= (unsigned int)header->polyCount)
				continue;

			const dtPolyRef newRef = polyRefBase | (dtPolyRef)newPolyIdMap[ip];
			offMeshLink.ref = newRef;
		}
	}

	// Store detail meshes.
	unsigned int vbase = 0;
	unsigned int tbase = 0;
	for (int i = 0; i < polyCount; i++)
	{
		const int oldPolyId = oldPolyIdMap[i];
		const dtPoly& oldPoly = tile->polys[oldPolyId];

		rdAssert(oldPoly.getType() != DT_POLYTYPE_OFFMESH_CONNECTION);

		const dtPolyDetail& oldDetail = tile->detailMeshes[oldPolyId];
		dtPolyDetail& newDetail = navDMeshes[i];

		const unsigned int vertBase = oldDetail.vertBase;
		const unsigned char dVertCount = oldDetail.vertCount;
		const unsigned int triBase = oldDetail.triBase;
		const unsigned char triCount = oldDetail.triCount;

		newDetail.vertBase = vbase;
		newDetail.vertCount = dVertCount;
		newDetail.triBase = tbase;
		newDetail.triCount = triCount;

		for (unsigned char j = 0; j < triCount; j++)
		{
			// Copy four bytes (first 3 for vertex indices for the triangle, 4th for flags)
			memcpy(&navDTris[tbase++*4], &tile->detailTris[(triBase+j)*4], sizeof(unsigned char)*4);
		}

		for (unsigned char j = 0; j < dVertCount; j++)
			rdVcopy(&navDVerts[vbase++*3], &tile->detailVerts[(vertBase+j)*3]);
	}

	// Store BVTree.
	if (bvTreeSize)
	{
		for (int i = 0; i < (int)treeItems.size(); i++)
		{
			const BVItem& item = treeItems[i];
			dtBVNode& node = navBvtree[i];

			node.bmin[0] = item.bmin[0];
			node.bmin[1] = item.bmin[1];
			node.bmin[2] = item.bmin[2];
			node.bmax[0] = item.bmax[0];
			node.bmax[1] = item.bmax[1];
			node.bmax[2] = item.bmax[2];
			node.i = item.i;
		}
	}

	// Store Off-Mesh connections.
	for (int i = 0; i < offMeshConCount; i++)
	{
		const dtOffMeshConnection& oldConn = tile->offMeshCons[oldOffMeshConnIdMap[i]];
		dtOffMeshConnection& newConn = offMeshCons[i];

		rdVcopy(&newConn.pos[0], &oldConn.pos[0]);
		rdVcopy(&newConn.pos[3], &oldConn.pos[3]);
		newConn.rad = oldConn.rad;
		newConn.poly = newPolyIdMap[oldConn.poly];
		newConn.side = oldConn.side;
		newConn.userId = oldConn.userId;
#if DT_NAVMESH_SET_VERSION >= 7
		newConn.traverseType = oldConn.traverseType;
		newConn.hintIndex = oldConn.hintIndex;
#else
		newConn.flags = oldConn.flags;
		newConn.traverseContext = oldConn.traverseContext;
#endif
		rdVcopy(newConn.refPos, oldConn.refPos);
		newConn.refYaw = oldConn.refYaw;
#if DT_NAVMESH_SET_VERSION >= 9
		rdVcopy(&newConn.secPos[0], &oldConn.secPos[0]);
		rdVcopy(&newConn.secPos[3], &oldConn.secPos[3]);
#endif
	}

#if DT_NAVMESH_SET_VERSION >= 8
	// Store polygon cells.
	for (int i = 0; i < numCellsKept; i++)
	{
		const CellItem& cellItem = cellItems[i];
		dtCell& cell = navCells[i];

		rdVcopy(cell.pos, cellItem.pos);
		cell.polyIndex = cellItem.polyIndex;
		cell.setOccupied();
	}
#endif

	// Free old data.
	rdFree(tile->data);

	// Store tile.
	tile->linksFreeList = DT_NULL_LINK; // All null links are pruned at this point.
	tile->header = newHeader;
	tile->verts = navVerts;
	tile->polys = navPolys;
	tile->polyMap = polyMap;
	tile->links = links;
	tile->detailMeshes = navDMeshes;
	tile->detailVerts = navDVerts;
	tile->detailTris = navDTris;
	tile->bvTree = navBvtree;
	tile->offMeshCons = offMeshCons;
#if DT_NAVMESH_SET_VERSION >= 8
	tile->cells = navCells;
#endif
	tile->data = data;
	tile->dataSize = dataSize;

	return true;
}

/// @par
///
/// @warning This function assumes that the header is in the correct endianess already. 
/// Call #dtNavMeshHeaderSwapEndian() first on the data if the data is expected to be in wrong endianess 
/// to start with. Call #dtNavMeshHeaderSwapEndian() after the data has been swapped if converting from 
/// native to foreign endianess.
bool dtNavMeshDataSwapEndian(unsigned char* data, const int /*dataSize*/)
{
	// Make sure the data is in right format.
	dtMeshHeader* header = (dtMeshHeader*)data;
	if (header->magic != DT_NAVMESH_MAGIC)
		return false;
	if (header->version != DT_NAVMESH_VERSION)
		return false;
	
	// Patch header pointers.
	const int headerSize = rdAlign4(sizeof(dtMeshHeader));
	const int vertsSize = rdAlign4(sizeof(float)*3*header->vertCount);
	const int polysSize = rdAlign4(sizeof(dtPoly)*header->polyCount);
	const int linksSize = rdAlign4(sizeof(dtLink)*(header->maxLinkCount));
	const int detailMeshesSize = rdAlign4(sizeof(dtPolyDetail)*header->detailMeshCount);
	const int detailVertsSize = rdAlign4(sizeof(float)*3*header->detailVertCount);
	const int detailTrisSize = rdAlign4(sizeof(unsigned char)*4*header->detailTriCount);
	const int bvtreeSize = rdAlign4(sizeof(dtBVNode)*header->bvNodeCount);
	const int offMeshLinksSize = rdAlign4(sizeof(dtOffMeshConnection)*header->offMeshConCount);
	
	unsigned char* d = data + headerSize;
	float* verts = rdGetThenAdvanceBufferPointer<float>(d, vertsSize);
	dtPoly* polys = rdGetThenAdvanceBufferPointer<dtPoly>(d, polysSize);
	d += linksSize; // Ignore links; they technically should be endian-swapped but all their data is overwritten on load anyway.
	//dtLink* links = rdGetThenAdvanceBufferPointer<dtLink>(d, linksSize);
	dtPolyDetail* detailMeshes = rdGetThenAdvanceBufferPointer<dtPolyDetail>(d, detailMeshesSize);
	float* detailVerts = rdGetThenAdvanceBufferPointer<float>(d, detailVertsSize);
	d += detailTrisSize; // Ignore detail tris; single bytes can't be endian-swapped.
	//unsigned char* detailTris = rdGetThenAdvanceBufferPointer<unsigned char>(d, detailTrisSize);
	dtBVNode* bvTree = rdGetThenAdvanceBufferPointer<dtBVNode>(d, bvtreeSize);
	dtOffMeshConnection* offMeshCons = rdGetThenAdvanceBufferPointer<dtOffMeshConnection>(d, offMeshLinksSize);
	
	// Vertices
	for (int i = 0; i < header->vertCount*3; ++i)
	{
		rdSwapEndian(&verts[i]);
	}

	// Polys
	for (int i = 0; i < header->polyCount; ++i)
	{
		dtPoly* p = &polys[i];
		// poly->firstLink is update when tile is added, no need to swap.
		for (int j = 0; j < RD_VERTS_PER_POLYGON; ++j)
		{
			rdSwapEndian(&p->verts[j]);
			rdSwapEndian(&p->neis[j]);
		}
		rdSwapEndian(&p->flags);
	}

	// Links are rebuild when tile is added, no need to swap.

	// Detail meshes
	for (int i = 0; i < header->detailMeshCount; ++i)
	{
		dtPolyDetail* pd = &detailMeshes[i];
		rdSwapEndian(&pd->vertBase);
		rdSwapEndian(&pd->triBase);
	}
	
	// Detail verts
	for (int i = 0; i < header->detailVertCount*3; ++i)
	{
		rdSwapEndian(&detailVerts[i]);
	}

	// BV-tree
	for (int i = 0; i < header->bvNodeCount; ++i)
	{
		dtBVNode* node = &bvTree[i];
		for (int j = 0; j < 3; ++j)
		{
			rdSwapEndian(&node->bmin[j]);
			rdSwapEndian(&node->bmax[j]);
		}
		rdSwapEndian(&node->i);
	}

	// Off-mesh Connections.
	for (int i = 0; i < header->offMeshConCount; ++i)
	{
		dtOffMeshConnection* con = &offMeshCons[i];
		for (int j = 0; j < 6; ++j)
			rdSwapEndian(&con->pos[j]);
		rdSwapEndian(&con->rad);
		rdSwapEndian(&con->poly);
	}
	
	return true;
}
