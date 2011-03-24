// NsisIn.cpp

#include "StdAfx.h"

// #include <stdio.h>

#include "NsisIn.h"
#include "NsisInLegacy.h"
#include "NsisDecode.h"

#include "Windows/Defs.h"

#include "../../Common/StreamUtils.h"

#include "Common/StringConvert.h"
#include "Common/IntToString.h"

#include "../../../../C/CpuArch.h"

#define Get32(p) GetUi32(p)

namespace NArchive {
namespace NNsis {

Byte kSignature[kSignatureSize] = NSIS_SIGNATURE;

#ifdef NSIS_SCRIPT
static const char *kCrLf = "\x0D\x0A";
#endif

#define NS_UN_SKIP_CODE  0xE000
#define NS_UN_VAR_CODE   0xE001
#define NS_UN_SHELL_CODE 0xE002
#define NS_UN_LANG_CODE  0xE003
#define NS_UN_CODES_START NS_UN_SKIP_CODE
#define NS_UN_CODES_END   NS_UN_LANG_CODE

Byte CInArchive::ReadByte()
{
  if (_posInData >= _size)
    throw 1;
  return _data[_posInData++];
}

UInt32 CInArchive::ReadUInt32()
{
  UInt32 value = 0;
  for (int i = 0; i < 4; i++)
    value |= ((UInt32)(ReadByte()) << (8 * i));
  return value;
}

void CInArchive::ReadBlockHeader(CBlockHeader &bh)
{
  bh.Offset = ReadUInt32();
  bh.Num = ReadUInt32();
}

#define RINOZ(x) { int __tt = (x); if (__tt != 0) return __tt; }

static int CompareItems(void *const *p1, void *const *p2, void * /* param */)
{
  const CItem &i1 = **(CItem **)p1;
  const CItem &i2 = **(CItem **)p2;
  RINOZ(MyCompare(i1.Pos, i2.Pos));

  if (i1.Name[0] == L'$' && i2.Name[0] != L'$')
	  return 1;
  else if (i1.Name[0] != L'$' && i2.Name[0] == L'$')
	  return -1;
  else
  {
	RINOZ(i1.Prefix.Compare(i2.Prefix));
	RINOZ(i1.Name.Compare(i2.Name));
  }

  return 0;
}

static AString UIntToString(UInt32 v)
{
  char sz[32];
  ConvertUInt64ToString(v, sz);
  return sz;
}

static AString IntToString(Int32 v)
{
  char sz[32];
  ConvertInt64ToString(v, sz);
  return sz;
}

AString CInArchive::ReadStringA(UInt32 pos) const
{
  AString s;
  if (pos >= _size)
    return IntToString((Int32)pos);
  UInt32 offset = GetOffset() + _stringsPos + pos;
  for (;;)
  {
    if (offset >= _size)
      break; // throw 1;
    char c = _data[offset++];
    if (c == 0)
      break;
    s += c;
  }
  return s;
}

UString CInArchive::ReadStringU(UInt32 pos) const
{
  UString s;
  UInt32 offset = GetOffset() + _stringsPos + (pos * 2);
  for (;;)
  {
    if (offset >= _size || offset + 1 >= _size)
      return s; // throw 1;
    char c0 = _data[offset++];
    char c1 = _data[offset++];
    wchar_t c = (c0 | ((wchar_t)c1 << 8));
    if (c == 0)
      break;
    s += c;
  }
  return s;
}

/*
static AString ParsePrefix(const AString &prefix)
{
  AString res = prefix;
  if (prefix.Length() >= 3)
  {
    if ((Byte)prefix[0] == 0xFD && (Byte)prefix[1] == 0x95 && (Byte)prefix[2] == 0x80)
      res = "$INSTDIR" + prefix.Mid(3);
    else if ((Byte)prefix[0] == 0xFD && (Byte)prefix[1] == 0x96 && (Byte)prefix[2] == 0x80)
      res = "$OUTDIR" + prefix.Mid(3);
  }
  return res;
}
*/

#include "NsisInDef.h"

/*
# define CMDLINE 20 // everything before here doesn't have trailing slash removal
# define INSTDIR 21
# define OUTDIR 22
# define EXEDIR 23
# define LANGUAGE 24
# define TEMP   25
# define PLUGINSDIR 26
# define HWNDPARENT 27
# define _CLICK 28
# define _OUTDIR 29
*/

static const char *kVarStrings[] =
{
  "CMDLINE",
  "INSTDIR",
  "OUTDIR",
  "EXEDIR",
  "LANGUAGE",
  "TEMP",
  "PLUGINSDIR",
  "EXEPATH", // test it
  "EXEFILE", // test it
  "HWNDPARENT",
  "_CLICK",
  "_OUTDIR"
};

static const int kNumVarStrings = sizeof(kVarStrings) / sizeof(kVarStrings[0]);


static AString GetVar(UInt32 index)
{
  AString res = "$";
  if (index < 10)
    res += UIntToString(index);
  else if (index < 20)
  {
    res += "R";
    res += UIntToString(index - 10);
  }
  else if (index < 20 + kNumVarStrings)
    res += kVarStrings[index - 20];
  else
  {
    res += "[";
    res += UIntToString(index);
    res += "]";
  }
  return res;
}

#define NS_SKIP_CODE  252
#define NS_VAR_CODE   253
#define NS_SHELL_CODE 254
#define NS_LANG_CODE  255
#define NS_CODES_START NS_SKIP_CODE

static AString GetShellString(int index)
{
  AString res = "$";
  if (index < kNumShellStrings)
  {
    const char *sz = kShellStrings[index];
    if (sz[0] != 0)
      return res + sz;
  }
  res += "SHELL[";
  res += UIntToString(index);
  res += "]";
  return res;
}

// Based on Dave Laundon's simplified process_string
AString GetNsisString(const AString &s)
{
  AString res;
  for (int i = 0; i < s.Length();)
  {
    unsigned char nVarIdx = s[i++];
    if (nVarIdx > NS_CODES_START && i + 2 <= s.Length())
    {
      int nData = s[i++] & 0x7F;
      unsigned char c1 = s[i++];
      nData |= (((int)(c1 & 0x7F)) << 7);

      if (nVarIdx == NS_SHELL_CODE)
        res += GetShellString(c1);
      else if (nVarIdx == NS_VAR_CODE)
        res += GetVar(nData);
      else if (nVarIdx == NS_LANG_CODE)
        res += "NS_LANG_CODE";
    }
    else if (nVarIdx == NS_SKIP_CODE)
    {
      if (i < s.Length())
        res += s[i++];
    }
    else // Normal char
      res += (char)nVarIdx;
  }
  return res;
}

UString GetNsisString(const UString &s)
{
  UString res;
  for (int i = 0; i < s.Length();)
  {
    wchar_t nVarIdx = s[i++];
    if (nVarIdx > NS_UN_CODES_START && nVarIdx <= NS_UN_CODES_END)
    {
      if (i == s.Length())
        break;
      int nData = s[i++] & 0x7FFF;

      if (nVarIdx == NS_UN_SHELL_CODE)
        res += GetUnicodeString(GetShellString(nData >> 8));
      else if (nVarIdx == NS_UN_VAR_CODE)
        res += GetUnicodeString(GetVar(nData));
      else if (nVarIdx == NS_UN_LANG_CODE)
        res += L"NS_LANG_CODE";
    }
    else if (nVarIdx == NS_UN_SKIP_CODE)
    {
      if (i == s.Length())
        break;
      res += s[i++];
    }
    else // Normal char
      res += (char)nVarIdx;
  }
  return res;
}

AString CInArchive::ReadString2A(UInt32 pos) const
{
	if (IsLegacyVer)
		return GetNsisLegacyString(ReadStringA(pos));
	else
		return GetNsisString(ReadStringA(pos));
}

UString CInArchive::ReadString2U(UInt32 pos) const
{
  return GetNsisString(ReadStringU(pos));
}

AString CInArchive::ReadString2(UInt32 pos) const
{
  if (IsUnicode)
    return UnicodeStringToMultiByte(ReadString2U(pos));
  else
    return ReadString2A(pos);
}

AString CInArchive::ReadString2Qw(UInt32 pos) const
{
  return "\"" + ReadString2(pos) + "\"";
}

#define DEL_DIR 1
#define DEL_RECURSE 2
#define DEL_REBOOT 4
// #define DEL_SIMPLE 8

static const int kNumEntryParams = 6;

struct CEntry
{
  UInt32 Which;
  UInt32 Params[kNumEntryParams];
  AString GetParamsString(int numParams);
  CEntry()
  {
    Which = 0;
    for (UInt32 j = 0; j < kNumEntryParams; j++)
      Params[j] = 0;
  }
};

AString CEntry::GetParamsString(int numParams)
{
  AString s;
  for (int i = 0; i < numParams; i++)
  {
    s += " ";
    UInt32 v = Params[i];
    if (v > 0xFFF00000)
      s += IntToString((Int32)Params[i]);
    else
      s += UIntToString(Params[i]);
  }
  return s;
}

#ifdef NSIS_SCRIPT

static AString GetRegRootID(UInt32 val)
{
  const char *s;
  switch(val)
  {
    case 0:  s = "SHCTX"; break;
    case 0x80000000:  s = "HKCR"; break;
    case 0x80000001:  s = "HKCU"; break;
    case 0x80000002:  s = "HKLM"; break;
    case 0x80000003:  s = "HKU";  break;
    case 0x80000004:  s = "HKPD"; break;
    case 0x80000005:  s = "HKCC"; break;
    case 0x80000006:  s = "HKDD"; break;
    case 0x80000050:  s = "HKPT"; break;
    case 0x80000060:  s = "HKPN"; break;
    default:
      return UIntToString(val); break;
  }
  return s;
}

#endif

class ScriptVarCache
{
private:
	UStringVector m_varNames;
	UStringVector m_varValues;

