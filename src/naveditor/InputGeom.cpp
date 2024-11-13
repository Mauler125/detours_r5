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

#include "Recast/Include/Recast.h"
#include "NavEditor/Include/InputGeom.h"
#include "NavEditor/Include/ChunkyTriMesh.h"
#include "NavEditor/Include/MeshLoaderObj.h"
#include "NavEditor/Include/MeshLoaderPly.h"
#include "DebugUtils/Include/DebugDraw.h"
#include "DebugUtils/Include/RecastDebugDraw.h"
#include "Detour/Include/DetourNavMesh.h"
#include "NavEditor/Include/Editor.h"
#include <naveditor/include/GameUtils.h>

static char* parseRow(char* buf, char* bufEnd, char* row, int len)
{
	bool start = true;
	bool done = false;
	int n = 0;
	while (!done && buf < bufEnd)
	{
		char c = *buf;
		buf++;
		// multirow
		switch (c)
		{
			case '\n':
				if (start) break;
				done = true;
				break;
			case '\r':
				break;
			case '\t':
			case ' ':
				if (start) break;
				// else falls through
			default:
				start = false;
				row[n++] = c;
				if (n >= len-1)
					done = true;
				break;
		}
	}
	row[n] = '\0';
	return buf;
}



InputGeom::InputGeom() :
	m_chunkyMesh(0),
	m_mesh(0),
	m_hasBuildSettings(false),
	m_offMeshConCount(0),
	m_volumeCount(0)
{
	rdVset(m_meshBMin, 0.0f, 0.0f, 0.0f);
	rdVset(m_meshBMax, 0.0f, 0.0f, 0.0f);
	rdVset(m_navMeshBMin, 0.0f, 0.0f, 0.0f);
	rdVset(m_navMeshBMax, 0.0f, 0.0f, 0.0f);
}

InputGeom::~InputGeom()
{
	delete m_chunkyMesh;
	delete m_mesh;
}
		
bool InputGeom::loadMesh(rcContext* ctx, const std::string& filepath, const MeshFormat format)
{
	if (m_mesh)
	{
		delete m_chunkyMesh;
		m_chunkyMesh = 0;
		delete m_mesh;
		m_mesh = 0;
	}
	m_offMeshConCount = 0;
	m_volumeCount = 0;
	
	switch (format)
	{
	case MESH_OBJ:
		m_mesh = new rcMeshLoaderObj;
		break;
	case MESH_PLY:
		m_mesh = new rcMeshLoaderPly;
		break;
	default: // Unhandled mesh format.
		assert(0);
	}

	if (!m_mesh)
	{
		ctx->log(RC_LOG_ERROR, "loadMesh: Out of memory 'm_mesh'.");
		return false;
	}
	if (!m_mesh->load(filepath))
	{
		ctx->log(RC_LOG_ERROR, "buildTiledNavigation: Could not load '%s'", filepath.c_str());
		return false;
	}

	rcCalcBounds(m_mesh->getVerts(), m_mesh->getVertCount(), m_meshBMin, m_meshBMax);
	rdVcopy(m_navMeshBMin, m_meshBMin);
	rdVcopy(m_navMeshBMax, m_meshBMax);

	m_chunkyMesh = new rcChunkyTriMesh;
	if (!m_chunkyMesh)
	{
		ctx->log(RC_LOG_ERROR, "buildTiledNavigation: Out of memory 'm_chunkyMesh'.");
		return false;
	}
	if (!rcCreateChunkyTriMesh(m_mesh->getVerts(), m_mesh->getTris(), m_mesh->getTriCount(), 256, m_chunkyMesh))
	{
		ctx->log(RC_LOG_ERROR, "buildTiledNavigation: Failed to build chunky mesh.");
		return false;
	}

	return true;
}

