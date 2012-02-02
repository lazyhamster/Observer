//	------------------------------------------------------------------------------------------------
//	File name:		PdbTranslationMaps.cpp
//	Author:			Marc Ochsenmeier
//	Email:			info@winitor.net
//	Web:			www.winitor.net - 
//	Date:			20.08.2011
//
//	Description:	Implementation of a utility class to map DIA ids into our PdbParser format
//					and vice versa.
//
//	------------------------------------------------------------------------------------------------
#include "stdafx.h"
#include "PdbTranslationMaps.h"

//	static
std::map<PdbSymbolType, enum SymTagEnum> PdbTranslationMaps::mapPdbSymbolType;
std::map<int, PdbTargetCPU> PdbTranslationMaps::mapPdbTargetCPU;
std::map<int, PdbLanguage> PdbTranslationMaps::mapPdbLanguage;
std::map<std::wstring, int> PdbTranslationMaps::mapSymbolsToReject;

//	Ctor
PdbTranslationMaps::PdbTranslationMaps()
{
	Init();
}
PdbTranslationMaps::~PdbTranslationMaps()
{
}
enum SymTagEnum PdbTranslationMaps::MapSymbolToDiaType(PdbSymbolType type)
{
	enum SymTagEnum symbol;
	MapPdbSymbolType::iterator it = mapPdbSymbolType.find(type);
	if(it!=mapPdbSymbolType.end())
	{
		symbol = it->second;
	}
	return symbol;
}
void PdbTranslationMaps::Init()
{
	if( mapPdbSymbolType.size()==0 )
	{
		mapPdbSymbolType[PdbSymbolTypeNull]					= SymTagNull;
		mapPdbSymbolType[PdbSymbolType_Exe]					= SymTagExe;
		mapPdbSymbolType[PdbSymbolType_Compiland]			= SymTagCompiland;
		mapPdbSymbolType[PdbSymbolType_CompilandDetails]	= SymTagCompilandDetails;
		mapPdbSymbolType[PdbSymbolType_CompilandEnv]		= SymTagCompilandEnv;
		mapPdbSymbolType[PdbSymbolType_Function]			= SymTagFunction;
		mapPdbSymbolType[PdbSymbolType_Block]				= SymTagBlock;
		mapPdbSymbolType[PdbSymbolType_Data]				= SymTagData;
		mapPdbSymbolType[PdbSymbolType_Annotation]			= SymTagAnnotation;
		mapPdbSymbolType[PdbSymbolType_Label]				= SymTagLabel;
		mapPdbSymbolType[PdbSymbolType_PublicSymbol]		= SymTagPublicSymbol;
		mapPdbSymbolType[PdbSymbolType_UDT]					= SymTagUDT;
		mapPdbSymbolType[PdbSymbolType_Enum]				= SymTagEnum;
		mapPdbSymbolType[PdbSymbolType_FunctionType]		= SymTagFunctionType;
		mapPdbSymbolType[PdbSymbolType_PointerType]			= SymTagPointerType;
		mapPdbSymbolType[PdbSymbolType_ArrayType]			= SymTagArrayType;
		mapPdbSymbolType[PdbSymbolType_BaseType]			= SymTagBaseType;
		mapPdbSymbolType[PdbSymbolType_Typedef]				= SymTagTypedef;
		mapPdbSymbolType[PdbSymbolType_BaseClass]			= SymTagBaseClass;
		mapPdbSymbolType[PdbSymbolType_Friend]				= SymTagFriend;
		mapPdbSymbolType[PdbSymbolType_FunctionArgType]		= SymTagFunctionArgType;
		mapPdbSymbolType[PdbSymbolType_FuncDebugStart]		= SymTagFuncDebugStart;
		mapPdbSymbolType[PdbSymbolType_FuncDebugEnd]		= SymTagFuncDebugEnd;
		mapPdbSymbolType[PdbSymbolType_UsingNamespace]		= SymTagUsingNamespace;
		mapPdbSymbolType[PdbSymbolType_VTableShape]			= SymTagVTableShape;
		mapPdbSymbolType[PdbSymbolType_VTable]				= SymTagVTable;
		mapPdbSymbolType[PdbSymbolType_Custom]				= SymTagCustom;
		mapPdbSymbolType[PdbSymbolType_Thunk]				= SymTagThunk;
		mapPdbSymbolType[PdbSymbolType_CustomType]			= SymTagCustomType;
		mapPdbSymbolType[PdbSymbolType_ManagedType]			= SymTagManagedType;
		mapPdbSymbolType[PdbSymbolType_Dimension]			= SymTagDimension;
		mapPdbSymbolType[PdbSymbolType_Max]					= SymTagMax;
	}
	
	if(mapPdbTargetCPU.size()==0)
	{
		mapPdbTargetCPU[IMAGE_FILE_MACHINE_I386] = PdbTargetCPU_I386;
		mapPdbTargetCPU[IMAGE_FILE_MACHINE_IA64] = PdbTargetCPU_IA64;
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_R3000] =          0x0162  // MIPS little-endian, 0x160 big-endian
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_R4000             0x0166  // MIPS little-endian
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_R10000            0x0168  // MIPS little-endian
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_WCEMIPSV2         0x0169  // MIPS little-endian WCE v2
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_ALPHA             0x0184  // Alpha_AXP
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_SH3               0x01a2  // SH3 little-endian
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_SH3DSP            0x01a3
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_SH3E              0x01a4  // SH3E little-endian
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_SH4               0x01a6  // SH4 little-endian
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_SH5               0x01a8  // SH5
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_ARM               0x01c0  // ARM Little-Endian
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_THUMB             0x01c2
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_AM33              0x01d3
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_POWERPC           0x01F0  // IBM PowerPC Little-Endian
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_POWERPCFP         0x01f1
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_MIPS16            0x0266  // MIPS
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_ALPHA64           0x0284  // ALPHA64
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_MIPSFPU           0x0366  // MIPS
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_MIPSFPU16         0x0466  // MIPS
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_AXP64             IMAGE_FILE_MACHINE_ALPHA64
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_TRICORE           0x0520  // Infineon
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_CEF               0x0CEF
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_EBC               0x0EBC  // EFI Byte Code
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_AMD64             0x8664  // AMD64 (K8)
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_M32R              0x9041  // M32R little-endian
		//mapPdbTargetCPU[IMAGE_FILE_MACHINE_CEE               0xC0EE
	}

	if(mapPdbLanguage.size()==0)
	{
		mapPdbLanguage[CV_CFL_C]		= PdbLanguage_C;
		mapPdbLanguage[CV_CFL_CXX]		= PdbLanguage_CPP;
		mapPdbLanguage[CV_CFL_FORTRAN]	= PdbLanguage_Fortran;
		mapPdbLanguage[CV_CFL_MASM]		= PdbLanguage_MASM;
		mapPdbLanguage[CV_CFL_PASCAL]	= PdbLanguage_PASCAL;
		mapPdbLanguage[CV_CFL_BASIC]	= PdbLanguage_BASIC;
		mapPdbLanguage[CV_CFL_COBOL]	= PdbLanguage_COBOL;
		mapPdbLanguage[CV_CFL_LINK]		= PdbLanguage_LINK;
		mapPdbLanguage[CV_CFL_CVTRES]	= PdbLanguage_CVTRES;
		mapPdbLanguage[CV_CFL_CVTPGD]	= PdbLanguage_CVTPGD;
		mapPdbLanguage[CV_CFL_CSHARP]	= PdbLanguage_CSHARP;
		mapPdbLanguage[CV_CFL_VB]		= PdbLanguage_VB;
		mapPdbLanguage[CV_CFL_ILASM]	= PdbLanguage_ILASM;
		mapPdbLanguage[CV_CFL_JAVA]		= PdbLanguage_JAVA;
		mapPdbLanguage[CV_CFL_JSCRIPT]	= PdbLanguage_JSCRIPT;
		mapPdbLanguage[CV_CFL_MSIL]		= PdbLanguage_MSIL;
	}
	
	if( mapSymbolsToReject.size()==0 )
	{
		//	Collection of Symbols to reject
		mapSymbolsToReject[L"* Linker *"] = 0;
		mapSymbolsToReject[L"Import:"] = 1;
	}
}