	int GetVarIndex(const UString& name);

public:
	void SetVar(const UString& name, const UString& value);
	void ResetVar(const UString& name);
	UString ResolvePath(const UString& path);
};

void ScriptVarCache::SetVar(const UString& name, const UString& value)
{
	if (name.Length() == 0 || name[0] != '$') return;
	
	if (name.Length() == 2) return; // Ditch registers
	if ((name == L"$INSTDIR") || (name == value)) return;
	
	int vIndex = GetVarIndex(name);
	if (vIndex >= 0)
	{
		m_varValues[vIndex] = ResolvePath(value);
	}
	else
	{
		UString resolvedValue = ResolvePath(value);
		m_varNames.Add(name);
		m_varValues.Add(resolvedValue);
	}
}

int ScriptVarCache::GetVarIndex(const UString& name)
{
	return m_varNames.Find(name);
}

UString ScriptVarCache::ResolvePath(const UString& path)
{
	if (path.Length() == 0 || path[0] != '$')
		return path;

	// Get var name
	int slashIndex = path.Find('\\');
	UString varName = (slashIndex > 0) ? path.Left(slashIndex) : path;
	int varIndex = GetVarIndex(varName);
	if (varIndex >= 0)
	{
		UString resVar = m_varValues[varIndex];
		if (slashIndex > 0) resVar += path.Mid(slashIndex);

		return ResolvePath(resVar);
	}

	return path;
}

void ScriptVarCache::ResetVar(const UString& name)
{
	int varIndex = GetVarIndex(name);
	if (varIndex >= 0)
	{
		m_varNames.Delete(varIndex);
		m_varValues.Delete(varIndex);
	}
}

HRESULT CInArchive::ReadEntries(const CBlockHeader &bh)
{
	_posInData = bh.Offset + GetOffset();
	UString prefixU;
	ScriptVarCache varCache;
	for (UInt32 i = 0; i < bh.Num; i++)
	{
		CEntry e;
		e.Which = ReadUInt32();
		for (UInt32 j = 0; j < kNumEntryParams; j++)
			e.Params[j] = ReadUInt32();
		if (IsLegacyVer)
			e.Which = ResolveLegacyOpcode(e.Which);
#ifdef NSIS_SCRIPT
		if (e.Which != EW_PUSHPOP && e.Which < sizeof(kCommandPairs) / sizeof(kCommandPairs[0]))
		{
			const CCommandPair &pair = kCommandPairs[e.Which];
			Script += pair.Name;
		}
#endif

		switch (e.Which)
		{
		case EW_CREATEDIR:
			{
				prefixU.Empty();
				if (IsUnicode)
				{
					prefixU = ReadString2U(e.Params[0]);
				}
				else
				{
					AString prefixA = ReadString2A(e.Params[0]);
					prefixU = MultiByteToUnicodeString(prefixA);
				}
				varCache.SetVar(L"$OUTDIR", prefixU);
#ifdef NSIS_SCRIPT
				Script += " ";
				Script += UnicodeStringToMultiByte(prefixU);
#endif
				break;
			}

		case EW_EXTRACTFILE:
			{
				CItem item;
				if (IsUnicode)
				{
					item.Name = ReadString2U(e.Params[1]);
				}
				else
				{
					AString nameA = ReadString2A(e.Params[1]);
					item.Name = MultiByteToUnicodeString(nameA);
				}
				if ( (prefixU.Length() > 0) && (item.Name.Length() > 0) && (item.Name[0] != '$' || item.Name.Find('\\') <= 0) )
				{
					item.Prefix = varCache.ResolvePath(prefixU);
				}

				/* UInt32 overwriteFlag = e.Params[0]; */
				item.Pos = e.Params[2];
				item.MTime.dwLowDateTime = e.Params[3];
				item.MTime.dwHighDateTime = e.Params[4];
				/* UInt32 allowIgnore = e.Params[5]; */
				if (Items.Size() > 0)
				{
					/*
					if (item.Pos == Items.Back().Pos)
					continue;
					*/
				}
				Items.Add(item);
#ifdef NSIS_SCRIPT
				Script += " ";
				Script += UnicodeStringToMultiByte(item.Name);
#endif
				break;
			}


#ifdef NSIS_SCRIPT
		case EW_UPDATETEXT:
			{
				Script += " ";
				Script += ReadString2(e.Params[0]);
				Script += " ";
				Script += UIntToString(e.Params[1]);
				break;
			}
		case EW_SETFILEATTRIBUTES:
			{
				Script += " ";
				Script += ReadString2(e.Params[0]);
				Script += " ";
				Script += UIntToString(e.Params[1]);
				break;
			}
		case EW_IFFILEEXISTS:
			{
				Script += " ";
				Script += ReadString2(e.Params[0]);
				Script += " ";
				Script += UIntToString(e.Params[1]);
				Script += " ";
				Script += UIntToString(e.Params[2]);
				break;
			}
		case EW_RENAME:
			{
				Script += " ";
				Script += ReadString2(e.Params[0]);
				Script += " ";
				Script += ReadString2(e.Params[1]);
				Script += " ";
				Script += UIntToString(e.Params[2]);
				break;
			}
		case EW_GETFULLPATHNAME:
			{
				Script += " ";
				Script += ReadString2(e.Params[0]);
				Script += " ";
				Script += ReadString2(e.Params[1]);
				Script += " ";
				Script += UIntToString(e.Params[2]);
				break;
			}
		case EW_SEARCHPATH:
			{
				Script += " ";
				Script += ReadString2(e.Params[0]);
				Script += " ";
				Script += ReadString2(e.Params[1]);
				break;
			}
		case EW_GETTEMPFILENAME:
			{
				AString s;
				Script += " ";
				Script += ReadString2(e.Params[0]);
				Script += " ";
				Script += ReadString2(e.Params[1]);
				break;
			}

		case EW_DELETEFILE:
			{
				UInt64 flag = e.Params[1];
				if (flag != 0)
				{
					Script += " ";
					if (flag == DEL_REBOOT)
						Script += "/REBOOTOK";
					else
						Script += UIntToString(e.Params[1]);
				}
				Script += " ";
				Script += ReadString2(e.Params[0]);
				break;
			}
		case EW_RMDIR:
			{
				UInt64 flag = e.Params[1];
				if (flag != 0)
				{
					if ((flag & DEL_REBOOT) != 0)
						Script += " /REBOOTOK";
					if ((flag & DEL_RECURSE) != 0)
						Script += " /r";
				}
				Script += " ";
				Script += ReadString2(e.Params[0]);
				break;
			}
		case EW_STRLEN:
			{
				Script += " ";
				Script += GetVar(e.Params[0]);;
				Script += " ";
				Script += ReadString2Qw(e.Params[1]);
				break;
			}
		case EW_ASSIGNVAR:
			{
				AString sVarName = GetVar(e.Params[0]);
				AString sVarValue = ReadString2Qw(e.Params[1]);
				varCache.SetVar(MultiByteToUnicodeString(sVarName), MultiByteToUnicodeString(ReadString2(e.Params[1])));

				Script += " ";
				Script += sVarName;
				Script += " ";
				Script += sVarValue;
				AString maxLen, startOffset;
				if (e.Params[2] != 0)
					maxLen = ReadString2(e.Params[2]);
				if (e.Params[3] != 0)
					startOffset = ReadString2(e.Params[3]);
				if (!maxLen.IsEmpty() || !startOffset.IsEmpty())
				{
					Script += " ";
					if (maxLen.IsEmpty())
						Script += "\"\"";
					else
						Script += maxLen;
					if (!startOffset.IsEmpty())
					{
						Script += " ";
						Script += startOffset;
					}
				}
				break;
			}
		case EW_STRCMP:
			{
				Script += " ";

				Script += " ";
				Script += ReadString2Qw(e.Params[0]);

				Script += " ";
				Script += ReadString2Qw(e.Params[1]);

				for (int j = 2; j < 5; j++)
				{
					Script += " ";
					Script += UIntToString(e.Params[j]);
				}
				break;
			}
		case EW_INTCMP:
			{
				if (e.Params[5] != 0)
					Script += "U";

				Script += " ";
				Script += ReadString2(e.Params[0]);
				Script += " ";
				Script += ReadString2(e.Params[1]);

				for (int i = 2; i < 5; i++)
				{
					Script += " ";
					Script += UIntToString(e.Params[i]);
				}
				break;
			}
		case EW_INTOP:
			{
				Script += " ";
				Script += GetVar(e.Params[0]);
				Script += " ";
				int numOps = 2;
				AString op;
				switch (e.Params[3])
				{
				case 0: op = '+'; break;
				case 1: op = '-'; break;
				case 2: op = '*'; break;
				case 3: op = '/'; break;
				case 4: op = '|'; break;
				case 5: op = '&'; break;
				case 6: op = '^'; break;
				case 7: op = '~'; numOps = 1; break;
				case 8: op = '!'; numOps = 1; break;
				case 9: op = "||"; break;
				case 10: op = "&&"; break;
				case 11: op = '%'; break;
				default: op = UIntToString(e.Params[3]);
				}
				AString p1 = ReadString2(e.Params[1]);
				if (numOps == 1)
				{
					Script += op;
					Script += p1;
				}
				else
				{
					Script += p1;
					Script += " ";
					Script += op;
					Script += " ";
					Script += ReadString2(e.Params[2]);
				}
				break;
			}

		case EW_PUSHPOP:
			{
				int isPop = (e.Params[1] != 0);
				if (isPop)
				{
					Script += "Pop";
					Script += " ";
					Script += GetVar(e.Params[0]);
					
					// Reset variable in cache because we can not predict what will be popped
					varCache.ResetVar(MultiByteToUnicodeString(GetVar(e.Params[0])));
				}
				else
				{
					int isExch = (e.Params[2] != 0);
					if (isExch)
					{
						Script += "Exch";
					}
					else
					{
						Script += "Push";
						Script += " ";
						Script += ReadString2(e.Params[0]);
					}
				}
				break;
			}

		case EW_SENDMESSAGE:
			{
				// SendMessage: 6 [output, hwnd, msg, wparam, lparam, [wparamstring?1:0 | lparamstring?2:0 | timeout<<2]
				Script += " ";
				// Script += ReadString2(e.Params[0]);
				// Script += " ";
				Script += ReadString2(e.Params[1]);
				Script += " ";
				Script += ReadString2(e.Params[2]);

				Script += " ";
				UInt32 spec = e.Params[5];
				// if (spec & 1)
				Script += IntToString(e.Params[3]);
				// else
				//   Script += ReadString2(e.Params[3]);

				Script += " ";
				// if (spec & 2)
				Script += IntToString(e.Params[4]);
				// else
				//   Script += ReadString2(e.Params[4]);

				if ((Int32)e.Params[0] >= 0)
				{
					Script += " ";
					Script += GetVar(e.Params[1]);
				}

				spec >>= 2;
				if (spec != 0)
				{
					Script += " /TIMEOUT=";
					Script += IntToString(spec);
				}
				break;
			}

		case EW_GETDLGITEM:
			{
				Script += " ";
				Script += GetVar(e.Params[0]);;
				Script += " ";
				Script += ReadString2(e.Params[1]);
				Script += " ";
				Script += ReadString2(e.Params[2]);
				break;
			}


		case EW_REGISTERDLL:
			{
				Script += " ";
				Script += ReadString2(e.Params[0]);
				Script += " ";
				Script += ReadString2(e.Params[1]);
				Script += " ";
				Script += UIntToString(e.Params[2]);
				break;
			}

		case EW_CREATESHORTCUT:
			{
				AString s;

				Script += " ";
				Script += ReadString2Qw(e.Params[0]);

				Script += " ";
				Script += ReadString2Qw(e.Params[1]);

				for (int j = 2; j < 5; j++)
				{
					Script += " ";
					Script += UIntToString(e.Params[j]);
				}
				break;
			}

			/*
			case EW_DELREG:
			{
			AString keyName, valueName;
			keyName = ReadString2(e.Params[1]);
			bool isValue = (e.Params[2] != -1);
			if (isValue)
			{
			valueName = ReadString2(e.Params[2]);
			Script += "Key";
			}
			else
			Script += "Value";
			Script += " ";
			Script += UIntToString(e.Params[0]);
			Script += " ";
			Script += keyName;
			if (isValue)
			{
			Script += " ";
			Script += valueName;
			}
			Script += " ";
			Script += UIntToString(e.Params[3]);
			break;
			}
			*/

		case EW_WRITEREG:
			{
				AString s;
				switch(e.Params[4])
				{
				case 1:  s = "Str"; break;
				case 2:  s = "ExpandStr"; break;
				case 3:  s = "Bin"; break;
				case 4:  s = "DWORD"; break;
				default: s = "?" + UIntToString(e.Params[4]); break;
				}
				Script += s;
				Script += " ";
				Script += GetRegRootID(e.Params[0]);
				Script += " ";

				AString keyName, valueName;
				keyName = ReadString2Qw(e.Params[1]);
				Script += keyName;
				Script += " ";

				valueName = ReadString2Qw(e.Params[2]);
				Script += valueName;
				Script += " ";

				valueName = ReadString2Qw(e.Params[3]);
				Script += valueName;
				Script += " ";

				break;
			}

		case EW_WRITEUNINSTALLER:
			{
				Script += " ";
				Script += ReadString2(e.Params[0]);
				for (int j = 1; j < 3; j++)
				{
					Script += " ";
					Script += UIntToString(e.Params[j]);
				}
				break;
			}

		default:
			{
				int numParams = kNumEntryParams;
				if (e.Which < sizeof(kCommandPairs) / sizeof(kCommandPairs[0]))
				{
					const CCommandPair &pair = kCommandPairs[e.Which];
					// Script += pair.Name;
					numParams = pair.NumParams;
				}
				else
				{
					Script += "Unknown";
					Script += UIntToString(e.Which);
				}
				Script += e.GetParamsString(numParams);
			}
#endif
		}
#ifdef NSIS_SCRIPT
		Script += kCrLf;
#endif
	}

	{
		Items.Sort(CompareItems, 0);
		int i;
		// if (IsSolid)
		for (i = 0; i + 1 < Items.Size();)
		{
			const CItem& item1 = Items[i], item2 = Items[i+1];
			if ( (item1.Pos == item2.Pos) && (item1.Name == item2.Name) && (item1.Prefix == item2.Prefix) )
				Items.Delete(i + 1);
			else
				i++;
		}
		for (i = 0; i < Items.Size(); i++)
		{
			CItem &item = Items[i];
			UInt32 curPos = item.Pos + 4;
			for (int nextIndex = i + 1; nextIndex < Items.Size(); nextIndex++)
			{
				UInt32 nextPos = Items[nextIndex].Pos;
				if (curPos <= nextPos)
				{
					item.EstimatedSizeIsDefined = true;
					item.EstimatedSize = nextPos - curPos;
					break;
				}
			}
		}
		if (!IsSolid)
		{
			for (i = 0; i < Items.Size(); i++)
			{
				CItem &item = Items[i];
				RINOK(_stream->Seek(GetPosOfNonSolidItem(i), STREAM_SEEK_SET, NULL));
				const UInt32 kSigSize = 4 + 1 + 5;
				BYTE sig[kSigSize];
				size_t processedSize = kSigSize;
				RINOK(ReadStream(_stream, sig, &processedSize));
				if (processedSize < 4)
					return S_FALSE;
				UInt32 size = Get32(sig);
				if ((size & 0x80000000) != 0)
				{
					item.IsCompressed = true;
					// is compressed;
					size &= ~0x80000000;
					if (Method == NMethodType::kLZMA)
					{
						if (processedSize < 9)
							return S_FALSE;
						if (FilterFlag)
							item.UseFilter = (sig[4] != 0);
						item.DictionarySize = Get32(sig + 5 + (FilterFlag ? 1 : 0));
					}
				}
				else
				{
					item.IsCompressed = false;
					item.Size = size;
					item.SizeIsDefined = true;
				}
				item.CompressedSize = size;
				item.CompressedSizeIsDefined = true;
			}
		}
	}
	return S_OK;
}

HRESULT CInArchive::Parse()
{
  // UInt32 offset = ReadUInt32();
  // ???? offset == FirstHeader.HeaderLength
  /* UInt32 ehFlags = */ ReadUInt32();
  CBlockHeader bhPages, bhSections, bhEntries, bhStrings, bhLangTables, bhCtlColors, bhData;
  // CBlockHeader bgFont;
  ReadBlockHeader(bhPages);
  ReadBlockHeader(bhSections);
  ReadBlockHeader(bhEntries);
  ReadBlockHeader(bhStrings);
  ReadBlockHeader(bhLangTables);
  ReadBlockHeader(bhCtlColors);
  // ReadBlockHeader(bgFont);
  ReadBlockHeader(bhData);

  _stringsPos = bhStrings.Offset;
  UInt32 pos = GetOffset() + _stringsPos;
  int numZeros0 = 0;
  int numZeros1 = 0;
  int i;
  const int kBlockSize = 256;
  for (i = 0; i < kBlockSize; i++)
  {
    if (pos >= _size || pos + 1 >= _size)
      break;
    char c0 = _data[pos++];
    char c1 = _data[pos++];
    wchar_t c = (c0 | ((wchar_t)c1 << 8));

    if (c >= NS_UN_CODES_START && c < NS_UN_CODES_END)
    {
      if (pos >= _size || pos + 1 >= _size)
        break;
      pos += 2;
      numZeros1++;
    }
    else
    {
      if (c0 == 0 && c1 != 0)
        numZeros0++;
      if (c1 == 0)
        numZeros1++;
    }
    // printf("\nnumZeros0 = %2x %2x", _data[pos + 0],  _data[pos + 1]);
  }
  IsUnicode = (numZeros1 > numZeros0 * 3 + kBlockSize / 16);
  // printf("\nnumZeros0 = %3d    numZeros1 = %3d", numZeros0,  numZeros1);
  return ReadEntries(bhEntries);
}

HRESULT CInArchive::ParseLegacy()
{
	const UInt32 kLegacySectionSize = 24;
	const UInt32 kLegacyPageSize = 28;
	const UInt32 kLegacyEntrySize = 28;

	UInt32 language_tables_num = ReadUInt32(); // number of strings tables in array
	UInt32 language_table_size = ReadUInt32(); // size of each language table

	UInt32 num_entries = ReadUInt32(); // total number of entries
	UInt32 num_string_bytes = ReadUInt32(); // total number of bytes taken by strings

	UInt32 num_pages = ReadUInt32(); // number of used pages (including custom pages)

	_posInData += 48; // Skip rest of the common header
	_posInData += 64; // Skip to num_sections field

	UInt32 num_sections = ReadUInt32();

	// Structure:
	// header (~228 bytes)
	// sections (20 bytes each)
	//   pages (12 bytes each)
	//   entries (24 bytes each)
	//   string table
	//   language tables
	
	UInt32 entriesOffset = (UInt32)(_posInData + 12) + num_sections * kLegacySectionSize + num_pages * kLegacyPageSize - 4;

	_stringsPos = entriesOffset + num_entries * kLegacyEntrySize;
	IsUnicode = false;

	CBlockHeader bhEntries;
	bhEntries.Offset = entriesOffset;
	bhEntries.Num = num_entries;
	return ReadEntries(bhEntries);
}

static bool IsLZMA(const Byte *p, UInt32 &dictionary)
{
  dictionary = Get32(p + 1);
  return (p[0] == 0x5D && p[1] == 0x00 && p[2] == 0x00 && p[5] == 0x00);
}

static bool IsLZMA(const Byte *p, UInt32 &dictionary, bool &thereIsFlag)
{
  if (IsLZMA(p, dictionary))
  {
    thereIsFlag = false;
    return true;
  }
  if (IsLZMA(p + 1, dictionary))
  {
    thereIsFlag = true;
    return true;
  }
  return false;
}

static bool IsBZip2(const Byte *p)
{
  return (p[0] == 0x31 && p[1] < 14);
}

HRESULT CInArchive::Open2(
      DECL_EXTERNAL_CODECS_LOC_VARS2
      )
{
  RINOK(_stream->Seek(0, STREAM_SEEK_CUR, &StreamOffset));

  const UInt32 kSigSize = 4 + 1 + 5 + 1; // size, flag, lzma props, lzma first byte
  BYTE sig[kSigSize];
  RINOK(ReadStream_FALSE(_stream, sig, kSigSize));
  UInt64 position;
  RINOK(_stream->Seek(StreamOffset, STREAM_SEEK_SET, &position));

  _headerIsCompressed = true;
  IsSolid = true;
  FilterFlag = false;
  DictionarySize = 1;
  IsLegacyVer = false;

  UInt32 compressedHeaderSize = Get32(sig);
  
  if (compressedHeaderSize == FirstHeader.HeaderLength)
  {
    _headerIsCompressed = false;
    IsSolid = false;
    Method = NMethodType::kCopy;
  }
  else if (IsLZMA(sig, DictionarySize, FilterFlag))
  {
    Method = NMethodType::kLZMA;
  }
  else if (IsLZMA(sig + 4, DictionarySize, FilterFlag))
  {
    IsSolid = false;
    Method = NMethodType::kLZMA;
  }
  else if (sig[3] == 0x80)
  {
	  IsSolid = false;
    if (IsBZip2(sig + 4))
	  Method = NMethodType::kBZip2;
    else
	  Method = NMethodType::kDeflate;
  }
  else if (IsBZip2(sig))
  {
	  Method = NMethodType::kBZip2;
	  IsLegacyVer = true;
  }
  else
  {
    Method = NMethodType::kDeflate;
  }

  _posInData = 0;
  if (!IsSolid)
  {
    _headerIsCompressed = ((compressedHeaderSize & 0x80000000) != 0);
    if (_headerIsCompressed)
      compressedHeaderSize &= ~0x80000000;
    _nonSolidStartOffset = compressedHeaderSize;
    RINOK(_stream->Seek(StreamOffset + 4, STREAM_SEEK_SET, NULL));
  }
  UInt32 unpackSize = FirstHeader.HeaderLength;
  if (_headerIsCompressed)
  {
    // unpackSize = (1 << 23);
    _data.SetCapacity(unpackSize);
    RINOK(Decoder.Init(
        EXTERNAL_CODECS_LOC_VARS
        _stream, Method, FilterFlag, UseFilter));
    size_t processedSize = unpackSize;
    RINOK(Decoder.Read(_data, &processedSize));
    if (processedSize != unpackSize)
      return S_FALSE;
    _size = processedSize;
    if (IsSolid)
    {
      UInt32 size2 = ReadUInt32();
      if (size2 < _size)
        _size = size2;
    }
  }
  else
  {
    _data.SetCapacity(unpackSize);
    _size = (size_t)unpackSize;
    RINOK(ReadStream_FALSE(_stream, (Byte *)_data, unpackSize));
  }

  // Ugly hack to support 2.0 b4
  // (it has header structure of 2.0+ but lacks leading info byte as 2.0b3-)
  if (IsLegacyVer)
  {
	UInt32 next_num = ReadUInt32();
	_posInData -= sizeof(UInt32);
	if (next_num > 50) IsLegacyVer = false;
  }
  // end of ugly hack
  
  return IsLegacyVer ? ParseLegacy() : Parse();
}

/*
NsisExe =
{
  ExeStub
  Archive  // must start from 512 * N
  #ifndef NSIS_CONFIG_CRC_ANAL
  {
    Some additional data
  }
}

Archive
{
  FirstHeader
  Data
  #ifdef NSIS_CONFIG_CRC_SUPPORT && FirstHeader.ThereIsCrc()
  {
    CRC
  }
}

FirstHeader
{
  UInt32 Flags;
  Byte Signature[16];
  // points to the header+sections+entries+stringtable in the datablock
  UInt32 HeaderLength;
  UInt32 ArchiveSize;
}
*/

HRESULT CInArchive::Open(
    DECL_EXTERNAL_CODECS_LOC_VARS
    IInStream *inStream, const UInt64 *maxCheckStartPosition)
{
  Clear();
  RINOK(inStream->Seek(0, STREAM_SEEK_SET, NULL));
  UInt64 maxSize = ((maxCheckStartPosition != 0) ? *maxCheckStartPosition : 0);
  const UInt32 kStep = 512;
  Byte buffer[kStep];
  
  UInt64 position = 0;
  for (; position <= maxSize; position += kStep)
  {
    RINOK(ReadStream_FALSE(inStream, buffer, kStep));
    if (memcmp(buffer + 4, kSignature, kSignatureSize) == 0)
      break;
  }
  if (position > maxSize)
    return S_FALSE;
  const UInt32 kStartHeaderSize = 4 * 7;
  RINOK(inStream->Seek(0, STREAM_SEEK_END, &_archiveSize));
  RINOK(inStream->Seek(position + kStartHeaderSize, STREAM_SEEK_SET, 0));
  FirstHeader.Flags = Get32(buffer);
  FirstHeader.HeaderLength = Get32(buffer + kSignatureSize + 4);
  FirstHeader.ArchiveSize = Get32(buffer + kSignatureSize + 8);
  if (_archiveSize - position < FirstHeader.ArchiveSize)
    return S_FALSE;

  try
  {
    _stream = inStream;
    HRESULT res = Open2(EXTERNAL_CODECS_LOC_VARS2);
    if (res != S_OK)
      Clear();
    _stream.Release();
    return res;
  }
  catch(...) { Clear(); return S_FALSE; }
}

void CInArchive::Clear()
{
  #ifdef NSIS_SCRIPT
  Script.Empty();
  #endif
  Items.Clear();
  _stream.Release();
}

}}
