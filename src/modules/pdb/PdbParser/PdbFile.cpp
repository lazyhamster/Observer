//	------------------------------------------------------------------------------------------------
//	File name:		PdbFile.cpp
//	Author:			Marc Ochsenmeier
//	Email:			info@winitor.net
//	Web:			www.winitor.net - 
//	Date:			20.08.2011
//
//	------------------------------------------------------------------------------------------------
#include "stdafx.h"

#include "PdbTranslationMaps.h"
#include "PdbParserInternal.h"
#include "PdbFile.h"
#include "PdbModule.h"
#include "PdbModuleDetails.h"

//	------------------------------------------------------------------------------------------------
//	------------------------------------------------------------------------------------------------
const wchar_t*	TEXT_PDB_FILE_EXTENSION	= L".pdb";

//	------------------------------------------------------------------------------------------------
//	------------------------------------------------------------------------------------------------
PdbFile::PdbFile( PdbParserInternal* parser, const std::wstring& path ) :
	m_pPdbParser( parser ),
	m_pIDiaSession( NULL ), 
	m_pIDiaSymbol( NULL ),
	m_sFileName( path )
{
}
//	------------------------------------------------------------------------------------------------
//	------------------------------------------------------------------------------------------------
PdbFile::~PdbFile()
{
	Cleanup();
}
//	------------------------------------------------------------------------------------------------
//	------------------------------------------------------------------------------------------------
void PdbFile::Close()
{
	Cleanup();
}
//	------------------------------------------------------------------------------------------------
//	------------------------------------------------------------------------------------------------
void PdbFile::Cleanup()
{
	//	Clean Translation Map
	std::vector<IPdbModule*>::iterator it = m_vectorSymbols.begin();
	for( ;it!=m_vectorSymbols.end(); it++)
	{
		PdbModule* pPdbSymbol = (PdbModule*)*it;
		delete pPdbSymbol;
	}
	m_vectorSymbols.clear();

	if ( m_pIDiaSymbol ) 
	{
		m_pIDiaSymbol->Release();
		m_pIDiaSymbol = NULL;
	}
	if ( m_pIDiaSession ) 
	{
		m_pIDiaSession->Release();
		m_pIDiaSession = NULL;
	}
}
//	------------------------------------------------------------------------------------------------
// Load a PDB file
//	------------------------------------------------------------------------------------------------
bool PdbFile::Load( const std::wstring& sFileName )
{
	if( sFileName.size()==0 )
	{
		return false;
	}

	bool bRet = false;

	//	Extract file extension
	wchar_t wszExt[MAX_PATH];
	if( _wsplitpath_s( sFileName.c_str(), NULL, 0, NULL, 0, NULL, 0, wszExt, MAX_PATH)==0 )
	{
		//	PDB File?
		if( _wcsicmp( wszExt, TEXT_PDB_FILE_EXTENSION )==0 )
		{
			// Open (WITHOUT validation) the PDB file as a debug data source
			if( LoadSymbolsFileWithoutValidation( sFileName ) )
			{
				// Retrieve a reference to the global scope
				bRet = m_pIDiaSession->get_globalScope( &m_pIDiaSymbol )==S_OK?true:false;
				_ASSERT( bRet );
			}
		}
	}
	return bRet;
}
bool PdbFile::LoadSymbolsFileWithoutValidation(const std::wstring& sPdbFileName)
{
	bool bRet = false;

	//	Retrieve the DIA datasource
	IDiaDataSource* pIDiaDataSource = m_pPdbParser->GetDiaDataSource();

	if(pIDiaDataSource)
	{
		bool bContinue = false;

		//	Load the PDB file
		HRESULT hr = pIDiaDataSource->loadDataFromPdb(sPdbFileName.c_str());
		if(SUCCEEDED(hr))
		{
			bContinue = true;
		}
		else
		{
			switch(hr)
			{
				//	Failed to open the file, or determined that the file has an invalid format.
			case E_PDB_NOT_FOUND:
				break;
				//	Attempted to access a file with an obsolete format.
			case E_PDB_FORMAT:
				break;
				//	Invalid parameter.
			case E_INVALIDARG:
				break;
				//	Data source has already been prepared.
			case E_UNEXPECTED:
				bContinue = true;
				break;
			default:
				break;
			}
		}
		
		if(bContinue)
		{
			// Open a session for querying symbols
			bRet = pIDiaDataSource->openSession( &m_pIDiaSession )==S_OK?true:false;
		}
	}
	return bRet;
}
GUID PdbFile::GetGuid() const
{
	GUID guid = GUID_NULL;
	if( m_pIDiaSymbol )
	{
		HRESULT hr = m_pIDiaSymbol->get_guid( &guid );
		if(hr==S_OK)
		{
			LPOLESTR lp = NULL;
			hr = StringFromCLSID( guid, &lp );
			if(hr==S_OK)
			{
				CoTaskMemFree(lp);
			}
		}
	}
	return guid;
}
bool PdbFile::IsStripped()
{
	bool bRet = false;
	if( m_pIDiaSymbol )
	{
		BOOL b = FALSE;
		HRESULT hr = m_pIDiaSymbol->get_isStripped(&b);
		if(hr==S_OK)
		{
			bRet = b?true:false;
		}
	}
	return bRet;
}

std::vector<IPdbModule*>& PdbFile::GetModules()
{
	//	Cleanup Translations Map from previous call
	std::vector<IPdbModule*>::iterator it = m_vectorSymbols.begin();
	for( ;it!=m_vectorSymbols.end(); it++)
	{
		PdbModule* module = (PdbModule*) *it;
		delete module;
	}
	m_vectorSymbols.clear();

	if( m_pIDiaSymbol )
	{
		IDiaEnumSymbols* pIDiaEnumSymbols = NULL;

		HRESULT hr = m_pIDiaSymbol->findChildren( 
			SymTagCompiland, 
			NULL, 
			nsNone, 
			&pIDiaEnumSymbols );

		if(hr==S_OK)
		{
			LONG lCount = 0;
			hr = pIDiaEnumSymbols->get_Count( &lCount );
			
			IDiaSymbol* pIDiaSymbol = NULL;
			for(DWORD dwPos=0; dwPos<(DWORD)lCount; dwPos++)
			{
				hr = pIDiaEnumSymbols->Item( dwPos, &pIDiaSymbol );
				if(hr==S_OK)
				{
					PdbModule* pSymbol = new PdbModule( this, pIDiaSymbol );
					
					//	Keep this symbol in the List? (e.g. Reject Symbol beginning with "Import:")
					if( pSymbol->Reject() )
					{
						delete pSymbol;
						pSymbol = NULL;
					}
					else
					{
						m_vectorSymbols.push_back( pSymbol );
					}
				}
			}
			pIDiaEnumSymbols->Release();
		}
	}
	return m_vectorSymbols;
}