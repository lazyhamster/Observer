/*
 * HLLib
 * Copyright (C) 2006-2010 Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#ifndef MEMORYSTREAM_H
#define MEMORYSTREAM_H

#include "Stream.h"

namespace HLLib
{
	namespace Streams
	{
		class HLLIB_API CMemoryStream : public IStream
		{
		private:
			hlBool bOpened;
			hlUInt uiMode;

			hlVoid *lpData;
			hlULongLong uiBufferSize;

			hlULongLong uiPointer;
			hlULongLong uiLength;

		public:
			CMemoryStream(hlVoid *lpData, hlULongLong uiBufferSize);
			~CMemoryStream();

			virtual HLStreamType GetType() const;

			const hlVoid *GetBuffer() const;
			hlULongLong GetBufferSize() const;
			virtual const hlWChar *GetFileName() const;

			virtual hlBool GetOpened() const;
			virtual hlUInt GetMode() const;

			virtual hlBool Open(hlUInt uiMode);
			virtual hlVoid Close();

			virtual hlULongLong GetStreamSize() const;
			virtual hlULongLong GetStreamPointer() const;

			virtual hlULongLong Seek(hlLongLong iOffset, HLSeekMode eSeekMode);

			virtual hlBool Read(hlChar &cChar);
			virtual hlUInt Read(hlVoid *lpData, hlUInt uiBytes);

			virtual hlBool Write(hlChar iChar);
			virtual hlUInt Write(const hlVoid *lpData, hlUInt uiBytes);
		};
	}
}

#endif
