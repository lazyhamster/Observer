/*
 * iszsdk.cpp -- source code of the ISZ SDK
 * Copyright (C) 2006 EZB Systems, Inc.
 */
#include "stdafx.h"
#include "zlib\zlib.h"
#include "bzip2\bzlib.h"

#include "isz.h"
//#include "aes.h"
#include "iszsdk.h"

#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif

typedef struct isz_pointer_st {
    unsigned char flag;
    __int64 blk_offs;
    unsigned int blk_len;
} isz_pointer;

struct isz_segment {
  wchar_t filespec[MAX_PATH];
  int first_chkno;               // first whole chunk number in current file
  int last_chkno;                // last whole chunk number in current file
  int num_chks;                  // number of whole chunks in current file
  int chk_off;                   // offset to first chunk in current file
  int left_size;                 // incomplete chunk bytes in next file
  __int64 size;
};

typedef struct _isz_reader {
    wchar_t filespec[MAX_PATH];
    HANDLE hFile;
    isz_header isz;
    unsigned char *inbuf;
    unsigned char *outbuf;
    unsigned char *keybuf;
    isz_pointer *pointer_block;
    unsigned int blk_no;
    unsigned int blk_sectors;
    struct isz_segment segment[ISZ_MAX_SEGMENT];
    int cur_seg;
    int num_seg;
    unsigned char key[ISZ_KEY_MAX+1];
    int key_len;
//    unsigned char aes[AES_CTX_SIZE];
} isz_reader;

#define CHECK_ZERO_BLOCKS

#define ZIP_OK   0

void bz_internal_error ( int errcode )
{
}

static void isz_get_ptr(unsigned char *pnt, unsigned int *len, unsigned char *compressed)
{
   *len = pnt[0] + (pnt[1] << 8) + ((pnt[2] & 0x3f) << 16);
   *compressed = pnt[2] & 0xc0;
}

static int isz_decode(unsigned char *buffer, unsigned int size)
{
   unsigned int i,k;
   char *pKey = ISZ_FLAG;

   k = 0;
   for(i = 0; i < size; ++i)
   {
      buffer[i] ^= (unsigned char)(~pKey[k]);
      ++k;
      if(k == ISZ_FLAG_LEN)
         k = 0;
   }

   return 1;
}

/*
static int isz_decrypt(isz_reader *r, unsigned char *buffer, unsigned int size)
{
   unsigned int n,i;

   CopyMemory(r->keybuf,buffer,size);
   n = size / AES_BLOCK_SIZE;
   i = 0;
   while(n > 0)
   {
      aes_decrypt(&r->aes,buffer+i,r->keybuf+i);
      --n;
      i += AES_BLOCK_SIZE;
   }

   return 1;
}
*/

static int h_fseek(HANDLE hFile, __int64 offs, DWORD moveto)
{
    LARGE_INTEGER lipos;
    DWORD dwPtrLow;

    lipos.QuadPart = offs;

    dwPtrLow = SetFilePointer(hFile,lipos.LowPart,&lipos.HighPart, moveto);

    if((dwPtrLow == INVALID_SET_FILE_POINTER) && GetLastError() != NO_ERROR)
    {
        return 0;
    }
    return 1;
}

static unsigned int h_fread(void *buffer, int count, int size,HANDLE hFile)
{
    unsigned long bytecount,byteread;

    bytecount = count * size;

    ReadFile(hFile,buffer,bytecount,&byteread,NULL);
    if(byteread != bytecount)
    {
       return byteread/count;
    }

    return size;
}

static int isz_open_segment(isz_reader *r)
{
    if(r->hFile != INVALID_HANDLE_VALUE)  // Close previous opened image
    {
       CloseHandle(r->hFile);
       r->hFile = INVALID_HANDLE_VALUE;
    }

    r->hFile = CreateFile(r->segment[r->cur_seg].filespec,
               GENERIC_READ, FILE_SHARE_READ,
               NULL, OPEN_EXISTING, 0, NULL);

    if(r->hFile == INVALID_HANDLE_VALUE)
    {
       
       //Ask for segment file here

       return -1;
    }

    return 0;
}

