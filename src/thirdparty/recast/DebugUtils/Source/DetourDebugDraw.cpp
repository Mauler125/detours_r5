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

#include "DebugUtils/Include/DebugDraw.h"
#include "DebugUtils/Include/DetourDebugDraw.h"
#include "Detour/Include/DetourNavMesh.h"
#include "Detour/Include/DetourNode.h"
#include "Shared/Include/SharedCommon.h"

static void drawPolyVerts(duDebugDraw* dd, const dtMeshTile* tile, const float* offset)
{
	const dtMeshHeader* header = tile->header;
	const unsigned int vcol = duRGBA(0, 0, 0, 220);

	dd->begin(DU_DRAW_POINTS, 4.0f, offset);
	for (int i = 0; i < header->vertCount; ++i)
	{
		const float* v = &tile->verts[i * 3];
		dd->vertex(v[0], v[1], v[2], vcol);
	}
	dd->end();
}

static unsigned int getPolySurfaceColor(const duDebugDraw* dd, const dtPoly* poly, const unsigned int alpha)
{
	return poly->groupId == DT_UNLINKED_POLY_GROUP
		? duRGBA(240,20,10, alpha)
		: duTransCol(dd->areaToFaceCol(poly->getArea()), alpha);
}

static unsigned int getPolyBoundaryColor(const duDebugDraw* dd, const dtPoly* poly, const bool inner)
{
	return poly->groupId == DT_UNLINKED_POLY_GROUP
		? duRGBA(32,24,0, inner ? 32 : 220)
		: duTransCol(dd->areaToEdgeCol(poly->getArea()), inner ? 32 : 220);
}

static void drawPolyMeshFaces(duDebugDraw* dd, const dtNavMesh& mesh, const dtNavMeshQuery* query, const dtMeshTile* tile, const float* offset, unsigned int flags)
{
	const dtMeshHeader* header = tile->header;
	const dtPolyRef base = mesh.getPolyRefBase(tile);

	// If the "Alpha" flag isn't set, force the colour to be opaque instead of semi-transparent.
	const int tileAlpha = flags & DU_DRAW_DETOURMESH_ALPHA ? 170 : 255;

	dd->begin(DU_DRAW_TRIS, 1.0f, offset);
	for (int i = 0; i < header->polyCount; ++i)
	{
		const dtPoly* p = &tile->polys[i];
		if (p->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)	// Skip off-mesh links.
			continue;
			
		const dtPolyDetail* pd = &tile->detailMeshes[i];

		unsigned int col;
		if (query && query->isInClosedList(base | (dtPolyRef)i))
			col = duRGBA(255,196,0,64);
		else
		{
			if (flags & DU_DRAW_DETOURMESH_TILE_COLORS)
				col = duIntToCol(mesh.decodePolyIdTile(base), tileAlpha);
			else if (flags & DU_DRAW_DETOURMESH_POLY_GROUPS)
				col = duIntToCol(p->groupId, tileAlpha);
			else
				col = getPolySurfaceColor(dd, p, tileAlpha);
		}
		
		for (int j = 0; j < pd->triCount; ++j)
		{
			const unsigned char* t = &tile->detailTris[(pd->triBase+j)*4];
			for (int k = 0; k < 3; ++k)
			{
				if (t[k] < p->vertCount)
					dd->vertex(&tile->verts[p->verts[t[k]]*3], col);
				else
					dd->vertex(&tile->detailVerts[(pd->vertBase+t[k]-p->vertCount)*3], col);
			}
		}
	}
	dd->end();
}

static void drawPolyMeshEdges(duDebugDraw* dd, const dtMeshTile* tile, const float* offset)
{
	const dtMeshHeader* header = tile->header;
	unsigned int c = duRGBA(0,0,0,48);

	for (int i = 0; i < header->polyCount; ++i)
	{
		const dtPoly* p = &tile->polys[i];
		if (p->getType() == DT_POLYTYPE_OFFMESH_CONNECTION) continue;

		const dtPolyDetail* pd = &tile->detailMeshes[i];

		dd->begin(DU_DRAW_LINES, 1.0f, offset);
		for (int j = 0; j < pd->triCount; ++j)
		{
			const unsigned char* t = &tile->detailTris[(pd->triBase+j)*4];
			const float* tv[3];
			for (int k = 0; k < 3; ++k)
			{
				if (t[k] < p->vertCount)
					tv[k] = &tile->verts[p->verts[t[k]]*3];
				else
					tv[k] = &tile->detailVerts[(pd->vertBase+(t[k]-p->vertCount))*3];
			}
			for (int k = 0, l = 2; k < 3; l = k++)
			{
				if ((dtGetDetailTriEdgeFlags(t[3], l) & RD_DETAIL_EDGE_BOUNDARY))
					continue;

				dd->vertex(tv[l], c);
				dd->vertex(tv[k], c);
			}
		}
		dd->end();
	}
}

