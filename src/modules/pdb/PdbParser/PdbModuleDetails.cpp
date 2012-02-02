//	------------------------------------------------------------------------------------------------
//	File name:		PdbModuleDetails.cpp
//	Author:			Marc Ochsenmeier
//	Email:			info@winitor.net
//	Web:			www.winitor.net - 
//	Date:			20.08.2011
//
//	Description:	Implementation of the Pdb Module details.
//
//	------------------------------------------------------------------------------------------------
#include "stdafx.h"

#include "PdbTranslationMaps.h"
#include "PdbModule.h"
#include "PdbParser.h"
#include "PdbModuleDetails.h"

PdbModuleDetails::PdbModuleDetails( PdbModule* module ) : 
	m_pModule( module ),
	m_bManagedCode( false ),
	m_PdbLanguage( PdbLanguage_Unavailable ),
	m_PdbTargetCPU( PdbTargetCPU_Unavailable )
{
}
PdbModuleDetails::~PdbModuleDetails()
{
}
const std::wstring& PdbModuleDetails::GetName() const
{
	return m_pModule->GetName();
}
void PdbModuleDetails::SetCompilerName( const std::wstring& s )
{
	m_sCompilerName = s;
}
void PdbModuleDetails::SetGetBackEndBuildNumber( const std::wstring& s )
{
	m_sBackEndBuildNumber = s;
}
void PdbModuleDetails::SetManagedCode(bool b)
{
	m_bManagedCode = b;
}
PdbLanguage PdbModuleDetails::GetLanguage() const
{
	return m_PdbLanguage;
}
bool PdbModuleDetails::IsManagedCode() const
{
	return m_bManagedCode;
}
const std::wstring& PdbModuleDetails::GetBackEndBuildNumber() const 
{
	return m_sBackEndBuildNumber;
}
const std::wstring& PdbModuleDetails::GetCompilerName() const 
{
	return m_sCompilerName;
}
PdbTargetCPU PdbModuleDetails::GetTargetCPU() const 
{
	return m_PdbTargetCPU;
}
PdbTargetCPU PdbModuleDetails::MapMachineTypeToTargetCPU( DWORD dwType ) const
{
	PdbTargetCPU targetCPU = PdbTargetCPU_Unknown;
	PdbTranslationMaps::MapPdbTargetCPU::const_iterator it = PdbTranslationMaps::mapPdbTargetCPU.find(dwType);
	if(it!=PdbTranslationMaps::mapPdbTargetCPU.end())
	{
		targetCPU = it->second;
	}
	return targetCPU;
}