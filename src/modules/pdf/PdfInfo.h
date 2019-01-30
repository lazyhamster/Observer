#ifndef PdfInfo_h__
#define PdfInfo_h__

#include "poppler/PDFDoc.h"
#include "poppler/FileSpec.h"

enum class PseudoFileSaveResult
{
	PFSR_OK,
	PFSR_ERROR_READ,
	PFSR_ERROR_WRITE
};

class PdfPseudoFile
{
protected:
	PdfPseudoFile() {}
	
	std::wstring m_strName;

public:
	const wchar_t* GetName() const { return m_strName.c_str(); }

	virtual void GetCreationTime(FILETIME* time) const { /* Do nothing */ }
	virtual void GetModificationTime(FILETIME* time) const { /* Do nothing */ }
	
	virtual const wchar_t* GetPrefix() const = 0;
	virtual __int64 GetSize() const = 0;
	virtual PseudoFileSaveResult Save(const wchar_t* filePath) const = 0;
};

class PdfEmbeddedFile : public PdfPseudoFile
{
private:
	FileSpec* m_pSpec;
	EmbFile* m_pEmbFile;

public:
	PdfEmbeddedFile(FileSpec* spec);
	~PdfEmbeddedFile();

	virtual const wchar_t* GetPrefix() const { return L"{files}\\"; }
	virtual __int64 GetSize() const { return m_pEmbFile ? m_pEmbFile->size() : 0; }

	virtual void GetCreationTime(FILETIME* time) const;
	virtual void GetModificationTime(FILETIME* time) const;

	virtual PseudoFileSaveResult Save(const wchar_t* filePath) const;
};

class PdfScriptFile : public PdfPseudoFile
{
private:
	std::string m_strText;

public:
	PdfScriptFile(const char* name, const char* text, size_t textSize);

	virtual const wchar_t* GetPrefix() const { return L"{scripts}\\"; }
	virtual __int64 GetSize() const { return m_strText.size(); }

	virtual PseudoFileSaveResult Save(const wchar_t* filePath) const;
};

class PdfMetadataFile : public PdfPseudoFile
{
private:
	std::string m_strMetadata;

public:
	PdfMetadataFile(const std::string& data);

	virtual const wchar_t* GetPrefix() const { return L""; }
	virtual __int64 GetSize() const { return m_strMetadata.size(); }

	virtual PseudoFileSaveResult Save(const wchar_t* filePath) const;
};

struct PdfInfo
{
	PdfInfo() : docRef(nullptr) {}

	bool LoadInfo(PDFDoc* doc, bool openAllFiles);
	void Cleanup();

	int GetNumFiles() const { return (int) m_vFiles.size(); }
	const PdfPseudoFile* GetFile(int i) const { return m_vFiles[i]; }

private:
	PDFDoc* docRef;
	std::vector<PdfPseudoFile*> m_vFiles;
	
	void LoadMetaData(PDFDoc *doc, std::string &output);
};

#endif // PdfInfo_h__
