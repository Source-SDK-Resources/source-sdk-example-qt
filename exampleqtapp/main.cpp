#include <QApplication>

#include "sourceinit.h"

#include <tier0/icommandline.h>
#include <filesystem_init.h>
#include <materialsystem/imaterialsystem.h>
#include <materialsystem/imaterialproxyfactory.h>
#include <istudiorender.h>
#include <vphysics_interface.h>
#include <datacache/imdlcache.h>
#include <datacache/idatacache.h>
#include <tier3/tier3.h>

#include "ui.h"



// Currently blank, but might be worth filling in if you need mat proxies
class CMaterialProxyFactory : public IMaterialProxyFactory
{
public:
	virtual IMaterialProxy* CreateProxy(const char* proxyName) { return nullptr; }
	virtual void DeleteProxy(IMaterialProxy* pProxy) { }
};
static CMaterialProxyFactory s_materialProxyFactory;

// Setup the filesystem for accessing game data
bool initFilesystem()
{
	// Find the route to our gameinfo file
	CFSSteamSetupInfo steamInfo;
	steamInfo.m_pDirectoryName = NULL;
	steamInfo.m_bOnlyUseDirectoryName = false;
	steamInfo.m_bToolsMode = true;
	steamInfo.m_bSetSteamDLLPath = true;
	steamInfo.m_bSteam = false;
	if (FileSystem_SetupSteamEnvironment(steamInfo) != FS_OK)
		return false;

	// Use our vproject's path
	FileSystem_UseVProjectBinDir(true);
	if (FileSystem_SetBasePaths(g_pFullFileSystem) != FS_OK)
		return false;

	// Mount the mod's search paths
	CFSSearchPathsInit searchPathsInit;
	searchPathsInit.m_pDirectoryName = steamInfo.m_GameInfoPath;
	searchPathsInit.m_pFileSystem = g_pFullFileSystem;
	if (FileSystem_LoadSearchPaths(searchPathsInit) != FS_OK)
		return false;

	// Add platform to our search path
	FileSystem_AddSearchPath_Platform(g_pFullFileSystem, steamInfo.m_GameInfoPath);

	return true;
}

SpewRetval_t spewOut(SpewType_t spewType, const tchar* pMsg)
{
	printf(pMsg);
	return SpewRetval_t::SPEW_CONTINUE;
}

int initSource()
{
	SpewOutputFunc(spewOut);

	SourceApp::systemReq_t appSystems[] =
	{
		{ "filesystem_stdio",   FILESYSTEM_INTERFACE_VERSION      },
		{ "materialsystem",		MATERIAL_SYSTEM_INTERFACE_VERSION },
		{ "studiorender",		STUDIO_RENDER_INTERFACE_VERSION   },
		{ "vphysics",			VPHYSICS_INTERFACE_VERSION        }, // Needed for datacache
		{ "datacache",			DATACACHE_INTERFACE_VERSION       },
		{ "datacache",			MDLCACHE_INTERFACE_VERSION        },
	};
	SourceApp::SetSystemRequest(appSystems, sizeof(appSystems) / sizeof(SourceApp::systemReq_t));

	int err = 0;
	if (err = SourceApp::Load()) return err;

	g_pFullFileSystem = (IFileSystem*)     SourceApp::FindSystem(FILESYSTEM_INTERFACE_VERSION);
	g_pMaterialSystem = (IMaterialSystem*) SourceApp::FindSystem(MATERIAL_SYSTEM_INTERFACE_VERSION);
	g_pStudioRender   = (IStudioRender*)   SourceApp::FindSystem(STUDIO_RENDER_INTERFACE_VERSION);
	g_pMDLCache       = (IMDLCache*)       SourceApp::FindSystem(MDLCACHE_INTERFACE_VERSION);

	if (!g_pFullFileSystem || !g_pMaterialSystem || !g_pStudioRender || !g_pMDLCache)
	{
		Error("Unable to load required library interfaces!\n");
		return -1;
	}
	
	g_pMaterialSystem->SetShaderAPI("shaderapidx9");

	if (err = SourceApp::Connect()) return err;


	initFilesystem();
	MathLib_Init();
	
	// Give it the fullscreen texture flag so we can take screenshots during runtime
	g_pMaterialSystem->SetAdapter(0, MATERIAL_INIT_ALLOCATE_FULLSCREEN_TEXTURE);

	if(err = SourceApp::Init()) return err;

	g_pMaterialSystem->SetMaterialProxyFactory(&s_materialProxyFactory);
	g_pMaterialSystem->ModInit();

	return 0;
}
void shutdownSource()
{
	g_pMaterialSystem->ModShutdown();

	SourceApp::Shutdown();
}


int main(int argc, char** argv)
{
	CommandLine()->CreateCmdLine(argc, argv);
	
	// If we don't have a game specified, try hl2?
	if (!CommandLine()->FindParm("-game"))
		CommandLine()->AppendParm("-game", "../hl2");

	int initErr = initSource();
	if (initErr)
	{
		printf("Failed to start up Source!\n");
		return initErr;
	}
	
	QApplication app(argc, argv);

	CMainWindow* window = new CMainWindow(nullptr);
	window->show();
	window->init();
	
	CBoxView* boxView = new CBoxView(0);
	boxView->show();
	boxView->init();


	int ret = QApplication::exec();

	// Qt's closed down now. Let's shut down source engine
	shutdownSource();

	return ret;
}

