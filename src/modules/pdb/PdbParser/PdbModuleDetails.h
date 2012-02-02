#pragma once

class PdbModuleDetails : public IPdbModuleDetails
{
public:
	PdbModuleDetails( PdbModule* );
	virtual ~PdbModuleDetails();

	//	Set Properties
	void SetCompilerName( const std::wstring& s );;
	void SetGetBackEndBuildNumber( const std::wstring& s );
	void SetManagedCode( bool b );

	//	Retrieve Properties.
	const std::wstring&		GetName() const;
	const std::wstring&		GetCompilerName() const;
	const std::wstring&		GetBackEndBuildNumber() const;
	PdbLanguage				GetLanguage() const;
	PdbTargetCPU			GetTargetCPU() const;

	//	Tests
	bool IsManagedCode() const;

private:
	PdbTargetCPU MapMachineTypeToTargetCPU(DWORD dwType) const;
	
	std::wstring	m_sCompilerName;
	std::wstring	m_sBackEndBuildNumber;
	bool			m_bManagedCode;
	PdbLanguage		m_PdbLanguage;
	PdbTargetCPU	m_PdbTargetCPU; 

	//	Reference the Module
	PdbModule*		m_pModule;
};