bool InputGeom::loadGeomSet(rcContext* ctx, const std::string& filepath)
{
	//NB(warmist): tf2 not implemented here
	char* buf = 0;
	FILE* fp = fopen(filepath.c_str(), "rb");
	if (!fp)
	{
		return false;
	}
	if (fseek(fp, 0, SEEK_END) != 0)
	{
		fclose(fp);
		return false;
	}

	long bufSize = ftell(fp);
	if (bufSize < 0)
	{
		fclose(fp);
		return false;
	}
	if (fseek(fp, 0, SEEK_SET) != 0)
	{
		fclose(fp);
		return false;
	}
	buf = new char[bufSize];
	if (!buf)
	{
		fclose(fp);
		return false;
	}
	size_t readLen = fread(buf, bufSize, 1, fp);
	fclose(fp);
	if (readLen != 1)
	{
		delete[] buf;
		return false;
	}
	
	m_offMeshConCount = 0;
	m_volumeCount = 0;
	delete m_mesh;
	m_mesh = 0;

	char* src = buf;
	char* srcEnd = buf + bufSize;
	char row[512];
	while (src < srcEnd)
	{
		// Parse one row
		row[0] = '\0';
		src = parseRow(src, srcEnd, row, sizeof(row)/sizeof(char));
		if (row[0] == 'f')
		{
			// File name.
			const char* name = row+1;
			// Skip white spaces
			while (*name && isspace(*name))
				name++;
			if (*name)
			{
				if (!load(ctx, name))
				{
					delete [] buf;
					return false;
				}
			}
		}
		else if (row[0] == 'o')
		{
			// Off-mesh connection
			if (m_offMeshConCount < MAX_OFFMESH_CONNECTIONS)
			{
				float* verts = &m_offMeshConVerts[m_offMeshConCount*3*2];
				float* refs = &m_offMeshConRefPos[m_offMeshConCount*3];
				float rad;
				float yaw;
				int bidir = 0, jump = 0, order = 0, area = 0, flags = 0;
				sscanf(row+1, "%f %f %f %f %f %f %f %f %f %f %f %d %d %d %d %d",
					   &verts[0], &verts[1], &verts[2],
					   &verts[3], &verts[4], &verts[5],
					   &refs[0], &refs[1], &refs[2],
					   &rad,
					   &yaw,
					   &bidir, &jump, &order, &area, &flags);

				if (std::isnan(yaw))
				{
					const float offset[3] = { 0.0f,0.0f,rad };
					yaw = dtCalcOffMeshRefYaw(&verts[0], &verts[3]);
					dtCalcOffMeshRefPos(verts, yaw, offset, refs);
				}

				m_offMeshConRads[m_offMeshConCount] = rad;
				m_offMeshConRefYaws[m_offMeshConCount] = yaw;
				m_offMeshConDirs[m_offMeshConCount] = (unsigned char)bidir;
				m_offMeshConJumps[m_offMeshConCount] = (unsigned char)jump;
				m_offMeshConOrders[m_offMeshConCount] = (unsigned char)order;
				m_offMeshConAreas[m_offMeshConCount] = (unsigned char)area;
				m_offMeshConFlags[m_offMeshConCount] = (unsigned short)flags;
				m_offMeshConId[m_offMeshConCount] = 1000 + m_offMeshConCount;
				m_offMeshConCount++;
			}
		}
		else if (row[0] == 'b')
		{
			// Box volumes
			if (m_volumeCount < MAX_VOLUMES)
			{
				ShapeVolume* vol = &m_volumes[m_volumeCount++];

				sscanf(row+1, "%hu %hhu %f %f %f %f %f %f", &vol->flags, &vol->area, 
					&vol->verts[0], &vol->verts[1], &vol->verts[2],
					&vol->verts[3], &vol->verts[4], &vol->verts[5]);

				vol->hmin = 0.0f;
				vol->hmax = 0.0f;
				vol->nverts = 6;
				vol->type = VOLUME_BOX;
			}
		}
		else if (row[0] == 'c')
		{
			// Cylinder volumes
			if (m_volumeCount < MAX_VOLUMES)
			{
				ShapeVolume* vol = &m_volumes[m_volumeCount++];

				sscanf(row + 1, "%hu %hhu %f %f %f %f %f", &vol->flags, &vol->area,
					&vol->verts[0], &vol->verts[1], &vol->verts[2],
					&vol->verts[3], &vol->verts[4]);

				vol->hmin = 0.0f;
				vol->hmax = 0.0f;
				vol->nverts = 5;
				vol->type = VOLUME_CYLINDER;
			}
		}
		else if (row[0] == 'p')
		{
			// Convex polygon volumes
			if (m_volumeCount < MAX_VOLUMES)
			{
				ShapeVolume* vol = &m_volumes[m_volumeCount++];
				sscanf(row+1, "%d %hu %hhu %f %f", &vol->nverts, &vol->flags, &vol->area, &vol->hmin, &vol->hmax);
				for (int i = 0; i < vol->nverts; ++i)
				{
					row[0] = '\0';
					src = parseRow(src, srcEnd, row, sizeof(row)/sizeof(char));
					sscanf(row, "%f %f %f", &vol->verts[i*3+0], &vol->verts[i*3+1], &vol->verts[i*3+2]);
				}

				vol->type = VOLUME_CONVEX;
			}
		}
		else if (row[0] == 's')
		{
			// Settings
			m_hasBuildSettings = true;
			sscanf(row + 1, "%f %f %f %f %f %f %d %d %d %f %d %d %f %f %d %f %f %f %f %f %f %d",
							&m_buildSettings.cellSize,
							&m_buildSettings.cellHeight,
							&m_buildSettings.agentHeight,
							&m_buildSettings.agentRadius,
							&m_buildSettings.agentMaxClimb,
							&m_buildSettings.agentMaxSlope,
							&m_buildSettings.regionMinSize,
							&m_buildSettings.regionMergeSize,
							&m_buildSettings.edgeMaxLen,
							&m_buildSettings.edgeMaxError,
							&m_buildSettings.vertsPerPoly,
							&m_buildSettings.polyCellRes,
							&m_buildSettings.detailSampleDist,
							&m_buildSettings.detailSampleMaxError,
							&m_buildSettings.partitionType,
							&m_buildSettings.navMeshBMin[0],
							&m_buildSettings.navMeshBMin[1],
							&m_buildSettings.navMeshBMin[2],
							&m_buildSettings.navMeshBMax[0],
							&m_buildSettings.navMeshBMax[1],
							&m_buildSettings.navMeshBMax[2],
							&m_buildSettings.tileSize);

			// Copy the original values over so we can reset to them in the
			// editor after changes have been made.
			rdVcopy(m_buildSettings.origNavMeshBMin, m_buildSettings.navMeshBMin);
			rdVcopy(m_buildSettings.origNavMeshBMax, m_buildSettings.navMeshBMax);
		}
	}
	
	delete [] buf;
	
	return true;
}

