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

#include "Shared/Include/SharedAlloc.h"
#include "DebugUtils/Include/RecastDebugDraw.h"
#include "Recast/Include/Recast.h"
#include "Detour/Include/DetourNavMeshQuery.h"
#include "NavEditor/Include/InputGeom.h"
#include "NavEditor/Include/TestCase.h"
#include "NavEditor/Include/Filelist.h"
#include "NavEditor/Include/Editor_SoloMesh.h"
#include "NavEditor/Include/Editor_TileMesh.h"
#include "NavEditor/Include/Editor_Debug.h"
#include "NavEditor/include/DroidSans.h"
#include "NavEditor/include/Icon.h"

using std::string;
using std::vector;

struct SampleItem
{
	Editor* (*create)();
	const string name;
};
Editor* createSolo() { return new Editor_SoloMesh(); }
Editor* createTile() { return new Editor_TileMesh(); }
Editor* createDebug() { return new Editor_Debug(); }

void save_ply(std::vector<float>& pts,std::vector<int>& colors,rdIntArray& tris)
{
	static int counter = 0;
	char fname[255];
	sprintf(fname, "out_%d.ply", counter);
	counter++;
	auto f = fopen(fname, "wb");
	fprintf(f,
R"(ply
format ascii 1.0
element vertex %zu
property float x
property float y
property float z
property uchar red
property uchar green
property uchar blue
element face %zu
property list uchar int vertex_index
end_header
)", size_t(pts.size()/3), size_t(tris.size()/3));

	for (size_t i = 0; i < size_t(pts.size()); i+=3)
	{
		auto c = colors[i / 3];
		fprintf(f, "%g %g %g %d %d %d\n", 
			pts[i], pts[i+1], pts[i+2], c & 0xff, (c >> 8) & 0xff, (c >> 16) & 0xff);
	}
	for (size_t i = 0; i < size_t(tris.size()); i += 3)
	{
		fprintf(f, "3 %d %d %d\n", 
			tris[int(i)], tris[int(i+1)], tris[int(i+2)]);
	}
	
	fclose(f);
}
float area2(const float* a, const float* b, const float* c)
{
	return (b[0] - a[0]) * (c[1] - a[1]) - (c[0] - a[0]) * (b[1] - a[1]);
}
void convex_hull(std::vector<float>& pts, std::vector<int>& hull)
{
	size_t pt_count = pts.size() / 3;
	size_t cur_pt = 0;
	float min_x = pts[0];
	for(size_t i=0;i<pt_count;i++)
		if (pts[i * 3] < min_x)
		{
			min_x = pts[i * 3];
			cur_pt = i;
		}

	
	size_t point_on_hull = cur_pt;
	size_t endpoint = 0;
	do
	{
		hull.push_back(int(point_on_hull));
		endpoint = (point_on_hull + 1) % pt_count;
		for (size_t i = 0; i < pt_count; i++)
		{
			if (area2(&pts[point_on_hull*3], &pts[i*3], &pts[endpoint*3]) > 0) //reverse this comparison for flipped hull direction
				endpoint = i;
		}
		point_on_hull = endpoint;
	} while (endpoint != hull[0]);
}
float frand()
{
	return rand() / (float)RAND_MAX;
}
void generate_points(float* pts, int count, float dx, float dy, float dz)
{
	for (int i = 0; i < count; i++)
	{
		pts[i * 3+0] = frand()*dx * 2 - dx;
		pts[i * 3+1] = frand()*dy * 2 - dy;
		pts[i * 3+2] = frand()*dz * 2 - dz;
	}
}

void get_model_name(const std::string& nameIn, std::string& nameOut)
{
	const size_t charPos = nameIn.find_last_of(".");

	nameOut = charPos == string::npos 
		? nameIn
		: nameIn.substr(0, charPos);
}

void auto_load(const char* path, BuildContext& ctx, Editor*& editor,InputGeom*& geom, string& meshName)
{
	string geom_path = std::string(path);
	meshName = geom_path.substr(geom_path.rfind("\\") + 1);
	geom = new InputGeom;
	if (!geom->load(&ctx, geom_path))
	{
		delete geom;
		geom = 0;

		// Destroy the editor if it already had geometry loaded, as we've just deleted it!
		/*if (editor && editor->getInputGeom())
		{
			delete editor;
			editor = 0;
		}*/
		ctx.dumpLog("Geom load log %s:", meshName.c_str());
	}
	if (editor && geom)
	{
		editor->handleMeshChanged(geom);
		get_model_name(meshName, editor->m_modelName);
	}
}

void update_camera(const float* bmin, const float* bmax,float* cameraPos,float* cameraEulers,float& camr)
{
	// Reset camera and fog to match the mesh bounds.
	if (bmin && bmax)
	{
		camr = sqrtf(rdSqr(bmax[0] - bmin[0]) +
			rdSqr(bmax[1] - bmin[1]) +
			rdSqr(bmax[2] - bmin[2])) / 2;
		cameraPos[0] = (bmax[0] + bmin[0]) / 2 + camr;
		cameraPos[1] = (bmax[1] + bmin[1]) / 2 + camr;
		cameraPos[2] = (bmax[2] + bmin[2]) / 2 + camr;
		camr *= 3;
	}
	cameraEulers[0] = 45;
	cameraEulers[1] = -125;
	glFogf(GL_FOG_START, camr * 0.1f);
	glFogf(GL_FOG_END, camr * 1.25f);
}

