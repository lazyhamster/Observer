#ifndef ISUCabFile_h__
#define ISUCabFile_h__

#include "ISCabFile.h"
#include "IS6/IS6CabFile.h"

namespace ISU
{

class ISUCabFile : public IS6::IS6CabFile
{
private:
	void GenerateInfoFile();
	bool IsVersionCompatible(DWORD version);

public:	
	ISUCabFile();
	~ISUCabFile();

	bool GetFileInfo(int itemIndex, StorageItemInfo* itemInfo) const;
};

} // namespace ISU

#endif // ISUCabFile_h__