static int isz_load_segment(isz_reader *r, int blk_no)
{
   int i;

   for(i = 0; i < r->num_seg; ++i)
   {
      if(blk_no >= r->segment[i].first_chkno &&
         blk_no <= r->segment[i].last_chkno)
         break;
   }
   
   if(i < r->num_seg)
   {
      r->cur_seg = i;
      
      if(isz_open_segment(r))
         return 0;

      return 1;
   }
   
   return 0;
}

static int isz_next_segment(isz_reader *r)
{
   ++r->cur_seg;
   if(r->cur_seg < r->num_seg)
   {
      if(isz_open_segment(r))
         return 0;

      return 1;
   }

   //--r->cur_seg;
   return 0;
}

static int isz_read_chunk(isz_reader *r,unsigned char *buffer, int blk_no)
{
   unsigned int len = r->pointer_block[blk_no].blk_len;
   LARGE_INTEGER lipos;
   DWORD dwPtrLow;
   unsigned long byteread;
   __int64 hsecno;
   unsigned int n,nleft;

   if(r->num_seg == 0)   // Single segment
   {
      lipos.QuadPart = r->pointer_block[blk_no].blk_offs;

      dwPtrLow = SetFilePointer(r->hFile,lipos.LowPart,&lipos.HighPart, FILE_BEGIN);

      if((dwPtrLow == INVALID_SET_FILE_POINTER) && GetLastError() != NO_ERROR)
      {
         return -1;
      }

      ReadFile(r->hFile,buffer,len,&byteread,NULL);

      if(byteread != len)
      {
         return -1;
      }

      if(r->isz.has_password)
         return -1;//isz_decrypt(r,buffer,len);

      return 0;
   }

   if(blk_no < r->segment[r->cur_seg].first_chkno ||
      blk_no > r->segment[r->cur_seg].last_chkno)
   {
      if(!isz_load_segment(r, blk_no))
         return -1;
   }

   hsecno = r->pointer_block[blk_no].blk_offs - r->pointer_block[r->segment[r->cur_seg].first_chkno].blk_offs;
   lipos.QuadPart = hsecno + r->segment[r->cur_seg].chk_off;

   dwPtrLow = SetFilePointer(r->hFile,lipos.LowPart,&lipos.HighPart, FILE_BEGIN);

   if((dwPtrLow == INVALID_SET_FILE_POINTER) && GetLastError() != NO_ERROR)
   {
      return -1;
   }

   if(blk_no == r->segment[r->cur_seg].last_chkno)  // is sector
   {
      nleft = r->segment[r->cur_seg].left_size;
      n = len - nleft;
      ReadFile(r->hFile,buffer,n,&byteread,NULL);

      if(byteread != n)
      {
         return -1;
      }

      if(nleft > 0)
      {
         if(!isz_next_segment(r))
            return -1;

         lipos.QuadPart = r->segment[r->cur_seg].chk_off - nleft;

         dwPtrLow = SetFilePointer(r->hFile,lipos.LowPart,&lipos.HighPart, FILE_BEGIN);

         if((dwPtrLow == INVALID_SET_FILE_POINTER) && GetLastError() != NO_ERROR)
         {
            return -1;
         }

         ReadFile(r->hFile,buffer+n,nleft,&byteread,NULL);
         if(byteread != nleft)
         {
            return -1;
         }
      }
   }
   else
   {
      ReadFile(r->hFile,buffer,len,&byteread,NULL);
      if(byteread != len)
      {
         return -1;
      }
   }

   if(r->isz.has_password)
      return -1;//isz_decrypt(r,buffer,len);

   return 0;
}

