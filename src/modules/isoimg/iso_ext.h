#ifndef iso_ext_h__
#define iso_ext_h__

int ExtractFile(IsoImage *image, Directory *dir, const wchar_t *destPath, const ExtractProcessCallbacks* epc);

FILETIME VolumeDateTimeToFileTime(VolumeDateTime &vdtime);

DWORD GetDirectoryAttributes(Directory* dir);
bool IsDirectory(Directory *dir);
DWORD GetSize(Directory *dir);

FILETIME StringToTime(const char* data);

template <typename T>
void TrimRight(T* buf, size_t bufSize)
{
	int i = (int) bufSize - 1;
	while ((i >= 0) && (buf[i] == 32))
	{
		buf[i] = 0;
		i--;
	}
}

#endif // iso_ext_h__