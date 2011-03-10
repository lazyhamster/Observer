/* 
 * iszsdk.h -- interface of the ISZ SDK
 * Copyright (C) 2006 EZB Systems, Inc.
 */

#ifndef __ISZ_SDK__
#define __ISZ_SDK__

/*
 * open ISZ file
 * return isz_r pointer if successfully
 * return INVALID_HANDLE_VALUE for error
 */
extern HANDLE isz_open(HANDLE filePtr, const wchar_t *filespec);

/*
 * Set password
 * set isz_key = NULL to inquiry password status as
   ISZ_PLAIN       0
   ISZ_PASSWORD    1
   ISZ_AES128      2
   ISZ_AES192      3
   ISZ_AES256      4
 */
extern int isz_setpassword(HANDLE h_isz, const char *isz_key);

/*
 * Get capacity
 * Returns total_sectors and set value of sect_size
 */
extern unsigned int isz_get_capacity(HANDLE h_isz, unsigned int *sect_size);

/*
 * Read sectors
 * Returns 0 for error
 */
extern unsigned int isz_read_secs(HANDLE h_isz,void *buffer, unsigned int startsecno, unsigned int sectorcount);

/*
 * Close file
 */
extern void isz_close(HANDLE h_isz);

bool isz_needpassword(HANDLE h_isz);

#endif
