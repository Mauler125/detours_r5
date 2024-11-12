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
#include "Detour/Include/DetourNode.h"


inline bool overlapSlabs(const float* amin, const float* amax,
						 const float* bmin, const float* bmax,
						 const float px, const float pz)
{
	// Check for horizontal overlap.
	// The segment is shrunken a little so that slabs which touch
	// at end points are not connected.
	const float minx = rdMax(amin[0]+px,bmin[0]+px);
	const float maxx = rdMin(amax[0]-px,bmax[0]-px);
	if (minx > maxx)
		return false;
	
	// Check vertical overlap.
	const float ad = (amax[1]-amin[1]) / (amax[0]-amin[0]);
	const float ak = amin[1] - ad*amin[0];
	const float bd = (bmax[1]-bmin[1]) / (bmax[0]-bmin[0]);
	const float bk = bmin[1] - bd*bmin[0];
	const float aminy = ad*minx + ak;
	const float amaxy = ad*maxx + ak;
	const float bminy = bd*minx + bk;
	const float bmaxy = bd*maxx + bk;
	const float dmin = bminy - aminy;
	const float dmax = bmaxy - amaxy;
		
	// Crossing segments always overlap.
	if (dmin*dmax < 0)
		return true;
		
	// Check for overlap at endpoints.
	const float thr = rdSqr(pz*2);
	if (dmin*dmin <= thr || dmax*dmax <= thr)
		return true;
		
	return false;
}

static float getSlabCoord(const float* va, const int side)
{
	if (side == 0 || side == 4)
		return va[0];
	else if (side == 2 || side == 6)
		return va[1];
	return 0;
}

static void calcSlabEndPoints(const float* va, const float* vb, float* bmin, float* bmax, const int side)
{
	if (side == 0 || side == 4)
	{
		if (va[1] < vb[1])
		{
			bmin[0] = va[1];
			bmin[1] = va[2];
			bmax[0] = vb[1];
			bmax[1] = vb[2];
		}
		else
		{
			bmin[0] = vb[1];
			bmin[1] = vb[2];
			bmax[0] = va[1];
			bmax[1] = va[2];
		}
	}
	else if (side == 2 || side == 6)
	{
		if (va[0] < vb[0])
		{
			bmin[0] = va[0];
			bmin[1] = va[2];
			bmax[0] = vb[0];
			bmax[1] = vb[2];
		}
		else
		{
			bmin[0] = vb[0];
			bmin[1] = vb[2];
			bmax[0] = va[0];
			bmax[1] = va[2];
		}
	}
}

static void alignPortalLimits(const float* portal1Pos, const float* portal1Norm, const float* portal2Pos,
	const float portalTmin, const float portalTmax, float& outPortalTmin, float& outPortalTmax, const float maxAlign)
{
	// note(amos): if we take an extreme scenario, where:
	// - span = portalTmax(1.0f) - portalTmin(0.99f)
	// - shiftAmount = maxAlign(1.0f) * abs(crossZ(1.0f)) * span(0.1f)
	// the portal will collapse as we will shift portalTmin(0.99f) with
	// shiftAmount(0.1f)! Also, any value above 0.5 will cause too much
	// shifting; to avoid rounding mins into maxs or the other way around
	// during quantization, maxAlign should never be greater than 0.5.
	rdAssert(maxAlign <= 0.5f);

	float delta[3];
	delta[0] = portal2Pos[0]-portal1Pos[0];
	delta[1] = portal2Pos[1]-portal1Pos[1];
	delta[2] = 0.0f;
	rdVnormalize2D(delta);

	float cross[3];
	rdVcross(cross, delta, portal1Norm);

	if (maxAlign > 0 && (cross[2] < 0 || cross[2] > 0))
	{
		const float span = portalTmax-portalTmin;
		const float shiftAmount = maxAlign*rdMathFabsf(cross[2]) * span;

		if (cross[2] < 0)
		{
			outPortalTmin = rdMin(portalTmax, portalTmin+shiftAmount);
			outPortalTmax = rdMin(1.0f, portalTmax+shiftAmount);
		}
		else // On right side.
		{
			outPortalTmin = rdMax(0.0f, portalTmin-shiftAmount);
			outPortalTmax = rdMax(portalTmin, portalTmax-shiftAmount);
		}
	}
	else
	{
		outPortalTmin = portalTmin;
		outPortalTmax = portalTmax;
	}
}

inline int computeTileHash(int x, int y, const int mask)
{
	const unsigned int h1 = 0x8da6b343; // Large multiplicative constants;
	const unsigned int h2 = 0xd8163841; // here arbitrarily chosen primes
	unsigned int n = h1 * x + h2 * y;
	return (int)(n & mask);
}

unsigned int dtMeshTile::allocLink()
{
	if (linksFreeList == DT_NULL_LINK)
		return DT_NULL_LINK;
	unsigned int link = linksFreeList;
	linksFreeList = links[link].next;
	return link;
}

void dtMeshTile::freeLink(unsigned int link)
{
	links[link].next = linksFreeList;
	linksFreeList = link;
}

bool dtMeshTile::linkCountAvailable(const int count) const
{
	rdAssert(count > 0);
	unsigned int link = linksFreeList;

	if (link == DT_NULL_LINK)
		return false;

	for (int i = 1; i < count; i++)
	{
		link = links[link].next;

		if (link == DT_NULL_LINK)
			return false;
	}

	return true;
}

void dtMeshTile::getTightBounds(float* bminOut, float* bmaxOut) const
{
	float hmin = FLT_MAX;
	float hmax = -FLT_MAX;

	if (detailVerts && header->detailVertCount)
	{
		for (int i = 0; i < header->detailVertCount; ++i)
		{
			const float h = detailVerts[i*3+2];
			hmin = rdMin(hmin, h);
			hmax = rdMax(hmax, h);
		}
	}
	else
	{
		for (int i = 0; i < header->vertCount; ++i)
		{
			const float h = verts[i*3+2];
			hmin = rdMin(hmin, h);
			hmax = rdMax(hmax, h);
		}
	}

	hmin -= header->walkableClimb;
	hmax += header->walkableClimb;

	rdVcopy(bminOut, header->bmin);
	rdVcopy(bmaxOut, header->bmax);

	bminOut[2] = hmin;
	bmaxOut[2] = hmax;
}

int dtCalcTraverseTableCellIndex(const int numPolyGroups,
	const unsigned short polyGroup1, const unsigned short polyGroup2)
{
	return polyGroup1*((numPolyGroups+(RD_BITS_PER_BIT_CELL-1))/RD_BITS_PER_BIT_CELL)+(polyGroup2/RD_BITS_PER_BIT_CELL);
}

int dtCalcTraverseTableSize(const int numPolyGroups)
{
	return sizeof(int)*(numPolyGroups*((numPolyGroups+(RD_BITS_PER_BIT_CELL-1))/RD_BITS_PER_BIT_CELL));
}

dtNavMesh* dtAllocNavMesh()
{
	void* mem = rdAlloc(sizeof(dtNavMesh), RD_ALLOC_PERM);
	if (!mem) return 0;
	return new(mem) dtNavMesh;
}

/// @par
///
/// This function will only free the memory for tiles with the #DT_TILE_FREE_DATA
/// flag set.
void dtFreeNavMesh(dtNavMesh* navmesh)
{
	if (!navmesh) return;
	navmesh->~dtNavMesh();
	rdFree(navmesh);
}

//////////////////////////////////////////////////////////////////////////////////////////

/**
@class dtNavMesh

The navigation mesh consists of one or more tiles defining three primary types of structural data:

A polygon mesh which defines most of the navigation graph. (See rcPolyMesh for its structure.)
A detail mesh used for determining surface height on the polygon mesh. (See rcPolyMeshDetail for its structure.)
Off-mesh connections, which define custom point-to-point edges within the navigation graph.

The general build process is as follows:

-# Create rcPolyMesh and rcPolyMeshDetail data using the Recast build pipeline.
-# Optionally, create off-mesh connection data.
-# Combine the source data into a dtNavMeshCreateParams structure.
-# Create a tile data array using dtCreateNavMeshData().
-# Allocate at dtNavMesh object and initialize it. (For single tile navigation meshes,
   the tile data is loaded during this step.)
-# For multi-tile navigation meshes, load the tile data using dtNavMesh::addTile().

Notes:

- This class is usually used in conjunction with the dtNavMeshQuery class for pathfinding.
- Technically, all navigation meshes are tiled. A 'solo' mesh is simply a navigation mesh initialized 
  to have only a single tile.
- This class does not implement any asynchronous methods. So the ::dtStatus result of all methods will 
  always contain either a success or failure flag.

@see dtNavMeshQuery, dtCreateNavMeshData, dtNavMeshCreateParams, #dtAllocNavMesh, #dtFreeNavMesh
*/

dtNavMesh::dtNavMesh() :
	m_tileWidth(0),
	m_tileHeight(0),
	m_tileCount(0),
	m_maxTiles(0),
	m_tileLutSize(0),
	m_tileLutMask(0),
	m_posLookup(0),
	m_nextFree(0),
	m_tiles(0),
	m_traverseTables(0),
	m_hints(0),
	m_unused0(0),
	m_unused1(0)
{
#ifndef DT_POLYREF64
	m_saltBits = 0;
	m_tileBits = 0;
	m_polyBits = 0;
#endif
	memset(&m_params, 0, sizeof(dtNavMeshParams));
	rdVset(m_orig, 0.0f,0.0f,0.0f);
}

dtNavMesh::~dtNavMesh()
{
	for (int i = 0; i < m_maxTiles; ++i)
	{
		dtMeshTile& tile = m_tiles[i];
		const int flags = tile.flags;

		if (flags & DT_TILE_FREE_DATA)
		{
			if (flags & DT_CELL_FREE_DATA)
				rdFree(tile.cells);

			rdFree(tile.data);
			tile.data = 0;
			tile.dataSize = 0;

			if (tile.tracker)
				tile.tracker->destroy(1);
		}
	}

	rdFree(m_posLookup);
	rdFree(m_tiles);

	freeTraverseTables();
#if DT_NAVMESH_SET_VERSION >= 7
	freeHints();
#endif
}

