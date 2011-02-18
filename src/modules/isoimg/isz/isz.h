/* 
 * isz.h -- header file of the ISZ format
 * Copyright (C) 2006 EZB Systems, Inc.
 */

#ifndef __ISZ_H__
#define __ISZ_H__

#define ISZ_VER         0x01

/* Compressed ISO file magic */
#define ISZ_FLAG        "IsZ!"
#define ISZ_FLAG_LEN    4

/* Encyption definations */
#define ISZ_PLAIN       0
#define ISZ_PASSWORD    1
#define ISZ_AES128      2
#define ISZ_AES192      3
#define ISZ_AES256      4

#define ISZ_KEY_MAX     32

/* chunk data definations */
#define ISZ_ZERO        0x00    // all zeros
#define ISZ_DATA        0x40    // non-compressed data
#define ISZ_ZLIB        0x80    // ZLIB compressed
#define ISZ_BZ2         0xC0    // BZIP2 compressed

#define ISZ_MAX_SEGMENT 100

#pragma pack(push,1)

/* VERY VERY VERY IMPORTANT: Must be a multiple of 4 bytes */
typedef struct isz_file_header {
  char signature[4];
  unsigned char header_size;     // header size in bytes
  char ver;
  unsigned int vsn;              // Volume serial number

  unsigned short sect_size;      // sector size in bytes
  unsigned int total_sectors;    // total sectors of ISO
  char has_password;             // is Password protected?
  __int64 segment_size;          // size of segments

  unsigned int nblocks;          // number of chunks in image
  unsigned int block_size;       // chunk size in bytes, must be multiple of sector_size
  unsigned char ptr_len;         // 3 bytes

  char seg_no;                   // segment number of this segment file, max 99

  unsigned int ptr_offs;         // offset of chunk pointers, zero = none

  unsigned int seg_offs;         // offset of chunk pointers, zero = none

  unsigned int data_offs;        // data offset

} isz_header;

typedef struct isz_seg_st {
  __int64 size;
  int num_chks;                  // number of whole chunks in current file
  int first_chkno;               // first whole chunk number in current file
  int chk_off;                   // offset to first chunk in current file
  int left_size;                 // uncompltete chunk bytes in next file
} isz_seg;

#pragma pack(pop)

#endif