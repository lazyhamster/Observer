/* Build against zlib. */
#define ENABLE_ZLIB 1

/* Use zlib instead of builtin zlib decoder to uncompress flate streams. */
#define ENABLE_ZLIB_UNCOMPRESS

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define if you have the iconv() function and it works. */
#define HAVE_ICONV 1

/* Define to 1 if you have the `z' library (-lz). */
#define HAVE_LIBZ 1

/* Define to 1 if you have the `strcpy_s' function. */
#define HAVE_STRCPY_S 1

/* Define to 1 if you have the `strcat_s' function. */
#define HAVE_STRCAT_S 1

/* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'.
   */
#define HAVE_SYS_DIR_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <codecvt> header file. */
#define HAVE_CODECVT

/* Define as const if the declaration of iconv() needs const. */
#define ICONV_CONST const

/* Name of package */
#define PACKAGE "poppler"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "https://bugs.freedesktop.org/enter_bug.cgi?product=poppler"

/* Define to the full name of this package. */
#define PACKAGE_NAME "poppler"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "poppler 0.75.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "poppler"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.75.0"

/* Poppler data dir */
#define POPPLER_DATADIR "C:/poppler/share/poppler"

/* Enable word list support. */
#define TEXTOUT_WORD_LIST 1

/* Version number of package */
#define VERSION "0.75.0"

/* Use win32 font configuration backend */
#define WITH_FONTCONFIGURATION_WIN32 1

/* MS defined snprintf as deprecated but then added it in Visual Studio 2015. */
#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#endif

//------------------------------------------------------------------------
// popen
//------------------------------------------------------------------------
#if defined(_MSC_VER) || defined(__BORLANDC__)
#define popen _popen
#define pclose _pclose
#endif

/* Number of bits in a file offset, on hosts where this is settable. */
#define _FILE_OFFSET_BITS 64

/* Define to 1 to make fseeko visible on some hosts (e.g. glibc 2.2). */
/* TODO This is wrong, port if needed #undef _LARGEFILE_SOURCE */

/* Define for large files, on AIX-style hosts. */
/* TODO This is wrong, port if needed #undef _LARGE_FILES */