dtStatus dtNavMesh::init(const dtNavMeshParams* params)
{
	memcpy(&m_params, params, sizeof(dtNavMeshParams));
	rdVcopy(m_orig, params->orig);

	m_tileWidth = params->tileWidth;
	m_tileHeight = params->tileHeight;

	// Init tiles
	m_maxTiles = params->maxTiles;
	m_tileLutSize = rdNextPow2(params->maxTiles/4);
	if (!m_tileLutSize) m_tileLutSize = 1;
	m_tileLutMask = m_tileLutSize-1;
	
	m_tiles = (dtMeshTile*)rdAlloc(sizeof(dtMeshTile)*m_maxTiles, RD_ALLOC_PERM);
	if (!m_tiles)
		return DT_FAILURE | DT_OUT_OF_MEMORY;
	m_posLookup = (dtMeshTile**)rdAlloc(sizeof(dtMeshTile*)*m_tileLutSize, RD_ALLOC_PERM);
	if (!m_posLookup)
		return DT_FAILURE | DT_OUT_OF_MEMORY;
	memset(m_tiles, 0, sizeof(dtMeshTile) * m_maxTiles);
	memset(m_posLookup, 0, sizeof(dtMeshTile*) * m_tileLutSize);

	const int traverseTableCount = params->traverseTableCount;
	if (traverseTableCount)
	{
		if (!allocTraverseTables(params->traverseTableCount))
			return DT_FAILURE | DT_OUT_OF_MEMORY;
	}

	m_nextFree = 0;
	for (int i = m_maxTiles-1; i >= 0; --i)
	{
		m_tiles[i].salt = 1;
		m_tiles[i].next = m_nextFree;
		m_nextFree = &m_tiles[i];
	}

	// Init ID generator values.
#ifndef DT_POLYREF64
	m_tileBits = rdIlog2(rdNextPow2((unsigned int)params->maxTiles));
	m_polyBits = rdIlog2(rdNextPow2((unsigned int)params->maxPolys));
	m_saltBits = 32 - m_tileBits - m_polyBits;
#endif
	
	return DT_SUCCESS;
}

dtStatus dtNavMesh::init(unsigned char* data, const int dataSize, const int tableCount, const int flags)
{
	// Make sure the data is in right format.
	dtMeshHeader* header = (dtMeshHeader*)data;
	if (header->magic != DT_NAVMESH_MAGIC)
		return DT_FAILURE | DT_WRONG_MAGIC;
	if (header->version != DT_NAVMESH_VERSION)
		return DT_FAILURE | DT_WRONG_VERSION;

	dtNavMeshParams params;
	rdVcopy(params.orig, header->bmin);
	params.orig[0] = header->bmax[0];
	params.tileWidth = header->bmax[0] - header->bmin[0];
	params.tileHeight = header->bmax[1] - header->bmin[1];
	params.maxTiles = 1;
	params.maxPolys = header->polyCount;
	params.polyGroupCount = 0;
	params.traverseTableSize = 0;
	params.traverseTableCount = tableCount;
#if DT_NAVMESH_SET_VERSION >= 7
	params.hintCount = 0;
#endif
	
	dtStatus status = init(&params);
	if (dtStatusFailed(status))
		return status;

	dtTileRef tileRef;
	status = addTile(data,dataSize,flags,0,&tileRef);

	if (dtStatusFailed(status))
		return status;

	return connectTile(tileRef);
}

//////////////////////////////////////////////////////////////////////////////////////////
int dtNavMesh::findConnectingPolys(const float* va, const float* vb,
								   const dtMeshTile* tile, int side,
								   dtPolyRef* con, float* conarea, int maxcon) const
{
	const float near_thresh = 0.01f;

	if (!tile) return 0;
	
	float amin[2], amax[2];
	calcSlabEndPoints(va, vb, amin, amax, side);
	const float apos = getSlabCoord(va, side);

	// Remove links pointing to 'side' and compact the links array. 
	float bmin[2], bmax[2];
	unsigned short m = DT_EXT_LINK | (unsigned short)side;
	int n = 0;
	
	dtPolyRef base = getPolyRefBase(tile);
	
	for (int i = 0; i < tile->header->polyCount; ++i)
	{
		dtPoly* poly = &tile->polys[i];
		const int nv = poly->vertCount;
		for (int j = 0; j < nv; ++j)
		{
			// Skip edges which do not point to the right side.
			if (poly->neis[j] != m) continue;
			
			const float* vc = &tile->verts[poly->verts[j]*3];
			const float* vd = &tile->verts[poly->verts[(j+1) % nv]*3];
			const float bpos = getSlabCoord(vc, side);
			
			// Segments are not close enough.
			if (rdAbs(apos-bpos) > near_thresh)
				continue;
			
			// Check if the segments touch.
			calcSlabEndPoints(vc,vd, bmin,bmax, side);
			
			if (!overlapSlabs(amin,amax, bmin,bmax, near_thresh, tile->header->walkableClimb)) continue;
			
			// Add return value.
			if (n < maxcon)
			{
				conarea[n*2+0] = rdMax(amin[0], bmin[0]);
				conarea[n*2+1] = rdMin(amax[0], bmax[0]);
				con[n] = base | (dtPolyRef)i;
				n++;
			}
			break;
		}
	}
	return n;
}

void dtNavMesh::unconnectLinks(dtMeshTile* tile, dtMeshTile* target)
{
	if (!tile || !target) return;

	const unsigned int targetNum = decodePolyIdTile(getTileRef(target));

	for (int i = 0; i < tile->header->polyCount; ++i)
	{
		dtPoly* poly = &tile->polys[i];
		unsigned int j = poly->firstLink;
		unsigned int pj = DT_NULL_LINK;
		while (j != DT_NULL_LINK)
		{
			if (decodePolyIdTile(tile->links[j].ref) == targetNum)
			{
				// Remove link.
				unsigned int nj = tile->links[j].next;
				if (pj == DT_NULL_LINK)
					poly->firstLink = nj;
				else
					tile->links[pj].next = nj;
				tile->freeLink(j);
				j = nj;
			}
			else
			{
				// Advance
				pj = j;
				j = tile->links[j].next;
			}
		}
	}
}

void dtNavMesh::connectExtLinks(dtMeshTile* tile, dtMeshTile* target, int side)
{
	if (!tile) return;
	
	// Connect border links.
	for (int i = 0; i < tile->header->polyCount; ++i)
	{
		dtPoly* poly = &tile->polys[i];

		// Create new links.
//		unsigned short m = DT_EXT_LINK | (unsigned short)side;
		
		const int nv = poly->vertCount;
		for (int j = 0; j < nv; ++j)
		{
			// Skip non-portal edges.
			if ((poly->neis[j] & DT_EXT_LINK) == 0)
				continue;
			
			const int dir = (int)(poly->neis[j] & 0xff);
			if (side != -1 && dir != side)
				continue;
			
			// Create new links
			const float* va = &tile->verts[poly->verts[j]*3];
			const float* vb = &tile->verts[poly->verts[(j+1) % nv]*3];
			dtPolyRef nei[4];
			float neia[4*2];
			int nnei = findConnectingPolys(va,vb, target, rdOppositeTile(dir), nei,neia,4);

			// If the portal edge has no neighbor, mark the edge as a boundary edge.
			if (!nnei)
			{
				poly->neis[j] = 0;
				continue;
			}

			for (int k = 0; k < nnei; ++k)
			{
				unsigned int idx = tile->allocLink();
				if (idx != DT_NULL_LINK)
				{
					dtLink* link = &tile->links[idx];
					link->ref = nei[k];
					link->edge = (unsigned char)j;
					link->side = (unsigned char)dir;
					
					link->next = poly->firstLink;
					poly->firstLink = idx;

					link->traverseType = DT_NULL_TRAVERSE_TYPE;
					link->traverseDist = 0;
					link->reverseLink = DT_NULL_TRAVERSE_REVERSE_LINK;

					// Compress portal limits to a byte value.
					if (dir == 0 || dir == 4)
					{
						float tmin = (neia[k*2+0]-va[1]) / (vb[1]-va[1]);
						float tmax = (neia[k*2+1]-va[1]) / (vb[1]-va[1]);
						if (tmin > tmax)
							rdSwap(tmin,tmax);
						link->bmin = (unsigned char)rdMathRoundf(rdClamp(tmin, 0.0f, 1.0f)*255.0f);
						link->bmax = (unsigned char)rdMathRoundf(rdClamp(tmax, 0.0f, 1.0f)*255.0f);
					}
					else if (dir == 2 || dir == 6)
					{
						float tmin = (neia[k*2+0]-va[0]) / (vb[0]-va[0]);
						float tmax = (neia[k*2+1]-va[0]) / (vb[0]-va[0]);
						if (tmin > tmax)
							rdSwap(tmin,tmax);
						link->bmin = (unsigned char)rdMathRoundf(rdClamp(tmin, 0.0f, 1.0f)*255.0f);
						link->bmax = (unsigned char)rdMathRoundf(rdClamp(tmax, 0.0f, 1.0f)*255.0f);
					}
				}
			}
		}
	}
}

dtPolyRef dtNavMesh::clampOffMeshVertToPoly(dtOffMeshConnection* con, dtMeshTile* conTile, 
	const dtMeshTile* lookupTile, const bool start)
{
	const float* p = start ? &con->pos[0] : &con->pos[3];
	const float halfExtents[3] = { con->rad, con->rad, con->rad };

	float nearestPt[3];
	dtPolyRef ref = findNearestPolyInTile(lookupTile, p, halfExtents, nearestPt);
	if (!ref)
		return 0;
	// findNearestPoly may return too optimistic results, further check to make sure. 
	if (rdSqr(nearestPt[0]-p[0])+rdSqr(nearestPt[1]-p[1]) > rdSqr(con->rad))
		return 0;

	const dtPoly* poly = &conTile->polys[con->poly];

	// Make sure the location is on current mesh.
	float* polyVert = &conTile->verts[poly->verts[start?0:1]*3];
	rdVcopy(polyVert, nearestPt);

	if (start)
	{
		// Offset the ref position towards the new start point.
		float offset[3];

		rdVsub(offset, con->refPos, con->pos);
		rdVadd(con->refPos, nearestPt, offset);
	}

	// Update the off-mesh connection positions as well.
	float* conPos = &con->pos[start?0:3];
	rdVcopy(conPos, nearestPt);

	return ref;
}

static bool connectOffMeshLink(dtMeshTile* tile, dtPoly* fromPoly, const dtPolyRef toPolyRef, const unsigned char side,
	unsigned char edge, unsigned char traverseType, unsigned char order)
{
	unsigned int idx = tile->allocLink();

	if (idx == DT_NULL_LINK)
		return false;

	dtLink* link = &tile->links[idx];
	link->ref = toPolyRef;
	link->edge = edge;
	link->side = side;
	link->bmin = link->bmax = 0;
	// Add to linked list.
	link->next = fromPoly->firstLink;
	fromPoly->firstLink = idx;
	link->traverseType = traverseType | order;
	link->traverseDist = 0;
	link->reverseLink = DT_NULL_TRAVERSE_REVERSE_LINK;

	return true;
}