static int isz_read_sector(isz_reader *r,char *buf, unsigned int sector_no)
{
    unsigned int blk_no;
    unsigned int len;
    int zerr;
    unsigned int bytes;
    int sector_offs;
    int iRet;

    if(sector_no >= r->isz.total_sectors)
    {
       memset(buf, 0, r->isz.sect_size);
       return -1;
    }

    blk_no = sector_no / r->blk_sectors;
    sector_offs = sector_no - blk_no * r->blk_sectors;
    if(blk_no != r->blk_no)
    {
       len = r->pointer_block[blk_no].blk_len;
       bytes = r->isz.block_size;

       iRet = 0;
       switch(r->pointer_block[blk_no].flag)
       {
          case ISZ_ZERO:
            memset(r->outbuf, 0, len);
            bytes = len;
            iRet = 0;
            zerr = ZIP_OK;
            break;

          case ISZ_DATA:
            iRet = isz_read_chunk(r,r->outbuf,blk_no);
            bytes = len;
            zerr = ZIP_OK;
            break;

          case ISZ_ZLIB:
            iRet = isz_read_chunk(r,r->inbuf,blk_no);
            zerr = uncompress(r->outbuf, (uLong *)&bytes, r->inbuf, len);
            break;

          case ISZ_BZ2:
            iRet = isz_read_chunk(r,r->inbuf,blk_no);
            zerr = BZ2_bzBuffToBuffDecompress((char *)r->outbuf, &bytes, (char *)r->inbuf, len,0,0);
            break;

          default:   // Unknown type
            return -1;
       }

       if(iRet || zerr != ZIP_OK )
          return -1;

       r->blk_no = blk_no;
    }

    memcpy(buf,r->outbuf+sector_offs*r->isz.sect_size,r->isz.sect_size);

    return 0;
}

