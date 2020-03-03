#ifndef NameDecode_h__
#define NameDecode_h__

//std::wstring GetEntityName(mimetic::MimeEntity* entity);
//void AppendDigit(std::wstring &fileName, int num);

void GetEntityName(GMimePart* entity, wchar_t* dest, size_t destSize, GMimeParserOptions* parserOpts);

#endif // NameDecode_h__