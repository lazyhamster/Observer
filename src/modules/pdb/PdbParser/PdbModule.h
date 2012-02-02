#pragma once

//	Forwarded references
class PdbTranslationMaps;
class PdbModuleDetails;
class PdbFile;

class PdbModule : public IPdbModule 
{
public:	
	//	Interface
	const std::wstring&				GetName() const;
	IPdbModuleDetails*				GetModuleDetails();
	std::vector<IPdbSourceFile*>&	GetSourceFiles();

	//	Internal
	PdbModule( PdbFile*, IDiaSymbol* );
	virtual ~PdbModule();
	
	DWORD			GetTypeId();
	bool			Reject();
	std::wstring			GetCompilerName();
	std::wstring			GetBackEndBuildNumber();
	bool			GetManagedCode();
	
private:
	//	Accessor
	IDiaSymbol* GetDiaSymbol() const;
	std::vector<IPdbModule*>&	GetModules(PdbSymbolType);

	//	Reference to interfaces
	PdbFile*			m_pPdbFile;
	IDiaSymbol*			m_pIDiaSymbol;

	//	The Name of this module
	mutable std::wstring		m_moduleName;

	TCHAR m_buffer[_MAX_PATH];

	//	Collection of dhildren Modules of this module
	typedef std::vector< IPdbModule* > Modules;
	Modules						m_vectorModules;
	PdbModuleDetails*			m_pModuleDetails;

	//	Collection fo Sources of this module
	std::vector< IPdbSourceFile* >	m_vectorSources;
};