bool InputGeom::load(rcContext* ctx, const std::string& filepath)
{
	size_t extensionPos = filepath.find_last_of('.');
	if (extensionPos == std::string::npos)
		return false;

	std::string extension = filepath.substr(extensionPos);
	std::transform(extension.begin(), extension.end(), extension.begin(),
		[](unsigned char c) { return char(::tolower(c)); });

	if (extension == ".gset")
		return loadGeomSet(ctx, filepath);
	if (extension == ".obj")
		return loadMesh(ctx, filepath, MESH_OBJ);
	if (extension == ".ply")
		return loadMesh(ctx, filepath, MESH_PLY);

	return false;
}

bool InputGeom::saveGeomSet(const BuildSettings* settings)
{
	if (!m_mesh) return false;
	
	// Change extension
	std::string filepath = m_mesh->getFileName();
	size_t extPos = filepath.find_last_of('.');
	if (extPos != std::string::npos)
		filepath = filepath.substr(0, extPos);

	filepath += ".gset";

	FILE* fp = fopen(filepath.c_str(), "w");
	if (!fp) return false;
	
	// Store mesh filename.
	fprintf(fp, "f %s\n", m_mesh->getFileName().c_str());

	// Store settings if any
	if (settings)
	{
		fprintf(fp,
			"s %f %f %f %f %f %f %d %d %d %f %d %d %f %f %d %f %f %f %f %f %f %d\n",
			settings->cellSize,
			settings->cellHeight,
			settings->agentHeight,
			settings->agentRadius,
			settings->agentMaxClimb,
			settings->agentMaxSlope,
			settings->regionMinSize,
			settings->regionMergeSize,
			settings->edgeMaxLen,
			settings->edgeMaxError,
			settings->vertsPerPoly,
			settings->polyCellRes,
			settings->detailSampleDist,
			settings->detailSampleMaxError,
			settings->partitionType,
			settings->navMeshBMin[0],
			settings->navMeshBMin[1],
			settings->navMeshBMin[2],
			settings->navMeshBMax[0],
			settings->navMeshBMax[1],
			settings->navMeshBMax[2],
			settings->tileSize);
	}
	
	// Store off-mesh links.
	for (int i = 0; i < m_offMeshConCount; ++i)
	{
		const float* verts = &m_offMeshConVerts[i*3*2];
		const float* refs = &m_offMeshConRefPos[i*3];
		const float rad = m_offMeshConRads[i];
		const float yaw = m_offMeshConRefYaws[i];
		const int bidir = m_offMeshConDirs[i];
		const int jump = m_offMeshConJumps[i];
		const int order = m_offMeshConOrders[i];
		const int area = m_offMeshConAreas[i];
		const int flags = m_offMeshConFlags[i];
		fprintf(fp, "o %f %f %f %f %f %f %f %f %f %f %f %d %d %d %d %d\n",
				verts[0], verts[1], verts[2],
				verts[3], verts[4], verts[5],
				refs[0], refs[1], refs[2],
				rad,
				yaw,
				bidir, jump, order, area, flags);
	}

	// Convex volumes
	for (int i = 0; i < m_volumeCount; ++i)
	{
		ShapeVolume* vol = &m_volumes[i];

		switch (vol->type)
		{
		case VOLUME_BOX:
			fprintf(fp, "b %hu %hhu %f %f %f %f %f %f\n", vol->flags, vol->area,
				vol->verts[0], vol->verts[1], vol->verts[2],
				vol->verts[3], vol->verts[4], vol->verts[5]);
			break;
		case VOLUME_CYLINDER:
			fprintf(fp, "c %hu %hhu %f %f %f %f %f\n", vol->flags, vol->area,
				vol->verts[0], vol->verts[1], vol->verts[2],
				vol->verts[3], vol->verts[4]);
			break;
		case VOLUME_CONVEX:
			fprintf(fp, "p %d %hu %hhu %f %f\n", vol->nverts, vol->flags, vol->area, vol->hmin, vol->hmax);
			for (int j = 0; j < vol->nverts; ++j)
				fprintf(fp, "%f %f %f\n", vol->verts[j*3+0], vol->verts[j*3+1], vol->verts[j*3+2]);
		}
	}
	
	fclose(fp);
	
	return true;
}