static void drawPolyBoundaries(duDebugDraw* dd, const dtMeshTile* tile,
							   const float linew, const float* offset, const int flags, bool inner)
{
	dd->begin(DU_DRAW_LINES, linew, offset);

	const dtMeshHeader* header = tile->header;
	const float walkableRadius = header->walkableRadius;

	static const float thr = 0.01f*0.01f;

	for (int i = 0; i < header->polyCount; ++i)
	{
		const dtPoly* p = &tile->polys[i];
		
		if (p->getType() == DT_POLYTYPE_OFFMESH_CONNECTION) continue;

		const bool isLinked = p->groupId != DT_UNLINKED_POLY_GROUP;
		
		for (int j = 0, nj = (int)p->vertCount; j < nj; ++j)
		{
			unsigned int c = getPolyBoundaryColor(dd, p, inner);
			if (inner)
			{
				if (p->neis[j] == 0) continue;
				if (p->neis[j] & DT_EXT_LINK)
				{
					bool con = false;
					for (unsigned int k = p->firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
					{
						if (tile->links[k].edge == j)
						{
							con = true;
							break;
						}
					}
					if (con)
						c = duRGBA(255,255,255,48);
					else
						c = isLinked ? duRGBA(255,0,0,255) : duRGBA(0,0,255,255);
				}
				else
					c = isLinked ? duRGBA(0,48,64,32) : duRGBA(24,48,0,32);
			}
			else
			{
				if (p->neis[j] != 0) continue;
			}
			
			const float* v0 = &tile->verts[p->verts[j]*3];
			const float* v1 = &tile->verts[p->verts[(j+1) % nj]*3];

			if (!inner && flags & DU_DRAW_DETOURMESH_LEDGE_SPANS)
			{
				float normal[3];
				rdCalcEdgeNormalPt2D(v0, v1, normal);

				float mid[3];
				rdVsad(mid, v0, v1, 0.5f);

				float ledgeEnd[3] = {
					mid[0] + normal[0] * walkableRadius,
					mid[1] + normal[1] * walkableRadius,
					mid[2]
				};

				dd->vertex(mid, c);
				dd->vertex(ledgeEnd, c);
			}

			const dtPolyDetail* pd = &tile->detailMeshes[i];

			// Draw detail mesh edges which align with the actual poly edge.
			// This is really slow.
			for (int k = 0; k < pd->triCount; ++k)
			{
				const unsigned char* t = &tile->detailTris[(pd->triBase+k)*4];
				const float* tv[3];
				for (int m = 0; m < 3; ++m)
				{
					if (t[m] < p->vertCount)
						tv[m] = &tile->verts[p->verts[t[m]]*3];
					else
						tv[m] = &tile->detailVerts[(pd->vertBase+(t[m]-p->vertCount))*3];
				}
				for (int m = 0, n = 2; m < 3; n=m++)
				{
					if ((dtGetDetailTriEdgeFlags(t[3], n) & RD_DETAIL_EDGE_BOUNDARY) == 0)
						continue;

					if (rdDistancePtLine2D(tv[n],v0,v1) < thr &&
						rdDistancePtLine2D(tv[m],v0,v1) < thr)
					{
						dd->vertex(tv[n], c);
						dd->vertex(tv[m], c);
					}
				}
			}
		}
	}
	dd->end();
}

static void drawPolyCenters(duDebugDraw* dd, const dtMeshTile* tile, const unsigned int col, const float linew, const float* offset)
{
	for (int i = 0; i < tile->header->polyCount; ++i)
	{
		const dtPoly* p = &tile->polys[i];
		duDebugDrawCross(dd, p->center[0], p->center[1], p->center[2], 3.0f, col, linew, offset);
	}
}

static void drawTraverseLinks(duDebugDraw* dd, const dtNavMesh& mesh, const dtNavMeshQuery* query,
	const dtMeshTile* tile, const float* offset, const duDrawTraverseLinkParams& traverseLinkParams)
{
	for (int i = 0; i < tile->header->polyCount; ++i)
	{
		const dtPoly* startPoly = &tile->polys[i];

		// Start poly's that are off-mesh links should be skipped
		// as only the links connecting base/land poly's to the
		// off-mesh connection contain traverse type information.
		if (startPoly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
			continue;

		if (tile->links[i].ref == 0)
			continue;

		// Iterate through links in the poly.
		for (unsigned int j = startPoly->firstLink; j != DT_NULL_LINK; j = tile->links[j].next)
		{
			const dtLink* link = &tile->links[j];

			// Skip "normal" links (non-jumping ones).
			if (!link->hasTraverseType())
				continue;

			// Filter, drawLinkType -1 means draw all types
			const int drawTraverseType = traverseLinkParams.traverseLinkType;
			const unsigned char linkTraverseType = link->getTraverseType();

			if (drawTraverseType != -1 && linkTraverseType != drawTraverseType)
				continue;

			// Filter, drawLinkDistance -1 means draw all distances
			const int drawLinkDistance = traverseLinkParams.traverseLinkDistance;

			if (drawLinkDistance != -1 && link->traverseDist > drawLinkDistance)
				continue;

			// Filter, drawAnimType -1 means draw all distances
			const int drawAnimType = traverseLinkParams.traverseAnimType;
			const dtPolyRef basePolyRef = mesh.getPolyRefBase(tile) | (dtPolyRef)i;

			if (drawAnimType != -2 &&
				!mesh.isGoalPolyReachable(basePolyRef, link->ref, drawAnimType == -1, drawAnimType) &&
				!mesh.isGoalPolyReachable(link->ref, basePolyRef, drawAnimType == -1, drawAnimType))
				continue;

			unsigned int salt, it, ip;
			mesh.decodePolyId(link->ref, salt, it, ip);
			const dtMeshTile* endTile = mesh.getTile(it);
			const dtPoly* endPoly = &endTile->polys[ip];

			// Unique color for each type.
			const int col = duIntToCol(linkTraverseType, 196);

			if (endPoly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
			{
				const dtOffMeshConnection* con = &endTile->offMeshCons[ip - endTile->header->offMeshBase];

				dd->begin(DU_DRAW_LINES, 2.0f, offset);

				dd->vertex(&con->pos[0], col);
				dd->vertex(&con->pos[3], col);

				const float* polyVerts = endTile->verts;
				const bool vertOrder = !(link->traverseType & DT_OFFMESH_CON_TRAVERSE_ON_VERT);

				const float* baseVert = &polyVerts[endPoly->verts[vertOrder?1:0]*3];
				const float* landVert = &polyVerts[endPoly->verts[vertOrder?0:1]*3];

				duAppendCross(dd, baseVert[0], baseVert[1], baseVert[2], con->rad, duRGBA(220,32,16,196));
				duAppendCross(dd, landVert[0], landVert[1], landVert[2], con->rad, duRGBA(32,220,16,196));

				dd->end();
				continue;
			}

			float startPos[3];
			float endPos[3];

			query->getEdgeMidPoint(basePolyRef, link->ref, startPos);
			query->getEdgeMidPoint(link->ref, basePolyRef, endPos);

			const float walkableRadius = tile->header->walkableRadius;
			float offsetAmount;

			if (traverseLinkParams.dynamicOffset)
			{
				const float totLedgeSpan = walkableRadius+traverseLinkParams.extraOffset;
				const float slopeAngle = rdMathFabsf(rdCalcSlopeAngle(startPos, endPos));
				const float maxAngle = rdCalcMaxLOSAngle(totLedgeSpan, traverseLinkParams.cellHeight);

				offsetAmount = rdCalcLedgeSpanOffsetAmount(totLedgeSpan, slopeAngle, maxAngle);
			}
			else
				offsetAmount = walkableRadius + traverseLinkParams.extraOffset;

			const bool startPointHighest = startPos[2] > endPos[2];
			float* highestPos = startPointHighest ? startPos : endPos;

			const dtPolyRef lowPolyRef = startPointHighest ? link->ref : basePolyRef;
			const dtPolyRef highPolyRef = startPointHighest ? basePolyRef : link->ref;

			float normal[3];
			query->getEdgeNormal(highPolyRef, lowPolyRef, normal);

			// The offset between the height point and the ray point
			// used to account for the ledge span.
			const float offsetEndPos[3] = {
				highestPos[0] + normal[0] * offsetAmount,
				highestPos[1] + normal[1] * offsetAmount,
				highestPos[2]
			};

			dd->begin(DU_DRAW_LINES, 2.0f, offset);

			const float* targetStartPos = startPointHighest ? offsetEndPos : startPos;
			const float* targetEndPos = startPointHighest ? startPos : offsetEndPos;

			dd->vertex(targetStartPos, col);
			dd->vertex(targetEndPos, col);

			bool hasValidReverseLink = false;

			if (link->reverseLink != DT_NULL_TRAVERSE_REVERSE_LINK)
			{
				dtLink& reverseLink = endTile->links[link->reverseLink];

				if (reverseLink.ref == basePolyRef)
					hasValidReverseLink = true;
			}

			// If the reverse link is set, render light crosses to confirm
			// the links are set properly, else render dark which will make
			// them look incomplete.
			const unsigned int crossCol = hasValidReverseLink ? duRGBA(255, 255, 255, 196) : duRGBA(0, 0, 0, 196);
			duAppendCross(dd, startPos[0], startPos[1], startPos[2], 10.f, crossCol);

			dd->end();
		}
	}
}

static void drawTileCells(duDebugDraw* dd, const dtMeshTile* tile, const float* offset)
{
#if DT_NAVMESH_SET_VERSION >= 8 
	for (int i = 0; i < tile->header->maxCellCount; i++)
	{
		const dtCell& cell = tile->cells[i];
		duDebugDrawCross(dd, cell.pos[0], cell.pos[1], cell.pos[2], 25.f, duRGBA(255,0,0,255), 2, offset);
	}
#else
	rdIgnoreUnused(dd);
	rdIgnoreUnused(tile);
	rdIgnoreUnused(offset);
#endif
}

static void drawTileBounds(duDebugDraw* dd, const dtMeshTile* tile, const float* offset)
{
	float bmin[3];
	float bmax[3];

	tile->getTightBounds(bmin, bmax);
	duDebugDrawBoxWire(dd, bmin[0],bmin[1],bmin[2], bmax[0],bmax[1],bmax[2], duRGBA(255,255,255,128), 1.0f,offset);
}

static void drawOffMeshConnectionRefPosition(duDebugDraw* dd, const dtOffMeshConnection* con)
{
	float refPosDir[3];
	const float arrowLength[3] = { 35.f, 35.f, 0.f };
	dtCalcOffMeshRefPos(con->refPos, con->refYaw, arrowLength, refPosDir);

	duAppendArrow(dd, con->refPos[0], con->refPos[1], con->refPos[2],
		refPosDir[0], refPosDir[1], refPosDir[2], 0.f, 10.f, duRGBA(255,255,0,255));
}

static void drawOffMeshConnections(duDebugDraw* dd, const dtNavMesh& mesh, const dtNavMeshQuery* query,
	const dtMeshTile* tile, const float* offset)
{
	const dtMeshHeader* header = tile->header;
	const dtPolyRef base = mesh.getPolyRefBase(tile);

	dd->begin(DU_DRAW_LINES, 2.0f, offset);
	for (int i = 0; i < header->polyCount; ++i)
	{
		const dtPoly* p = &tile->polys[i];
		if (p->getType() != DT_POLYTYPE_OFFMESH_CONNECTION)	// Skip regular polys.
			continue;
		
		const dtOffMeshConnection* con = &tile->offMeshCons[i - header->offMeshBase];

		unsigned int col;
		if (query && query->isInClosedList(base | (dtPolyRef)i))
			col = duRGBA(255,196,0,220);
		else
			col = duDarkenCol(duTransCol(dd->areaToFaceCol(p->getArea()), 220));

		const float* va = &tile->verts[p->verts[0]*3];
		const float* vb = &tile->verts[p->verts[1]*3];

		// Check to see if start and end end-points have links.
		bool startSet = false;
		bool endSet = false;
		for (unsigned int k = p->firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
		{
			const dtLink& link = tile->links[k];

			if (link.edge == 0)
				startSet = true;
			if (link.edge == 1)
				endSet = true;
		}
		
		// End points and their on-mesh locations.
		dd->vertex(va[0],va[1],va[2], col);
		dd->vertex(con->pos[0],con->pos[1],con->pos[2], col);
		duAppendCircle(dd, con->pos[0],con->pos[1],con->pos[2]+5.0f, con->rad, duRGBA(220,32,16,196));

		if (startSet)
			duAppendCross(dd, con->pos[0],con->pos[1],con->pos[2]+5.0f, con->rad, duRGBA(220,220,16,196));

		dd->vertex(vb[0],vb[1],vb[2], col);
		dd->vertex(con->pos[3],con->pos[4],con->pos[5], col);
		duAppendCircle(dd, con->pos[3],con->pos[4],con->pos[5]+5.0f, con->rad, duRGBA(32,220,16,196));

		if (endSet)
			duAppendCross(dd, con->pos[3],con->pos[4],con->pos[5]+5.0f, con->rad, duRGBA(220,220,16,196));
		
		// End point vertices.
		dd->vertex(con->pos[0],con->pos[1],con->pos[2], duRGBA(0,48,64,196));
		dd->vertex(con->pos[0],con->pos[1],con->pos[2]+10.0f, duRGBA(0,48,64,196));
		
		dd->vertex(con->pos[3],con->pos[4],con->pos[5], duRGBA(0,48,64,196));
		dd->vertex(con->pos[3],con->pos[4],con->pos[5]+10.0f, duRGBA(0,48,64,196));
		
		// Connection arc.
		duAppendArc(dd, con->pos[0],con->pos[1],con->pos[2], con->pos[3],con->pos[4],con->pos[5], 0.25f,
#if DT_NAVMESH_SET_VERSION >= 7
					30.f,
#else
					(con->flags & DT_OFFMESH_CON_BIDIR) ? 30.0f : 0.0f, 
#endif
			30.0f, col);

		// Reference positions.
		drawOffMeshConnectionRefPosition(dd, con);
	}
	dd->end();
}

void duDebugDrawMeshTile(duDebugDraw* dd, const dtNavMesh& mesh, const dtNavMeshQuery* query,
						 const dtMeshTile* tile, const float* offset, unsigned int flags, const duDrawTraverseLinkParams& traverseLinkParams)
{
	const bool depthTest = flags & DU_DRAW_DETOURMESH_DEPTH_MASK;

	dd->depthMask(depthTest);

	if (flags & DU_DRAW_DETOURMESH_POLY_FACES)
		drawPolyMeshFaces(dd, mesh, query, tile, offset, flags);

	if (flags & DU_DRAW_DETOURMESH_POLY_EDGES)
		drawPolyMeshEdges(dd, tile, offset);

	if (flags & DU_DRAW_DETOURMESH_POLY_VERTS)
		drawPolyVerts(dd, tile, offset);

	// Draw inner poly boundaries
	if (flags & DU_DRAW_DETOURMESH_POLY_BOUNDS_INNER)
		drawPolyBoundaries(dd, tile, 1.5f, offset, flags, true);
	
	// Draw outer poly boundaries
	if (flags & DU_DRAW_DETOURMESH_POLY_BOUNDS_OUTER)
		drawPolyBoundaries(dd, tile, 3.5f, offset, flags, false);

	// Draw poly centers
	if (flags & DU_DRAW_DETOURMESH_POLY_CENTERS)
		drawPolyCenters(dd, tile, duRGBA(255, 255, 255, 100), 1.0f, offset);

	if (query && (flags & DU_DRAW_DETOURMESH_TRAVERSE_LINKS))
		drawTraverseLinks(dd, mesh, query, tile, offset, traverseLinkParams);

	if (flags & DU_DRAW_DETOURMESH_TILE_CELLS)
		drawTileCells(dd, tile, offset);

	if (flags & DU_DRAW_DETOURMESH_TILE_BOUNDS)
		drawTileBounds(dd, tile, offset);

	if (flags & DU_DRAW_DETOURMESH_OFFMESHCONS)
		drawOffMeshConnections(dd, mesh, query, tile, offset);

	if (!depthTest)
		dd->depthMask(true);
}

void duDebugDrawNavMesh(duDebugDraw* dd, const dtNavMesh& mesh, const float* offset, unsigned int flags, const duDrawTraverseLinkParams& traverseLinkParams)
{
	if (!dd) return;
	
	for (int i = 0; i < mesh.getMaxTiles(); ++i)
	{
		const dtMeshTile* tile = mesh.getTile(i);
		if (!tile->header) continue;
		duDebugDrawMeshTile(dd, mesh, 0, tile, offset, flags, traverseLinkParams);
	}
}

void duDebugDrawNavMeshWithClosedList(struct duDebugDraw* dd, const dtNavMesh& mesh, const dtNavMeshQuery& query, const float* offset, unsigned int flags, const duDrawTraverseLinkParams& traverseLinkParams)
{
	if (!dd) return;

	const dtNavMeshQuery* q = (flags & DU_DRAW_DETOURMESH_WITH_CLOSED_LIST) ? &query : 0;
	
	for (int i = 0; i < mesh.getMaxTiles(); ++i)
	{
		const dtMeshTile* tile = mesh.getTile(i);
		if (!tile->header) continue;
		duDebugDrawMeshTile(dd, mesh, q, tile, offset, flags, traverseLinkParams);
	}

	if (flags & DU_DRAW_DETOURMESH_BVTREE)
		duDebugDrawNavMeshBVTree(dd, mesh, offset);
	if (flags & DU_DRAW_DETOURMESH_PORTALS)
		duDebugDrawNavMeshPortals(dd, mesh, offset);
	if (flags & DU_DRAW_DETOURMESH_QUERY_NODES)
		duDebugDrawNavMeshNodes(dd, query, offset);
}

void duDebugDrawNavMeshNodes(struct duDebugDraw* dd, const dtNavMeshQuery& query, const float* offset)
{
	if (!dd) return;
	
	const dtNodePool* pool = query.getNodePool();
	if (pool)
	{
		const float off = 0.5f;
		dd->begin(DU_DRAW_POINTS, 4.0f, offset);
		for (int i = 0; i < pool->getHashSize(); ++i)
		{
			for (dtNodeIndex j = pool->getFirst(i); j != DT_NULL_IDX; j = pool->getNext(j))
			{
				const dtNode* node = pool->getNodeAtIdx(j+1);
				if (!node) continue;
				dd->vertex(node->pos[0],node->pos[1],node->pos[2] + off, duRGBA(255,192,0,255));
			}
		}
		dd->end();
		
		dd->begin(DU_DRAW_LINES, 2.0f, offset);
		for (int i = 0; i < pool->getHashSize(); ++i)
		{
			for (dtNodeIndex j = pool->getFirst(i); j != DT_NULL_IDX; j = pool->getNext(j))
			{
				const dtNode* node = pool->getNodeAtIdx(j+1);
				if (!node) continue;
				if (!node->pidx) continue;
				const dtNode* parent = pool->getNodeAtIdx(node->pidx);
				if (!parent) continue;
				dd->vertex(node->pos[0],node->pos[1],node->pos[2] + off, duRGBA(255,192,0,128));
				dd->vertex(parent->pos[0],parent->pos[1],parent->pos[2] + off, duRGBA(255,192,0,128));
			}
		}
		dd->end();
	}
}


static void drawMeshTileBVTree(duDebugDraw* dd, const dtMeshTile* tile, const float* offset)
{
	// Draw BV nodes.
	const float cs = 1.0f / tile->header->bvQuantFactor;
	dd->begin(DU_DRAW_LINES, 1.0f, offset);
	for (int i = 0; i < tile->header->bvNodeCount; ++i)
	{
		const dtBVNode* n = &tile->bvTree[i];
		if (n->i < 0) // Leaf indices are positive.
			continue;
		duAppendBoxWire(dd, 
						tile->header->bmax[0] - n->bmax[0]*cs,
						tile->header->bmin[1] + n->bmin[1]*cs,
						tile->header->bmin[2] + n->bmin[2]*cs,
						tile->header->bmax[0] - n->bmin[0]*cs,
						tile->header->bmin[1] + n->bmax[1]*cs,
						tile->header->bmin[2] + n->bmax[2]*cs,
						duRGBA(255,255,255,128));
	}
	dd->end();
}

void duDebugDrawNavMeshBVTree(duDebugDraw* dd, const dtNavMesh& mesh, const float* offset)
{
	if (!dd) return;
	
	for (int i = 0; i < mesh.getMaxTiles(); ++i)
	{
		const dtMeshTile* tile = mesh.getTile(i);
		if (!tile->header) continue;
		drawMeshTileBVTree(dd, tile, offset);
	}
}

static void drawMeshTilePortal(duDebugDraw* dd, const dtMeshTile* tile, const float* offset)
{
	// Draw portals
	const float padx = 0.04f;
	const float padz = tile->header->walkableClimb;

	dd->begin(DU_DRAW_LINES, 2.0f, offset);

	for (int side = 0; side < 8; ++side)
	{
		unsigned short m = DT_EXT_LINK | (unsigned short)side;
		
		for (int i = 0; i < tile->header->polyCount; ++i)
		{
			dtPoly* poly = &tile->polys[i];
			
			// Create new links.
			const int nv = poly->vertCount;
			for (int j = 0; j < nv; ++j)
			{
				// Skip edges which do not point to the right side.
				if (poly->neis[j] != m)
					continue;
				
				// Create new links
				const float* va = &tile->verts[poly->verts[j]*3];
				const float* vb = &tile->verts[poly->verts[(j+1) % nv]*3];
				
				if (side == 0 || side == 4)
				{
					unsigned int col = side == 0 ? duRGBA(128,0,0,128) : duRGBA(128,0,128,128);

					const float x = va[0] + ((side == 0) ? -padx : padx);
					
					dd->vertex(x, va[1], va[2] - padz, col);
					dd->vertex(x, va[1], va[2] + padz, col);

					dd->vertex(x, va[1], va[2] + padz, col);
					dd->vertex(x, vb[1], vb[2] + padz, col);

					dd->vertex(x, vb[1], vb[2] + padz, col);
					dd->vertex(x, vb[1], vb[2] - padz, col);

					dd->vertex(x, vb[1], vb[2] - padz, col);
					dd->vertex(x, va[1], va[2] - padz, col);
				}
				else if (side == 2 || side == 6)
				{
					unsigned int col = side == 2 ? duRGBA(0,128,0,128) : duRGBA(0,128,128,128);

					const float y = va[1] + ((side == 2) ? -padx : padx);
					
					dd->vertex(va[0], y, va[2] - padz, col);
					dd->vertex(va[0], y, va[2] + padz, col);

					dd->vertex(va[0], y, va[2] + padz, col);
					dd->vertex(vb[0], y, vb[2] + padz, col);

					dd->vertex(vb[0], y, vb[2] + padz, col);
					dd->vertex(vb[0], y, vb[2] - padz, col);

					dd->vertex(vb[0], y, vb[2] - padz, col);
					dd->vertex(va[0], y, va[2] - padz, col);
				}
			}
		}
	}
	
	dd->end();
}

void duDebugDrawNavMeshPortals(duDebugDraw* dd, const dtNavMesh& mesh, const float* offset)
{
	if (!dd) return;
	
	for (int i = 0; i < mesh.getMaxTiles(); ++i)
	{
		const dtMeshTile* tile = mesh.getTile(i);
		if (!tile->header) continue;
		drawMeshTilePortal(dd, tile, offset);
	}
}

void duDebugDrawNavMeshPolysWithFlags(struct duDebugDraw* dd, const dtNavMesh& mesh, const unsigned short polyFlags,
									  const float* offset, const unsigned int drawFlags, const unsigned int col, const bool soften)
{
	if (!dd) return;
	
	for (int i = 0; i < mesh.getMaxTiles(); ++i)
	{
		const dtMeshTile* tile = mesh.getTile(i);
		if (!tile->header) continue;
		dtPolyRef base = mesh.getPolyRefBase(tile);

		for (int j = 0; j < tile->header->polyCount; ++j)
		{
			const dtPoly* p = &tile->polys[j];
			if ((p->flags & polyFlags) == 0) continue;
			duDebugDrawNavMeshPoly(dd, mesh, base|(dtPolyRef)j, offset, col, drawFlags, soften);
		}
	}
}

void duDebugDrawNavMeshPoly(duDebugDraw* dd, const dtNavMesh& mesh, dtPolyRef ref, const float* offset, const unsigned int drawFlags, const unsigned int col, const bool soften)
{
	if (!dd) return;
	
	const dtMeshTile* tile = 0;
	const dtPoly* poly = 0;
	if (dtStatusFailed(mesh.getTileAndPolyByRef(ref, &tile, &poly)))
		return;
	
	const bool depthTest = drawFlags & DU_DRAW_DETOURMESH_DEPTH_MASK;
	dd->depthMask(depthTest);
	
	const unsigned int c = soften ? duTransCol(col, 64) : col;
	const unsigned int ip = (unsigned int)(poly - tile->polys);

	if (poly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
	{
		if (drawFlags & DU_DRAW_DETOURMESH_OFFMESHCONS)
		{
			dtOffMeshConnection* con = &tile->offMeshCons[ip - tile->header->offMeshBase];

			dd->begin(DU_DRAW_LINES, 2.0f, offset);

			// Connection arc.
			duAppendArc(dd, con->pos[0],con->pos[1],con->pos[2], con->pos[3],con->pos[4],con->pos[5], 0.25f,
#if DT_NAVMESH_SET_VERSION >= 7
						30.f,
#else
						(con->flags & DT_OFFMESH_CON_BIDIR) ? 30.0f : 0.0f,
#endif
				30.0f, c);

			// Reference positions.
			drawOffMeshConnectionRefPosition(dd, con);
		
			dd->end();
		}
	}
	else
	{
		const dtPolyDetail* pd = &tile->detailMeshes[ip];

		dd->begin(DU_DRAW_TRIS, 1.0f, offset);
		for (int i = 0; i < pd->triCount; ++i)
		{
			const unsigned char* t = &tile->detailTris[(pd->triBase+i)*4];
			for (int j = 0; j < 3; ++j)
			{
				if (t[j] < poly->vertCount)
					dd->vertex(&tile->verts[poly->verts[t[j]]*3], c);
				else
					dd->vertex(&tile->detailVerts[(pd->vertBase+t[j]-poly->vertCount)*3], c);
			}
		}
		dd->end();

		if (drawFlags & DU_DRAW_DETOURMESH_POLY_EDGES)
		{
			const unsigned int edgeCol = duRGBA(0,0,0,48);

			dd->begin(DU_DRAW_LINES, 1.0f, offset);
			for (int i = 0; i < pd->triCount; ++i)
			{
				const unsigned char* t = &tile->detailTris[(pd->triBase+i)*4];
				for (int j = 0; j < 3; ++j)
				{
					if (t[j] < poly->vertCount)
						dd->vertex(&tile->verts[poly->verts[t[j]]*3], edgeCol);
					else
						dd->vertex(&tile->detailVerts[(pd->vertBase+t[j]-poly->vertCount)*3], edgeCol);
				}
			}
			dd->end();
		}
	}
	
	if (!depthTest)
		dd->depthMask(true);
}

static void debugDrawTileCachePortals(struct duDebugDraw* dd, const dtTileCacheLayer& layer, const float cs, const float ch, const float* offset)
{
	const int w = (int)layer.header->width;
	const int h = (int)layer.header->height;
	const float* bmin = layer.header->bmin;

	// Portals
	unsigned int pcol = duRGBA(255,255,255,255);
	
	const int segs[4*4] = {0,0,0,1, 0,1,1,1, 1,1,1,0, 1,0,0,0};
	
	// Layer portals
	dd->begin(DU_DRAW_LINES, 2.0f, offset);
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const int idx = x+y*w;
			const int lh = (int)layer.heights[idx];
			if (lh == 0xff) continue;
			
			for (int dir = 0; dir < 4; ++dir)
			{
				if (layer.cons[idx] & (1<<(dir+4)))
				{
					const int* seg = &segs[dir*4];
					const float ax = bmin[0] + (x+seg[0])*cs;
					const float ay = bmin[1] + (y+seg[1])*cs;
					const float az = bmin[2] + (lh+1)*ch;
					const float bx = bmin[0] + (x+seg[1])*cs;
					const float by = bmin[1] + (y+seg[1])*cs;
					const float bz = bmin[2] + (lh+3)*ch;
					dd->vertex(ax, ay, az, pcol);
					dd->vertex(bx, by, bz, pcol);
				}
			}
		}
	}
	dd->end();
}

void duDebugDrawTileCacheLayerAreas(struct duDebugDraw* dd, const dtTileCacheLayer& layer, const float cs, const float ch, const float* offset)
{
	const int w = (int)layer.header->width;
	const int h = (int)layer.header->height;
	const float* bmin = layer.header->bmin;
	const float* bmax = layer.header->bmax;
	const int idx = layer.header->tlayer;
	
	unsigned int color = duIntToCol(idx+1, 255);
	
	// Layer bounds
	float lbmin[3], lbmax[3];
	lbmin[0] = bmin[0] + layer.header->minx*cs;
	lbmin[1] = bmin[1] + layer.header->miny*cs;
	lbmin[2] = bmin[2];
	lbmax[0] = bmin[0] + (layer.header->maxx+1)*cs;
	lbmax[1] = bmax[1] + (layer.header->maxy+1)*cs;
	lbmax[2] = bmin[2];
	duDebugDrawBoxWire(dd, lbmin[0],lbmin[1],lbmin[2], lbmax[0],lbmax[1],lbmax[2], duTransCol(color,128), 2.0f, offset);
	
	// Layer height
	dd->begin(DU_DRAW_QUADS, 1.0f, offset);
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const int lidx = x+y*w;
			const int lh = (int)layer.heights[lidx];
			if (lh == 0xff) continue;

			const unsigned char area = layer.areas[lidx];
			unsigned int col;
			if (area == 63)
				col = duLerpCol(color, duRGBA(0,192,255,64), 32);
			else if (area == 0)
				col = duLerpCol(color, duRGBA(0,0,0,64), 32);
			else
				col = duLerpCol(color, dd->areaToFaceCol(area), 32);
			
			const float fx = bmin[0] + x*cs;
			const float fy = bmin[1] + y*cs;
			const float fz = bmin[2] + (lh+1)*ch;
			
			dd->vertex(fx, fy, fz, col);
			dd->vertex(fx, fy+cs, fz, col);
			dd->vertex(fx+cs, fy+cs, fz, col);
			dd->vertex(fx+cs, fy, fz, col);
		}
	}
	dd->end();
	
	debugDrawTileCachePortals(dd, layer, cs, ch, offset);
}

void duDebugDrawTileCacheLayerRegions(struct duDebugDraw* dd, const dtTileCacheLayer& layer, const float cs, const float ch, const float* offset)
{
	const int w = (int)layer.header->width;
	const int h = (int)layer.header->height;
	const float* bmin = layer.header->bmin;
	const float* bmax = layer.header->bmax;
	const int idx = layer.header->tlayer;
	
	unsigned int color = duIntToCol(idx+1, 255);
	
	// Layer bounds
	float lbmin[3], lbmax[3];
	lbmin[0] = bmin[0] + layer.header->minx*cs;
	lbmin[1] = bmin[1] + layer.header->miny*cs;
	lbmin[2] = bmin[2];
	lbmax[0] = bmin[0] + (layer.header->maxx+1)*cs;
	lbmax[1] = bmax[1] + (layer.header->maxy+1)*cs;
	lbmax[2] = bmin[2];
	duDebugDrawBoxWire(dd, lbmin[0],lbmin[1],lbmin[2], lbmax[0],lbmax[1],lbmax[2], duTransCol(color,128), 2.0f, offset);
	
	// Layer height
	dd->begin(DU_DRAW_QUADS, 1.0f, offset);
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const int lidx = x+y*w;
			const int lh = (int)layer.heights[lidx];
			if (lh == 0xff) continue;
			const unsigned char reg = layer.regs[lidx];
			
			unsigned int col = duLerpCol(color, duIntToCol(reg, 255), 192);
			
			const float fx = bmin[0] + x*cs;
			const float fy = bmin[1] + y*cs;
			const float fz = bmin[2] + (lh+1)*ch;
			
			dd->vertex(fx, fy, fz, col);
			dd->vertex(fx, fy+cs, fz, col);
			dd->vertex(fx+cs, fy+cs, fz, col);
			dd->vertex(fx+cs, fy, fz, col);
		}
	}
	dd->end();
	
	debugDrawTileCachePortals(dd, layer, cs, ch, offset);
}




