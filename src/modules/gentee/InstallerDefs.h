#ifndef InstallerDefs_h__
#define InstallerDefs_h__

#define SIG_TEXT "Gentee Launcher"

#pragma pack(push, 1)

struct lahead_data
{
	uint32_t  exesize;
	uint32_t  minsize;
	uint8_t   exeext;
	uint8_t   pack;
	uint16_t  flags;
	uint32_t  dllsize;
	uint32_t  gesize;
	int64_t  extsize[8];
};

struct lahead_v1
{
	char      sign1[16];     // 'Gentee Launcher' sign
	uint32_t  exesize;       // Размер exe-файла. Если не 0, то файл является SFX архивом и далее идут прикрепленныe данные
	uint32_t  minsize;       // Если не 0, то выдавать ошибку если размер файла меньше указанного
	uint8_t   console;       // 1 если консольное приложение
	uint8_t   exeext;        // Количество дополнительных блоков
	uint16_t  flags;         // flags
	uint32_t  dllsize;       // Упакованный размер Dll файла. Если 0, то динамическое подключение gentee.dll
	uint32_t  gesize;        // Упакованный размер байт-кода.
	uint32_t  mutex;         // ID для mutex, если не 0, то будет проверка
	uint32_t  param;         // Зашитый параметр
	uint32_t  offset;        // Смещение данной структуры
	int64_t  extsize[ 8 ]; // Зарезервированно для размеров 8 ext блоков. Каждый размер занимает long
};

struct lahead_v2
{
	char      sign1[16];     // 'Gentee Launcher' sign
	uint32_t  exesize;       // Размер exe-файла. Если не 0, то файл является SFX архивом и далее идут прикрепленныe данные
	uint32_t  minsize;       // Если не 0, то выдавать ошибку если размер файла меньше указанного
	uint8_t   console;       // 1 если консольное приложение
	uint8_t   exeext;        // Количество дополнительных блоков
	uint8_t   pack;          // 1 если байт код и dll упакованы
	uint16_t  flags;         // flags
	uint32_t  dllsize;       // Упакованный размер Dll файла. Если 0, то динамическое подключение gentee.dll
	uint32_t  gesize;        // Упакованный размер байт-кода.
	uint32_t  mutex;         // ID для mutex, если не 0, то будет проверка
	uint32_t  param;         // Зашитый параметр
	uint32_t  offset;        // Смещение данной структуры
	int64_t  extsize[ 8 ]; // Зарезервированно для размеров 8 ext блоков. Каждый размер занимает long
};

#pragma pack(pop)

#endif // InstallerDefs_h__