HANDLE isz_open(HANDLE filePtr, const wchar_t *filespec)
{
    void *isz_r;
    isz_reader *r;
    unsigned char *isz_ptr,*ptr;
    unsigned int cb_header_size;
    unsigned int i;
    __int64 blk_offs;
    unsigned int ptr_size;
    unsigned char cf;
    unsigned int len;

    if (filespec == NULL || filePtr == INVALID_HANDLE_VALUE)
       return INVALID_HANDLE_VALUE;

    isz_r = (void *)malloc(sizeof(isz_reader));
    if(isz_r == NULL)                   // Cannot allocate memory
       return INVALID_HANDLE_VALUE;

    r = (isz_reader *)isz_r;
    memset(r,0,sizeof(isz_reader));
    wcscpy(r->filespec,filespec); 
    r->hFile = INVALID_HANDLE_VALUE;

    // Initialize CB_HEADER_SIZE
    cb_header_size = sizeof(isz_header);

    h_fseek(filePtr,0,FILE_BEGIN);
    if(h_fread(&r->isz,1,cb_header_size,filePtr) != cb_header_size)
    {
       isz_close(r);
       return INVALID_HANDLE_VALUE;
    }

    if(memcmp(r->isz.signature,ISZ_FLAG,ISZ_FLAG_LEN) != 0 ||
       r->isz.ver > ISZ_VER ||
       (r->isz.block_size % r->isz.sect_size))
    {
       isz_close(r);
       return INVALID_HANDLE_VALUE;
    }

    cb_header_size = r->isz.header_size;
    
    r->blk_sectors = r->isz.block_size / r->isz.sect_size;

    r->inbuf = (unsigned char *)malloc(r->isz.block_size);
    if(r->inbuf == NULL)
    {
       isz_close(r);
       return INVALID_HANDLE_VALUE;
    }

    r->outbuf = (unsigned char *)malloc(r->isz.block_size);
    if(r->outbuf == NULL)
    {
       isz_close(r);
       return INVALID_HANDLE_VALUE;
    }

    r->pointer_block = (isz_pointer *)malloc(r->isz.nblocks * sizeof(isz_pointer));
    if(r->pointer_block == NULL)
    {
       isz_close(r);
       return INVALID_HANDLE_VALUE;
    }

    if(r->isz.ptr_offs)   // Has pointer, read it
    {
       ptr_size = r->isz.nblocks * r->isz.ptr_len;
       isz_ptr = (unsigned char *)malloc(ptr_size);
       if(isz_ptr == NULL)
       {
          isz_close(r);
          return INVALID_HANDLE_VALUE;
       }

       h_fseek(filePtr,r->isz.ptr_offs,FILE_BEGIN);
       if(h_fread(isz_ptr,1,ptr_size,filePtr) != ptr_size)
       {
          isz_close(r);
          return INVALID_HANDLE_VALUE;
       }

       isz_decode(isz_ptr,ptr_size);
       
       ptr = isz_ptr;
       blk_offs = r->isz.data_offs;
       for(i = 0; i < r->isz.nblocks; ++i)
       {
          isz_get_ptr(ptr,&len,&cf);
          r->pointer_block[i].blk_offs = blk_offs;
          r->pointer_block[i].flag = cf;
          r->pointer_block[i].blk_len = len;

          if(cf != ISZ_ZERO)
             blk_offs += len;

          ptr += r->isz.ptr_len;
       }

       free(isz_ptr);

       if(r->isz.seg_offs)   // Has segment, read it
       {
          isz_seg *s;
          unsigned int n_seg;

          ptr_size = r->isz.data_offs - r->isz.seg_offs;
          n_seg = ptr_size / sizeof(isz_seg);
          
          isz_ptr = (unsigned char *)malloc(ptr_size);
          if(isz_ptr == NULL)
          {
             isz_close(r);
             return INVALID_HANDLE_VALUE;
          }

          h_fseek(filePtr,r->isz.seg_offs,FILE_BEGIN);
          if(h_fread(isz_ptr,1,ptr_size,filePtr) != ptr_size)
          {
             isz_close(r);
             return INVALID_HANDLE_VALUE;
          }

          isz_decode(isz_ptr,ptr_size);

          s = (isz_seg *)isz_ptr;
          for(i = 0; i < n_seg; ++i)
          {
             if(s[i].num_chks == 0)
                break;

             r->segment[i].first_chkno = s[i].first_chkno;
             r->segment[i].num_chks = s[i].num_chks;
             r->segment[i].last_chkno = s[i].first_chkno + s[i].num_chks - 1;
             r->segment[i].chk_off = s[i].chk_off;
             r->segment[i].left_size = s[i].left_size;
             r->segment[i].size = s[i].size;
          }

          r->num_seg = i;

          free(isz_ptr);
       }
       else
       {
          r->num_seg = 0;
       }
    }
    else  // No pointer, no compress
    {
       __int64 out_size;

       blk_offs = r->isz.data_offs;
       len = r->isz.block_size;
       for(i = 0; i < r->isz.nblocks; ++i)
       {
          r->pointer_block[i].blk_offs = blk_offs;
          r->pointer_block[i].flag = ISZ_DATA;
          r->pointer_block[i].blk_len = len;

          blk_offs += len;
       }

       // Last block
       r->pointer_block[i-1].blk_len = (r->isz.total_sectors - (r->blk_sectors * (r->isz.nblocks - 1))) * r->isz.sect_size;

       out_size = ((__int64)r->isz.total_sectors) * r->isz.sect_size;
       if(out_size + cb_header_size > r->isz.segment_size)
       {
          int chkno,n,nleft;
          __int64 dl;

          i = 0;
          chkno = 0;
          nleft = 0;
          while(out_size > 0)
          {
             if(out_size  + nleft + cb_header_size > r->isz.segment_size)
             {
                dl = r->isz.segment_size - (nleft + cb_header_size);
             }
             else
             {
                dl = out_size;
             }

             n = (int) ((dl + r->isz.block_size - 1) / r->isz.block_size);

             out_size -= n * r->isz.block_size;

             r->segment[i].size = dl + nleft + cb_header_size;

             r->segment[i].first_chkno = chkno;
             r->segment[i].num_chks = n;
             chkno += n;
             r->segment[i].last_chkno = chkno - 1;
             r->segment[i].chk_off = nleft + cb_header_size;

             if(out_size > 0)
             {
                nleft = (int)(n * r->isz.block_size - dl);
             }
             else
                nleft = 0;

             r->segment[i].left_size = nleft;

             ++i;
          }

          r->num_seg = i;
       }
       else
          r->num_seg = 0;
    }

    if(r->num_seg > 0)
    {
       wchar_t sFile[MAX_PATH];

       wcscpy(sFile,r->filespec);
       wchar_t* p = wcsrchr(sFile,'.');
       ++p;    // extension 
       *p = '\0';

       wcscpy(r->segment[0].filespec,r->filespec);  // first segment

       for(i = 1; i < (unsigned int)r->num_seg; ++i)
       {
          swprintf(r->segment[i].filespec, MAX_PATH, L"%si%02d",sFile,i);
       }
    }

    r->hFile = filePtr;
    r->cur_seg = 0;
    r->blk_no = -1;

    r->keybuf = (unsigned char *)malloc(r->isz.block_size);

    return isz_r;
}