bool imgui_init(SDL_Window* window, SDL_Renderer* /*renderer*/, SDL_GLContext context)
{
	IMGUI_CHECKVERSION();
	ImGuiContext* const imguiContext = ImGui::CreateContext();

	if (!imguiContext)
		return false;

	ImPlotContext* const implotContext = ImPlot::CreateContext();

	if (!implotContext)
		return false;

	// Disable ctrl+tab menu.
	imguiContext->ConfigNavWindowingKeyNext = 0;
	imguiContext->ConfigNavWindowingKeyPrev = 0;

	ImGui_SetStyle(ImGuiStyle_t::DEFAULT);

	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_Separator] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);

	if (!ImGui_ImplSDL2_InitForOpenGL(window, context))
	{
		return false;
	}

	if (!ImGui_ImplOpenGL2_Init())
	{
		return false;
	}

	ImFontConfig fontCfg;
	ImGuiIO& imguiIo = ImGui::GetIO();

	fontCfg.FontDataOwnedByAtlas = false;
	imguiIo.Fonts->AddFontFromMemoryTTF((void*)g_droidSansData, g_droidSansDataSize, 15, &fontCfg);

	return true;
}

void imgui_shutdown()
{
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplSDL2_Shutdown();

	ImGui::DestroyContext();
}

bool window_decoration_init(SDL_Window* window)
{
	SDL_RWops* const rw = SDL_RWFromMem((void*)g_recastNavigationIcon, sizeof(g_recastNavigationIcon));

	if (!rw)
	{
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to create r/w structure from icon data: %s", SDL_GetError());
		return false;
	}

	SDL_Surface* const surface = SDL_LoadBMP_RW(rw, 1);

	if (!surface)
	{
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to load icon data from r/w structure: %s", SDL_GetError());
		return false;
	}

	SDL_SetWindowIcon(window, surface);
	SDL_FreeSurface(surface);

	SDL_SetWindowTitle(window, "Recast Navigation");
	return true;
}

bool sdl_init(SDL_Window*& window, SDL_Renderer*& renderer, int &width, int &height, bool presentationMode)
{
	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_WARN);

	// Init SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		SDL_LogError(SDL_LOG_PRIORITY_CRITICAL, "Failed to initialise SDL: %s\n", SDL_GetError());
		return false;
	}

	// Enable depth buffer.
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	// Set color channel depth.
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

	// 4x MSAA.
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

	SDL_DisplayMode displayMode;
	SDL_GetCurrentDisplayMode(0, &displayMode);

	Uint32 flags = SDL_WINDOW_OPENGL | SDL_RENDERER_PRESENTVSYNC | SDL_WINDOW_RESIZABLE;
	if (presentationMode)
	{
		// Create a fullscreen window at the native resolution.
		width = displayMode.w;
		height = displayMode.h;
		flags |= SDL_WINDOW_FULLSCREEN;
	}
	else
	{
		float aspect = 16.0f / 9.0f;
		width = rdMin(displayMode.w, static_cast<int>((displayMode.h * aspect))) - 80;
		height = displayMode.h - 80;
	}

	const int errorCode = SDL_CreateWindowAndRenderer(width, height, flags, &window, &renderer);

	if (errorCode != 0 || !window || !renderer)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialise SDL OpenGL: %s\n", SDL_GetError());
		SDL_Quit();
		return false;
	}

	window_decoration_init(window);

	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_GLContext context = SDL_GL_CreateContext(window);

	if (!imgui_init(window, renderer, context))
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "failed to initialise GUI renderer.\n");
		SDL_Quit();
		return false;
	}

	return true;
}

// Gradient background
void draw_background(const GLfloat width, const GLfloat height)
{
	glBegin(GL_QUADS);

	// top-left
	glColor3f(0.40f, 0.42f, 0.44f);
	glVertex2f(0.0f, 0.0f);

	// top-right
	glColor3f(0.40f, 0.42f, 0.44f);
	glVertex2f(width, 0.0f);

	// bottom-right
	glColor3f(0.10f, 0.12f, 0.14f);
	glVertex2f(width, height);

	// bottom-left
	glColor3f(0.10f, 0.12f, 0.14f);
	glVertex2f(0.0f, height);

	glEnd();
}

#if 1
int main(int argc, char** argv)
#else
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
//just quick tests for stuff



extern void delaunayHull(rcContext* ctx, const int npts, const float* pts,
	const int nhull, const int* hull,
	rdIntArray& tris, rdIntArray& edges);

