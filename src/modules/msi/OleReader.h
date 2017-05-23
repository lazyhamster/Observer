#ifndef OleReader_h__
#define OleReader_h__

class COleStorage
{
public:
	COleStorage();
	~COleStorage();

	bool Open(const wchar_t* storagePath);
	void Close();

	bool ExtractStream(const wchar_t* streamFileName, const wchar_t* destPath);

private:
	void* m_pStoragePtr;
	std::map<std::wstring, std::wstring> m_mStreamNames;

	bool CompoundMsiNameToFileName(const wchar_t *name, std::wstring &res);
};

#endif // OleReader_h__