/*
int isz_setpassword(HANDLE h_isz, char *isz_key)
{
   isz_reader *r = (isz_reader *)h_isz;

   if(isz_key == NULL)
   {
      return r->isz.has_password;
   }

   if(r->isz.has_password)
   {
      strncpy((char *)r->key,isz_key,ISZ_KEY_MAX);

      r->key_len = strlen((char *)r->key);

      if(r->isz.has_password != ISZ_PASSWORD)
      {
         unsigned char aes_key[32+1];
         int keylen;

         switch(r->isz.has_password)
         {
            case ISZ_AES256:
               keylen = 32;
               break;

            case ISZ_AES192:
               keylen = 24;
               break;

            //case ISZ_AES128:
            default:
               keylen = 16;
               break;
         }

         memset(aes_key,0,keylen);
         strncpy((char *)aes_key,(char *)r->key,keylen);
         aes_set_key(&r->aes,aes_key,keylen);
      }

      return 0;
   }

   return -1;
}
*/

unsigned int isz_get_capacity(HANDLE h_isz, unsigned int *sect_size)
{
   isz_reader *r = (isz_reader *)h_isz;

   *sect_size = r->isz.sect_size; 
   return r->isz.total_sectors;
}

unsigned int isz_read_secs(HANDLE h_isz,void *buffer, unsigned int startsecno, unsigned int sectorcount)
{
    isz_reader *r = (isz_reader *)h_isz;
    unsigned int i;
    char *buf = (char *)buffer;

    for(i = 0; i < sectorcount; i++)
    {
		if (isz_read_sector(r,buf,startsecno+i))
			return 0;
        buf += r->isz.sect_size ;
    }
    return sectorcount * r->isz.sect_size;
}

void isz_close(HANDLE h_isz)
{
    isz_reader *r = (isz_reader *)h_isz;

    if(h_isz == INVALID_HANDLE_VALUE)
       return;

    if(r->hFile != INVALID_HANDLE_VALUE)
    {
       CloseHandle(r->hFile);
       r->hFile = INVALID_HANDLE_VALUE;
    }

    if(r->keybuf)
    {
       free(r->keybuf);
       r->keybuf = NULL;
    }

    if(r->inbuf)
    {
       free(r->inbuf);
       r->inbuf = NULL;
    }

    if(r->outbuf)
    {
       free(r->outbuf);
       r->outbuf = NULL;
    }

    if(r->pointer_block)
    {
       free(r->pointer_block);
       r->pointer_block = NULL;
    }

    free(r);
}