static const int MAX_CHUNK_INDICES = 0xffff;
static int s_chunkIndices[MAX_CHUNK_INDICES];

bool InputGeom::raycastMesh(const float* src, const float* dst, const unsigned int mask, int* vidx, float* tmin) const
{
	if (vidx)
		*vidx = -1;

	// Prune hit ray.
	float btmin = 0, btmax = 1;
	if (!rdIntersectSegmentAABB(src, dst, m_meshBMin, m_meshBMax, btmin, btmax))
		return false;

	bool hit = false;
	const int nvol = m_volumeCount;

	const bool traceClip = mask & TRACE_CLIP;
	const bool traceTrigger = mask & TRACE_TRIGGER;

	float isectTmin = 1.0f;
	int isectVolIdx = -1;

	for (int i = 0; i < nvol; i++)
	{
		const ShapeVolume& vol = m_volumes[i];
		float tsmin = 0.0f, tsmax = 1.0f;
		bool isect = false;

		if (vol.area == RC_NULL_AREA && !traceClip)
			continue;

		if (vol.area == DT_POLYAREA_TRIGGER && !traceTrigger)
			continue;

		if (vol.type == VOLUME_BOX)
		{
			if (rdIntersectSegmentAABB(src, dst, &vol.verts[0], &vol.verts[3], tsmin, tsmax))
				isect = true;
		}
		else if (vol.type == VOLUME_CYLINDER)
		{
			if (rdIntersectSegmentCylinder(src, dst, &vol.verts[0], vol.verts[3], vol.verts[4], tsmin, tsmax))
				isect = true;
		}
		else if (vol.type == VOLUME_CONVEX)
		{
			if (rdIntersectSegmentConvexHull(src, dst, vol.verts, vol.nverts, vol.hmin, vol.hmax, tsmin, tsmax))
				isect = true;
		}

		if (isect)
		{
			hit = true;

			// Caller isn't interested in finding the closest intersection; break out.
			if (!tmin)
			{
				isectVolIdx = i;
				break;
			}
			else if (tsmin < isectTmin)
			{
				isectTmin = tsmin;
				isectVolIdx = i;
			}
		}
	}

	if (hit)
	{
		if (vidx)
			*vidx = isectVolIdx;

		if (tmin)
			*tmin = isectTmin;
		else
			return true;
	}

	const bool traceWorld = mask & TRACE_WORLD;

	if (!traceWorld)
		return false;

	float p[3], q[3];
	p[0] = src[0] + (dst[0]-src[0]) * btmin;
	p[1] = src[1] + (dst[1]-src[1]) * btmin;
	p[2] = src[2] + (dst[2]-src[2]) * btmin;
	q[0] = src[0] + (dst[0]-src[0]) * btmax;
	q[1] = src[1] + (dst[1]-src[1]) * btmax;
	q[2] = src[2] + (dst[2]-src[2]) * btmax;
	
	const int ncid = rcGetChunksOverlappingSegment(m_chunkyMesh, p, q, s_chunkIndices, MAX_CHUNK_INDICES);
	if (!ncid)
		return false;

	const float* verts = m_mesh->getVerts();
	
	for (int i = 0; i < ncid; ++i)
	{
		const rcChunkyTriMeshNode& node = m_chunkyMesh->nodes[s_chunkIndices[i]];
		const int* tris = &m_chunkyMesh->tris[node.i*3];
		const int ntris = node.n;

		for (int j = 0; j < ntris*3; j += 3)
		{
			float t = 1;
			if (rdIntersectSegmentTriangle(src, dst,
										   &verts[tris[j]*3],
										   &verts[tris[j+1]*3],
										   &verts[tris[j+2]*3], t))
			{
				// Caller isn't interested in finding the closest intersection; return out.
				if (!tmin)
					return true;

				hit = true;

				if (t < isectTmin)
					isectTmin = t;
				else
					break;
			}
		}
	}

	if (tmin)
		*tmin = isectTmin;
	
	return hit;
}

