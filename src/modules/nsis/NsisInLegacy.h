#ifndef NsisInLegacy_h__
#define NsisInLegacy_h__

namespace NArchive {
namespace NNsis {

UInt32 ResolveLegacyOpcode(UInt32 modernOpcode);
AString GetNsisLegacyString(const AString &s);

}}

#endif // NsisInLegacy_h__