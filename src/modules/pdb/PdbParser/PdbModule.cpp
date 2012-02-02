//	------------------------------------------------------------------------------------------------
//	File name:		PdbModule.cpp
//	Author:			Marc Ochsenmeier
//	Email:			info@winitor.net
//	Web:			www.winitor.net - 
//	Date:			20.08.2011
//
//	Description:	Implementation of the Pdb Module class.
//
//	------------------------------------------------------------------------------------------------
#include "stdafx.h"

#include "PdbTranslationMaps.h"
#include "PdbModule.h"
#include "PdbParser.h"
#include "PdbParserInternal.h"
#include "PdbModuleDetails.h"
#include "PdbSourceFile.h"
#include "PdbFile.h"

//	----------------------------------------------------------------------------
//	----------------------------------------------------------------------------
PdbModule::PdbModule( PdbFile* file, IDiaSymbol* pIDiaSymbol) : 
	m_pPdbFile( file ),
	m_pIDiaSymbol( pIDiaSymbol ),
	m_pModuleDetails( NULL )
{
}
//	----------------------------------------------------------------------------
//	----------------------------------------------------------------------------
PdbModule::~PdbModule()
{
	//	Cleanup Children Symbols collection
	std::vector<IPdbModule*>::iterator it1 = m_vectorModules.begin();
	for( ;it1!=m_vectorModules.end(); it1++)
	{
		PdbModule* pPdbSymbol = (PdbModule*) *it1;
		delete pPdbSymbol;
	}

	//	Cleanup Source Files collection
	std::vector<IPdbSourceFile*>::iterator it2 = m_vectorSources.begin();
	for( ;it2!=m_vectorSources.end(); it2++)
	{
		PdbSourceFile* pPdbSymbol = (PdbSourceFile*) *it2;
		delete pPdbSymbol;
	}

	if( m_pIDiaSymbol )
	{
		m_pIDiaSymbol->Release();
		m_pIDiaSymbol = NULL;
	}
	delete m_pModuleDetails;
	m_pModuleDetails = NULL;
}
//	----------------------------------------------------------------------------
//	----------------------------------------------------------------------------
IDiaSymbol* PdbModule::GetDiaSymbol() const 
{ 
	return m_pIDiaSymbol; 
}
//	----------------------------------------------------------------------------
//	----------------------------------------------------------------------------
std::vector<IPdbModule*>& PdbModule::GetModules( PdbSymbolType type )
{
	//	Cleanup Children Symbols collection
	std::vector<IPdbModule*>::iterator it = m_vectorModules.begin();
	for( ;it!=m_vectorModules.end(); it++)
	{
		PdbModule* pPdbSymbol = (PdbModule*) *it;
		delete pPdbSymbol;
	}
	m_vectorModules.clear();

	if( m_pIDiaSymbol )
	{
		//	Map input to DIA type
		enum SymTagEnum symbol = PdbTranslationMaps::MapSymbolToDiaType( type );

		IDiaEnumSymbols* pIDiaEnumSymbols = NULL;

		HRESULT hr = m_pIDiaSymbol->findChildren( 
			symbol, 
			NULL, 
			nsNone, 
			&pIDiaEnumSymbols );

		if( hr==S_OK )
		{
			LONG lCount = 0;
			hr = pIDiaEnumSymbols->get_Count( &lCount );
			
			IDiaSymbol* pIDiaSymbol = NULL;
			for( DWORD dwPos=0; dwPos<(DWORD)lCount; dwPos++ )
			{
				hr = pIDiaEnumSymbols->Item( dwPos, &pIDiaSymbol );
				if( hr==S_OK )
				{
					PdbModule* pSymbol = new PdbModule( m_pPdbFile, pIDiaSymbol );
					
					//	Keep this symbol in the List? (e.g. Reject Symbol beginning with "Import:")
					if( pSymbol->Reject() )
					{
						delete pSymbol;
						pSymbol = NULL;
					}
					else
					{
						m_vectorModules.push_back( pSymbol );

						////	TEST...
						////BSTR bstrValue;
						//GUID guid = GUID_NULL;
						//HRESULT hr = pIDiaSymbol->get_guid( &guid );
						//if( hr==S_OK )
						//{
						//	//wstring s = bstrValue;
						//	//SysFreeString( bstrValue );
						//	int pause = 1;
						//}
					}
				}
			}
			pIDiaEnumSymbols->Release();
		}
	}
	return m_vectorModules;
}
//	----------------------------------------------------------------------------
//	----------------------------------------------------------------------------
std::vector<IPdbSourceFile*>& PdbModule::GetSourceFiles()
{
	//	Cleanup Children Symbols from previous collection
	std::vector<IPdbSourceFile*>::iterator it = m_vectorSources.begin();
	for( ;it!=m_vectorSources.end(); it++)
	{
		PdbSourceFile* pPdbSource = (PdbSourceFile*) *it;
		delete pPdbSource;
	}
	m_vectorSources.clear();

	if( m_pIDiaSymbol )
	{
		IDiaSession* pIDiaSession = m_pPdbFile->GetDiaSession();
		if( pIDiaSession )
		{
			IDiaEnumSourceFiles *pEnumSourceFiles = NULL;

			if( pIDiaSession->findFile( 
				m_pIDiaSymbol, 
				NULL, 
				nsNone, 
				&pEnumSourceFiles)==S_OK )
			{
				LONG lCount = 0;
				pEnumSourceFiles->get_Count( &lCount );

				for( DWORD dw=0; dw<(DWORD)lCount; dw++ )
				{
					IDiaSourceFile* pIDiaSourceFile = NULL;
					if( pEnumSourceFiles->Item( dw, &pIDiaSourceFile )==S_OK )
					{
						PdbSourceFile* pPdbSourceFile = new PdbSourceFile( this, pIDiaSourceFile );
						m_vectorSources.push_back( pPdbSourceFile );
					}
				}
			}
			pEnumSourceFiles->Release();
		}
	}
	return m_vectorSources;
}