int InputGeom::addOffMeshConnection(const float* spos, const float* epos, const float rad,
									 unsigned char bidir, unsigned char jump, unsigned char order,
									 unsigned char area, unsigned short flags)
{
	if (m_offMeshConCount >= MAX_OFFMESH_CONNECTIONS) return -1;
	rdAssert(jump < DT_MAX_TRAVERSE_TYPES);

	float* verts = &m_offMeshConVerts[m_offMeshConCount*3*2];
	rdVcopy(&verts[0], spos);
	rdVcopy(&verts[3], epos);

	float* refs = &m_offMeshConRefPos[m_offMeshConCount*3];
	const float yaw = dtCalcOffMeshRefYaw(spos, epos);
	const float offset[3] = { 0.0f,0.0f,rad };
	dtCalcOffMeshRefPos(spos, yaw, offset, refs);

	m_offMeshConRads[m_offMeshConCount] = rad;
	m_offMeshConRefYaws[m_offMeshConCount] = yaw;
	m_offMeshConDirs[m_offMeshConCount] = bidir;
	m_offMeshConJumps[m_offMeshConCount] = jump;
	m_offMeshConOrders[m_offMeshConCount] = order;
	m_offMeshConAreas[m_offMeshConCount] = area;
	m_offMeshConFlags[m_offMeshConCount] = flags;
	m_offMeshConId[m_offMeshConCount] = 1000 + m_offMeshConCount;

	return m_offMeshConCount++;
}

