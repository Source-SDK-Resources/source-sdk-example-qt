#pragma once
#include "utldict.h"


namespace SourceApp
{
	struct systemReq_t
	{
		const char* moduleName;
		const char* interfaceName;
	};

	// Array of system infos
	void SetSystemRequest(const systemReq_t* request, size_t requestCount);

	// Loads all specified modules and interfaces. Returns 0 on success or the index of the request it failed to load plus one
	int Load();

	// Returns the specified module
	void* FindSystem(const char* interfaceName);

	// Connects all apps. Returns 0 on success or the index of the request it failed to connect plus one
	int Connect();
	
	// Inits all apps. Returns errors as the rest do
	int Init();

	void Shutdown();

	void* GetFactory();
};