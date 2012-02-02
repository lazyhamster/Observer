#pragma once

//	Forwarded references
class PdbFile;

class PdbParserInternal : public IPdbParser
{
public:
	//	Interface
	IPdbFile* OpenFile(const std::wstring& path);

	//	Internal
	PdbParserInternal( IDiaDataSource* );
	virtual ~PdbParserInternal();

	IDiaDataSource* GetDiaDataSource() const;

private:
	IDiaDataSource* m_pIDiaDataSource;
	static PdbFile* m_pIPdbFile;
};