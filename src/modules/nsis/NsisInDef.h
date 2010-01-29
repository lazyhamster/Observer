#ifndef NsisInDef_h__
#define NsisInDef_h__

#define SYSREGKEY "Software\\Microsoft\\Windows\\CurrentVersion"

/*
#  define CSIDL_PROGRAMS 0x2
#  define CSIDL_PRINTERS 0x4
#  define CSIDL_PERSONAL 0x5
#  define CSIDL_FAVORITES 0x6
#  define CSIDL_STARTUP 0x7
#  define CSIDL_RECENT 0x8
#  define CSIDL_SENDTO 0x9
#  define CSIDL_STARTMENU 0xB
#  define CSIDL_MYMUSIC 0xD
#  define CSIDL_MYVIDEO 0xE

#  define CSIDL_DESKTOPDIRECTORY 0x10
#  define CSIDL_NETHOOD 0x13
#  define CSIDL_FONTS 0x14
#  define CSIDL_TEMPLATES 0x15
#  define CSIDL_COMMON_STARTMENU 0x16
#  define CSIDL_COMMON_PROGRAMS 0x17
#  define CSIDL_COMMON_STARTUP 0x18
#  define CSIDL_COMMON_DESKTOPDIRECTORY 0x19
#  define CSIDL_APPDATA 0x1A
#  define CSIDL_PRINTHOOD 0x1B
#  define CSIDL_LOCAL_APPDATA 0x1C
#  define CSIDL_ALTSTARTUP 0x1D
#  define CSIDL_COMMON_ALTSTARTUP 0x1E
#  define CSIDL_COMMON_FAVORITES 0x1F

#  define CSIDL_INTERNET_CACHE 0x20
#  define CSIDL_COOKIES 0x21
#  define CSIDL_HISTORY 0x22
#  define CSIDL_COMMON_APPDATA 0x23
#  define CSIDL_WINDOWS 0x24
#  define CSIDL_SYSTEM 0x25
#  define CSIDL_PROGRAM_FILES 0x26
#  define CSIDL_MYPICTURES 0x27
#  define CSIDL_PROFILE 0x28
#  define CSIDL_PROGRAM_FILES_COMMON 0x2B
#  define CSIDL_COMMON_TEMPLATES 0x2D
#  define CSIDL_COMMON_DOCUMENTS 0x2E
#  define CSIDL_COMMON_ADMINTOOLS 0x2F

#  define CSIDL_ADMINTOOLS 0x30
#  define CSIDL_COMMON_MUSIC 0x35
#  define CSIDL_COMMON_PICTURES 0x36
#  define CSIDL_COMMON_VIDEO 0x37
#  define CSIDL_RESOURCES 0x38
#  define CSIDL_RESOURCES_LOCALIZED 0x39
#  define CSIDL_CDBURN_AREA 0x3B
*/

struct CCommandPair
{
  int NumParams;
  const char *Name;
};

enum
{
  // 0
  EW_INVALID_OPCODE,    // zero is invalid. useful for catching errors. (otherwise an all zeroes instruction
                        // does nothing, which is easily ignored but means something is wrong.
  EW_RET,               // return from function call
  EW_NOP,               // Nop/Jump, do nothing: 1, [?new address+1:advance one]
  EW_ABORT,             // Abort: 1 [status]
  EW_QUIT,              // Quit: 0
  EW_CALL,              // Call: 1 [new address+1]
  EW_UPDATETEXT,        // Update status text: 2 [update str, ui_st_updateflag=?ui_st_updateflag:this]
  EW_SLEEP,             // Sleep: 1 [sleep time in milliseconds]
  EW_BRINGTOFRONT,      // BringToFront: 0
  EW_CHDETAILSVIEW,     // SetDetailsView: 2 [listaction,buttonaction]
  
  // 10
  EW_SETFILEATTRIBUTES, // SetFileAttributes: 2 [filename, attributes]
  EW_CREATEDIR,         // Create directory: 2, [path, ?update$INSTDIR]
  EW_IFFILEEXISTS,      // IfFileExists: 3, [file name, jump amount if exists, jump amount if not exists]
  EW_SETFLAG,           // Sets a flag: 2 [id, data]
  EW_IFFLAG,            // If a flag: 4 [on, off, id, new value mask]
  EW_GETFLAG,           // Gets a flag: 2 [output, id]
  EW_RENAME,            // Rename: 3 [old, new, rebootok]
  EW_GETFULLPATHNAME,   // GetFullPathName: 2 [output, input, ?lfn:sfn]
  EW_SEARCHPATH,        // SearchPath: 2 [output, filename]
  EW_GETTEMPFILENAME,   // GetTempFileName: 2 [output, base_dir]
  
