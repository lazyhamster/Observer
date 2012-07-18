#ifndef wcx_loader_h__
#define wcx_loader_h__

#include "WcxModule.h"

class WcxLoader
{
private:
	IWcxModule* LoadSingleModule(const wchar_t* path);
	int LoadModulesInDirectory(const wchar_t* basePath, bool recursive);

public:
	std::vector<IWcxModule*> Modules;

	int LoadModules(const wchar_t* basePath, bool recursive);
	void UnloadModules();
};

#endif // wcx_loader_h__