void InputGeom::deleteOffMeshConnection(int i)
{
	m_offMeshConCount--;

	float* vertsSrc = &m_offMeshConVerts[m_offMeshConCount*3*2];
	float* vertsDst = &m_offMeshConVerts[i*3*2];
	rdVcopy(&vertsDst[0], &vertsSrc[0]);
	rdVcopy(&vertsDst[3], &vertsSrc[3]);

	float* refSrc = &m_offMeshConRefPos[m_offMeshConCount*3];
	float* refDst = &m_offMeshConRefPos[i*3];
	rdVcopy(&refDst[0], &refSrc[0]);

	m_offMeshConRads[i] = m_offMeshConRads[m_offMeshConCount];
	m_offMeshConRefYaws[i] = m_offMeshConRefYaws[m_offMeshConCount];
	m_offMeshConDirs[i] = m_offMeshConDirs[m_offMeshConCount];
	m_offMeshConJumps[i] = m_offMeshConJumps[m_offMeshConCount];
	m_offMeshConOrders[i] = m_offMeshConOrders[m_offMeshConCount];
	m_offMeshConAreas[i] = m_offMeshConAreas[m_offMeshConCount];
	m_offMeshConFlags[i] = m_offMeshConFlags[m_offMeshConCount];
}

void InputGeom::drawOffMeshConnections(duDebugDraw* dd, const float* offset, const int hilightIdx)
{
	const unsigned int baseColor = duRGBA(0,0,0,64);
	dd->depthMask(false);

	dd->begin(DU_DRAW_LINES, 2.0f, offset);
	for (int i = 0; i < m_offMeshConCount; ++i)
	{
		float* v = &m_offMeshConVerts[i*3*2];

		dd->vertex(v[0],v[1],v[2], baseColor);
		dd->vertex(v[0],v[1],v[2]+10.0f, baseColor);
		
		dd->vertex(v[3],v[4],v[5], baseColor);
		dd->vertex(v[3],v[4],v[5]+10.0f, baseColor);
		
		duAppendCircle(dd, v[0],v[1],v[2]+5.0f, m_offMeshConRads[i], baseColor);
		duAppendCircle(dd, v[3],v[4],v[5]+5.0f, m_offMeshConRads[i], baseColor);

		const unsigned int conColor = hilightIdx == i ? duRGBA(192,0,128,192) : duRGBA(152,0,78,192);

		duAppendArc(dd, v[0], v[1], v[2], v[3], v[4], v[5], 0.25f,
			(m_offMeshConDirs[i] & DT_OFFMESH_CON_BIDIR) ? 30.0f : 0.0f, 30.0f, conColor);

		float* r = &m_offMeshConRefPos[i*3];
		float refPosDir[3];

		const float arrowLength[3] = { 35.f, 35.f, 0.f };
		dtCalcOffMeshRefPos(r, m_offMeshConRefYaws[i], arrowLength, refPosDir);

		duAppendArrow(dd, r[0],r[1],r[2], refPosDir[0],refPosDir[1],refPosDir[2], 0.f, 10.f, duRGBA(255,255,0,255));
	}	
	dd->end();

	dd->depthMask(true);
}

int InputGeom::addBoxVolume(const float* bmin, const float* bmax,
						 unsigned short flags, unsigned char area)
{
	if (m_volumeCount >= MAX_VOLUMES) return -1;
	ShapeVolume* vol = &m_volumes[m_volumeCount++];
	rdVcopy(&vol->verts[0], bmin);
	rdVcopy(&vol->verts[3], bmax);
	vol->hmin = 0.0f;
	vol->hmax = 0.0f;
	vol->nverts = 6;
	vol->flags = flags;
	vol->area = area;
	vol->type = VOLUME_BOX;

	return m_volumeCount-1;
}