  // 20
  EW_EXTRACTFILE,       // File to extract: 6 [overwriteflag, output filename, compressed filedata, filedatetimelow, filedatetimehigh, allow ignore]
                        //  overwriteflag: 0x1 = no. 0x0=force, 0x2=try, 0x3=if date is newer
  EW_DELETEFILE,        // Delete File: 2, [filename, rebootok]
  EW_MESSAGEBOX,        // MessageBox: 5,[MB_flags,text,retv1:retv2,moveonretv1:moveonretv2]
  EW_RMDIR,             // RMDir: 2 [path, recursiveflag]
  EW_STRLEN,            // StrLen: 2 [output, input]
  EW_ASSIGNVAR,         // Assign: 4 [variable (0-9) to assign, string to assign, maxlen, startpos]
  EW_STRCMP,            // StrCmp: 5 [str1, str2, jump_if_equal, jump_if_not_equal, case-sensitive?]
  EW_READENVSTR,        // ReadEnvStr/ExpandEnvStrings: 3 [output, string_with_env_variables, IsRead]
  EW_INTCMP,            // IntCmp: 6 [val1, val2, equal, val1<val2, val1>val2, unsigned?]
  EW_INTOP,             // IntOp: 4 [output, input1, input2, op] where op: 0=add, 1=sub, 2=mul, 3=div, 4=bor, 5=band, 6=bxor, 7=bnot input1, 8=lnot input1, 9=lor, 10=land], 11=1%2
  
  // 30
  EW_INTFMT,            // IntFmt: [output, format, input]
  EW_PUSHPOP,           // Push/Pop/Exchange: 3 [variable/string, ?pop:push, ?exch]
  EW_FINDWINDOW,        // FindWindow: 5, [outputvar, window class,window name, window_parent, window_after]
  EW_SENDMESSAGE,       // SendMessage: 6 [output, hwnd, msg, wparam, lparam, [wparamstring?1:0 | lparamstring?2:0 | timeout<<2]
  EW_ISWINDOW,          // IsWindow: 3 [hwnd, jump_if_window, jump_if_notwindow]
  EW_GETDLGITEM,        // GetDlgItem:        3: [outputvar, dialog, item_id]
  EW_SETCTLCOLORS,      // SerCtlColors:      3: [hwnd, pointer to struct colors]
  EW_SETBRANDINGIMAGE,  // SetBrandingImage:  1: [Bitmap file]
  EW_CREATEFONT,        // CreateFont:        5: [handle output, face name, height, weight, flags]
  EW_SHOWWINDOW,        // ShowWindow:        2: [hwnd, show state]
  
  // 40
  EW_SHELLEXEC,         // ShellExecute program: 4, [shell action, complete commandline, parameters, showwindow]
  EW_EXECUTE,           // Execute program: 3,[complete command line,waitflag,>=0?output errorcode]
  EW_GETFILETIME,       // GetFileTime; 3 [file highout lowout]
  EW_GETDLLVERSION,     // GetDLLVersion: 3 [file highout lowout]
  EW_REGISTERDLL,       // Register DLL: 3,[DLL file name, string ptr of function to call, text to put in display (<0 if none/pass parms), 1 - no unload, 0 - unload]
  EW_CREATESHORTCUT,    // Make Shortcut: 5, [link file, target file, parameters, icon file, iconindex|show mode<<8|hotkey<<16]
  EW_COPYFILES,         // CopyFiles: 3 [source mask, destination location, flags]
  EW_REBOOT,            // Reboot: 0
  EW_WRITEINI,          // Write INI String: 4, [Section, Name, Value, INI File]
  EW_READINISTR,        // ReadINIStr: 4 [output, section, name, ini_file]
  
  // 50
  EW_DELREG,            // DeleteRegValue/DeleteRegKey: 4, [root key(int), KeyName, ValueName, delkeyonlyifempty]. ValueName is -1 if delete key
  EW_WRITEREG,          // Write Registry value: 5, [RootKey(int),KeyName,ItemName,ItemData,typelen]
                        //  typelen=1 for str, 2 for dword, 3 for binary, 0 for expanded str
  EW_READREGSTR,        // ReadRegStr: 5 [output, rootkey(int), keyname, itemname, ==1?int::str]
  EW_REGENUM,           // RegEnum: 5 [output, rootkey, keyname, index, ?key:value]
  EW_FCLOSE,            // FileClose: 1 [handle]
  EW_FOPEN,             // FileOpen: 4  [name, openmode, createmode, outputhandle]
  EW_FPUTS,             // FileWrite: 3 [handle, string, ?int:string]
  EW_FGETS,             // FileRead: 4  [handle, output, maxlen, ?getchar:gets]
  EW_FSEEK,             // FileSeek: 4  [handle, offset, mode, >=0?positionoutput]
  EW_FINDCLOSE,         // FindClose: 1 [handle]
  
