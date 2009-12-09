#include "StdAfx.h"

#include "7zip/Common/RegisterCodec.h"
#include "NsisBzipDecoder.h"

static void *CreateCodecBzipNsis() { return (void *)(ICompressCoder *)(new NCompress::NBzip2::NDecoder::CDecoder()); }

static CCodecInfo g_CodecInfo =
{ CreateCodecBzipNsis, 0,   0x040902, L"BzipNSIS", 1, false };

REGISTER_CODEC(BzipNsis)