int InputGeom::addCylinderVolume(const float* pos, const float radius,
						 const float height, unsigned short flags, unsigned char area)
{
	if (m_volumeCount >= MAX_VOLUMES) return -1;
	ShapeVolume* vol = &m_volumes[m_volumeCount++];
	rdVcopy(vol->verts, pos);
	vol->verts[3] = radius;
	vol->verts[4] = height;
	vol->hmin = 0.0f;
	vol->hmax = 0.0f;
	vol->nverts = 5;
	vol->flags = flags;
	vol->area = area;
	vol->type = VOLUME_CYLINDER;

	return m_volumeCount-1;
}

int InputGeom::addConvexVolume(const float* verts, const int nverts,
								const float minh, const float maxh, unsigned short flags, unsigned char area)
{
	if (m_volumeCount >= MAX_VOLUMES) return -1;
	ShapeVolume* vol = &m_volumes[m_volumeCount++];
	memcpy(vol->verts, verts, sizeof(float)*3*nverts);
	vol->hmin = minh;
	vol->hmax = maxh;
	vol->nverts = nverts;
	vol->flags = flags;
	vol->area = area;
	vol->type = VOLUME_CONVEX;

	return m_volumeCount-1;
}

void InputGeom::deleteShapeVolume(int i)
{
	m_volumeCount--;
	m_volumes[i] = m_volumes[m_volumeCount];
}

static const int FACE_ALPHA = 168;
static const int WIRE_ALPHA = 220;

void InputGeom::drawBoxVolumes(struct duDebugDraw* dd, const float* offset, const int hilightIdx)
{
	for (int i = 0; i < m_volumeCount; ++i)
	{
		const ShapeVolume* vol = &m_volumes[i];

		if (vol->type != VOLUME_BOX)
			continue;

		const int faceAlpha = hilightIdx == i ? WIRE_ALPHA : FACE_ALPHA;

		const unsigned int faceCol = vol->area == RC_NULL_AREA
			? duRGBA(255, 0, 0, faceAlpha) // Use red for visibility (null acts as deletion).
			: duTransCol(dd->areaToFaceCol(vol->area), faceAlpha);

		unsigned int fcol[6] = { faceCol, faceCol, faceCol, faceCol, faceCol, faceCol };

		duDebugDrawBox(dd, 
			vol->verts[0],vol->verts[1],vol->verts[2],
			vol->verts[3],vol->verts[4],vol->verts[5], 
			fcol, offset);

		const unsigned int wireCol = vol->area == RC_NULL_AREA
			? duRGBA(255, 0, 0, WIRE_ALPHA)
			: duTransCol(dd->areaToFaceCol(vol->area), WIRE_ALPHA);

		duDebugDrawBoxWire(dd,
			vol->verts[0],vol->verts[1],vol->verts[2],
			vol->verts[3],vol->verts[4],vol->verts[5], 
			wireCol, 2.0f, offset);
	}
}

void InputGeom::drawCylinderVolumes(struct duDebugDraw* dd, const float* offset, const int hilightIdx)
{
	for (int i = 0; i < m_volumeCount; ++i)
	{
		const ShapeVolume* vol = &m_volumes[i];

		if (vol->type != VOLUME_CYLINDER)
			continue;

		const int faceAlpha = hilightIdx == i ? WIRE_ALPHA : FACE_ALPHA;

		const unsigned int faceCol = vol->area == RC_NULL_AREA
			? duRGBA(255, 0, 0, faceAlpha) // Use red for visibility (null acts as deletion).
			: duTransCol(dd->areaToFaceCol(vol->area), faceAlpha);

		const float radius = vol->verts[3];
		const float height = vol->verts[4];

		duDebugDrawCylinder(dd, 
			vol->verts[0]-radius,vol->verts[1]-radius,vol->verts[2]+0.1f,
			vol->verts[0]+radius,vol->verts[1]+radius,vol->verts[2]+height, faceCol, offset);

		const unsigned int wireCol = vol->area == RC_NULL_AREA
			? duRGBA(255, 0, 0, WIRE_ALPHA)
			: duTransCol(dd->areaToFaceCol(vol->area), WIRE_ALPHA);

		duDebugDrawCylinderWire(dd, 
			vol->verts[0]-radius,vol->verts[1]-radius,vol->verts[2]+0.1f,
			vol->verts[0]+radius,vol->verts[1]+radius,vol->verts[2]+height, wireCol, 2.0f, offset);
	}
}

