//	------------------------------------------------------------------------------------------------
//	File name:		Parser.h
//	Author:			Marc Ochsenmeier
//	Email:			info@winitor.net
//	Web:			www.winitor.net
//	Date:			20.08.2011
//
//	Description:	This file contains a set of Interfaces that can be used to inquire the internals
//					of a Program Data Base (PDB) file. PDB files are used by Visual Studio and other
//					debugging tools like WinDbg to allow debugging and program inspection.
//
//					The items presented here describes the interface of a C++ based DLL which
//					is called "PdbParser.dll". 
//
//					This file must be included in the "consumer" application.
//
//					Our PdbParser uses the "Debug Interface Access" (DIA) which is 
//					part of the Visual Studio 2008 SDK of Microsoft Corporation.
//
//	------------------------------------------------------------------------------------------------
#pragma once

#include <comutil.h>

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the PDBPARSER_EXPORTS
// symbol defined. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// PDBPARSER_EXPORTS functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
/*
#ifdef PDBPARSER_EXPORTS
#define PDBPARSER_API __declspec(dllexport)
#else
#define PDBPARSER_API __declspec(dllimport)
#endif
*/
#define PDBPARSER_API

namespace PdbParser
{
	//	Forwarded references
	struct IPdbParser;
	struct IPdbFile;
	struct IPdbModule;
	struct IPdbModuleDetails;
	struct IPdbSourceFile;

	enum CheckSumType
	{
	    CheckSumType_None,			// No checksum is available
		CheckSumType_MD5,
		CheckSumType_SHA1,
		CheckSumType_Unknown
	};

	/*!	@brief An enumeration of the several potential languages that can be used as source. */
	enum PdbLanguage { 
		PdbLanguage_Unavailable,	//	language information is not available
		PdbLanguage_C,				//	language is C.
		PdbLanguage_CPP,			//	language is C++.
		PdbLanguage_Fortran,		//	language is FORTRAN.
		PdbLanguage_MASM,			//	language is Microsoft Macro Assembler.
		PdbLanguage_PASCAL,			//	language is Pascal.
		PdbLanguage_BASIC,			//	language is BASIC.
		PdbLanguage_COBOL,			//	language is COBOL.
		PdbLanguage_LINK,			//	Linker-generated module.
		PdbLanguage_CVTRES,			//	Resource module converted with CVTRES tool.
		PdbLanguage_CVTPGD,			//	POGO optimized module generated with CVTPGD tool.
		PdbLanguage_CSHARP,			//	Application language is C#.
		PdbLanguage_VB,				//	Application language is Visual Basic.
		PdbLanguage_ILASM,			//	language is intermediate language assembly.
		PdbLanguage_JAVA,			//	language is Java.
		PdbLanguage_JSCRIPT,		//	language is Jscript.
		PdbLanguage_MSIL			//	language is an unknown MSIL.
	};

	/*!	@brief	An enumeration of several potential CPU types that can be targeted by a program.*/
	enum PdbTargetCPU { 
		PdbTargetCPU_Unavailable,
		PdbTargetCPU_I386,
		PdbTargetCPU_PENTIUM,
		PdbTargetCPU_PENTIUMII,
		PdbTargetCPU_PENTIUMPRO,
		PdbTargetCPU_PENTIUMIII,
		PdbTargetCPU_MIPS ,
		PdbTargetCPU_MIPSR4000,
		PdbTargetCPU_MIPS16 ,
		PdbTargetCPU_MIPS32 ,
		PdbTargetCPU_MIPS64 ,
		PdbTargetCPU_MIPSI,
		PdbTargetCPU_MIPSII ,
		PdbTargetCPU_MIPSIII,
		PdbTargetCPU_MIPSIV ,
		PdbTargetCPU_MIPSV,
		PdbTargetCPU_IA64,
		PdbTargetCPU_IA64_1,
		PdbTargetCPU_IA64_2,
		PdbTargetCPU_TRICORE,
		PdbTargetCPU_X64,
		PdbTargetCPU_AMD64,
		PdbTargetCPU_Unknown
	};