dtStatus dtNavMesh::connectOffMeshLinks(const dtTileRef tileRef)
{
	const int tileIndex = (int)decodePolyIdTile((dtPolyRef)tileRef);
	if (tileIndex >= m_maxTiles)
		return DT_FAILURE | DT_OUT_OF_MEMORY;

	dtMeshTile* tile = &m_tiles[tileIndex];
	const dtMeshHeader* header = tile->header;

	dtPolyRef base = getPolyRefBase(tile);

	for (int i = 0; i < header->offMeshConCount; ++i)
	{
		// Base off-mesh connection start points.
		dtOffMeshConnection* con = &tile->offMeshCons[i];
		dtPoly* conPoly = &tile->polys[con->poly];

		const dtPolyRef basePolyRef = clampOffMeshVertToPoly(con, tile, tile, true);

		if (!basePolyRef)
			continue;

		const unsigned char traverseType = con->getTraverseType();
		const bool invertVertLookup = con->getVertLookupOrder();

#if DT_NAVMESH_SET_VERSION >= 7
		// NOTE: need to remove the vert lookup inversion flag from here as the
		// engine uses this value directly to index into the activity array.
		con->setTraverseType(traverseType, 0);
#endif

		// Link off-mesh connection to target poly.
		if (!connectOffMeshLink(tile, conPoly, basePolyRef, 0xff, 0, DT_NULL_TRAVERSE_TYPE, 0))
			return DT_FAILURE | DT_OUT_OF_MEMORY;

		// Start end-point is always connect back to off-mesh connection.
		const unsigned int basePolyIdx = decodePolyIdPoly(basePolyRef);
		dtPoly* basePoly = &tile->polys[basePolyIdx];

		const dtPolyRef conPolyRef = base | (dtPolyRef)(con->poly);

		if (!connectOffMeshLink(tile, basePoly, conPolyRef, 0xff, 0xff, traverseType,
			invertVertLookup ? DT_OFFMESH_CON_TRAVERSE_ON_VERT : DT_OFFMESH_CON_TRAVERSE_ON_POLY))
			return DT_FAILURE | DT_OUT_OF_MEMORY;

		// connect to land points.
		const float halfExtents[3] = { con->rad, con->rad, con->rad };
		float bmin[3], bmax[3];
		rdVsub(bmin, &con->pos[3], halfExtents);
		rdVadd(bmax, &con->pos[3], halfExtents);

		// Find tiles the query touches.
		int tx, ty;
		calcTileLoc(&con->pos[3], &tx, &ty);

		static const int MAX_NEIS = 32;
		dtMeshTile* neis[MAX_NEIS];

		const int nneis = getTilesAt(tx, ty, neis, MAX_NEIS);

		const unsigned char side = rdClassifyPointOutsideBounds(&con->pos[3], header->bmin, header->bmax);
		const unsigned char oppositeSide = (side == 0xff) ? 0xff : rdOppositeTile(side);

		for (int j = 0; j < nneis; ++j)
		{
			dtMeshTile* neiTile = neis[j];

			// Find polygon to connect to.
			const dtPolyRef landPolyRef = clampOffMeshVertToPoly(con, tile, neiTile, false);

			if (!landPolyRef)
				continue;

			// Link off-mesh connection to target poly.
			if (!connectOffMeshLink(tile, conPoly, landPolyRef, oppositeSide, 1, DT_NULL_TRAVERSE_TYPE, 0))
				return DT_FAILURE | DT_OUT_OF_MEMORY;

			// Link target poly to off-mesh connection.
#if DT_NAVMESH_SET_VERSION < 7
			if (con->flags & DT_OFFMESH_CON_BIDIR)
#endif
			{
				const unsigned int landPolyIdx = decodePolyIdPoly(landPolyRef);
				dtPoly* landPoly = &neiTile->polys[landPolyIdx];

				if (!connectOffMeshLink(neiTile, landPoly, conPolyRef, side, 0xff, traverseType,
					invertVertLookup ? DT_OFFMESH_CON_TRAVERSE_ON_POLY : DT_OFFMESH_CON_TRAVERSE_ON_VERT))
					return DT_FAILURE | DT_OUT_OF_MEMORY;
			}

			// Off-mesh link is fully linked, mark it.
			conPoly->flags |= DT_POLYFLAGS_JUMP_LINKED;

			// All links have been established, break out entirely.
			break;
		}
	}

	return DT_SUCCESS;
}

