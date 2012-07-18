#ifndef wcx_loader_h__
#define wcx_loader_h__

#include "WcxModule.h"

class WcxLoader
{
private:
	IWcxModule* LoadSingleModule(const wchar_t* path);

public:
	std::vector<IWcxModule*> Modules;

	int LoadModules(const wchar_t* basePath);
	void UnloadModules();
};

#endif // wcx_loader_h__