	/*!	@brief	An enumeration of all the potential PDB symbols. */
	enum PdbSymbolType {
		PdbSymbolTypeNull,
		PdbSymbolType_Exe,
		PdbSymbolType_Compiland,
		PdbSymbolType_CompilandDetails,
		PdbSymbolType_CompilandEnv,
		PdbSymbolType_Function,
		PdbSymbolType_Block,
		PdbSymbolType_Data,
		PdbSymbolType_Annotation,
		PdbSymbolType_Label,
		PdbSymbolType_PublicSymbol,
		PdbSymbolType_UDT,
		PdbSymbolType_Enum,
		PdbSymbolType_FunctionType,
		PdbSymbolType_PointerType,
		PdbSymbolType_ArrayType,
		PdbSymbolType_BaseType,
		PdbSymbolType_Typedef,
		PdbSymbolType_BaseClass,
		PdbSymbolType_Friend,
		PdbSymbolType_FunctionArgType,
		PdbSymbolType_FuncDebugStart,
		PdbSymbolType_FuncDebugEnd,
		PdbSymbolType_UsingNamespace,
		PdbSymbolType_VTableShape,
		PdbSymbolType_VTable,
		PdbSymbolType_Custom,
		PdbSymbolType_Thunk,
		PdbSymbolType_CustomType,
		PdbSymbolType_ManagedType,
		PdbSymbolType_Dimension,
		PdbSymbolType_Max
	};

	/*!	@brief	A Class to instantiate the PDB engine.*/
	struct PDBPARSER_API IPdbParserFactory
	{
		/*!
		@brief		Instantiate the PDB parser Engine
		@return		Is a pointer to an instance of the PDB Parser engine, NULL if a failure occured.*/
		static IPdbParser* Create(); 
		
		/*!
		@brief		Cleanup the resources alloacated by the PDB engine.*/
		static void Destroy();
	};

	/*	@brief	A class to Parse Program Database (PDB) files. */
	struct IPdbParser
	{
		/*!
		@brief		Instantiate the IPdbFile interface by opening a PDB file
		@param		Is the path of the Image file to open
		@return		Is a pointer to an instance of the IPdbFile interface the file has been successfully opened, NULL otherwise.*/
		virtual IPdbFile* OpenFile(const std::wstring& path) = 0;
	};

	/*	@brief	A class that represents a PDB file that has been opened using IPdbParser::Open(..). */
	struct IPdbFile 
	{
		/*!
		@brief		Retrieve the GUID of the file.
		@return		Is the GUID embbeded in the file, zero is not available.*/
		virtual GUID GetGuid() const = 0;

		/*!
		@brief .	Get the File Modules
		@return		Is a collection of interface to the Module objects.*/
		virtual std::vector<IPdbModule*>& GetModules() = 0;

		/*!
		@brief		Check whether the PDB file has been stripped (private symbols have been removed).
		@return		Is true when the PDB file has been stripped, false otherwise.*/
		virtual bool IsStripped() = 0;

		/*!
		@brief .	Close the File.*/
		virtual void Close() = 0;
	};

	/*	@brief	A class that represents a Module referenced in a PDB file. */
	struct IPdbModule
	{
		/*!
		@brief		Retrieve the Module name...
		@return ..	Is the Name of the module.*/
		virtual const std::wstring& GetName() const = 0;

		/*!
		@brief		Retrieve the details of a Module
		@return		Is a pointer to the Interface provinding the Details when successfull, NULL otherwise.*/
		virtual IPdbModuleDetails* GetModuleDetails() = 0;

		/*!
		@brief .	Get the Sources Files 
		@return		Is a collection of interface to the Module source file object.*/
		virtual std::vector< IPdbSourceFile* >& GetSourceFiles() = 0;
	};

	/*	@brief	A class that contains some details of a Module referenced in a PDB file. */
	struct IPdbModuleDetails
	{
		/*!
		@brief		Retrieve the Language (e.g C++, C#..) used to create the Module.
		@retutn	.	Is the Language used to create the Module.*/
		virtual PdbLanguage GetLanguage() const = 0;

		/*
		@brief		Retrieve the name of the compiler used to build the Module
		@return		Is the Name of the Compiler.*/
		virtual const std::wstring& GetCompilerName() const = 0;

		/*
		@brief		Returns the target CPU
		@return ..	*/
		virtual PdbTargetCPU GetTargetCPU() const = 0;

		/*!
		@brief ..	Retrieve the Back End build Number <xx>.<yy>.<zzzz>.
		@return ..	*/
		virtual const std::wstring& GetBackEndBuildNumber() const = 0;

		/*!
		@brief ..	Check whether the Module is Managed code.
		@return ..	Is true when the Code is managed, false otherwise.*/
		virtual bool IsManagedCode() const = 0;
	};

	/*	@brief	A class to handle a source of a Module. */
	struct IPdbSourceFile
	{
		virtual const std::wstring&		GetSourceFileName() = 0;
		virtual DWORD				GetUniqueId() = 0;
		virtual CheckSumType		GetCheckSumType() const = 0;
		virtual const char*			GetCheckSum() const = 0;
	};
}

