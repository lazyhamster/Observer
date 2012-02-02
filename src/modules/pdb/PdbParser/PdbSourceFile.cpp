//	------------------------------------------------------------------------------------------------
//	File name:		PdbSource.cpp
//	Author:			Marc Ochsenmeier
//	Email:			info@winitor.net
//	Web:			www.winitor.net - 
//	Date:			20.08.2011
//
//	------------------------------------------------------------------------------------------------
#include "stdafx.h"

#include "PdbTranslationMaps.h"
#include "PdbFile.h"
#include "PdbSourceFile.h"
#include "PdbModule.h"
#include "PdbParserInternal.h"

using namespace std;

//	----------------------------------------------------------------------------
//	----------------------------------------------------------------------------
PdbSourceFile::PdbSourceFile( IPdbModule* module, IDiaSourceFile* file ) : 
	m_pIPdbModule( module ),
	m_pIDiaSourceFile( file ),
	m_checkSumType( CheckSumType_Unknown )
{
	if( file==NULL )
	{
		return;
	}

	memset( m_checkSum, 0, sizeof(m_checkSum) );

	char buffer[ _MAX_PATH] = { 0 };
	DWORD size = sizeof(buffer);
	if( file->get_checksum( size, &size, (BYTE*)buffer) == S_OK ) 
	{
		char hexval[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
		for( int j= 0; j<16; j++ )
		{
			m_checkSum[j*2] = hexval[((buffer[j] >> 4) & 0xF)]; 
			m_checkSum[(j*2) + 1] = hexval[(buffer[j]) & 0x0F];
		}

		DWORD checksumType = CHKSUM_TYPE_NONE;
		if( file->get_checksumType( &checksumType) == S_OK ) 
		{
			switch( checksumType ) 
			{
			case CHKSUM_TYPE_NONE :
				m_checkSumType = CheckSumType_None;
				break;

			case CHKSUM_TYPE_MD5 :
				m_checkSumType = CheckSumType_MD5;
				break;

			case CHKSUM_TYPE_SHA1 :
				m_checkSumType = CheckSumType_SHA1;
				break;

			default :
				m_checkSumType = CheckSumType_Unknown;
				break;
			}
		}
	}
}
//	----------------------------------------------------------------------------
//	----------------------------------------------------------------------------
PdbSourceFile::~PdbSourceFile()
{
	if( m_pIDiaSourceFile )
	{
		m_pIDiaSourceFile->Release();
		m_pIDiaSourceFile = NULL;
	}
}
//	----------------------------------------------------------------------------
//	----------------------------------------------------------------------------
const wstring& PdbSourceFile::GetSourceFileName()
{
	m_fileName.empty();
	if( m_pIDiaSourceFile )
	{
		BSTR bstrName;
		if( m_pIDiaSourceFile->get_fileName( &bstrName )==S_OK )
		{
			m_fileName = bstrName;
			SysFreeString( bstrName );
		}
	}
	return m_fileName;
}
//	----------------------------------------------------------------------------
//	----------------------------------------------------------------------------
DWORD PdbSourceFile::GetUniqueId()
{
	DWORD dwUniqueId = 0;

	if( m_pIDiaSourceFile )
	{
		DWORD value = 0;
		if( m_pIDiaSourceFile->get_uniqueId( &value )==S_OK )
		{
			dwUniqueId = value;
		}
	}
	return dwUniqueId;
}
//	----------------------------------------------------------------------------
//	----------------------------------------------------------------------------
CheckSumType PdbSourceFile::GetCheckSumType() const
{
	return m_checkSumType;
}
//	----------------------------------------------------------------------------
//	----------------------------------------------------------------------------
const char* PdbSourceFile::GetCheckSum() const
{
	return m_checkSum;
}