void InputGeom::drawConvexVolumes(struct duDebugDraw* dd, const float* offset, const int hilightIdx)
{
	dd->begin(DU_DRAW_TRIS, 1.0f, offset);
	
	for (int i = 0; i < m_volumeCount; ++i)
	{
		const ShapeVolume* vol = &m_volumes[i];

		if (vol->type != VOLUME_CONVEX)
			continue;

		unsigned int col;
		const int faceAlpha = hilightIdx == i ? WIRE_ALPHA : FACE_ALPHA;

		if (vol->area == RC_NULL_AREA)
			col = duRGBA(255, 0, 0, faceAlpha); // Use red for visibility (null acts as deletion).
		else
			col = duTransCol(dd->areaToFaceCol(vol->area), faceAlpha);

		for (int j = 0, k = vol->nverts-1; j < vol->nverts; k = j++)
		{
			const float* va = &vol->verts[k*3];
			const float* vb = &vol->verts[j*3];

			dd->vertex(va[0],va[1],vol->hmax, col);
			dd->vertex(vb[0],vb[1],vol->hmax, col);
			dd->vertex(vol->verts[0],vol->verts[1],vol->hmax, col);

			dd->vertex(vb[0],vb[1],vol->hmax, col);
			dd->vertex(va[0],va[1],vol->hmax, col);
			dd->vertex(va[0],va[1],vol->hmin, duDarkenCol(col));

			dd->vertex(vb[0],vb[1],vol->hmin, duDarkenCol(col));
			dd->vertex(vb[0],vb[1],vol->hmax, col);
			dd->vertex(va[0],va[1],vol->hmin, duDarkenCol(col));
		}
	}
	
	dd->end();

	dd->begin(DU_DRAW_LINES, 2.0f, offset);
	for (int i = 0; i < m_volumeCount; ++i)
	{
		const ShapeVolume* vol = &m_volumes[i];

		if (vol->type != VOLUME_CONVEX)
			continue;

		unsigned int col;

		if (vol->area == RC_NULL_AREA)
			col = duRGBA(255, 0, 0, WIRE_ALPHA);
		else
			col = duTransCol(dd->areaToFaceCol(vol->area), WIRE_ALPHA);

		for (int j = 0, k = vol->nverts-1; j < vol->nverts; k = j++)
		{
			const float* va = &vol->verts[k*3];
			const float* vb = &vol->verts[j*3];
			dd->vertex(vb[0],vb[1],vol->hmin, duDarkenCol(col));
			dd->vertex(va[0],va[1],vol->hmin, duDarkenCol(col));
			dd->vertex(vb[0],vb[1],vol->hmax, col);
			dd->vertex(va[0],va[1],vol->hmax, col);
			dd->vertex(va[0],va[1],vol->hmax, col);
			dd->vertex(va[0],va[1],vol->hmin, duDarkenCol(col));
		}
	}
	dd->end();

	dd->begin(DU_DRAW_POINTS, 3.0f, offset);
	for (int i = 0; i < m_volumeCount; ++i)
	{
		const ShapeVolume* vol = &m_volumes[i];

		if (vol->type != VOLUME_CONVEX)
			continue;

		unsigned int col;

		if (vol->area == RC_NULL_AREA)
			col = duDarkenCol(duRGBA(255, 0, 0, WIRE_ALPHA));
		else
			col = duDarkenCol(duTransCol(dd->areaToFaceCol(vol->area), WIRE_ALPHA));

		for (int j = 0; j < vol->nverts; ++j)
		{
			dd->vertex(vol->verts[j*3+0],vol->verts[j*3+1],vol->hmax, col);
			dd->vertex(vol->verts[j*3+0],vol->verts[j*3+1],vol->hmin, col);
			dd->vertex(vol->verts[j*3+0],vol->verts[j*3+1]+0.1f,vol->verts[j*3+2], col);
		}
	}
	dd->end();
}
