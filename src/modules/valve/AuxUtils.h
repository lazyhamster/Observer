#ifndef AuxUtils_h__
#define AuxUtils_h__

HLPackageType GetPackageType(const wchar_t* filepath);
HLLib::CPackage* CreatePackage(HLPackageType ePackageType);

#endif // AuxUtils_h__