/*struct dtTileCacheContour
{
	int nverts;
	unsigned char* verts;
	unsigned char reg;
	unsigned char area;
};

struct dtTileCacheContourSet
{
	int nconts;
	dtTileCacheContour* conts;
};*/

void duDebugDrawTileCacheContours(duDebugDraw* dd, const struct dtTileCacheContourSet& lcset,
								  const float* orig, const float cs, const float ch, const float* offset)
{
	if (!dd) return;
	
	const unsigned char a = 255;// (unsigned char)(alpha*255.0f);
	
	const int offs[2*4] = {-1,0, 0,1, 1,0, 0,-1};
	
	dd->begin(DU_DRAW_LINES, 2.0f, offset);
	
	for (int i = 0; i < lcset.nconts; ++i)
	{
		const dtTileCacheContour& c = lcset.conts[i];
		unsigned int color = 0;
		
		color = duIntToCol(i, a);
		
		for (int j = 0; j < c.nverts; ++j)
		{
			const int k = (j+1) % c.nverts;
			const unsigned char* va = &c.verts[j*4];
			const unsigned char* vb = &c.verts[k*4];
			const float ax = orig[0] + va[0]*cs;
			const float ay = orig[1] + va[1]*cs;
			const float az = orig[2] +(va[2]+1+(i&1))*ch;
			const float bx = orig[0] + vb[0]*cs;
			const float by = orig[1] + vb[1]*cs;
			const float bz = orig[2] +(vb[2]+1+(i&1))*ch;
			unsigned int col = color;
			if ((va[3] & 0xf) != 0xf)
			{
				// Portal segment
				col = duRGBA(255,255,255,128);
				int d = va[3] & 0xf;
				
				const float cx = (ax+bx)*0.5f;
				const float cy = (ay+by)*0.5f;
				const float cz = (az+bz)*0.5f;
				
				const float dx = cx + offs[d*2+0]*2*cs;
				const float dy = cy + offs[d*2+1]*2*cs;
				const float dz = cz;
				
				dd->vertex(cx,cy,cz,duRGBA(255,0,0,255));
				dd->vertex(dx,dy,dz,duRGBA(255,0,0,255));
			}
			
			duAppendArrow(dd, ax,ay,az, bx,by,bz, 0.0f, cs*0.5f, col);
		}
	}
	dd->end();
	
	dd->begin(DU_DRAW_POINTS, 4.0f, offset);
	
	for (int i = 0; i < lcset.nconts; ++i)
	{
		const dtTileCacheContour& c = lcset.conts[i];
		unsigned int color = 0;
		
		for (int j = 0; j < c.nverts; ++j)
		{
			const unsigned char* va = &c.verts[j*4];
			
			color = duDarkenCol(duIntToCol(i, a));
			if (va[3] & 0x80)
			{
				// Border vertex
				color = duRGBA(255,0,0,255);
			}
			
			float fx = orig[0] + va[0]*cs;
			float fy = orig[1] + va[1]*cs;
			float fz = orig[2] +(va[2]+1+(i&1))*ch;
			dd->vertex(fx,fy,fz, color);
		}
	}
	dd->end();
}

