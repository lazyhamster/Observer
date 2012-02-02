#pragma once

class PdbTranslationMaps
{
public:
	PdbTranslationMaps();
	virtual ~PdbTranslationMaps();

	static enum SymTagEnum MapSymbolToDiaType(PdbSymbolType);

	//	Map Target CPU
	typedef std::map<int, PdbTargetCPU> MapPdbTargetCPU;
	static MapPdbTargetCPU mapPdbTargetCPU;

	//	Map Language
	typedef std::map<int, PdbLanguage> MapPdbLanguage;
	static MapPdbLanguage mapPdbLanguage;

	//	Map PdbSymbol
	typedef std::map<PdbSymbolType, enum SymTagEnum> MapPdbSymbolType;
	static MapPdbSymbolType mapPdbSymbolType;
	
	//	Collection of symbols to reject
	typedef std::map<std::wstring, int> MapSymbolsToReject;
	static MapSymbolsToReject mapSymbolsToReject;

private:
	void Init();
};