//	----------------------------------------------------------------------------
//	----------------------------------------------------------------------------
IPdbModuleDetails* PdbModule::GetModuleDetails()
{	
	//	Cleanup from any previous call
	delete m_pModuleDetails;

	//	Create output object
	m_pModuleDetails = new PdbModuleDetails( this );

	//	Collect the Modules
	Modules modules = GetModules( PdbSymbolType_CompilandDetails );
	Modules::iterator it = modules.begin();

	for( ;it!=modules.end(); it++)
	{
		PdbModule* module = (PdbModule*)*it;

		//	Configure module
		m_pModuleDetails->SetCompilerName( module->GetCompilerName() );
		m_pModuleDetails->SetGetBackEndBuildNumber( module->GetBackEndBuildNumber() );
		m_pModuleDetails->SetManagedCode( module->GetManagedCode() );
	}
	return m_pModuleDetails;
}
//	----------------------------------------------------------------------------
//	----------------------------------------------------------------------------
DWORD PdbModule::GetTypeId()
{
	DWORD dwRet = -1;
	if( m_pIDiaSymbol )
	{
		HRESULT hr = m_pIDiaSymbol->get_typeId(&dwRet);
		dwRet = hr==S_OK?dwRet:-1;
	}
	return dwRet;
}

//	----------------------------------------------------------------------------
//	----------------------------------------------------------------------------
const std::wstring& PdbModule::GetName() const
{
	m_moduleName = L"";

	if( m_pIDiaSymbol )
	{
		BSTR bstrName;
		HRESULT hr = m_pIDiaSymbol->get_name( &bstrName );
		if( hr==S_OK )
		{
			m_moduleName = bstrName;
			SysFreeString( bstrName );
		}
	}
	return m_moduleName;
}
//	----------------------------------------------------------------------------
//	----------------------------------------------------------------------------
bool PdbModule::Reject()
{
	bool bRet = false;
	std::wstring sName = GetName();
	PdbTranslationMaps::MapSymbolsToReject::iterator it = PdbTranslationMaps::mapSymbolsToReject.begin();
	std::wstring sItemRejected;
	size_t iPos = std::wstring::npos;
	for( ;it!=PdbTranslationMaps::mapSymbolsToReject.end(); it++)
	{
		sItemRejected = it->first;
		iPos = sName.find(sItemRejected);
		if (iPos != std::wstring::npos)
		{
			bRet = true;
			break;
		}
	}
	return bRet;
}
//	----------------------------------------------------------------------------
//	----------------------------------------------------------------------------
std::wstring PdbModule::GetCompilerName()
{
	std::wstring sRet = L"";
	if( m_pIDiaSymbol )
	{
		BSTR bstrName;
		HRESULT hr = m_pIDiaSymbol->get_compilerName( &bstrName );
		if( hr==S_OK )
		{
			sRet = bstrName;
			SysFreeString( bstrName );
		}
	}
	return sRet;
}
//	----------------------------------------------------------------------------
//	----------------------------------------------------------------------------
std::wstring PdbModule::GetBackEndBuildNumber()
{
	std::wstring sRet;	
	if( m_pIDiaSymbol )
	{
		DWORD dwVerMajor, dwVerMinor, dwVerBuild = 0;
		if (( m_pIDiaSymbol->get_backEndMajor( &dwVerMajor ) == S_OK ) &&
			( m_pIDiaSymbol->get_backEndMinor( &dwVerMinor ) == S_OK ) &&
			( m_pIDiaSymbol->get_backEndBuild( &dwVerBuild ) == S_OK )) 
		{
			ZeroMemory(m_buffer, 64*sizeof(TCHAR));
			int size = swprintf(m_buffer, sizeof(m_buffer) / sizeof(m_buffer[0]), L"%u.%u.%u", dwVerMajor, dwVerMinor, dwVerBuild); 
			sRet.assign (m_buffer, size );
		}
	}
	return sRet;
}
bool PdbModule::GetManagedCode()
{
	bool bRet = false;
	if( m_pIDiaSymbol )
	{
		BOOL b = FALSE;
		HRESULT hr = m_pIDiaSymbol->get_managed( &b );
		bRet = hr==S_OK?true:false;
	}
	return bRet;
}