#ifndef Unpacker_h__
#define Unpacker_h__

bool inflateData(HANDLE inFile, HANDLE outFile, unsigned int &packedSize, unsigned int &unpackedSize, unsigned long &crc);

#endif // Unpacker_h__