#ifndef LOCAL_ERROR_INCLUDED
#define LOCAL_ERROR_INCLUDED

int error();
const char * errormsg(int code);
void error(int code);
void error(const char *pszError, ...);
int fileerror(int err);
void clrerr();
int TranslateGZError(int GZError);

#endif //!defined(LOCAL_ERROR_INCLUDED)