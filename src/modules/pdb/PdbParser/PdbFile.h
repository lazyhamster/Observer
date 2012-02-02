#pragma once

// Forwarded classes
class PdbParserInternal;
class PdbModule;
class PdbTranslationMaps;

class PdbFile : public IPdbFile
{
public:
	//	Interface
	GUID GetGuid() const;
	std::vector<IPdbModule*>& GetModules();
	bool IsStripped();

	//	Internal
	PdbFile( PdbParserInternal*, const std::wstring& path );
	virtual ~PdbFile();

	bool	Load(const std::wstring& sFileName);
	void	Close();
	IDiaSession* GetDiaSession() const { return m_pIDiaSession; }

private:
	void Cleanup();
	bool LoadSymbolsFileWithoutValidation(const std::wstring& sPdbFileName);

	const std::wstring&			m_sFileName;
	PdbParserInternal*					m_pPdbParser;
	IDiaSession*				m_pIDiaSession;
	IDiaSymbol*					m_pIDiaSymbol;
	std::vector<IPdbModule*>	m_vectorSymbols;
	PdbTranslationMaps			m_PdbTranslationMaps;
};
