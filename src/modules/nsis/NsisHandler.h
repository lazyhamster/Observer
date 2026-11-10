// NSisHandler.h

#ifndef __NSIS_HANDLER_H
#define __NSIS_HANDLER_H

#include "../../../Common/MyCom.h"

#include "../../Common/CreateCoder.h"

#include "../IArchive.h"

#include "NsisIn.h"

namespace NArchive {
namespace NNsis {

class CHandler:
  public IInArchive,
  public CMyUnknownImp
{
  CInArchive _archive;
  AString _methodString;

  /* Observer extracts solid archives one item per Extract() call, in order.
     Decoder state is kept between calls, so each call continues from the
     previous position instead of unpacking the stream again from the start. */
  bool _solidDecoderReady;
  CByteBuffer _tempBuf;
  CByteBuffer _tempBuf2;
  UInt64 _tempPos;

  void ResetSolidState()
  {
    _solidDecoderReady = false;
    _tempPos = (UInt64)(Int64)-1;
    _tempBuf.Free();
    _tempBuf2.Free();
  }

  bool GetUncompressedSize(unsigned index, UInt32 &size) const;
  bool GetCompressedSize(unsigned index, UInt32 &size) const;

  // AString GetMethod(NMethodType::EEnum method, bool useItemFilter, UInt32 dictionary) const;
public:
  CHandler() { ResetSolidState(); }

  MY_UNKNOWN_IMP1(IInArchive)

  INTERFACE_IInArchive(;)
};

}}

#endif