void dtNavMesh::connectIntLinks(dtMeshTile* tile)
{
	if (!tile) return;

	dtPolyRef base = getPolyRefBase(tile);

	for (int i = 0; i < tile->header->polyCount; ++i)
	{
		dtPoly* poly = &tile->polys[i];
		poly->firstLink = DT_NULL_LINK;

		if (poly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
			continue;
			
		// Build edge links backwards so that the links will be
		// in the linked list from lowest index to highest.
		for (int j = poly->vertCount-1; j >= 0; --j)
		{
			// Skip hard and non-internal edges.
			if (poly->neis[j] == 0 || (poly->neis[j] & DT_EXT_LINK)) continue;

			unsigned int idx = tile->allocLink();
			if (idx != DT_NULL_LINK)
			{
				dtLink* link = &tile->links[idx];
				link->ref = base | (dtPolyRef)(poly->neis[j]-1);
				link->edge = (unsigned char)j;
				link->side = 0xff;
				link->bmin = link->bmax = 0;
				// Add to linked list.
				link->next = poly->firstLink;
				poly->firstLink = idx;
				link->traverseType = DT_NULL_TRAVERSE_TYPE;
				link->traverseDist = 0;
				link->reverseLink = DT_NULL_TRAVERSE_REVERSE_LINK;
			}
		}
	}
}

dtStatus dtNavMesh::connectTraverseLinks(const dtTileRef tileRef, const dtTraverseLinkConnectParams& params)
{
	const int tileIndex = (int)decodePolyIdTile((dtPolyRef)tileRef);
	if (tileIndex >= m_maxTiles)
		return DT_FAILURE | DT_OUT_OF_MEMORY;

	dtMeshTile* const baseTile = &m_tiles[tileIndex];
	const dtMeshHeader* const baseHeader = baseTile->header;

	if (!baseHeader)
		return DT_FAILURE | DT_INVALID_PARAM; // Invalid tile.

	if (!baseHeader->detailMeshCount)
		return DT_FAILURE | DT_INVALID_PARAM; // Detail meshes are required for traverse links.

	// If we link to the same tile, we need at least 2 links.
	if (!baseTile->linkCountAvailable(params.linkToNeighbor ? 1 : 2))
		return DT_FAILURE | DT_OUT_OF_MEMORY;

	static const float detailEdgeAlignThresh = 0.01f*0.01f;

	const dtPolyRef basePolyRefBase = getPolyRefBase(baseTile);
	bool firstBaseTileLinkUsed = false;

	for (int i = 0; i < baseHeader->polyCount; ++i)
	{
		dtPoly* const basePoly = &baseTile->polys[i];

		if (basePoly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
			continue;

		dtPolyDetail* const baseDetail = &baseTile->detailMeshes[i];

		for (int j = 0; j < basePoly->vertCount; ++j)
		{
			// Hard edges only!
			if (basePoly->neis[j] != 0)
				continue;

			// Polygon 1 edge
			const float* const basePolySpos = &baseTile->verts[basePoly->verts[j]*3];
			const float* const basePolyEpos = &baseTile->verts[basePoly->verts[(j+1)%basePoly->vertCount]*3];

			for (int k = 0; k < baseDetail->triCount; ++k)
			{
				const unsigned char* baseTri = &baseTile->detailTris[(baseDetail->triBase+k)*4];
				const float* baseTriVerts[3];
				for (int l = 0; l < 3; ++l)
				{
					if (baseTri[l] < basePoly->vertCount)
						baseTriVerts[l] = &baseTile->verts[basePoly->verts[baseTri[l]]*3];
					else
						baseTriVerts[l] = &baseTile->detailVerts[(baseDetail->vertBase+(baseTri[l]-basePoly->vertCount))*3];
				}
				for (int l = 0, m = 2; l < 3; m = l++)
				{
					if ((dtGetDetailTriEdgeFlags(baseTri[3], m) & RD_DETAIL_EDGE_BOUNDARY) == 0)
						continue;

					if (rdDistancePtLine2D(baseTriVerts[m], basePolySpos, basePolyEpos) >= detailEdgeAlignThresh ||
						rdDistancePtLine2D(baseTriVerts[l], basePolySpos, basePolyEpos) >= detailEdgeAlignThresh)
						continue;

					const int MAX_NEIS = 32; // Max neighbors
					dtMeshTile* neis[MAX_NEIS];

					int nneis = 0;

					if (params.linkToNeighbor) // Retrieve the neighboring tiles.
					{
						// Get the neighboring tiles starting from north in the compass rose.
						// It is possible we don't end up linking to some of these tiles if
						// we happen to run out of links on the base tile.
						for (int n = 0; n < 8; ++n)
						{
							const int numSlotsLeft = MAX_NEIS-nneis;

							if (!numSlotsLeft)
								break;

							nneis += getNeighbourTilesAt(baseHeader->x, baseHeader->y, n, &neis[nneis], numSlotsLeft);
						}

						// No neighbors, nothing to link to.
						if (!nneis)
							continue;
					}
					else
					{
						// Internal links.
						nneis = 1;
						neis[0] = baseTile;
					}

					const float* baseDetailPolyEdgeSpos = baseTriVerts[m];
					const float* baseDetailPolyEdgeEpos = baseTriVerts[l];

					float basePolyEdgeMid[3];
					rdVsad(basePolyEdgeMid, baseDetailPolyEdgeSpos, baseDetailPolyEdgeEpos, 0.5f);

					float baseEdgeDir[3];
					rdVsub(baseEdgeDir, baseDetailPolyEdgeEpos, baseDetailPolyEdgeSpos);

					float baseEdgeNorm[3];
					rdCalcEdgeNormal2D(baseEdgeDir, baseEdgeNorm);

					float baseTmin;
					float baseTmax;
					rdCalcSubEdgeArea2D(basePolySpos, basePolyEpos, baseDetailPolyEdgeSpos, baseDetailPolyEdgeEpos, baseTmin, baseTmax);

					for (int n = nneis - 1; n >= 0; --n)
					{
						dtMeshTile* landTile = neis[n];
						const bool sameTile = baseTile == landTile;

						// Don't connect to same tile edges yet, leave that for the second pass.
						if (params.linkToNeighbor && sameTile)
							continue;

						const dtMeshHeader* landHeader = landTile->header;

						if (!landHeader->detailMeshCount)
							continue; // Detail meshes are required for traverse links.

						// Skip same polygon.
						if (sameTile && i == n)
							continue;

						if (!landTile->linkCountAvailable(1))
							continue;

						const dtPolyRef landPolyRefBase = getPolyRefBase(landTile);
						bool firstLandTileLinkUsed = false;

						bool moveToNextTile = false;

						for (int o = 0; (o < landHeader->polyCount) && !moveToNextTile; ++o)
						{
							dtPoly* const landPoly = &landTile->polys[o];

							if (landPoly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
								continue;

							if (landPoly == basePoly)
								continue;

							// If both polygons are sharing an edge, we should not establish the link as
							// it will cause pathfinding to fail in this area when both polygons have
							// their first link set to another; the path will never exit these polygons.
							if (arePolysAdjacent(basePoly, baseTile, landPoly, landTile))
								continue;

							dtPolyDetail* const landDetail = &landTile->detailMeshes[o];

							for (int p = 0; (p < landPoly->vertCount) && !moveToNextTile; ++p)
							{
								if (landPoly->neis[p] != 0)
									continue;

								// Polygon 2 edge
								const float* const landPolySpos = &landTile->verts[landPoly->verts[p]*3];
								const float* const landPolyEpos = &landTile->verts[landPoly->verts[(p+1)%landPoly->vertCount]*3];

								for (int q = 0; (q < landDetail->triCount) && !moveToNextTile; ++q)
								{
									const unsigned char* landTri = &landTile->detailTris[(landDetail->triBase+q)*4];
									rdAssert(landTri != baseTri);

									const float* landTriVerts[3];
									for (int r = 0; r < 3; ++r)
									{
										if (landTri[r] < landPoly->vertCount)
											landTriVerts[r] = &landTile->verts[landPoly->verts[landTri[r]]*3];
										else
											landTriVerts[r] = &landTile->detailVerts[(landDetail->vertBase+(landTri[r]-landPoly->vertCount))*3];
									}
									for (int r = 0, s = 2; r < 3; s = r++)
									{
										// We need at least 2 links available, figure out if
										// we link to the same tile or another one.
										if (params.linkToNeighbor)
										{
											if (firstLandTileLinkUsed && !landTile->linkCountAvailable(1))
											{
												moveToNextTile = true;
												break;
											}

											else if (firstBaseTileLinkUsed && !baseTile->linkCountAvailable(1))
												return DT_FAILURE | DT_OUT_OF_MEMORY;
										}
										else if (firstBaseTileLinkUsed && !baseTile->linkCountAvailable(2))
											return DT_FAILURE | DT_OUT_OF_MEMORY;

										if ((dtGetDetailTriEdgeFlags(landTri[3], s) & RD_DETAIL_EDGE_BOUNDARY) == 0)
											continue;

										if (rdDistancePtLine2D(landTriVerts[s], landPolySpos, landPolyEpos) >= detailEdgeAlignThresh ||
											rdDistancePtLine2D(landTriVerts[r], landPolySpos, landPolyEpos) >= detailEdgeAlignThresh)
											continue;

										const float* landDetailPolyEdgeSpos = landTriVerts[s];
										const float* landDetailPolyEdgeEpos = landTriVerts[r];

										float landPolyEdgeMid[3];
										rdVsad(landPolyEdgeMid, landDetailPolyEdgeSpos, landDetailPolyEdgeEpos, 0.5f);

										const float dist = dtCalcLinkDistance(basePolyEdgeMid, landPolyEdgeMid);
										const unsigned char quantDist = dtQuantLinkDistance(dist);

										if (quantDist == 0)
											continue; // Link distance is greater than maximum supported.

										float landEdgeDir[3];
										rdVsub(landEdgeDir, landDetailPolyEdgeEpos, landDetailPolyEdgeSpos);

										const float elevation = rdMathFabsf(basePolyEdgeMid[2] - landPolyEdgeMid[2]);
										const float slopeAngle = rdMathFabsf(rdCalcSlopeAngle(basePolyEdgeMid, landPolyEdgeMid));
										const bool baseOverlaps = rdCalcEdgeOverlap2D(baseDetailPolyEdgeSpos, baseDetailPolyEdgeEpos, landDetailPolyEdgeSpos, landDetailPolyEdgeEpos, baseEdgeDir) > params.minEdgeOverlap;
										const bool landOverlaps = rdCalcEdgeOverlap2D(landDetailPolyEdgeSpos, landDetailPolyEdgeEpos, baseDetailPolyEdgeSpos, baseDetailPolyEdgeEpos, landEdgeDir) > params.minEdgeOverlap;

										const unsigned char traverseType = params.getTraverseType(params.userData, dist, elevation, slopeAngle, baseOverlaps, landOverlaps);

										if (traverseType == DT_NULL_TRAVERSE_TYPE)
											continue;

										const dtPolyRef basePolyRef = basePolyRefBase | i;
										const dtPolyRef landPolyRef = landPolyRefBase | o;

										unsigned int* const linkedTraverseType = params.findPolyLink(params.userData, basePolyRef, landPolyRef);

										if (params.singlePortalPerPair && linkedTraverseType)
											continue; // User has specified to limit link count between 2 polygons to 1.

										if (linkedTraverseType && (rdBitCellBit(traverseType) & *linkedTraverseType))
											continue; // These 2 polygons are already linked with the same traverse type.

										float landEdgeNorm[3];
										rdCalcEdgeNormal2D(landEdgeDir, landEdgeNorm);

										const bool basePolyHigher = basePolyEdgeMid[2] > landPolyEdgeMid[2];
										const float* const lowerEdgeMid = basePolyHigher ? landPolyEdgeMid : basePolyEdgeMid;
										const float* const higherEdgeMid = basePolyHigher ? basePolyEdgeMid : landPolyEdgeMid;
										const float* const lowerEdgeNorm = basePolyHigher ? landEdgeNorm : baseEdgeNorm;
										const float* const higherEdgeNorm = basePolyHigher ? baseEdgeNorm : landEdgeNorm;

										const float walkableHeight = basePolyHigher ? baseHeader->walkableHeight : landHeader->walkableHeight;
										const float walkableRadius = basePolyHigher ? baseHeader->walkableRadius : landHeader->walkableRadius;

										if (!params.traverseLinkInLOS(params.userData, lowerEdgeMid, higherEdgeMid, lowerEdgeNorm, higherEdgeNorm, walkableHeight, walkableRadius, slopeAngle))
											continue;

										const unsigned char landSide = params.linkToNeighbor
											? rdClassifyPointOutsideBounds(landPolyEdgeMid, baseHeader->bmin, baseHeader->bmax)
											: rdClassifyPointInsideBounds(landPolyEdgeMid, landHeader->bmin, landHeader->bmax);
										const unsigned char baseSide = rdOppositeTile(landSide);

										float landTmin;
										float landTmax;
										rdCalcSubEdgeArea2D(landPolySpos, landPolyEpos, landDetailPolyEdgeSpos, landDetailPolyEdgeEpos, landTmin, landTmax);

										float newLandTmin;
										float newLandTmax;
										alignPortalLimits(landPolyEdgeMid, landEdgeNorm, basePolyEdgeMid, landTmin, landTmax, newLandTmin, newLandTmax, params.maxPortalAlign);

										float newBaseTmin;
										float newBaseTmax;
										alignPortalLimits(basePolyEdgeMid, baseEdgeNorm, landPolyEdgeMid, baseTmin, baseTmax, newBaseTmin, newBaseTmax, params.maxPortalAlign);

										const unsigned int forwardIdx = baseTile->allocLink();
										const unsigned int reverseIdx = landTile->allocLink();

										// Allocated 2 new links, need to check for enough space on subsequent runs.
										// This optimization saves a lot of time generating navmeshes for larger or
										// more complicated geometry.
										firstBaseTileLinkUsed = true;
										firstLandTileLinkUsed = true;

										dtLink* const forwardLink = &baseTile->links[forwardIdx];

										forwardLink->ref = landPolyRef;
										forwardLink->edge = (unsigned char)j;
										forwardLink->side = landSide;
										forwardLink->bmin = (unsigned char)rdMathRoundf(newBaseTmin*255.f);
										forwardLink->bmax = (unsigned char)rdMathRoundf(newBaseTmax*255.f);
										forwardLink->next = basePoly->firstLink;
										basePoly->firstLink = forwardIdx;
										forwardLink->traverseType = (unsigned char)traverseType;
										forwardLink->traverseDist = quantDist;
										forwardLink->reverseLink = (unsigned short)reverseIdx;

										dtLink* const reverseLink = &landTile->links[reverseIdx];

										reverseLink->ref = basePolyRef;
										reverseLink->edge = (unsigned char)p;
										reverseLink->side = baseSide;
										reverseLink->bmin = (unsigned char)rdMathRoundf(newLandTmin*255.f);
										reverseLink->bmax = (unsigned char)rdMathRoundf(newLandTmax*255.f);
										reverseLink->next = landPoly->firstLink;
										landPoly->firstLink = reverseIdx;
										reverseLink->traverseType = (unsigned char)traverseType;
										reverseLink->traverseDist = quantDist;
										reverseLink->reverseLink = (unsigned short)forwardIdx;

										if (linkedTraverseType)
											*linkedTraverseType |= 1<<traverseType;
										else
										{
											const int ret = params.addPolyLink(params.userData, basePolyRef, landPolyRef, 1<<traverseType);

											if (ret < 0)
												return DT_FAILURE | DT_OUT_OF_MEMORY;
											if (ret > 0)
												return DT_FAILURE | DT_INVALID_PARAM;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return DT_SUCCESS;
}

namespace
{
	template<bool onlyBoundary>
	void closestPointOnDetailEdges(const dtMeshTile* tile, const dtPoly* poly, const float* pos, float* closest)
	{
		const unsigned int ip = (unsigned int)(poly - tile->polys);
		const dtPolyDetail* pd = &tile->detailMeshes[ip];

		float dmin = FLT_MAX;
		float tmin = 0;
		const float* pmin = 0;
		const float* pmax = 0;

		for (int i = 0; i < pd->triCount; i++)
		{
			const unsigned char* tris = &tile->detailTris[(pd->triBase + i) * 4];
			const int ANY_BOUNDARY_EDGE =
				(RD_DETAIL_EDGE_BOUNDARY << 0) |
				(RD_DETAIL_EDGE_BOUNDARY << 2) |
				(RD_DETAIL_EDGE_BOUNDARY << 4);
			if (onlyBoundary && (tris[3] & ANY_BOUNDARY_EDGE) == 0)
				continue;

			const float* v[3];
			for (int j = 0; j < 3; ++j)
			{
				if (tris[j] < poly->vertCount)
					v[j] = &tile->verts[poly->verts[tris[j]] * 3];
				else
					v[j] = &tile->detailVerts[(pd->vertBase + (tris[j] - poly->vertCount)) * 3];
			}

			for (int k = 0, j = 2; k < 3; j = k++)
			{
				if ((dtGetDetailTriEdgeFlags(tris[3], j) & RD_DETAIL_EDGE_BOUNDARY) == 0 &&
					(onlyBoundary || tris[j] < tris[k]))
				{
					// Only looking at boundary edges and this is internal, or
					// this is an inner edge that we will see again or have already seen.
					continue;
				}

				float t;
				float d = rdDistancePtSegSqr2D(pos, v[j], v[k], t);
				if (d < dmin)
				{
					dmin = d;
					tmin = t;
					pmin = v[j];
					pmax = v[k];
				}
			}
		}

		rdVlerp(closest, pmin, pmax, tmin);
	}
}

bool dtNavMesh::getPolyHeight(const dtMeshTile* tile, const dtPoly* poly, const float* pos, float* height) const
{
	// Off-mesh connections do not have detail polys and getting height
	// over them does not make sense.
	if (poly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
		return false;

	const unsigned int ip = (unsigned int)(poly - tile->polys);
	const dtPolyDetail* pd = &tile->detailMeshes[ip];
	
	float verts[RD_VERTS_PER_POLYGON*3];	
	const int nv = poly->vertCount;
	for (int i = 0; i < nv; ++i)
		rdVcopy(&verts[i*3], &tile->verts[poly->verts[i]*3]);
	
	if (!rdPointInPolygon(pos, verts, nv))
		return false;

	if (!height)
		return true;
	
	// Find height at the location.
	for (int j = 0; j < pd->triCount; ++j)
	{
		const unsigned char* t = &tile->detailTris[(pd->triBase+j)*4];
		const float* v[3];
		for (int k = 0; k < 3; ++k)
		{
			if (t[k] < poly->vertCount)
				v[k] = &tile->verts[poly->verts[t[k]]*3];
			else
				v[k] = &tile->detailVerts[(pd->vertBase+(t[k]-poly->vertCount))*3];
		}
		float h;
		if (rdClosestHeightPointTriangle(pos, v[0], v[1], v[2], h))
		{
			*height = h;
			return true;
		}
	}

	// If all triangle checks failed above (can happen with degenerate triangles
	// or larger floating point values) the point is on an edge, so just select
	// closest. This should almost never happen so the extra iteration here is
	// ok.
	float closest[3];
	closestPointOnDetailEdges<false>(tile, poly, pos, closest);
	*height = closest[2];
	return true;
}

void dtNavMesh::closestPointOnPoly(dtPolyRef ref, const float* pos, float* closest, bool* posOverPoly) const
{
	const dtMeshTile* tile = 0;
	const dtPoly* poly = 0;
	getTileAndPolyByRefUnsafe(ref, &tile, &poly);

	rdVcopy(closest, pos);
	if (getPolyHeight(tile, poly, pos, &closest[2]))
	{
		if (posOverPoly)
			*posOverPoly = true;
		return;
	}

	if (posOverPoly)
		*posOverPoly = false;

	// Off-mesh connections don't have detail polygons.
	if (poly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
	{
		const float* v0 = &tile->verts[poly->verts[0]*3];
		const float* v1 = &tile->verts[poly->verts[1]*3];
		float t;
		rdDistancePtSegSqr2D(pos, v0, v1, t);
		rdVlerp(closest, v0, v1, t);
		return;
	}

	// Outside poly that is not an offmesh connection.
	closestPointOnDetailEdges<true>(tile, poly, pos, closest);
}

dtPolyRef dtNavMesh::findNearestPolyInTile(const dtMeshTile* tile,
										   const float* center, const float* halfExtents,
										   float* nearestPt) const
{
	float bmin[3], bmax[3];
	rdVsub(bmin, center, halfExtents);
	rdVadd(bmax, center, halfExtents);
	
	// Get nearby polygons from proximity grid.
	dtPolyRef polys[128];
	int polyCount = queryPolygonsInTile(tile, bmin, bmax, polys, 128);
	
	// Find nearest polygon amongst the nearby polygons.
	dtPolyRef nearest = 0;
	float nearestDistanceSqr = FLT_MAX;
	for (int i = 0; i < polyCount; ++i)
	{
		dtPolyRef ref = polys[i];
		float closestPtPoly[3];
		float diff[3];
		bool posOverPoly = false;
		float d;
		closestPointOnPoly(ref, center, closestPtPoly, &posOverPoly);

		// If a point is directly over a polygon and closer than
		// climb height, favor that instead of straight line nearest point.
		rdVsub(diff, center, closestPtPoly);
		if (posOverPoly)
		{
			d = rdAbs(diff[2]) - tile->header->walkableClimb;
			d = d > 0 ? d*d : 0;			
		}
		else
		{
			d = rdVlenSqr(diff);
		}
		
		if (d < nearestDistanceSqr)
		{
			rdVcopy(nearestPt, closestPtPoly);
			nearestDistanceSqr = d;
			nearest = ref;
		}
	}
	
	return nearest;
}

int dtNavMesh::queryPolygonsInTile(const dtMeshTile* tile, const float* qmin, const float* qmax,
								   dtPolyRef* polys, const int maxPolys) const
{
	if (tile->bvTree)
	{
		const dtBVNode* node = &tile->bvTree[0];
		const dtBVNode* end = &tile->bvTree[tile->header->bvNodeCount];
		const float* tbmin = tile->header->bmin;
		const float* tbmax = tile->header->bmax;
		const float qfac = tile->header->bvQuantFactor;
		
		// Calculate quantized box
		unsigned short bmin[3], bmax[3];
		// dtClamp query box to world box.
		float minx = -(rdClamp(qmax[0], tbmin[0], tbmax[0]) - tbmax[0]);
		float miny = rdClamp(qmin[1], tbmin[1], tbmax[1]) - tbmin[1];
		float minz = rdClamp(qmin[2], tbmin[2], tbmax[2]) - tbmin[2];
		float maxx = -(rdClamp(qmin[0], tbmin[0], tbmax[0]) - tbmax[0]);
		float maxy = rdClamp(qmax[1], tbmin[1], tbmax[1]) - tbmin[1];
		float maxz = rdClamp(qmax[2], tbmin[2], tbmax[2]) - tbmin[2];
		// Quantize
		bmin[0] = (unsigned short)(qfac * minx) & 0xfffe;
		bmin[1] = (unsigned short)(qfac * miny) & 0xfffe;
		bmin[2] = (unsigned short)(qfac * minz) & 0xfffe;
		bmax[0] = (unsigned short)(qfac * maxx + 1) | 1;
		bmax[1] = (unsigned short)(qfac * maxy + 1) | 1;
		bmax[2] = (unsigned short)(qfac * maxz + 1) | 1;
		
		// Traverse tree
		dtPolyRef base = getPolyRefBase(tile);
		int n = 0;
		while (node < end)
		{
			const bool overlap = rdOverlapQuantBounds(bmin, bmax, node->bmin, node->bmax);
			const bool isLeafNode = node->i >= 0;
			
			if (isLeafNode && overlap)
			{
				if (n < maxPolys)
					polys[n++] = base | (dtPolyRef)node->i;
			}
			
			if (overlap || isLeafNode)
				node++;
			else
			{
				const int escapeIndex = -node->i;
				node += escapeIndex;
			}
		}
		
		return n;
	}
	else
	{
		float bmin[3], bmax[3];
		int n = 0;
		dtPolyRef base = getPolyRefBase(tile);
		for (int i = 0; i < tile->header->polyCount; ++i)
		{
			dtPoly* p = &tile->polys[i];
			// Do not return off-mesh connection polygons.
			if (p->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
				continue;
			// Calc polygon bounds.
			const float* v = &tile->verts[p->verts[0]*3];
			rdVcopy(bmin, v);
			rdVcopy(bmax, v);
			for (int j = 1; j < p->vertCount; ++j)
			{
				v = &tile->verts[p->verts[j]*3];
				rdVmin(bmin, v);
				rdVmax(bmax, v);
			}
			if (rdOverlapBounds(qmin,qmax, bmin,bmax))
			{
				if (n < maxPolys)
					polys[n++] = base | (dtPolyRef)i;
			}
		}
		return n;
	}
}

/// @par
///
/// The add operation will fail if the data is in the wrong format, the allocated tile
/// space is full, or there is a tile already at the specified reference.
///
/// The lastRef parameter is used to restore a tile with the same tile
/// reference it had previously used.  In this case the #dtPolyRef's for the
/// tile will be restored to the same values they were before the tile was 
/// removed.
///
/// The nav mesh assumes exclusive access to the data passed and will make
/// changes to the dynamic portion of the data. For that reason the data
/// should not be reused in other nav meshes until the tile has been successfully
/// removed from this nav mesh.
///
/// @see dtCreateNavMeshData, #removeTile
dtStatus dtNavMesh::addTile(unsigned char* data, int dataSize, int flags,
							dtTileRef lastRef, dtTileRef* result)
{
	// Make sure the data is in right format.
	dtMeshHeader* header = (dtMeshHeader*)data;
	if (header->magic != DT_NAVMESH_MAGIC)
		return DT_FAILURE | DT_WRONG_MAGIC;
	if (header->version != DT_NAVMESH_VERSION)
		return DT_FAILURE | DT_WRONG_VERSION;

#ifndef DT_POLYREF64
	// Do not allow adding more polygons than specified in the NavMesh's maxPolys constraint.
	// Otherwise, the poly ID cannot be represented with the given number of bits.
	if (m_polyBits < rdIlog2(rdNextPow2((unsigned int)header->polyCount)))
		return DT_FAILURE | DT_INVALID_PARAM;
#endif
		
	// Make sure the location is free.
	if (!header->userId != DT_FULL_UNLINKED_TILE_USER_ID && getTileAt(header->x, header->y, header->layer))
		return DT_FAILURE | DT_ALREADY_OCCUPIED;
		
	// Allocate a tile.
	dtMeshTile* tile = 0;
	if (!lastRef)
	{
		if (m_nextFree)
		{
			tile = m_nextFree;
			m_nextFree = tile->next;
			tile->next = 0;
		}
	}
	else
	{
		// Try to relocate the tile to specific index with same salt.
		int tileIndex = (int)decodePolyIdTile((dtPolyRef)lastRef);
		if (tileIndex >= m_maxTiles)
			return DT_FAILURE | DT_OUT_OF_MEMORY;
		// Try to find the specific tile id from the free list.
		dtMeshTile* target = &m_tiles[tileIndex];
		dtMeshTile* prev = 0;
		tile = m_nextFree;
		while (tile && tile != target)
		{
			prev = tile;
			tile = tile->next;
		}
		// Could not find the correct location.
		if (tile != target)
			return DT_FAILURE | DT_OUT_OF_MEMORY;
		// Remove from freelist
		if (!prev)
			m_nextFree = tile->next;
		else
			prev->next = tile->next;

		// Restore salt.
		tile->salt = decodePolyIdSalt((dtPolyRef)lastRef);
	}

	// Make sure we could allocate a tile.
	if (!tile)
		return DT_FAILURE | DT_OUT_OF_MEMORY;

	tile->tracker = nullptr;
	
	// Insert tile into the position lut.
	if (header->userId != DT_FULL_UNLINKED_TILE_USER_ID)
	{
		int h = computeTileHash(header->x, header->y, m_tileLutMask);
		tile->next = m_posLookup[h];
		m_posLookup[h] = tile;
	}
	
	// Patch header pointers.
	const int headerSize = rdAlign4(sizeof(dtMeshHeader));
	const int vertsSize = rdAlign4(sizeof(float)*3*header->vertCount);
	const int polysSize = rdAlign4(sizeof(dtPoly)*header->polyCount);
	const int polyMapSize = rdAlign4(sizeof(int)*(header->polyCount*header->polyMapCount));
	const int linksSize = rdAlign4(sizeof(dtLink)*(header->maxLinkCount));
	const int detailMeshesSize = rdAlign4(sizeof(dtPolyDetail)*header->detailMeshCount);
	const int detailVertsSize = rdAlign4(sizeof(float)*3*header->detailVertCount);
	const int detailTrisSize = rdAlign4(sizeof(unsigned char)*4*header->detailTriCount);
	const int bvtreeSize = rdAlign4(sizeof(dtBVNode)*header->bvNodeCount);
	const int offMeshLinksSize = rdAlign4(sizeof(dtOffMeshConnection)*header->offMeshConCount);
#if DT_NAVMESH_SET_VERSION >= 8
	const int cellsSize = rdAlign4(sizeof(dtCell)*header->maxCellCount);
#endif
	
	unsigned char* d = data + headerSize;
	tile->verts = rdGetThenAdvanceBufferPointer<float>(d, vertsSize);
	tile->polys = rdGetThenAdvanceBufferPointer<dtPoly>(d, polysSize);
	tile->polyMap = rdGetThenAdvanceBufferPointer<unsigned int>(d, polyMapSize);
	tile->links = rdGetThenAdvanceBufferPointer<dtLink>(d, linksSize);
	tile->detailMeshes = rdGetThenAdvanceBufferPointer<dtPolyDetail>(d, detailMeshesSize);
	tile->detailVerts = rdGetThenAdvanceBufferPointer<float>(d, detailVertsSize);
	tile->detailTris = rdGetThenAdvanceBufferPointer<unsigned char>(d, detailTrisSize);
	tile->bvTree = rdGetThenAdvanceBufferPointer<dtBVNode>(d, bvtreeSize);
	tile->offMeshCons = rdGetThenAdvanceBufferPointer<dtOffMeshConnection>(d, offMeshLinksSize);
#if DT_NAVMESH_SET_VERSION >= 8
	tile->cells = rdGetThenAdvanceBufferPointer<dtCell>(d, cellsSize);
#endif

	// If there are no items in the bvtree, reset the tree pointer.
	if (!bvtreeSize)
		tile->bvTree = 0;

	// Init tile.
	tile->header = header;
	tile->data = data;
	tile->dataSize = dataSize;
	tile->flags = flags;

	m_tileCount++;

	if (result)
		*result = getTileRef(tile);
	
	return DT_SUCCESS;
}

dtStatus dtNavMesh::connectTile(const dtTileRef tileRef)
{
	const int tileIndex = (int)decodePolyIdTile((dtPolyRef)tileRef);
	if (tileIndex >= m_maxTiles)
		return DT_FAILURE | DT_OUT_OF_MEMORY;

	dtMeshTile* tile = &m_tiles[tileIndex];
	const dtMeshHeader* header = tile->header;

	// Build links freelist
	tile->linksFreeList = 0;
	tile->links[header->maxLinkCount - 1].next = DT_NULL_LINK;
	for (int i = 0; i < header->maxLinkCount - 1; ++i)
		tile->links[i].next = i + 1;

	connectIntLinks(tile);

	// Create connections with neighbour tiles.
	static const int MAX_NEIS = 32;
	dtMeshTile* neis[MAX_NEIS];
	int nneis;
	
	// Connect with layers in current tile.
	nneis = getTilesAt(header->x, header->y, neis, MAX_NEIS);
	for (int j = 0; j < nneis; ++j)
	{
		if (neis[j] == tile)
			continue;
	
		connectExtLinks(tile, neis[j], -1);
		connectExtLinks(neis[j], tile, -1);
	}
	
	// Connect with neighbour tiles.
	for (int i = 0; i < 8; ++i)
	{
		nneis = getNeighbourTilesAt(header->x, header->y, i, neis, MAX_NEIS);
		for (int j = 0; j < nneis; ++j)
		{
			connectExtLinks(tile, neis[j], i);
			connectExtLinks(neis[j], tile, rdOppositeTile(i));
		}
	}

	return DT_SUCCESS;
}

const dtMeshTile* dtNavMesh::getTileAt(const int x, const int y, const int layer) const
{
	// Find tile based on hash.
	int h = computeTileHash(x,y,m_tileLutMask);
	dtMeshTile* tile = m_posLookup[h];
	while (tile)
	{
		if (tile->header &&
			tile->header->x == x &&
			tile->header->y == y &&
			tile->header->layer == layer)
		{
			return tile;
		}
		tile = tile->next;
	}
	return 0;
}

int dtNavMesh::getNeighbourTilesAt(const int x, const int y, const int side, dtMeshTile** tiles, const int maxTiles) const
{
	int nx = x, ny = y;
	switch (side)
	{
		case 0: nx++; break;
		case 1: nx++; ny++; break;
		case 2: ny++; break;
		case 3: nx--; ny++; break;
		case 4: nx--; break;
		case 5: nx--; ny--; break;
		case 6: ny--; break;
		case 7: nx++; ny--; break;
	};

	return getTilesAt(nx, ny, tiles, maxTiles);
}

int dtNavMesh::getTilesAt(const int x, const int y, dtMeshTile** tiles, const int maxTiles) const
{
	int n = 0;
	
	// Find tile based on hash.
	int h = computeTileHash(x,y,m_tileLutMask);
	dtMeshTile* tile = m_posLookup[h];
	while (tile)
	{
		if (tile->header &&
			tile->header->x == x &&
			tile->header->y == y)
		{
			if (n < maxTiles)
				tiles[n++] = tile;
		}
		tile = tile->next;
	}
	
	return n;
}

/// @par
///
/// This function will not fail if the tiles array is too small to hold the
/// entire result set.  It will simply fill the array to capacity.
int dtNavMesh::getTilesAt(const int x, const int y, dtMeshTile const** tiles, const int maxTiles) const
{
	int n = 0;
	
	// Find tile based on hash.
	int h = computeTileHash(x,y,m_tileLutMask);
	dtMeshTile* tile = m_posLookup[h];
	while (tile)
	{
		if (tile->header &&
			tile->header->x == x &&
			tile->header->y == y)
		{
			if (n < maxTiles)
				tiles[n++] = tile;
		}
		tile = tile->next;
	}
	
	return n;
}


dtTileRef dtNavMesh::getTileRefAt(const int x, const int y, const int layer) const
{
	// Find tile based on hash.
	int h = computeTileHash(x,y,m_tileLutMask);
	dtMeshTile* tile = m_posLookup[h];
	while (tile)
	{
		if (tile->header &&
			tile->header->x == x &&
			tile->header->y == y &&
			tile->header->layer == layer)
		{
			return getTileRef(tile);
		}
		tile = tile->next;
	}
	return 0;
}

const dtMeshTile* dtNavMesh::getTileByRef(dtTileRef ref) const
{
	if (!ref)
		return 0;
	unsigned int tileIndex = decodePolyIdTile((dtPolyRef)ref);
	unsigned int tileSalt = decodePolyIdSalt((dtPolyRef)ref);
	if ((int)tileIndex >= m_maxTiles)
		return 0;
	const dtMeshTile* tile = &m_tiles[tileIndex];
	if (tile->salt != tileSalt)
		return 0;
	return tile;
}

int dtNavMesh::getMaxTiles() const
{
	return m_maxTiles;
}

dtMeshTile* dtNavMesh::getTile(int i)
{
	rdAssert(i >= 0 && i < m_maxTiles);
	return &m_tiles[i];
}

const dtMeshTile* dtNavMesh::getTile(int i) const
{
	rdAssert(i >= 0 && i < m_maxTiles);
	return &m_tiles[i];
}

void dtNavMesh::calcTileLoc(const float* pos, int* tx, int* ty) const
{
	*tx = (int)rdMathFloorf((m_orig[0]-pos[0]) / m_tileWidth);
	*ty = (int)rdMathFloorf((pos[1]-m_orig[1]) / m_tileHeight);
}

dtStatus dtNavMesh::getTileAndPolyByRef(const dtPolyRef ref, const dtMeshTile** tile, const dtPoly** poly) const
{
	if (!ref) return DT_FAILURE;
	unsigned int salt, it, ip;
	decodePolyId(ref, salt, it, ip);
	if (it >= (unsigned int)m_maxTiles) return DT_FAILURE | DT_INVALID_PARAM;
	if (m_tiles[it].salt != salt || m_tiles[it].header == 0) return DT_FAILURE | DT_INVALID_PARAM;
	if (ip >= (unsigned int)m_tiles[it].header->polyCount) return DT_FAILURE | DT_INVALID_PARAM;
	*tile = &m_tiles[it];
	*poly = &m_tiles[it].polys[ip];
	return DT_SUCCESS;
}

/// @par
///
/// @warning Only use this function if it is known that the provided polygon
/// reference is valid. This function is faster than #getTileAndPolyByRef, but
/// it does not validate the reference.
void dtNavMesh::getTileAndPolyByRefUnsafe(const dtPolyRef ref, const dtMeshTile** tile, const dtPoly** poly) const
{
	unsigned int salt, it, ip;
	decodePolyId(ref, salt, it, ip);
	*tile = &m_tiles[it];
	*poly = &m_tiles[it].polys[ip];
}

bool dtNavMesh::arePolysAdjacent(const dtPolyRef fromRef, const dtPolyRef goalRef) const
{
	const dtMeshTile* fromTile = nullptr;
	const dtMeshTile* goalTile = nullptr;
	const dtPoly* fromPoly = nullptr;
	const dtPoly* goalPoly = nullptr;

	getTileAndPolyByRefUnsafe(fromRef, &fromTile, &fromPoly);
	getTileAndPolyByRefUnsafe(goalRef, &goalTile, &goalPoly);

	return arePolysAdjacent(fromPoly, fromTile, goalPoly, goalTile);
}

bool dtNavMesh::arePolysAdjacent(const dtPoly* const basePoly, const dtMeshTile* const baseTile,
								 const dtPoly* const landPoly, const dtMeshTile* const landTile) const
{
	if (baseTile == landTile)
	{
		for (int i = 0; i < landPoly->vertCount; ++i)
		{
			const unsigned short nei = landPoly->neis[i];

			if (!nei)
				continue;

			if (nei & DT_EXT_LINK)
				continue;

			const int idx = (nei-1);

			if (&baseTile->polys[idx] == basePoly)
				return true;
		}
	}
	else // Check external linkage.
	{
		unsigned int linkIndex = landPoly->firstLink;
		while (linkIndex != DT_NULL_LINK)
		{
			const dtLink* const link = &landTile->links[linkIndex];

			if (link->side != 0xff && link->traverseType == DT_NULL_TRAVERSE_TYPE)
			{
				const dtMeshTile* neiTile;
				const dtPoly* neiPoly;
				getTileAndPolyByRefUnsafe(link->ref, &neiTile, &neiPoly);

				if (neiPoly == basePoly)
					return true;
			}

			linkIndex = link->next;
		}
	}

	return false;
}

bool dtNavMesh::isGoalPolyReachable(const dtPolyRef fromRef, const dtPolyRef goalRef, 
									const bool checkDisjointGroupsOnly, const int traverseTableIndex) const
{
	// Same poly is always reachable.
	if (fromRef == goalRef)
		return true;

	const dtMeshTile* fromTile = nullptr;
	const dtMeshTile* goalTile = nullptr;
	const dtPoly* fromPoly = nullptr;
	const dtPoly* goalPoly = nullptr;

	getTileAndPolyByRefUnsafe(fromRef, &fromTile, &fromPoly);
	getTileAndPolyByRefUnsafe(goalRef, &goalTile, &goalPoly);

	const unsigned short fromPolyGroupId = fromPoly->groupId;
	const unsigned short goalPolyGroupId = goalPoly->groupId;

	// If we don't utilize traverse portals, check if we are on the same island as
	// its impossible to reach the goal if we aren't.
	if (checkDisjointGroupsOnly)
		return fromPolyGroupId == goalPolyGroupId;

	rdAssert(traverseTableIndex >= 0 && traverseTableIndex < m_params.traverseTableCount);
	const int* const traverseTable = m_traverseTables[traverseTableIndex];

	// Traverse table doesn't exist, attempt the path finding anyways (this is
	// a bug in the NavMesh, rebuild it!).
	if (!traverseTable)
		return true;

	const int polyGroupCount = m_params.polyGroupCount;
	const int fromPolyBitCell = traverseTable[dtCalcTraverseTableCellIndex(polyGroupCount, fromPolyGroupId, goalPolyGroupId)];

	// Check if the bit corresponding to our goal poly is set, if it isn't then
	// there are no available traverse links from the current poly to the goal.
	return fromPolyBitCell & rdBitCellBit(goalPolyGroupId);
}

bool dtNavMesh::isValidPolyRef(dtPolyRef ref) const
{
	if (!ref) return false;
	unsigned int salt, it, ip;
	decodePolyId(ref, salt, it, ip);
	if (it >= (unsigned int)m_maxTiles) return false;
	if (m_tiles[it].salt != salt || m_tiles[it].header == 0) return false;
	if (ip >= (unsigned int)m_tiles[it].header->polyCount) return false;
	return true;
}

/// @par
///
/// This function returns the data for the tile so that, if desired,
/// it can be added back to the navigation mesh at a later point.
///
/// @see #addTile
dtStatus dtNavMesh::removeTile(dtTileRef ref, unsigned char** data, int* dataSize)
{
	if (!ref)
		return DT_FAILURE | DT_INVALID_PARAM;
	unsigned int tileIndex = decodePolyIdTile((dtPolyRef)ref);
	unsigned int tileSalt = decodePolyIdSalt((dtPolyRef)ref);
	if ((int)tileIndex >= m_maxTiles)
		return DT_FAILURE | DT_INVALID_PARAM;
	dtMeshTile* tile = &m_tiles[tileIndex];
	if (tile->salt != tileSalt)
		return DT_FAILURE | DT_INVALID_PARAM;
	dtMeshHeader* header = tile->header;
	
	// Remove tile from hash lookup.
	int h = computeTileHash(header->x,header->y,m_tileLutMask);
	dtMeshTile* prev = 0;
	dtMeshTile* cur = m_posLookup[h];
	while (cur)
	{
		if (cur == tile)
		{
			if (prev)
				prev->next = cur->next;
			else
				m_posLookup[h] = cur->next;
			break;
		}
		prev = cur;
		cur = cur->next;
	}

	// Disconnect from off-mesh links originating from other tiles.
	for (int i = 0; i < m_maxTiles; ++i)
	{
		dtMeshTile* offTile = &m_tiles[i];

		if (offTile == tile)
			continue;

		const dtMeshHeader* offHeader = offTile->header;

		if (!offHeader)
			continue;

		if (!offHeader->offMeshConCount)
			continue;

		unconnectLinks(offTile, tile);
	}

	// If we have off-mesh links, disconnect the land tiles from it.
	for (int i = 0; i < header->offMeshConCount; i++)
	{
		dtOffMeshConnection* con = &tile->offMeshCons[i];
		dtPoly* poly = &tile->polys[con->poly];

		for (unsigned int k = poly->firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
		{
			const dtLink& link = tile->links[k];

			unsigned int salt, it, ip;
			decodePolyId(link.ref, salt, it, ip);

			dtMeshTile* landTile = &m_tiles[it];

			if (landTile == tile)
				continue;

			unconnectLinks(landTile, tile);
		}
	}
	
	// Remove connections to neighbour tiles.
	static const int MAX_NEIS = 32;
	dtMeshTile* neis[MAX_NEIS];
	int nneis;
	
	// Disconnect from other layers in current tile.
	nneis = getTilesAt(header->x, header->y, neis, MAX_NEIS);
	for (int j = 0; j < nneis; ++j)
	{
		if (neis[j] == tile) continue;
		unconnectLinks(neis[j], tile);
	}
	
	// Disconnect from neighbour tiles.
	for (int i = 0; i < 8; ++i)
	{
		nneis = getNeighbourTilesAt(header->x, header->y, i, neis, MAX_NEIS);
		for (int j = 0; j < nneis; ++j)
			unconnectLinks(neis[j], tile);
	}
		
	// Reset tile.
	if (tile->flags & DT_TILE_FREE_DATA)
	{
		if (tile->flags & DT_CELL_FREE_DATA)
		{
			rdFree(tile->cells);
			tile->cells = 0;
		}

		// Owns data
		rdFree(tile->data);
		tile->data = 0;
		tile->dataSize = 0;
		if (data) *data = 0;
		if (dataSize) *dataSize = 0;
	}
	else
	{
		if (data) *data = tile->data;
		if (dataSize) *dataSize = tile->dataSize;
	}

	tile->header = 0;
	tile->flags = 0;
	tile->linksFreeList = 0;
	tile->polys = 0;
	tile->verts = 0;
	tile->links = 0;
	tile->detailMeshes = 0;
	tile->detailVerts = 0;
	tile->detailTris = 0;
	tile->bvTree = 0;
	tile->offMeshCons = 0;

	// Update salt, salt should never be zero.
#ifdef DT_POLYREF64
	tile->salt = (tile->salt+1) & ((1<<DT_SALT_BITS)-1);
#else
	tile->salt = (tile->salt+1) & ((1<<m_saltBits)-1);
#endif
	if (tile->salt == 0)
		tile->salt++;

	// Add to free list.
	tile->next = m_nextFree;
	m_nextFree = tile;

	m_tileCount--;

	return DT_SUCCESS;
}

dtTileRef dtNavMesh::getTileRef(const dtMeshTile* tile) const
{
	if (!tile) return 0;
	const unsigned int it = (unsigned int)(tile - m_tiles);
	return (dtTileRef)encodePolyId(tile->salt, it, 0);
}

/// @par
///
/// Example use case:
/// @code
///
/// const dtPolyRef base = navmesh->getPolyRefBase(tile);
/// for (int i = 0; i < tile->header->polyCount; ++i)
/// {
///     const dtPoly* p = &tile->polys[i];
///     const dtPolyRef ref = base | (dtPolyRef)i;
///     
///     // Use the reference to access the polygon data.
/// }
/// @endcode
dtPolyRef dtNavMesh::getPolyRefBase(const dtMeshTile* tile) const
{
	if (!tile) return 0;
	const unsigned int it = (unsigned int)(tile - m_tiles);
	return encodePolyId(tile->salt, it, 0);
}

struct dtTileState
{
	int magic;								// Magic number, used to identify the data.
	int version;							// Data version number.
	dtTileRef ref;							// Tile ref at the time of storing the data.
};

struct dtPolyState
{
	unsigned short flags;						// Flags (see dtPolyFlags).
	unsigned char area;							// Area ID of the polygon.
};

///  @see #storeTileState
int dtNavMesh::getTileStateSize(const dtMeshTile* tile) const
{
	if (!tile) return 0;
	const int headerSize = rdAlign4(sizeof(dtTileState));
	const int polyStateSize = rdAlign4(sizeof(dtPolyState) * tile->header->polyCount);
	return headerSize + polyStateSize;
}

/// @par
///
/// Tile state includes non-structural data such as polygon flags, area ids, etc.
/// @note The state data is only valid until the tile reference changes.
/// @see #getTileStateSize, #restoreTileState
dtStatus dtNavMesh::storeTileState(const dtMeshTile* tile, unsigned char* data, const int maxDataSize) const
{
	// Make sure there is enough space to store the state.
	const int sizeReq = getTileStateSize(tile);
	if (maxDataSize < sizeReq)
		return DT_FAILURE | DT_BUFFER_TOO_SMALL;
		
	dtTileState* tileState = rdGetThenAdvanceBufferPointer<dtTileState>(data, rdAlign4(sizeof(dtTileState)));
	dtPolyState* polyStates = rdGetThenAdvanceBufferPointer<dtPolyState>(data, rdAlign4(sizeof(dtPolyState) * tile->header->polyCount));
	
	// Store tile state.
	tileState->magic = DT_NAVMESH_STATE_MAGIC;
	tileState->version = DT_NAVMESH_STATE_VERSION;
	tileState->ref = getTileRef(tile);
	
	// Store per poly state.
	for (int i = 0; i < tile->header->polyCount; ++i)
	{
		const dtPoly* p = &tile->polys[i];
		dtPolyState* s = &polyStates[i];
		s->flags = p->flags;
		s->area = p->getArea();
	}
	
	return DT_SUCCESS;
}

/// @par
///
/// Tile state includes non-structural data such as polygon flags, area ids, etc.
/// @note This function does not impact the tile's #dtTileRef and #dtPolyRef's.
/// @see #storeTileState
dtStatus dtNavMesh::restoreTileState(dtMeshTile* tile, const unsigned char* data, const int maxDataSize)
{
	// Make sure there is enough space to store the state.
	const int sizeReq = getTileStateSize(tile);
	if (maxDataSize < sizeReq)
		return DT_FAILURE | DT_INVALID_PARAM;
	
	const dtTileState* tileState = rdGetThenAdvanceBufferPointer<const dtTileState>(data, rdAlign4(sizeof(dtTileState)));
	const dtPolyState* polyStates = rdGetThenAdvanceBufferPointer<const dtPolyState>(data, rdAlign4(sizeof(dtPolyState) * tile->header->polyCount));
	
	// Check that the restore is possible.
	if (tileState->magic != DT_NAVMESH_STATE_MAGIC)
		return DT_FAILURE | DT_WRONG_MAGIC;
	if (tileState->version != DT_NAVMESH_STATE_VERSION)
		return DT_FAILURE | DT_WRONG_VERSION;
	if (tileState->ref != getTileRef(tile))
		return DT_FAILURE | DT_INVALID_PARAM;
	
	// Restore per poly state.
	for (int i = 0; i < tile->header->polyCount; ++i)
	{
		dtPoly* p = &tile->polys[i];
		const dtPolyState* s = &polyStates[i];
		p->flags = s->flags;
		p->setArea(s->area);
	}
	
	return DT_SUCCESS;
}

/// @par
///
/// Off-mesh connections are stored in the navigation mesh as special 2-vertex 
/// polygons with a single edge. At least one of the vertices is expected to be 
/// inside a normal polygon. So an off-mesh connection is "entered" from a 
/// normal polygon at one of its endpoints. This is the polygon identified by 
/// the prevRef parameter.
dtStatus dtNavMesh::getOffMeshConnectionPolyEndPoints(dtPolyRef prevRef, dtPolyRef polyRef, float* startPos, float* endPos) const
{
	unsigned int salt, it, ip;

	if (!polyRef)
		return DT_FAILURE;
	
	// Get current polygon
	decodePolyId(polyRef, salt, it, ip);
	if (it >= (unsigned int)m_maxTiles) return DT_FAILURE | DT_INVALID_PARAM;
	if (m_tiles[it].salt != salt || m_tiles[it].header == 0) return DT_FAILURE | DT_INVALID_PARAM;
	const dtMeshTile* tile = &m_tiles[it];
	if (ip >= (unsigned int)tile->header->polyCount) return DT_FAILURE | DT_INVALID_PARAM;
	const dtPoly* poly = &tile->polys[ip];

	// Make sure that the current poly is indeed off-mesh link.
	if (poly->getType() != DT_POLYTYPE_OFFMESH_CONNECTION)
		return DT_FAILURE;

	// Figure out which way to hand out the vertices.
	int idx0 = 0, idx1 = 1;
	
	// Find link that points to first vertex.
	for (unsigned int i = poly->firstLink; i != DT_NULL_LINK; i = tile->links[i].next)
	{
		if (tile->links[i].edge == 0)
		{
			if (tile->links[i].ref != prevRef)
			{
				idx0 = 1;
				idx1 = 0;
			}
			break;
		}
	}
	
	rdVcopy(startPos, &tile->verts[poly->verts[idx0]*3]);
	rdVcopy(endPos, &tile->verts[poly->verts[idx1]*3]);

	return DT_SUCCESS;
}


const dtOffMeshConnection* dtNavMesh::getOffMeshConnectionByRef(dtPolyRef ref) const
{
	unsigned int salt, it, ip;
	
	if (!ref)
		return 0;
	
	// Get current polygon
	decodePolyId(ref, salt, it, ip);
	if (it >= (unsigned int)m_maxTiles) return 0;
	if (m_tiles[it].salt != salt || m_tiles[it].header == 0) return 0;
	const dtMeshTile* tile = &m_tiles[it];
	if (ip >= (unsigned int)tile->header->polyCount) return 0;
	const dtPoly* poly = &tile->polys[ip];
	
	// Make sure that the current poly is indeed off-mesh link.
	if (poly->getType() != DT_POLYTYPE_OFFMESH_CONNECTION)
		return 0;

	const unsigned int idx =  ip - tile->header->offMeshBase;
	rdAssert(idx < (unsigned int)tile->header->offMeshConCount);
	return &tile->offMeshCons[idx];
}

bool dtNavMesh::allocTraverseTables(const int count)
{
	rdAssert(count > 0 && count <= DT_MAX_TRAVERSE_TABLES);
	const int setTableBufSize = sizeof(int**) * count;

	m_traverseTables = (int**)rdAlloc(setTableBufSize, RD_ALLOC_PERM);
	if (!m_traverseTables)
		return false;

	memset(m_traverseTables, 0, setTableBufSize);
	return true;
}

void dtNavMesh::freeTraverseTables()
{
	for (int i = 0; i < m_params.traverseTableCount; i++)
	{
		int* traverseTable = m_traverseTables[i];

		if (traverseTable)
			rdFree(traverseTable);
	}

	rdFree(m_traverseTables);
}

void dtNavMesh::setTraverseTable(const int index, int* const table)
{
	rdAssert(index >= 0 && index < m_params.traverseTableCount);
	rdAssert(m_traverseTables);

	if (m_traverseTables[index])
		rdFree(m_traverseTables[index]);

	m_traverseTables[index] = table;
}

#if DT_NAVMESH_SET_VERSION >= 7
void dtNavMesh::freeHints()
{
	for (int i = 0; i < m_params.hintCount; i++)
	{
		dtHint& hint = m_hints[i];

		rdFree(hint.verts);
		rdFree(hint.tris);
	}

	rdFree(m_hints);
}
#endif

dtStatus dtNavMesh::setPolyFlags(dtPolyRef ref, unsigned short flags)
{
	if (!ref) return DT_FAILURE;
	unsigned int salt, it, ip;
	decodePolyId(ref, salt, it, ip);
	if (it >= (unsigned int)m_maxTiles) return DT_FAILURE | DT_INVALID_PARAM;
	if (m_tiles[it].salt != salt || m_tiles[it].header == 0) return DT_FAILURE | DT_INVALID_PARAM;
	dtMeshTile* tile = &m_tiles[it];
	if (ip >= (unsigned int)tile->header->polyCount) return DT_FAILURE | DT_INVALID_PARAM;
	dtPoly* poly = &tile->polys[ip];
	
	// Change flags.
	poly->flags = flags;
	
	return DT_SUCCESS;
}

dtStatus dtNavMesh::getPolyFlags(dtPolyRef ref, unsigned short* resultFlags) const
{
	if (!ref) return DT_FAILURE;
	unsigned int salt, it, ip;
	decodePolyId(ref, salt, it, ip);
	if (it >= (unsigned int)m_maxTiles) return DT_FAILURE | DT_INVALID_PARAM;
	if (m_tiles[it].salt != salt || m_tiles[it].header == 0) return DT_FAILURE | DT_INVALID_PARAM;
	const dtMeshTile* tile = &m_tiles[it];
	if (ip >= (unsigned int)tile->header->polyCount) return DT_FAILURE | DT_INVALID_PARAM;
	const dtPoly* poly = &tile->polys[ip];

	*resultFlags = poly->flags;
	
	return DT_SUCCESS;
}

dtStatus dtNavMesh::setPolyArea(dtPolyRef ref, unsigned char area)
{
	if (!ref) return DT_FAILURE;
	unsigned int salt, it, ip;
	decodePolyId(ref, salt, it, ip);
	if (it >= (unsigned int)m_maxTiles) return DT_FAILURE | DT_INVALID_PARAM;
	if (m_tiles[it].salt != salt || m_tiles[it].header == 0) return DT_FAILURE | DT_INVALID_PARAM;
	dtMeshTile* tile = &m_tiles[it];
	if (ip >= (unsigned int)tile->header->polyCount) return DT_FAILURE | DT_INVALID_PARAM;
	dtPoly* poly = &tile->polys[ip];
	
	poly->setArea(area);
	
	return DT_SUCCESS;
}

dtStatus dtNavMesh::getPolyArea(dtPolyRef ref, unsigned char* resultArea) const
{
	if (!ref) return DT_FAILURE;
	unsigned int salt, it, ip;
	decodePolyId(ref, salt, it, ip);
	if (it >= (unsigned int)m_maxTiles) return DT_FAILURE | DT_INVALID_PARAM;
	if (m_tiles[it].salt != salt || m_tiles[it].header == 0) return DT_FAILURE | DT_INVALID_PARAM;
	const dtMeshTile* tile = &m_tiles[it];
	if (ip >= (unsigned int)tile->header->polyCount) return DT_FAILURE | DT_INVALID_PARAM;
	const dtPoly* poly = &tile->polys[ip];
	
	*resultArea = poly->getArea();
	
	return DT_SUCCESS;
}

float dtCalcLinkDistance(const float* spos, const float* epos)
{
	return rdMathFabsf(rdVdist(spos, epos));
}

unsigned char dtQuantLinkDistance(const float distance)
{
	if (distance > DT_TRAVERSE_DIST_MAX) return (unsigned char)0;
	return (unsigned char)(rdMathRoundf(distance * DT_TRAVERSE_DIST_QUANT_FACTOR));
}

float dtCalcPolySurfaceArea(const dtPoly* poly, const float* verts)
{
	float polyArea = 0.0f;

	// Only run if we have more than 2 verts since poly's with 2 verts
	// (off-mesh connections) don't have any surface area.
	for (int i = 2; i < poly->vertCount; ++i)
	{
		const float* va = &verts[poly->verts[0]*3];
		const float* vb = &verts[poly->verts[i]*3];
		const float* vc = &verts[poly->verts[i-1]*3];
		polyArea += rdTriArea2D(va,vb,vc);
	}

	return polyArea;
}

float dtCalcOffMeshRefYaw(const float* spos, const float* epos)
{
	const float dx = epos[0]-spos[0];
	const float dy = epos[1]-spos[1];

	const float yawRad = rdMathAtan2f(dy, dx);
	return rdRadToDeg(yawRad);
}

void dtCalcOffMeshRefPos(const float* spos, const float yawDeg, const float* offset, float* res)
{
	const float yawRad = rdDegToRad(yawDeg);

	const float dx = offset[0]*rdMathCosf(yawRad);
	const float dy = offset[1]*rdMathSinf(yawRad);

	res[0] = spos[0]+dx;
	res[1] = spos[1]+dy;
	res[2] = spos[2]+offset[2];
}

int dtGetNavMeshVersionForSet(const int setVersion)
{
	// TODO: check r2tt (Titanfall 2 tech test). It might have a set version lower
	// than 5, which might be interesting to reverse as well and support.
	switch (setVersion)
	{
	case 5: return 13;
	case 6: return 14;
	case 7: return 15;
	case 8: // 8 & 9 use the same navmesh version, but they are different!
	case 9: return 16;
	default: rdAssert(0); return -1; // Unsupported set version!
	}
}
