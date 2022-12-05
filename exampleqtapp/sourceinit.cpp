#include "sourceinit.h"

#include <tier1/tier1.h>
#include <tier2/tier2.h>
#include <tier3/tier3.h>
#include <vstdlib/cvar.h>

static const SourceApp::systemReq_t* s_request;
static size_t s_requestCount = 0;

struct loadInfo_t
{
	CSysModule* sysModule;
	CreateInterfaceFn factory;
};
struct systemInfo_t
{
	unsigned short originModule;
	IAppSystem* app;
};

static CUtlDict<systemInfo_t, unsigned short> s_systemDict;
static CUtlDict<loadInfo_t, unsigned short>   s_moduleDict;

void* Factory(const char* interfaceName, int* retCode)
{
	// Return the interface if we know it
	unsigned short id = s_systemDict.Find(interfaceName);
	if (id != s_systemDict.InvalidIndex())
		return s_systemDict[id].app;

	// Do any of our apps know it?
	for (unsigned int i = 0; i < s_systemDict.Count(); i++)
	{
		void* q = s_systemDict.Element(i).app->QueryInterface(interfaceName);
		if (q)
			return q;
	}

	// Should we check the modules' factories?

	// Dunno what you're looking for
	return 0;
}



void SourceApp::SetSystemRequest(const systemReq_t* request, size_t requestCount)
{
	s_request = request;
	s_requestCount = requestCount;
}

int SourceApp::Load()
{
	// Load all modules for each request, once
	for (unsigned int i = 0; i < s_requestCount; i++)
	{
		unsigned short modid = 0;

		unsigned short idx = s_moduleDict.Find(s_request[i].moduleName);
		if (idx == s_moduleDict.InvalidIndex())
		{
			// We don't have this one yet. Load it
			CSysModule* module = Sys_LoadModule(s_request[i].moduleName);
			if (!module)
			{
				printf("Failed to load module %s!\n", s_request[i].moduleName);
				return i + 1;
			}
			
			// We need every module's factory
			CreateInterfaceFn factory = Sys_GetFactory(module);
			if (!factory)
			{
				printf("Failed to load module factory %s!\n", s_request[i].moduleName);
				return i + 1;
			}

			// Store it
			modid = s_moduleDict.Insert(s_request[i].moduleName, { module, factory });
		}
		else
		{
			// We have it, just assign it
			modid = idx;
		}


		if (s_systemDict.HasElement(s_request[i].interfaceName))
		{
			printf("Interface %s listed twice!", s_request[i].interfaceName);
			return i + 1;
		}
		s_systemDict.Insert(s_request[i].interfaceName, { modid, 0 });

	}
	
	// Add in cvars first 
	s_systemDict.Insert(CVAR_INTERFACE_VERSION, { s_systemDict.InvalidIndex(), (IAppSystem*)VStdLib_GetICVarFactory()(CVAR_INTERFACE_VERSION, 0) });

	// Load interfaces
	for (unsigned int i = 0; i < s_requestCount; i++)
	{
		systemInfo_t& sys = s_systemDict.Element(i);
		// Ordered, so we can just access without lookup.
		int ret = 0;
		void* app = s_moduleDict[sys.originModule].factory(s_request[i].interfaceName, &ret);
		if (!app || ret)
		{
			printf("Failed to load app %s!\n", s_request[i].interfaceName);
			return i + 1;
		}
		sys.app = (IAppSystem*)app;
	}
	
	return 0;
}

void* SourceApp::FindSystem(const char* interfaceName)
{
	return Factory(interfaceName, 0);
}

int SourceApp::Connect()
{
	CreateInterfaceFn factory = Factory;

	ConnectTier1Libraries(&factory, 1);
	ConnectTier2Libraries(&factory, 1);
	ConnectTier3Libraries(&factory, 1);

	for (unsigned int i = 0; i < s_systemDict.Count(); i++)
	{
		if (!s_systemDict.Element(i).app->Connect(factory))
		{
			printf("Failed to connect %s!\n", s_systemDict.GetElementName(i));
			return i + 1;
		}
	}

	return false;
}

int SourceApp::Init()
{
	for (unsigned int i = 0; i < s_systemDict.Count(); i++)
	{
		if (s_systemDict.Element(i).app->Init() != INIT_OK)
		{
			printf("Failed to init %s!\n", s_systemDict.GetElementName(i));
			return i + 1;
		}
	}
	return 0;
}

void SourceApp::Shutdown()
{
	// We do this in reverse in hopes of not unloading something while still in use

	// Shutdown all systems
	int fff = s_systemDict.Count();
	for (int i = fff - 1; i >= 0; i--)
	{
		s_systemDict.Element(i).app->Shutdown();
	}

	// Disconnect all systems
	for (int i = s_systemDict.Count() - 1; i >= 0; i--)
	{
		s_systemDict.Element(i).app->Disconnect();
	}

	// Bye bye, tiers!
	DisconnectTier3Libraries();
	DisconnectTier2Libraries();
	DisconnectTier1Libraries();

	// Unload all modules
	for (int i = s_moduleDict.Count() - 1; i >= 0; i--)
	{
		Sys_UnloadModule(s_moduleDict.Element(i).sysModule);
	}

}


void* SourceApp::GetFactory()
{
	return Factory;
}