int main_test_delaunay(int /*argc*/, char** /*argv*/)
{
	rcContext ctx;
	std::vector<float> pts(25*3);
	std::vector<int> colors(pts.size() / 3, 0xffffffff);
	generate_points(pts.data(), pts.size()/3, 10, 10, 2);
	

	std::vector<int> hull;
	convex_hull(pts,hull);

	for (auto h : hull)
		colors[h] = 0xffff0000;

	rdIntArray tris;
	save_ply(pts, colors, tris);

	rdIntArray edges;
	delaunayHull(&ctx, pts.size()/3, pts.data(), hull.size(), hull.data(), tris, edges);
	save_ply(pts, colors, tris);
	return 0;
}
void compact_tris(rdIntArray& tris)
{
	int j = 3;
	for (int i = 4; i < tris.size(); i++)
	{
		if (i % 4 == 3) continue;
		tris[j] = tris[i];
		j++;
	}
	tris.resize(j);
}
int main(int argc, char** argv)
{
	srand(17);
	rcContext ctx;
	std::vector<float> pts(8 * 3);
	std::vector<int> colors(pts.size() / 3, 0xffffffff);
	generate_points(pts.data(), pts.size() / 3, 10, 10, 10);


	std::vector<int> hull;
	convex_hull(pts, hull);

	for (auto h : hull)
		colors[h] = 0xffff0000;

	rdIntArray tris;
	//save_ply(pts, colors, tris);

	rdIntArray edges;
	delaunayHull(&ctx, pts.size() / 3, pts.data(), hull.size(), hull.data(), tris, edges);
	compact_tris(tris);
	save_ply(pts, colors, tris);
	int tri_count = tris.size() / 3;
	std::vector<unsigned char> areas;
	areas.resize(tri_count);
	for (int i = 0; i < tri_count; i++)
		areas[i] = i;


	float bmin[3];
	float bmax[3];
	rcCalcBounds(pts.data(), pts.size()/3, bmin, bmax);

	float cellSize = .05f;
	float cellHeight = .05f;

	int width;
	int height;

	rcCalcGridSize(bmin, bmax, cellSize, &width, &height);

	rcHeightfield solid;
	rcCreateHeightfield(&ctx, solid, width, height, bmin, bmax, cellSize, cellHeight);

	int flagMergeThr = 1;

	rcRasterizeTriangles(&ctx, pts.data(), pts.size() / 3, tris.data(), areas.data(), tri_count, solid, flagMergeThr);

	std::vector<unsigned char> img_data(width*height);
	float zdelta = bmax[2] - bmin[2];
	for(int x=0;x<width;x++)
		for (int y = 0; y < height; y++)
		{
			auto s = solid.spans[x + y * width];
			if (s )
			{
#if 1
				img_data[x + y * width] = s->area*(235/ (float)tri_count) +20;
#else
				img_data[x + y * width]=((s->smax*cellHeight)/zdelta)*255;
#endif
				//img_data[x + y * width] = 255;
			}
		}

	stbi_write_png("hmap.png", width, height, 1, img_data.data(), width);
	
	return 0;
}
int not_main(int argc, char** argv)
#endif
{
	const char* autoLoad = nullptr;
	bool commandLine = false;
	bool presentationMode = false;
	int width = 0;
	int height = 0;
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;

	if (argc > 1)
	{
		if (strcmp(argv[1], "-console") == 0)
		{
			if (argc > 2)
			{
				autoLoad = argv[2];
				commandLine = true;
			}
		}
		else
		{
			FreeConsole();
			autoLoad = argv[1];
			commandLine = true;
		}
	}
	else
	{
		FreeConsole();
	}

	if (!commandLine)
	{
		if (!sdl_init(window, renderer, width, height, presentationMode))
		{
			return EXIT_FAILURE;
		}
	}

	float cameraEulers[] = {45, 45};
	float cameraPos[] = {0, 0, 0};
	float camr = 1000;
	float origCameraEulers[] = {0, 0}; // Used to compute rotational changes across frames.
	
	vector<string> files;
	const string meshesFolder = "Levels";
	string meshName;
	const string testCasesFolder = "TestCases";
	
	float markerPosition[3] = {0, 0, 0};
	bool markerPositionSet = false;
	
	InputGeom* geom = nullptr;
	Editor* editor = nullptr;
	TestCase* test = nullptr;
	BuildContext ctx;
	
	//Load tiled editor

	editor = createTile();
	editor->setContext(&ctx);
	if (geom)
	{
		editor->handleMeshChanged(geom);
	}
	if (autoLoad)
	{
		auto_load(autoLoad, ctx, editor, geom, meshName);
		if (geom || editor)
		{
			const float* bmin = 0;
			const float* bmax = 0;
			if (geom)
			{
				bmin = geom->getOriginalNavMeshBoundsMin();
				bmax = geom->getOriginalNavMeshBoundsMax();
			}
			if (!commandLine)
			{
				update_camera(bmin, bmax, cameraPos, cameraEulers, camr);
			}
		}
		if (argc > 2)
		{
			auto ts = dynamic_cast<Editor_TileMesh*>(editor);
			ts->buildAllHulls();
			return EXIT_SUCCESS;
		}
	}
	// Fog.
	float fogColor[4] = { 0.30f, 0.31f, 0.32f, 1.0f };
	glEnable(GL_FOG);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_START, camr * 0.1f);
	glFogf(GL_FOG_END, camr * 1.25f);
	glFogfv(GL_FOG_COLOR, fogColor);

	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);

	float moveFront = 0.0f, moveBack = 0.0f, moveLeft = 0.0f, moveRight = 0.0f, moveUp = 0.0f, moveDown = 0.0f;

	float rayStart[3] = { 0.0f };
	float rayEnd[3] = { 0.0f };
	float scrollSide = 0.0f;
	float scrollZoom = 0.0f;
	bool rotate = false;
	bool movedDuringRotate = false;
	bool focusOnMenu = false;

	bool showMenu = !presentationMode;
	bool showLog = false;
	bool showTools = true;
	bool showLevels = false;
	bool showEditor = false;
	bool showTestCases = false;

	// Window scroll positions.
	//int propScroll = 0;

	float t = 0.0f;
	float timeAcc = 0.0f;
	Uint32 prevFrameTime = SDL_GetTicks();
	int mousePos[2] = { 0, 0 };
	int origMousePos[2] = { 0, 0 }; // Used to compute mouse movement totals across frames.

	bool done = false;
	while(!done)
	{
		// Handle input events.
		int mouseScroll = 0;
		bool processHitTest = false;
		bool processHitTestShift = false;
		
		SDL_Event event;
		
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);

			switch (event.type)
			{
				case SDL_KEYDOWN:
					// Handle any key presses here.
					if (event.key.keysym.sym == SDLK_ESCAPE)
					{
						done = true;
					}
					else if (event.key.keysym.sym == SDLK_t)
					{
						showLevels ^= false;
						showEditor ^= false;
						showTestCases ^= true;

						if (showTestCases)
							scanDirectory(testCasesFolder, ".txt", files);
					}
					else if (event.key.keysym.sym == SDLK_TAB)
					{
						showMenu = !showMenu;
					}
					else if (event.key.keysym.sym == SDLK_SPACE)
					{
						if (editor)
							editor->handleToggle();
					}
					else if (event.key.keysym.sym == SDLK_1)
					{
						if (editor)
							editor->handleStep();
					}
					else if (event.key.keysym.sym == SDLK_9)
					{
						if (editor && geom)
						{
							string savePath = meshesFolder + "/";
							BuildSettings settings;
							memset(&settings, 0, sizeof(settings));

							rdVcopy(settings.navMeshBMin, geom->getNavMeshBoundsMin());
							rdVcopy(settings.navMeshBMax, geom->getNavMeshBoundsMax());

							editor->collectSettings(settings);

							geom->saveGeomSet(&settings);
						}
					}
					break;
				
				case SDL_MOUSEWHEEL:
					if (event.wheel.x < 0)
					{
						// wheel down
						if (focusOnMenu)
						{
							mouseScroll++;
						}
						else
						{
							scrollSide += 120.0f;
						}
					}
					else if (event.wheel.x > 0)
					{
						if (focusOnMenu)
						{
							mouseScroll--;
						}
						else
						{
							scrollSide -= 120.0f;
						}
					}
					else if (event.wheel.y < 0)
					{
						// wheel down
						if (focusOnMenu)
						{
							mouseScroll++;
						}
						else
						{
							scrollZoom += 120.0f;
						}
					}
					else if (event.wheel.y > 0)
					{
						if (focusOnMenu)
						{
							mouseScroll--;
						}
						else
						{
							scrollZoom -= 120.0f;
						}
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == SDL_BUTTON_RIGHT)
					{
						if (!focusOnMenu)
						{
							// Rotate view
							rotate = true;
							movedDuringRotate = false;
							origMousePos[0] = mousePos[0];
							origMousePos[1] = mousePos[1];
							origCameraEulers[0] = cameraEulers[0];
							origCameraEulers[1] = cameraEulers[1];
						}
					}
					break;
					
				case SDL_MOUSEBUTTONUP:
					// Handle mouse clicks here.
					if (event.button.button == SDL_BUTTON_RIGHT)
					{
						rotate = false;
						if (!focusOnMenu)
						{
							if (!movedDuringRotate)
							{
								processHitTest = true;
								processHitTestShift = true;
							}
						}
					}
					else if (event.button.button == SDL_BUTTON_LEFT)
					{
						if (!focusOnMenu)
						{
							processHitTest = true;
							processHitTestShift = (SDL_GetModState() & KMOD_SHIFT) ? true : false;
						}
					}
					
					break;
					
				case SDL_MOUSEMOTION:
					mousePos[0] = event.motion.x;
					mousePos[1] = height-1 - event.motion.y;
					
					if (rotate)
					{
						int dx = mousePos[0] - origMousePos[0];
						int dy = mousePos[1] - origMousePos[1];
						cameraEulers[0] = origCameraEulers[0] - dy * 0.25f;
						cameraEulers[1] = origCameraEulers[1] + dx * 0.25f;
						if (dx * dx + dy * dy > 3 * 3)
						{
							movedDuringRotate = true;
						}
					}
					break;
					
				case SDL_WINDOWEVENT:
				{
					if (event.window.event == SDL_WINDOWEVENT_RESIZED)
					{
						// Get the new window size
						width = event.window.data1;
						height = event.window.data2;

						// Update OpenGL viewport
						glViewport(0, 0, width, height);

						glMatrixMode(GL_PROJECTION);
						glLoadIdentity();
						gluPerspective(50.0f, (float)width / (float)height, 1.0f, camr);
					}
				}
				break;

				case SDL_QUIT:
					done = true;
					break;
					
				default:
					break;
			}
		}
		
		Uint32 time = SDL_GetTicks();
		float dt = (time - prevFrameTime) / 1000.0f;
		prevFrameTime = time;
		
		t += dt;

		// Hit test mesh.
		if (processHitTest && geom && editor)
		{
			float hitTime;
			int volumeIndex;

			bool hit = geom->raycastMesh(rayStart, rayEnd, TRACE_ALL, &volumeIndex, &hitTime);
			
			if (hit)
			{
				if (SDL_GetModState() & KMOD_CTRL)
				{
					// Marker
					markerPositionSet = true;
					markerPosition[0] = rayStart[0] + (rayEnd[0] - rayStart[0]) * hitTime;
					markerPosition[1] = rayStart[1] + (rayEnd[1] - rayStart[1]) * hitTime;
					markerPosition[2] = rayStart[2] + (rayEnd[2] - rayStart[2]) * hitTime;
				}
				else
				{
					float pos[3];
					pos[0] = rayStart[0] + (rayEnd[0] - rayStart[0]) * hitTime;
					pos[1] = rayStart[1] + (rayEnd[1] - rayStart[1]) * hitTime;
					pos[2] = rayStart[2] + (rayEnd[2] - rayStart[2]) * hitTime;
					editor->handleClick(rayStart, pos, volumeIndex, processHitTestShift);
				}
			}
			else
			{
				if (SDL_GetModState() & KMOD_CTRL)
				{
					// Marker
					markerPositionSet = false;
				}
			}
		}
		
		// Update editor simulation.
		const float SIM_RATE = 20;
		const float DELTA_TIME = 1.0f / SIM_RATE;
		timeAcc = rdClamp(timeAcc + dt, -1.0f, 1.0f);
		int simIter = 0;
		while (timeAcc > DELTA_TIME)
		{
			timeAcc -= DELTA_TIME;
			if (simIter < 5 && editor)
			{
				editor->handleUpdate(DELTA_TIME);
			}
			simIter++;
		}

		// Clamp the framerate so that we do not hog all the CPU.
		const float MIN_FRAME_TIME = 1.0f / 40.0f;
		if (dt < MIN_FRAME_TIME)
		{
			int ms = (int)((MIN_FRAME_TIME - dt) * 1000.0f);
			if (ms > 10) ms = 10;
			if (ms >= 0) SDL_Delay(ms);
		}
		
		// Set the viewport.
		glViewport(0, 0, width, height);
		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);
		
		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT);
		draw_background((GLfloat)width, (GLfloat)height);
		glClear(GL_DEPTH_BUFFER_BIT);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_DEPTH_TEST);
		
		// Compute the projection matrix.
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(85.0f, (float)width/(float)height, 25.0f, camr);
		GLdouble projectionMatrix[16];
		glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix);
		
		// Compute the modelview matrix.
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(cameraEulers[0], 1, 0, 0);
		glRotatef(cameraEulers[1], 0, 1, 0);
		const float mXZY_to_XYZ[16] =
		{
			1,0,0,0,
			0,0,-1,0,
			0,1,0,0,
			0,0,0,1
		};
		glMultMatrixf(mXZY_to_XYZ);
		glTranslatef(-cameraPos[0], -cameraPos[1], -cameraPos[2]);
		GLdouble modelviewMatrix[16];
		glGetDoublev(GL_MODELVIEW_MATRIX, modelviewMatrix);
		
		// Get hit ray position and direction.
		GLdouble x, y, z;
		gluUnProject(mousePos[0], mousePos[1], 0.0f, modelviewMatrix, projectionMatrix, viewport, &x, &y, &z);
		rayStart[0] = (float)x;
		rayStart[1] = (float)y;
		rayStart[2] = (float)z;
		gluUnProject(mousePos[0], mousePos[1], 1.0f, modelviewMatrix, projectionMatrix, viewport, &x, &y, &z);
		rayEnd[0] = (float)x;
		rayEnd[1] = (float)y;
		rayEnd[2] = (float)z;
		
		if (!focusOnMenu)
		{
			// Handle keyboard movement.
			const Uint8* keystate = SDL_GetKeyboardState(NULL);
			moveFront	= rdClamp(moveFront	+ dt * 4 * ((keystate[SDL_SCANCODE_W] || keystate[SDL_SCANCODE_UP		]) ? 1 : -1), 0.0f, 1.0f);
			moveLeft	= rdClamp(moveLeft	+ dt * 4 * ((keystate[SDL_SCANCODE_A] || keystate[SDL_SCANCODE_LEFT		]) ? 1 : -1), 0.0f, 1.0f);
			moveBack	= rdClamp(moveBack	+ dt * 4 * ((keystate[SDL_SCANCODE_S] || keystate[SDL_SCANCODE_DOWN		]) ? 1 : -1), 0.0f, 1.0f);
			moveRight	= rdClamp(moveRight	+ dt * 4 * ((keystate[SDL_SCANCODE_D] || keystate[SDL_SCANCODE_RIGHT	]) ? 1 : -1), 0.0f, 1.0f);
			moveUp		= rdClamp(moveUp	+ dt * 4 * ((keystate[SDL_SCANCODE_Q] || keystate[SDL_SCANCODE_PAGEUP	]) ? 1 : -1), 0.0f, 1.0f);
			moveDown	= rdClamp(moveDown	+ dt * 4 * ((keystate[SDL_SCANCODE_E] || keystate[SDL_SCANCODE_PAGEDOWN	]) ? 1 : -1), 0.0f, 1.0f);
			
			float keybSpeed = 8800.0f;
			if (SDL_GetModState() & KMOD_SHIFT)
			{
				keybSpeed *= 2.0f;
			}
			
			float movex = (moveRight - moveLeft) * keybSpeed * dt + scrollSide * 2.0f;
			float movey = (moveBack - moveFront) * keybSpeed * dt + scrollZoom * 2.0f;

			scrollSide = 0.0f;
			scrollZoom = 0.0f;
			
			cameraPos[0] += movex * static_cast<float>(modelviewMatrix[0]);
			cameraPos[1] += movex * static_cast<float>(modelviewMatrix[4]);
			cameraPos[2] += movex * static_cast<float>(modelviewMatrix[8]);

			cameraPos[0] += movey * static_cast<float>(modelviewMatrix[2]);
			cameraPos[1] += movey * static_cast<float>(modelviewMatrix[6]);
			cameraPos[2] += movey * static_cast<float>(modelviewMatrix[10]);

			cameraPos[2] += (moveUp - moveDown) * keybSpeed * dt;
		}

		glEnable(GL_FOG);

		if (editor)
			editor->handleRender();
		if (test)
			test->handleRender();
		
		glDisable(GL_FOG);
		
		// Render GUI
		glDisable(GL_DEPTH_TEST);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0, width, 0, height);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		ImGuiIO& io = ImGui::GetIO();
		focusOnMenu = io.WantCaptureMouse||io.WantCaptureKeyboard;
		
		ImGui_ImplOpenGL2_NewFrame();
		ImGui_ImplSDL2_NewFrame();

		ImGui::NewFrame();
		
		if (editor)
		{
			editor->handleRenderOverlay(reinterpret_cast<double*>(projectionMatrix), 
				reinterpret_cast<double*>(modelviewMatrix), reinterpret_cast<int*>(viewport));
		}
		if (test)
		{
			test->handleRenderOverlay(reinterpret_cast<double*>(projectionMatrix), reinterpret_cast<double*>(modelviewMatrix), reinterpret_cast<int*>(viewport));
		}

		// Help text.
		if (showMenu)
		{
			ImGui_RenderText(ImGuiTextAlign_e::kAlignLeft, 
				ImVec2(300, 20), ImVec4(1.0f,1.0f,1.0f,0.5f), "W/S/A/D: Move  RMB: Rotate");
		}
		string geom_path;

		const ImGuiWindowFlags baseWindowFlags = ImGuiWindowFlags_None;

		if (showMenu)
		{
			ImGui::SetNextWindowPos(ImVec2((float)width-300-10, 10.f), ImGuiCond_Once);
			ImGui::SetNextWindowSize(ImVec2(300, (float)height-20), ImGuiCond_Once);
			ImGui::SetNextWindowSizeConstraints(ImVec2(300, 300), ImVec2(FLT_MAX, FLT_MAX));

			if (ImGui::Begin("Properties", nullptr, baseWindowFlags))
			{
				ImGui::Checkbox("Show Log", &showLog);
				ImGui::Checkbox("Show Tools", &showTools);

				ImGui::Separator();
				ImGui::Text("Input Level");

				if (ImGui::Button("Load Project..."))
				{
					char szFile[260];
					OPENFILENAMEA diag = { 0 };
					diag.lStructSize = sizeof(diag);

					SDL_SysWMinfo sdlinfo;
					SDL_version sdlver;
					SDL_VERSION(&sdlver);
					sdlinfo.version = sdlver;
					SDL_GetWindowWMInfo(window, &sdlinfo);

					diag.hwndOwner = sdlinfo.info.win.window;

					diag.lpstrFile = szFile;
					diag.lpstrFile[0] = 0;
					diag.nMaxFile = sizeof(szFile);
					diag.lpstrFilter = "GSET\0*.gset\0OBJ\0*.obj\0Ply\0*.ply\0"; //TODO: BSP\0*.bsp\0
					diag.nFilterIndex = 1;
					diag.lpstrFileTitle = NULL;
					diag.nMaxFileTitle = 0;
					diag.lpstrInitialDir = NULL;
					diag.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

					if (GetOpenFileNameA(&diag))
					{
						geom_path = std::string(szFile);
						meshName = geom_path.substr(geom_path.rfind("\\") + 1);
					}
				}
				if (geom && editor && ImGui::Button("Load NavMesh..."))
				{
					char szFile[260];
					OPENFILENAMEA diag = { 0 };
					diag.lStructSize = sizeof(diag);

					SDL_SysWMinfo sdlinfo;
					SDL_version sdlver;
					SDL_VERSION(&sdlver);
					sdlinfo.version = sdlver;
					SDL_GetWindowWMInfo(window, &sdlinfo);

					diag.hwndOwner = sdlinfo.info.win.window;

					diag.lpstrFile = szFile;
					diag.lpstrFile[0] = 0;
					diag.nMaxFile = sizeof(szFile);
					diag.lpstrFilter = "NM\0*.nm\0";
					diag.nFilterIndex = 1;
					diag.lpstrFileTitle = NULL;
					diag.nMaxFileTitle = 0;
					diag.lpstrInitialDir = NULL;
					diag.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

					if (GetOpenFileNameA(&diag))
					{
						editor->loadNavMesh(szFile, true);
					}
				}
				if (ImGui::Button(meshName.empty() ? "Choose Level..." : meshName.c_str()))
				{
					if (showLevels)
					{
						showLevels = false;
					}
					else
					{
						showEditor = false;
						showTestCases = false;
						showLevels = true;
						scanDirectory(meshesFolder, ".gset", files);
						scanDirectoryAppend(meshesFolder, ".obj", files);
						scanDirectoryAppend(meshesFolder, ".ply", files);
					}
				}
				if (geom)
				{
					char text[64];
					snprintf(text, sizeof(text), "Verts: %.1fk",
						geom->getMesh()->getVertCount() / 1000.0f);
					ImGui::Text(text);

					snprintf(text, sizeof(text), "Tris: %.1fk",
						geom->getMesh()->getTriCount() / 1000.0f);
					ImGui::Text(text);
				}

				ImGui::Separator();

				if (geom && editor)
				{

					editor->handleSettings();

					if (ImGui::Button("Build", ImVec2(165, 0)))
					{
						ctx.resetLog();
						if (!editor->handleBuild())
						{
							showLog = true;
						}
						ctx.dumpLog("Build log %s:", meshName.c_str());

						// Clear test.
						delete test;
						test = 0;
					}

					ImGui::Separator();
				}

				if (editor)
				{
					editor->handleDebugMode();
				}
			}
			ImGui::End();
		}
		
		// Editor selection dialog.
		if (showEditor)
		{
			static int levelScroll = 0;
			if (geom || editor)
			{
				const float* bmin = 0;
				const float* bmax = 0;
				if (geom)
				{
					bmin = geom->getMeshBoundsMin();
					bmax = geom->getMeshBoundsMax();
				}
				// Reset camera and fog to match the mesh bounds.
				update_camera(bmin, bmax, cameraPos, cameraEulers, camr);
			}
			
			//ImGui::EndChild();
		}

		// Level selection dialog.
		if (showLevels)
		{
			ImGui::SetNextWindowPos(ImVec2((float)width-10-250-10-300, 10.f), ImGuiCond_Once);
			ImGui::SetNextWindowSize(ImVec2(250.f, 450.f), ImGuiCond_Once);
			if (ImGui::Begin("Choose Level", nullptr, baseWindowFlags))
			{
				vector<string>::const_iterator fileIter = files.begin();
				vector<string>::const_iterator filesEnd = files.end();
				vector<string>::const_iterator levelToLoad = filesEnd;
				for (; fileIter != filesEnd; ++fileIter)
				{
					// was imguiItem
					if (ImGui::MenuItem(fileIter->c_str()))
					{
						levelToLoad = fileIter;
					}
				}

				if (levelToLoad != filesEnd)
				{
					meshName = *levelToLoad;
					showLevels = false;

					delete geom;
					geom = 0;

					geom_path = meshesFolder + "/" + meshName;
				}
			}
			
			ImGui::End();
		}
		if (!geom_path.empty())
		{
			geom = new InputGeom;
			if (!geom->load(&ctx, geom_path))
			{
				delete geom;
				geom = 0;

				// Destroy the editor if it already had geometry loaded, as we've just deleted it!
				if (editor && editor->getInputGeom())
				{
					delete editor;
					editor = 0;
				}

				showLog = true;
				ctx.dumpLog("Geom load log %s:", meshName.c_str());
			}
			if (editor && geom)
			{
				editor->handleMeshChanged(geom);
				get_model_name(meshName, editor->m_modelName);
			}

			if (geom || editor)
			{
				const float* bmin = 0;
				const float* bmax = 0;
				if (geom)
				{
					bmin = geom->getMeshBoundsMin();
					bmax = geom->getMeshBoundsMax();
				}
				// Reset camera and fog to match the mesh bounds.
				update_camera(bmin, bmax, cameraPos, cameraEulers, camr);
			}
		}
		// Test cases
		if (showTestCases)
		{
			ImGui::SetNextWindowPos(ImVec2((float)width-10-250-10-300, 10.f), ImGuiCond_Once);
			ImGui::SetNextWindowSize(ImVec2(250.f, 450.f), ImGuiCond_Once);

			if (ImGui::Begin("Choose Test To Run", nullptr, baseWindowFlags))
			{
				vector<string>::const_iterator fileIter = files.begin();
				vector<string>::const_iterator filesEnd = files.end();
				vector<string>::const_iterator testToLoad = filesEnd;
				for (; fileIter != filesEnd; ++fileIter)
				{
					if (ImGui::MenuItem(fileIter->c_str()))
					{
						testToLoad = fileIter;
					}
				}

				if (testToLoad != filesEnd)
				{
					string path = testCasesFolder + "/" + *testToLoad;
					test = new TestCase;
					if (test)
					{
						// Load the test.
						if (!test->load(path))
						{
							delete test;
							test = 0;
						}

						if (editor)
						{
							editor->setContext(&ctx);
							showEditor = false;
						}

						// Load geom.
						meshName = test->getGeomFileName();


						path = meshesFolder + "/" + meshName;

						delete geom;
						geom = new InputGeom;
						if (!geom || !geom->load(&ctx, path))
						{
							delete geom;
							geom = 0;
							delete editor;
							editor = 0;
							showLog = true;
							ctx.dumpLog("Geom load log %s:", meshName.c_str());
						}
						if (editor && geom)
						{
							editor->handleMeshChanged(geom);
							get_model_name(meshName, editor->m_modelName);
						}

						// This will ensure that tile & poly bits are updated in tiled editor.
						if (editor)
							editor->handleSettings();

						ctx.resetLog();
						if (editor && !editor->handleBuild())
						{
							ctx.dumpLog("Build log %s:", meshName.c_str());
						}

						if (geom || editor)
						{
							const float* bmin = 0;
							const float* bmax = 0;
							if (geom)
							{
								bmin = geom->getNavMeshBoundsMin();
								bmax = geom->getNavMeshBoundsMax();
							}
							// Reset camera and fog to match the mesh bounds.
							update_camera(bmin, bmax, cameraPos, cameraEulers, camr);
						}

						// Do the tests.
						if (editor)
							test->doTests(editor->getNavMesh(), editor->getNavMeshQuery());
					}
				}
			}

			ImGui::End();
		}

		
		// Log
		if (showLog && showMenu)
		{
			ImGui::SetNextWindowPos(ImVec2(250.f+30.f, height-450.f-10.f), ImGuiCond_Once);
			ImGui::SetNextWindowSize(ImVec2(200.f, 450.f), ImGuiCond_Once);

			if (ImGui::Begin("Log"))
			{
				for (int i = 0; i < ctx.getLogCount(); ++i)
					ImGui::Text(ctx.getLogText(i));
			}

			ImGui::End();
		}
		
		// Left column tools menu
		if (!showTestCases && showTools && showMenu) // && geom && editor)
		{
			ImGui::SetNextWindowPos(ImVec2(10.f, 10.f), ImGuiCond_Once);
			ImGui::SetNextWindowSize(ImVec2(280, (float)height-20), ImGuiCond_Once);
			ImGui::SetNextWindowSizeConstraints(ImVec2(280, 300), ImVec2(FLT_MAX, FLT_MAX));

			if (ImGui::Begin("Tools", nullptr, baseWindowFlags))
			{
				if (editor)
					editor->handleTools();
			}
			
			ImGui::End();
		}
		
		// Marker
		if (markerPositionSet && gluProject(static_cast<GLdouble>(markerPosition[0]), 
			static_cast<GLdouble>(markerPosition[1]), static_cast<GLdouble>(markerPosition[2]),
								  modelviewMatrix, projectionMatrix, viewport, &x, &y, &z))
		{
			// Draw marker circle
			glLineWidth(5.0f);
			glColor4ub(240,220,0,196);
			glBegin(GL_LINE_LOOP);
			const float r = 25.0f;
			for (int i = 0; i < 20; ++i)
			{
				const float a = (float)i / 20.0f * RD_PI*2;
				const float fx = (float)x + cosf(a)*r;
				const float fy = (float)y + sinf(a)*r;
				glVertex2f(fx,fy);
			}
			glEnd();
			glLineWidth(1.0f);
		}
		
		ImGui::EndFrame();

		ImGui::Render();
		glEnable(GL_DEPTH_TEST);

		ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
	}
	
	imgui_shutdown();
	SDL_Quit();
	
	delete editor;
	delete geom;
	
	return EXIT_SUCCESS;
}