  // 60
  EW_FINDNEXT,          // FindNext: 2  [output, handle]
  EW_FINDFIRST,         // FindFirst: 2 [filespec, output, handleoutput]
  EW_WRITEUNINSTALLER,  // WriteUninstaller: 3 [name, offset, icon_size]
  EW_LOG,               // LogText: 2 [0, text] / LogSet: [1, logstate]
  EW_SECTIONSET,        // SectionSetText:    3: [idx, 0, text]
                        // SectionGetText:    3: [idx, 1, output]
                        // SectionSetFlags:   3: [idx, 2, flags]
                        // SectionGetFlags:   3: [idx, 3, output]
  EW_INSTTYPESET,       // InstTypeSetFlags:  3: [idx, 0, flags]
                        // InstTypeGetFlags:  3: [idx, 1, output]
  // instructions not actually implemented in exehead, but used in compiler.
  EW_GETLABELADDR,      // both of these get converted to EW_ASSIGNVAR
  EW_GETFUNCTIONADDR,

  EW_LOCKWINDOW
};

#ifdef NSIS_SCRIPT
static CCommandPair kCommandPairs[] =
{
  { 0, "Invalid" },
  { 0, "Return" },
  { 1, "Goto" },
  { 0, "Abort" },
  { 0, "Quit" },
  { 1, "Call" },
  { 2, "UpdateSatusText" },
  { 1, "Sleep" },
  { 0, "BringToFront" },
  { 2, "SetDetailsView" },

  { 2, "SetFileAttributes" },
  { 2, "SetOutPath" },
  { 3, "IfFileExists" },
  { 2, "SetFlag" },
  { 4, "IfFlag" },
  { 2, "GetFlag" },
  { 3, "Rename" },
  { 2, "GetFullPathName" },
  { 2, "SearchPath" },
  { 2, "GetTempFileName" },

  { 6, "File" },
  { 2, "Delete" },
  { 5, "MessageBox" },
  { 2, "RMDir" },
  { 2, "StrLen" },
  { 4, "StrCpy" },
  { 5, "StrCmp" },
  { 3, "ReadEnvStr" },
  { 6, "IntCmp" },
  { 4, "IntOp" },

  { 3, "IntFmt" },
  { 3, "PushPop" },
  { 5, "FindWindow" },
  { 6, "SendMessage" },
  { 3, "IsWindow" },
  { 3, "GetDlgItem" },
  { 3, "SerCtlColors" },
  { 1, "SetBrandingImage" },
  { 5, "CreateFont" },
  { 2, "ShowWindow" },

  { 4, "ShellExecute" },
  { 3, "Execute" },
  { 3, "GetFileTime" },
  { 3, "GetDLLVersion" },
  { 3, "RegisterDLL" },
  { 5, "CreateShortCut" },
  { 3, "CopyFiles" },
  { 0, "Reboot" },
  { 4, "WriteINIStr" },
  { 4, "ReadINIStr" },

  { 4, "DelReg" },
  { 5, "WriteReg" },
  { 5, "ReadRegStr" },
  { 5, "RegEnum" },
  { 1, "FileClose" },
  { 4, "FileOpen" },
  { 3, "FileWrite" },
  { 4, "FileRead" },
  { 4, "FileSeek" },
  { 1, "FindClose" },

  { 2, "FindNext" },
  { 2, "FindFirst" },
  { 3, "WriteUninstaller" },
  { 2, "LogText" },
  { 3, "Section?etText" },
  { 3, "InstType?etFlags" },
  { 6, "GetLabelAddr" },
  { 2, "GetFunctionAddress" },
  { 6, "LockWindow" }
};

#endif

static const char *kShellStrings[] =
{
  "",
  "",

  "SMPROGRAMS",
  "",
  "PRINTERS",
  "DOCUMENTS",
  "FAVORITES",
  "SMSTARTUP",
  "RECENT",
  "SENDTO",
  "",
  "STARTMENU",
  "",
  "MUSIC",
  "VIDEO",
  "",

  "DESKTOP",
  "",
  "",
  "NETHOOD",
  "FONTS",
  "TEMPLATES",
  "COMMONSTARTMENU",
  "COMMONFILES",
  "COMMON_STARTUP",
  "COMMON_DESKTOPDIRECTORY",
  "QUICKLAUNCH",
  "PRINTHOOD",
  "LOCALAPPDATA",
  "ALTSTARTUP",
  "ALTSTARTUP",
  "FAVORITES",

  "INTERNET_CACHE",
  "COOKIES",
  "HISTORY",
  "APPDATA",
  "WINDIR",
  "SYSDIR",
  "PROGRAMFILES",
  "PICTURES",
  "PROFILE",
  "",
  "",
  "COMMONFILES",
  "",
  "TEMPLATES",
  "DOCUMENTS",
  "ADMINTOOLS",

  "ADMINTOOLS",
  "",
  "",
  "",
  "",
  "MUSIC",
  "PICTURES",
  "VIDEO",
  "RESOURCES",
  "RESOURCES_LOCALIZED",
  "",
  "CDBURN_AREA"
};

static const int kNumShellStrings = sizeof(kShellStrings) / sizeof(kShellStrings[0]);

#endif // NsisInDef_h__