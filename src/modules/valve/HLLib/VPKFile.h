/*
 * HLLib
 * Copyright (C) 2006-2010 Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#ifndef VPKFILE_H
#define VPKFILE_H

#include "Package.h"

namespace HLLib
{
	class HLLIB_API CVPKFile : public CPackage
	{
	private:
		#pragma pack(1)

		struct VPKHeader
		{
			hlUInt uiSignature;			// Always 0x55aa1234.
			hlUInt uiVersion;
			hlUInt uiDirectoryLength;
		};

		struct VPKDirectoryEntry
		{
			hlUInt uiCRC;
			hlUShort uiPreloadBytes;
			hlUShort uiArchiveIndex;
			hlUInt uiEntryOffset;
			hlUInt uiEntryLength;
			hlUShort uiDummy0;			// Always 0xffff.
		};

		#pragma pack()

		struct VPKArchive
		{
			Streams::IStream *pStream;
			Mapping::CMapping *pMapping;
		};

		struct VPKDirectoryItem
		{
			VPKDirectoryItem(const hlChar *lpExtention, const hlChar *lpPath, const hlChar *lpName, const VPKDirectoryEntry *pDirectoryEntry, const hlVoid *lpPreloadData) : lpExtention(lpExtention), lpPath(lpPath), lpName(lpName), pDirectoryEntry(pDirectoryEntry), lpPreloadData(lpPreloadData)
			{
			}

			const hlChar *lpExtention;
			const hlChar *lpPath;
			const hlChar *lpName;
			const VPKDirectoryEntry *pDirectoryEntry;
			const hlVoid *lpPreloadData;
		};

		typedef std::list<VPKDirectoryItem *> CDirectoryItemList;

	private:
		static const char *lpAttributeNames[];
		static const char *lpItemAttributeNames[];

		Mapping::CView *pView;

		hlUInt uiArchiveCount;
		VPKArchive *lpArchives;

		const VPKHeader *pHeader;
		CDirectoryItemList *pDirectoryItems;

	public:
		CVPKFile();
		virtual ~CVPKFile();

		virtual HLPackageType GetType() const;
		virtual const hlChar *GetExtension() const;
		virtual const hlWChar *GetDescription() const;

	protected:
		virtual hlBool MapDataStructures();
		virtual hlVoid UnmapDataStructures();

		virtual CDirectoryFolder *CreateRoot();

		virtual hlUInt GetAttributeCountInternal() const;
		virtual const hlChar *GetAttributeNameInternal(HLPackageAttribute eAttribute) const;
		virtual hlBool GetAttributeInternal(HLPackageAttribute eAttribute, HLAttribute &Attribute) const;

		virtual hlUInt GetItemAttributeCountInternal() const;
		virtual const hlChar *GetItemAttributeNameInternal(HLPackageAttribute eAttribute) const;
		virtual hlBool GetItemAttributeInternal(const CDirectoryItem *pItem, HLPackageAttribute eAttribute, HLAttribute &Attribute) const;

		virtual hlBool GetFileExtractableInternal(const CDirectoryFile *pFile, hlBool &bExtractable) const;
		virtual hlBool GetFileValidationInternal(const CDirectoryFile *pFile, HLValidation &eValidation) const;
		virtual hlBool GetFileSizeInternal(const CDirectoryFile *pFile, hlUInt &uiSize) const;
		virtual hlBool GetFileSizeOnDiskInternal(const CDirectoryFile *pFile, hlUInt &uiSize) const;

		virtual hlBool CreateStreamInternal(const CDirectoryFile *pFile, Streams::IStream *&pStream) const;
		virtual hlVoid ReleaseStreamInternal(Streams::IStream &Stream) const;

	private:
		hlBool MapString(const hlChar *&lpViewData, const hlChar *lpViewDirectoryDataEnd, const hlChar *&lpString);
	};
}

#endif
