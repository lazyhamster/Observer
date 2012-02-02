//	------------------------------------------------------------------------------------------------
//	File name:		PdbParser.cpp
//	Author:			Marc Ochsenmeier
//	Email:			info@winitor.net
//	Web:			www.winitor.net - 
//	Date:			20.08.2011
//
//	Description:	This file contains the entry points of our PDB interfaces accessors schema.
//
//	------------------------------------------------------------------------------------------------
#include "stdafx.h"

#include "PdbTranslationMaps.h"
#include "PdbFile.h"
#include "PdbParserInternal.h"

//	Global
PdbParserInternal* pIPdbParser = NULL;
IDiaDataSource* pIDiaDataSource = NULL;

//	Statics
PdbFile* PdbParserInternal::m_pIPdbFile = NULL;

//	---------------------------------------------------------------------------
//	---------------------------------------------------------------------------
IPdbParser* IPdbParserFactory::Create()
{
	if( pIPdbParser==NULL )
	{
		//	Initialize Appartment
		//HRESULT hr = CoInitialize(NULL);
		//_ASSERT( hr==S_OK );

		// Access the DIA provider
		IDiaDataSource* pIDiaDataSource = NULL;
		HRESULT hr = CoCreateInstance(__uuidof(DiaSource), 
			NULL, CLSCTX_INPROC_SERVER, 
			__uuidof(IDiaDataSource), 
			(void**)&pIDiaDataSource);
		
		if(FAILED(hr)) 
		{
			throw -1;
		}
		else
		{
			if( pIPdbParser==NULL )
			{
				pIPdbParser = new PdbParserInternal( pIDiaDataSource ); 
			}
		}
		_ASSERTE(pIPdbParser);
	}
	return pIPdbParser;
}
//	---------------------------------------------------------------------------
//	---------------------------------------------------------------------------
void IPdbParserFactory::Destroy()
{
	delete pIPdbParser;
	pIPdbParser = NULL;
	
	//	Leave COM appartment
	//CoUninitialize();
}
//	---------------------------------------------------------------------------
//	---------------------------------------------------------------------------
PdbParserInternal::PdbParserInternal(IDiaDataSource* pIDiaDataSource ) : 
	m_pIDiaDataSource(pIDiaDataSource)
{
}
//	---------------------------------------------------------------------------
//	---------------------------------------------------------------------------
PdbParserInternal::~PdbParserInternal()
{
	delete m_pIPdbFile;
	m_pIPdbFile = NULL;

	if( m_pIDiaDataSource ) 
	{
		m_pIDiaDataSource->Release();
		m_pIDiaDataSource = NULL;
	}
}
//	---------------------------------------------------------------------------
//	---------------------------------------------------------------------------
IDiaDataSource* PdbParserInternal::GetDiaDataSource() const
{
	return m_pIDiaDataSource;
}
//	---------------------------------------------------------------------------
//	---------------------------------------------------------------------------
IPdbFile* PdbParserInternal::OpenFile(const std::wstring& path)
{
	try
	{
		if( &path!=NULL )
		{
			//	Close previous one.
			if( m_pIPdbFile )
			{
				delete m_pIPdbFile;
				m_pIPdbFile = NULL;
			}

			m_pIPdbFile = new PdbFile(this, path);
			if( m_pIPdbFile->Load(path)==false )
			{
				//	the PDB file has NOT been found
				delete m_pIPdbFile;
				m_pIPdbFile = NULL;
			}
		}
	}
	catch(...)
	{
		delete m_pIPdbFile;
		m_pIPdbFile = NULL;
	}
	return m_pIPdbFile;
}