#ifndef vp_file_h__
#define vp_file_h__

#include "ModuleDef.h"

#define VP_HEADER_MAGIC "VPVP"

#pragma pack(push, 1)

struct VP_Header 
{
	char header[4]; //Always "VPVP"
	int version;    //As of this version, still 2.
	int diroffset;  //Offset to the file index
	int direntries; //Number of entries
};

struct VP_DirEntry
{
	int offset;		//Offset of the file data for this entry.
	int size;		//Size of the file data for this entry
	char name[32];	//Null-terminated filename, directory name, or ".." for backdir
	int timestamp;	//Time the file was last modified, in Unix time.
};

#pragma pack(pop)

// This is VP_DirEntry with some values converted
// Used for local storage
struct VP_FileRec
{
	int offset;
	int size;
	wchar_t full_path[MAX_PATH]; // Full local path
	FILETIME timestamp;

	bool IsDir() { return (offset == -1) || (size == 0); }
};

class CVPFile
{
private:
	HANDLE m_hFile;
	vector<VP_FileRec> m_vContent;

public:
	CVPFile();
	~CVPFile();

	bool Open(const wchar_t* path);
	void Close();

	int ExtractSingleFile(int index, const wchar_t* destPath, const ExtractProcessCallbacks* epc);

	size_t ItemCount() { return m_vContent.size(); }
	bool GetItem(int index, VP_FileRec &frec);
};

const wchar_t* GetFileName(const wchar_t* fullPath);

#endif // vp_file_h__