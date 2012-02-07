#include "stdafx.h"

static std::wstring GetLanguageName(PdbLanguage lang)
{
	switch (lang)	
	{
		case PdbLanguage_Unavailable:
			return L"Information is not available";
		case PdbLanguage_C:
			return L"C";
		case PdbLanguage_CPP:
			return L"C++";
		case PdbLanguage_Fortran:
			return L"FORTRAN";
		case PdbLanguage_MASM:
			return L"Microsoft Macro Assembler";
		case PdbLanguage_PASCAL:
			return L"Pascal";
		case PdbLanguage_BASIC:
			return L"BASIC";
		case PdbLanguage_COBOL:
			return L"COBOL";
		case PdbLanguage_LINK:
			return L"Linker-generated module";
		case PdbLanguage_CVTRES:
			return L"Resource module converted with CVTRES tool";
		case PdbLanguage_CVTPGD:
			return L"POGO optimized module generated with CVTPGD tool";
		case PdbLanguage_CSHARP:
			return L"C#";
		case PdbLanguage_VB:
			return L"Visual Basic";
		case PdbLanguage_ILASM:
			return L"Intermediate Language Assembly";
		case PdbLanguage_JAVA:
			return L"Java";
		case PdbLanguage_JSCRIPT:
			return L"JScript";
		case PdbLanguage_MSIL:
			return L"Unknown MSIL";
	}

	return L"Unknown";
}

static std::wstring GetTargetCPUName(PdbTargetCPU cpu)
{
	switch (cpu)
	{
		case PdbTargetCPU_Unavailable:
			return L"Unavailable";
		case PdbTargetCPU_I386:
			return L"I386";
		case PdbTargetCPU_PENTIUM:
			return L"PENTIUM";
		case PdbTargetCPU_PENTIUMII:
			return L"PENTIUMII";
		case PdbTargetCPU_PENTIUMPRO:
			return L"PENTIUMPRO";
		case PdbTargetCPU_PENTIUMIII:
			return L"PENTIUMIII";
		case PdbTargetCPU_MIPS:
			return L"MIPS";
		case PdbTargetCPU_MIPSR4000:
			return L"MIPSR4000";
		case PdbTargetCPU_MIPS16:
			return L"MIPS16";
		case PdbTargetCPU_MIPS32:
			return L"MIPS32";
		case PdbTargetCPU_MIPS64:
			return L"MIPS64";
		case PdbTargetCPU_MIPSI:
			return L"MIPSI";
		case PdbTargetCPU_MIPSII:
			return L"MIPSII";
		case PdbTargetCPU_MIPSIII:
			return L"MIPSIII";
		case PdbTargetCPU_MIPSIV:
			return L"MIPSIV";
		case PdbTargetCPU_MIPSV:
			return L"MIPSV";
		case PdbTargetCPU_IA64:
			return L"IA64";
		case PdbTargetCPU_IA64_1:
			return L"IA64_1";
		case PdbTargetCPU_IA64_2:
			return L"IA64_2";
		case PdbTargetCPU_TRICORE:
			return L"TRICORE";
		case PdbTargetCPU_X64:
			return L"X64";
		case PdbTargetCPU_AMD64:
			return L"AMD64";
		case PdbTargetCPU_Unknown:
			return L"Unknown";
	}

	return L"Unknown";
}

static std::wstring GetChecksumTypeNameStr(CheckSumType type)
{
	switch (type)
	{
		case CheckSumType_None:
			return L"None";
		case CheckSumType_MD5:
			return L"MD5";
		case CheckSumType_SHA1:
			return L"SHA1";
		case CheckSumType_Unknown:
			return L"Unknown";
	}
	
	return L"-";
}

static int ConvertAndAppend(std::wstring &input, std::string &output)
{
	int bufLen = 0;
	if (input.length() > 0)
	{
		bufLen = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, NULL, 0, NULL, NULL);
		
		char* tmpBuf = (char*) malloc(bufLen + 1);
		memset(tmpBuf, 0, bufLen + 1);
		WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, tmpBuf, bufLen + 1, NULL, NULL);
		output.append(tmpBuf);
		free(tmpBuf);
	}

	return bufLen;
}

size_t GenerateInfoFileContent(const PdbFileInfo* pdbInfo, std::string &buf)
{
	GUID guid = pdbInfo->pdb->GetGuid();
	
	char str[100] = {0};
	sprintf_s(str, sizeof(str) / sizeof(str[0]), "GUID: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\nIs stripped: %d",
		guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
		guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7], pdbInfo->pdb->IsStripped());
	
	buf.append(str);
	return strlen(str);
}

size_t GenerateModuleFileContent(IPdbModule* module, std::string &buf)
{
	std::wstringstream sstr;

	IPdbModuleDetails* details = module->GetModuleDetails();
	sstr << L"Language: " << GetLanguageName(details->GetLanguage()) << std::endl;
	sstr << L"Compiler: " << details->GetCompilerName() << std::endl;
	sstr << L"Target CPU: " << GetTargetCPUName(details->GetTargetCPU()) << std::endl;
	sstr << L"Managed code: " << details->IsManagedCode() << std::endl;
	sstr << L"Back-end build: " << details->GetBackEndBuildNumber() << std::endl;

	sstr << std::endl << std::endl << L"[Source Files]" << std::endl;

	std::vector<IPdbSourceFile*> srcList = module->GetSourceFiles();
	for (size_t i = 0; i < srcList.size(); i++)
	{
		IPdbSourceFile* src = srcList[i];
		CheckSumType sumType = src->GetCheckSumType();
		
		sstr << src->GetUniqueId();
		if (sumType != CheckSumType_Unknown && sumType != CheckSumType_None)
		{
			sstr << L"\t" << GetChecksumTypeNameStr(sumType) << L"\t" << src->GetCheckSum();
		}
		sstr << L"\t" << src->GetSourceFileName() << std::endl;
	}

	std::wstring strData = sstr.str();
	return ConvertAndAppend(strData, buf);
}