void duDebugDrawTileCachePolyMesh(duDebugDraw* dd, const struct dtTileCachePolyMesh& lmesh,
								  const float* orig, const float cs, const float ch, const float* offset)
{
	if (!dd) return;
	
	const int nvp = lmesh.nvp;
	
	const int offs[2*4] = {-1,0, 0,1, 1,0, 0,-1};
	
	dd->begin(DU_DRAW_TRIS, 1.0f, offset);
	
	for (int i = 0; i < lmesh.npolys; ++i)
	{
		const unsigned short* p = &lmesh.polys[i*nvp*2];
		const unsigned char area = lmesh.areas[i];
		
		unsigned int color;
		if (area == DT_TILECACHE_WALKABLE_AREA)
			color = duRGBA(0,192,255,64);
		else if (area == DT_TILECACHE_NULL_AREA)
			color = duRGBA(0,0,0,64);
		else
			color = dd->areaToFaceCol(area);
		
		unsigned short vi[3];
		for (int j = 2; j < nvp; ++j)
		{
			if (p[j] == DT_TILECACHE_NULL_IDX) break;
			vi[0] = p[0];
			vi[1] = p[j-1];
			vi[2] = p[j];
			for (int k = 0; k < 3; ++k)
			{
				const unsigned short* v = &lmesh.verts[vi[k]*3];
				const float x = orig[0] + v[0]*cs;
				const float y = orig[1] + v[1]*cs;
				const float z = orig[2] +(v[2]+1)*ch;
				dd->vertex(x,y,z, color);
			}
		}
	}
	dd->end();
	
	// Draw neighbours edges
	const unsigned int coln = duRGBA(0,48,64,32);
	dd->begin(DU_DRAW_LINES, 1.5f, offset);
	for (int i = 0; i < lmesh.npolys; ++i)
	{
		const unsigned short* p = &lmesh.polys[i*nvp*2];
		for (int j = 0; j < nvp; ++j)
		{
			if (p[j] == DT_TILECACHE_NULL_IDX) break;
			if (p[nvp+j] & 0x8000) continue;
			const int nj = (j+1 >= nvp || p[j+1] == DT_TILECACHE_NULL_IDX) ? 0 : j+1; 
			int vi[2] = {p[j], p[nj]};
			
			for (int k = 0; k < 2; ++k)
			{
				const unsigned short* v = &lmesh.verts[vi[k]*3];
				const float x = orig[0] + v[0]*cs;
				const float y = orig[1] + v[1]*cs;
				const float z = orig[2] +(v[2]+1)*ch + 0.1f;
				dd->vertex(x,y,z, coln);
			}
		}
	}
	dd->end();
	
	// Draw boundary edges
	const unsigned int colb = duRGBA(0,48,64,220);
	dd->begin(DU_DRAW_LINES, 3.5f, offset);
	for (int i = 0; i < lmesh.npolys; ++i)
	{
		const unsigned short* p = &lmesh.polys[i*nvp*2];
		for (int j = 0; j < nvp; ++j)
		{
			if (p[j] == DT_TILECACHE_NULL_IDX) break;
			if ((p[nvp+j] & 0x8000) == 0) continue;
			const int nj = (j+1 >= nvp || p[j+1] == DT_TILECACHE_NULL_IDX) ? 0 : j+1; 
			int vi[2] = {p[j], p[nj]};
			
			unsigned int col = colb;
			if ((p[nvp+j] & 0xf) != 0xf)
			{
				const unsigned short* va = &lmesh.verts[vi[0]*3];
				const unsigned short* vb = &lmesh.verts[vi[1]*3];
				
				const float ax = orig[0] + va[0]*cs;
				const float ay = orig[1] + va[1]*cs;
				const float az = orig[2] +(va[2]+1+(i&1))*ch;
				const float bx = orig[0] + vb[0]*cs;
				const float by = orig[1] + vb[1]*cs;
				const float bz = orig[2] +(vb[2]+1+(i&1))*ch;
				
				const float cx = (ax+bx)*0.5f;
				const float cy = (ay+by)*0.5f;
				const float cz = (az+bz)*0.5f;
				
				int d = p[nvp+j] & 0xf;
				
				const float dx = cx + offs[d*2+0]*2*cs;
				const float dy = cy + offs[d*2+1]*2*cs;
				const float dz = cz;
				
				dd->vertex(cx,cy,cz,duRGBA(255,0,0,255));
				dd->vertex(dx,dy,dz,duRGBA(255,0,0,255));
				
				col = duRGBA(255,255,255,128);
			}
			
			for (int k = 0; k < 2; ++k)
			{
				const unsigned short* v = &lmesh.verts[vi[k]*3];
				const float x = orig[0] + v[0]*cs;
				const float y = orig[1] + v[1]*cs;
				const float z = orig[2] +(v[2]+1)*ch + 0.1f;
				dd->vertex(x,y,z, col);
			}
		}
	}
	dd->end();
	
	dd->begin(DU_DRAW_POINTS, 4.0f, offset);
	const unsigned int colv = duRGBA(0,0,0,220);
	for (int i = 0; i < lmesh.nverts; ++i)
	{
		const unsigned short* v = &lmesh.verts[i*3];
		const float x = orig[0] + v[0]*cs;
		const float y = orig[1] + v[1]*cs;
		const float z = orig[2] +(v[2]+1)*ch + 0.1f;
		dd->vertex(x,y,z, colv);
	}
	dd->end();
}
