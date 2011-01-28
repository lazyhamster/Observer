//! \file
//! \brief Disk data structure definitions
//! \author Terry Mahaffey
//!
//! This file contains the data structure definitions as well as utility
//! functions described in the MS-PST documentation.
//!
//! \ingroup disk

#ifndef PSTSDK_DISK_DISK_H
#define PSTSDK_DISK_DISK_H

#include <cstddef>

#include "pstsdk/util/primitives.h"

//! \brief Contains the definition of all in memory representations of disk structures
namespace pstsdk
{
//! \brief Contains the definition of all structures as persisted to disk
namespace disk
{

//! \brief The combination of the id and physical location of a block or page
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \sa [MS-PST] 2.2.2.4
//! \ingroup disk
template<typename T>
struct block_reference
{
    typedef T block_id_disk;
    typedef T location;

    block_id_disk bid; //!< The id of the referenced object
    location ib;       //!< The location on disk (index byte) of the referenced object
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(block_reference<ulong>) == 8, "block_reference<ulong> incorrect size");
static_assert(sizeof(block_reference<ulonglong>) == 16, "block_reference<ulonglong> incorrect size");
//! \endcond

//
// header
//

//! \defgroup disk_headerrelated Header Structures
//! \ingroup disk

//! \brief The number of entries in the header's fmap structure
//! \sa [MS-PST] 2.2.2.6/rgbFM
//! \ingroup disk_headerrelated
const size_t header_fmap_entries = 128;

//! \brief The number of entries in the header's fpmap structure
//! \sa [MS-PST] 2.2.2.6/rgbFP
//! \ingroup disk_headerrelated
const size_t header_fpmap_size = 128;

//! \brief The number of entries in the header's lock structure
//! \sa [MS-PST] 2.2.2.6/rgbReserved3
//! \ingroup disk_headerrelated
const size_t header_lock_entries = 32;

//! \brief Valid database format values (ANSI vs. Unicode)
//! \sa [MS-PST] 2.2.2.6/wVer
//! \ingroup disk_headerrelated
enum database_format
{
    database_format_ansi_min = 14,      //!< Initial ANSI file version number
    database_format_ansi = 15,          //!< Current ANSI file version number
    database_format_unicode_min = 20,   //!< Initial unicode version number
    database_format_unicode = 23,       //!< Current unicode version number
};

//! \brief Vaild database types (OST vs. PST)
//! \sa [MS-PST] 2.2.2.6/wVerClient
//! \ingroup disk_headerrelated
enum database_type
{
    database_ost = 12, //!< A OST file
    database_pst = 19, //!< A PST file
};

//! \brief PST Magic number
//! \ingroup disk_headerrelated
const ushort pst_magic = 0x4D53;
//! \brief OST Magic number
//! \ingroup disk_headerrelated
const ushort ost_magic = 0x4F53;

//! \brief Valid "encryption" methods
//!
//! This value indicates what method was used to "encrypt" external data
//! (external data means the data section of external blocks) in the file.
//! \ingroup disk_headerrelated
enum crypt_method
{
    crypt_method_none = 0,    //!< No "encryption" was used.
    crypt_method_permute = 1, //!< The \ref permute method is used in this file.
    crypt_method_cyclic = 2,  //!< The \ref cyclic method is used in this file.
};

//! \brief The root of the database
//!
//! The root structures describes where the root pages of the NBT and BBT are
//! located, the EOF location, how much space is free, the allocation state 
//! flag, and more.
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \sa [MS-PST] 2.2.2.5
//! \ingroup disk_headerrelated
template<typename T>
struct root
{
    typedef T location;
    typedef T count;

    ulong cOrphans;             //!< The number of "orphans" in the BBT
    location ibFileEof;         //!< EOF of the file, according the header
    location ibAMapLast;        //!< The location of the last valid AMap page
    count cbAMapFree;           //!< Amount of space free in all AMap pages
    count cbPMapFree;           //!< Amount of space free in all PMap pages
    block_reference<T> brefNBT; //!< The location of the root of the NBT
    block_reference<T> brefBBT; //!< The location of the root of the BBT
    byte fAMapValid;            //!< Indicates if the AMap pages are valid or not
    byte bARVec;                //!< Indicates which AddRef vector is used
    ushort cARVec;              //!< Number of elements in the AddRef vector
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(root<ulong>) == 40, "root<ulong> incorrect size");
static_assert(sizeof(root<ulonglong>) == 80, "root<ulonglong> incorrect size");
static_assert(offsetof(root<ulonglong>, cOrphans) == 0, "root<ulonglong> cOrphans at incorrect offset");
static_assert(offsetof(root<ulonglong>, ibFileEof) == 8, "root<ulonglong> ibFileEof at incorrect offset");
static_assert(offsetof(root<ulonglong>, ibAMapLast) == 16, "root<ulonglong> ibAMapLast at incorrect offset");
static_assert(offsetof(root<ulonglong>, cbAMapFree) == 24, "root<ulonglong> cbAMapFree at incorrect offset");
static_assert(offsetof(root<ulonglong>, cbPMapFree) == 32, "root<ulonglong> cbPMapFree at incorrect offset");
static_assert(offsetof(root<ulonglong>, brefNBT) == 40, "root<ulonglong> brefNBT at incorrect offset");
static_assert(offsetof(root<ulonglong>, brefBBT) == 56, "root<ulonglong> brefBBT at incorrect offset");
static_assert(offsetof(root<ulonglong>, fAMapValid) == 72, "root<ulonglong> fAMapValid at incorrect offset");
static_assert(offsetof(root<ulonglong>, bARVec) == 73, "root<ulonglong> bARVec at incorrect offset");
static_assert(offsetof(root<ulonglong>, cARVec) == 74, "root<ulonglong> cARVec at incorrect offset");
//! \endcond

//! \brief High/Low magic number
//!
//! At one point there was also a Low/High magic number, but it was never used.
//! \sa [MS-PST] 2.2.2.6/dwMagic
//! \ingroup disk_headerrelated
const uint hlmagic = 0x4e444221;

//! \cond empty
template<typename T>
struct header
{
} PSTSDK_MS_STRUCT;
//! \endcond

//! \brief The Unicode header structure
//!
//! Located at offset zero, all parsing of a PST file begins with reading the header.
//! Most important among the header fields is the \ref root structure, which tells you
//! the location of the NBT and BBT root pages.
//! \sa [MS-PST] 2.2.2.6
//! \ingroup disk_headerrelated
template<>
struct header<ulonglong>
{
    typedef ulonglong block_id_disk;   //!< The id type used for blocks and pages in the file
    typedef ulonglong location;        //!< The location type used in the file
    typedef ulonglong count;           //!< The count type used in the file

    ulong dwMagic;                     //!< Always \ref hlmagic
    ulong dwCRCPartial;
    ushort wMagicClient;               //!< Client magic number, eg \ref pst_magic
    ushort wVer;                       //!< Version of the file, \ref database_format
    ushort wVerClient;                 //!< Client version, \ref database_type
    byte bPlatformCreate;              //!< Always 0x1
    byte bPlatformAccess;              //!< Always 0x1
    ulong dwOpenDBID;                  //!< Implementation specific
    ulong dwOpenClaimID;               //!< Implementation specific
    block_id_disk bidUnused;           //!< Unused
    block_id_disk bidNextP;            //!< The page id counter
    ulong dwUnique;
    node_id rgnid[nid_type_max];       //!< Array of node_id counters, one per node type
    root<ulonglong> root_info;         //!< The root info for this database
    byte rgbFM[header_fmap_entries];   //!< \deprecated The header's FMap entries
    byte rgbFP[header_fpmap_size];     //!< \deprecated The header's FPMap entries
    byte bSentinel;                    //!< \deprecated Sentinel byte indicating the end of the headers FPMap
    byte bCryptMethod;                 //!< The \ref crypt_method used in this file
    byte rgbReserved[2];               //!< Unused
#ifdef __GNUC__
    // GCC refuses to pack this next to rgbReserved
    byte bidNextB[8];
#else
#pragma pack(4)
    block_id_disk bidNextB;            //!< The block id counter
#pragma pack()
#endif
    ulong dwCRCFull;
    byte rgbVersionEncoded[3];
    byte bLockSemaphore;               //!< Implementation specific
    byte rgbLock[header_lock_entries]; //!< Implementation specific
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(header<ulonglong>) == 568, "header<ulonglong> incorrect size");
static_assert(offsetof(header<ulonglong>, rgnid) == 44, "header<ulonglong> rgnid at incorrect offset");
static_assert(offsetof(header<ulonglong>, root_info) == 176, "header<ulonglong> root_info at incorrect offset");
static_assert(offsetof(header<ulonglong>, rgbFM) == 256, "header<ulonglong> rgbFM at incorrect offset");
static_assert(offsetof(header<ulonglong>, dwCRCFull) == 524, "header<ulonglong> dwCRCFull at incorrect offset");
//! \endcond

//! \brief The ANSI header structure
//!
//! See the documentation for Unicode header. Note that some fields
//! in the ANSI header are in a different order (most notably \a bidNextB).
//! \sa [MS-PST] 2.2.2.6
//! \ingroup disk_headerrelated
template<>
struct header<ulong>
{
    typedef ulong block_id_disk;
    typedef ulong location;
    typedef ulong count;

    ulong dwMagic;
    ulong dwCRCPartial;
    ushort wMagicClient;
    ushort wVer;
    ushort wVerClient;
    byte bPlatformCreate;
    byte bPlatformAccess;
    ulong dwOpenDBID;
    ulong dwOpenClaimID;
#ifdef __GNUC__
    byte bidNextB[4];
#else
    block_id_disk bidNextB;
#endif
    block_id_disk bidNextP;
    ulong dwUnique;
    node_id rgnid[nid_type_max];
    root<ulong> root_info;
    byte rgbFM[header_fmap_entries];
    byte rgbFP[header_fpmap_size];
    byte bSentinel;
    byte bCryptMethod;
    byte rgbReserved[2];
    ulonglong ullReserved;
    ulong dwReserved;
    byte rgbVersionEncoded[3];
    byte bLockSemaphore;
    byte rgbLock[header_lock_entries];
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(header<ulong>) == 512, "header<ulong> incorrect size");
static_assert(offsetof(header<ulong>, rgnid) == 36, "header<ulong> rgnid at incorrect offset");
static_assert(offsetof(header<ulong>, rgbFM) == 204, "header<ulong> rgbFM at incorrect offset");
//! \endcond

//! \cond empty
template<typename T>
struct header_crc_locations
{
} PSTSDK_MS_STRUCT;
//! \endcond

//! \brief The byte offsets used to calculate the CRCs in an ANSI PST
//! \sa [MS-PST] 2.2.2.6/dwCRCPartial
//! \ingroup disk_headerrelated
template<>
struct header_crc_locations<ulong>
{
    static const size_t start = offsetof(header<ulong>, wMagicClient);
    static const size_t end = offsetof(header<ulong>, bLockSemaphore);
    static const size_t length = end - start;
} PSTSDK_MS_STRUCT;

//! \brief The byte offsets used to calculate the CRCs in a Unicode PST file
//! \sa [MS-PST] 2.2.2.6/dwCRCPartial
//! \sa [MS-PST] 2.2.2.6/dwCRCFull
//! \ingroup disk_headerrelated
template<>
struct header_crc_locations<ulonglong>
{
    static const size_t partial_start = header_crc_locations<ulong>::start;
    static const size_t partial_end = header_crc_locations<ulong>::end;
    static const size_t partial_length = header_crc_locations<ulong>::length;
    static const size_t full_start = offsetof(header<ulonglong>, wMagicClient);
    static const size_t full_end = offsetof(header<ulonglong>, dwCRCFull);
    static const size_t full_length = full_end - full_start;
} PSTSDK_MS_STRUCT;

//
// utility functions
//

//! \defgroup utilityrelated Utility Functions
//! \ingroup disk

//! \brief Precalulated CRC table, used by \ref compute_crc
//! \ingroup utilityrelated
const ulong crc_table[] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
    0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
    0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
    0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
    0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
    0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
    0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
    0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
    0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
    0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
    0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
    0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
    0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

//! \brief Data table1 used by \ref permute and \ref cyclic
//! \sa [MS-PST] 5.1/mpbbR
//! \ingroup utilityrelated
const byte table1[] =
{
      65,  54,  19,  98, 168,  33, 110, 187,
     244,  22, 204,   4, 127, 100, 232,  93,
      30, 242, 203,  42, 116, 197,  94,  53,
     210, 149,  71, 158, 150,  45, 154, 136,
      76, 125, 132,  63, 219, 172,  49, 182,
      72,  95, 246, 196, 216,  57, 139, 231,
      35,  59,  56, 142, 200, 193, 223,  37,
     177,  32, 165,  70,  96,  78, 156, 251,
     170, 211,  86,  81,  69, 124,  85,   0,
       7, 201,  43, 157, 133, 155,   9, 160,
     143, 173, 179,  15,  99, 171, 137,  75,
     215, 167,  21,  90, 113, 102,  66, 191,
      38,  74, 107, 152, 250, 234, 119,  83,
     178, 112,   5,  44, 253,  89,  58, 134,
     126, 206,   6, 235, 130, 120,  87, 199,
     141,  67, 175, 180,  28, 212,  91, 205,
     226, 233,  39,  79, 195,   8, 114, 128,
     207, 176, 239, 245,  40, 109, 190,  48,
      77,  52, 146, 213,  14,  60,  34,  50,
     229, 228, 249, 159, 194, 209,  10, 129,
      18, 225, 238, 145, 131, 118, 227, 151,
     230,  97, 138,  23, 121, 164, 183, 220,
     144, 122,  92, 140,   2, 166, 202, 105,
     222,  80,  26,  17, 147, 185,  82, 135,
      88, 252, 237,  29,  55,  73,  27, 106,
     224,  41,  51, 153, 189, 108, 217, 148,
     243,  64,  84, 111, 240, 198, 115, 184,
     214,  62, 101,  24,  68,  31, 221, 103,
      16, 241,  12,  25, 236, 174,   3, 161,
      20, 123, 169,  11, 255, 248, 163, 192,
     162,   1, 247,  46, 188,  36, 104, 117,
      13, 254, 186,  47, 181, 208, 218,  61,
};

//! \brief Data table2 used by \ref permute and \ref cyclic
//! \sa [MS-PST] 5.1/mpbbS
//! \ingroup utilityrelated
const byte table2[] =
{
      20,  83,  15,  86, 179, 200, 122, 156,
     235, 101,  72,  23,  22,  21, 159,   2,
     204,  84, 124, 131,   0,  13,  12,  11,
     162,  98, 168, 118, 219, 217, 237, 199,
     197, 164, 220, 172, 133, 116, 214, 208,
     167, 155, 174, 154, 150, 113, 102, 195,
      99, 153, 184, 221, 115, 146, 142, 132,
     125, 165,  94, 209,  93, 147, 177,  87,
      81,  80, 128, 137,  82, 148,  79,  78,
      10, 107, 188, 141, 127, 110,  71,  70,
      65,  64,  68,   1,  17, 203,   3,  63,
     247, 244, 225, 169, 143,  60,  58, 249,
     251, 240,  25,  48, 130,   9,  46, 201,
     157, 160, 134,  73, 238, 111,  77, 109,
     196,  45, 129,  52,  37, 135,  27, 136,
     170, 252,   6, 161,  18,  56, 253,  76,
      66, 114, 100,  19,  55,  36, 106, 117,
     119,  67, 255, 230, 180,  75,  54,  92,
     228, 216,  53,  61,  69, 185,  44, 236,
     183,  49,  43,  41,   7, 104, 163,  14,
     105, 123,  24, 158,  33,  57, 190,  40,
      26,  91, 120, 245,  35, 202,  42, 176,
     175,  62, 254,   4, 140, 231, 229, 152,
      50, 149, 211, 246,  74, 232, 166, 234,
     233, 243, 213,  47, 112,  32, 242,  31,
       5, 103, 173,  85,  16, 206, 205, 227,
      39,  59, 218, 186, 215, 194,  38, 212,
     145,  29, 210,  28,  34,  51, 248, 250,
     241,  90, 239, 207, 144, 182, 139, 181,
     189, 192, 191,   8, 151,  30, 108, 226,
      97, 224, 198, 193,  89, 171, 187,  88,
     222,  95, 223,  96, 121, 126, 178, 138,
};

//! \brief Data table3 used by \ref permute and \ref cyclic
//! \sa [MS-PST] 5.1/mpbbI
//! \ingroup utilityrelated
const byte table3[] =
{
      71, 241, 180, 230,  11, 106, 114,  72,
     133,  78, 158, 235, 226, 248, 148,  83,
     224, 187, 160,   2, 232,  90,   9, 171,
     219, 227, 186, 198, 124, 195,  16, 221,
      57,   5, 150,  48, 245,  55,  96, 130,
     140, 201,  19,  74, 107,  29, 243, 251,
     143,  38, 151, 202, 145,  23,   1, 196,
      50,  45, 110,  49, 149, 255, 217,  35,
     209,   0,  94, 121, 220,  68,  59,  26,
      40, 197,  97,  87,  32, 144,  61, 131,
     185,  67, 190, 103, 210,  70,  66, 118,
     192, 109,  91, 126, 178,  15,  22,  41,
      60, 169,   3,  84,  13, 218,  93, 223,
     246, 183, 199,  98, 205, 141,   6, 211,
     105,  92, 134, 214,  20, 247, 165, 102,
     117, 172, 177, 233,  69,  33, 112,  12,
     135, 159, 116, 164,  34,  76, 111, 191,
      31,  86, 170,  46, 179, 120,  51,  80,
     176, 163, 146, 188, 207,  25,  28, 167,
      99, 203,  30,  77,  62,  75,  27, 155,
      79, 231, 240, 238, 173,  58, 181,  89,
       4, 234,  64,  85,  37,  81, 229, 122,
     137,  56, 104,  82, 123, 252,  39, 174,
     215, 189, 250,   7, 244, 204, 142,  95,
     239,  53, 156, 132,  43,  21, 213, 119,
      52,  73, 182,  18,  10, 127, 113, 136,
     253, 157,  24,  65, 125, 147, 216,  88,
      44, 206, 254,  36, 175, 222, 184,  54,
     200, 161, 128, 166, 153, 152, 168,  47,
      14, 129, 101, 115, 228, 194, 162, 138,
     212, 225,  17, 208,   8, 139,  42, 242,
     237, 154, 100,  63, 193, 108, 249, 236
};

//! \brief Calculate the signature of an item
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \param[in] id The id of the item to calculate
//! \param[in] address The location on disk of the item
//! \returns The computed signature
//! \sa [MS-PST] 5.5
//! \ingroup utilityrelated
template<typename T>
ushort compute_signature(T id, T address);

//! \brief Calculate the signature of an item
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \param[in] reference A block_reference to the item
//! \returns The computed signature
//! \sa [MS-PST] 5.5
//! \ingroup utilityrelated
template<typename T>
ushort compute_signature(const block_reference<T>& reference) { return compute_signature(reference.bid, reference.ib); }

//! \brief Compute the CRC of a block of data
//! \param[in] pdata A pointer to the block of data
//! \param[in] cb The size of the data block
//! \returns The computed CRC
//! \sa [MS-PST] 5.3
//! \ingroup utilityrelated
ulong compute_crc(const void * pdata, ulong cb);

//! \brief Modifies the data block in place, according to the permute method
//!
//! This algorithm is called to "encrypt" external data if the \ref crypt_method of the file
//! is set to \ref crypt_method_permute
//! \param[in,out] pdata A pointer to the data to "encrypt". This data is modified in place
//! \param[in] cb The size of the block of data
//! \param[in] encrypt True if "encrypting", false is unencrypting
//! \sa [MS-PST] 5.1
//! \ingroup disk
void permute(void * pdata, ulong cb, bool encrypt);

//! \brief Modifies the data block in place, according to the cyclic method
//!
//! This algorithm is called to "encrypt" external data if the \ref crypt_method of the file
//! is set to \ref crypt_method_cyclic
//! \param[in, out] pdata A pointer to the data to "encrypt". This data is modified in place
//! \param[in] cb The size of the block of data
//! \param[in] key The key used in the cycle process. Typically this is the block_id of the data being "encrypted".
//! \sa [MS-PST] 5.2
//! \ingroup disk
void cyclic(void * pdata, ulong cb, ulong key);


//
// page structures
//

//! \defgroup disk_pagerelated Page Structures
//! \ingroup disk


//! \brief Size of all pages in the file in bytes, including the page trailer.
//! \sa [MS-PST] 2.2.2.7
//! \ingroup disk_pagerelated
const size_t page_size = 512;

//! \brief Valid page types
//!
//! Used in the page_type and page_type_repeat fields of the page trailer.
//! \sa [MS-PST] 2.2.2.7.1/ptype
//! \ingroup disk_pagerelated
enum page_type
{
    page_type_bbt = 0x80,   //!< A BBT (Blocks BTree) page
    page_type_nbt = 0x81,   //!< A NBT (Nodes BTree) page
    page_type_fmap = 0x82,  //!< \deprecated A FMap (Free Map) page
    page_type_pmap = 0x83,  //!< \deprecated A PMap (Page Map) page
    page_type_amap = 0x84,  //!< An AMap (Allocation Map) page
    page_type_fpmap = 0x85, //!< \deprecated A FPMap (Free Page Map) page. Unicode stores only.
    page_type_dlist = 0x86  //!< A DList (Density List) page
};

//! \cond empty
template<typename T>
struct page_trailer
{
} PSTSDK_MS_STRUCT;
//! \endcond

//! \brief The Unicode store version of the page trailer
//!
//! The last structure in every page, aligned to the \ref page_size
//! \sa [MS-PST] 2.2.2.7.1
//! \ingroup disk_pagerelated
template<>
struct page_trailer<ulonglong>
{
    typedef ulonglong block_id_disk;

    byte page_type;        //!< The \ref page_type of this page
    byte page_type_repeat; //!< Same as the page_type field, for validation purposes
    ushort signature;      //!< Signature of this page, as calculated by the \ref compute_signature function
    ulong crc;             //!< CRC of this page, as calculated by the \ref compute_crc function
    block_id_disk bid;     //!< The id of this page
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(page_trailer<ulonglong>) == 16, "page_trailer<ulonglong> incorrect size");
//! \endcond

//! \brief The ANSI store version of the page trailer
//!
//! See the documentation for the Unicode version of the page trailer.
//! Note that the \a bid and \a crc fields are in a different order in the ANSI file.
//! \sa [MS-PST] 2.2.2.7.1
//! \ingroup disk_pagerelated
template<>
struct page_trailer<ulong>
{
    typedef ulong block_id_disk;

    byte page_type;
    byte page_type_repeat;
    ushort signature;
    block_id_disk bid;
    ulong crc;
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(page_trailer<ulong>) == 12, "page_trailer<ulong> incorrect size");
//! \endcond

//! \brief Generic page structure
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \sa [MS-PST] 2.2.2.7
//! \ingroup disk_pagerelated
template<typename T>
struct page
{
    static const size_t page_data_size = page_size - sizeof(page_trailer<T>); //!< Amount of usable space in a page

    byte data[page_data_size]; //!< space used for actual data
    page_trailer<T> trailer;   //!< The page trailer for this page
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(page<ulong>) == page_size, "page<ulong> incorrect size");
static_assert(sizeof(page<ulonglong>) == page_size, "page<ulonglong> incorrect size");
//! \endcond

//! \brief Number of bytes each slot (bit) in an AMap page refers to
//! \sa [MS-PST] 2.2.2.7.2
//! \ingroup disk_pagerelated
const size_t bytes_per_slot = 64;

//! \brief The location of the first AMap page in the file
//! \sa [MS-PST] 2.2.2.7.2
//! \ingroup disk_pagerelated
const size_t first_amap_page_location = 0x4400;

//! \brief The authoritative source of free space in the file
//!
//! Each bit in an AMap page refers to \ref bytes_per_slot bytes of the file.
//! If that bit is set, that indicates those bytes in the file are occupied.
//! If the bit is not set, that indicates those bytes in the file are available
//! for allocation. Note that each AMap page "maps" itself. Since an AMap page
//! (like all pages) is \ref page_size bytes (512), this means the first 8 bytes
//! of an AMap page are by definition always 0xFF.
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \sa [MS-PST] 2.2.2.7.2.1
//! \ingroup disk_pagerelated
template<typename T>
struct amap_page : public page<T>
{
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(amap_page<ulong>) == page_size, "amap_page<ulong> incorrect size");
static_assert(sizeof(amap_page<ulonglong>) == page_size, "amap_page<ulonglong> incorrect size");
//! \endcond

//! \brief An old allocation system optimized for pages
//!
//! Similar to an \ref amap_page, except each bit refers to \ref page_size bytes
//! rather than \ref bytes_per_slot bytes. This allocation scheme is no longer used
//! as of Outlook 2007 SP2, however these pages are still created in the file
//! for backwards compatability purposes.
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \deprecated
//! \sa [MS-PST] 2.2.2.7.3.1
//! \ingroup disk_pagerelated
template<typename T>
struct pmap_page : public page<T>
{
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(pmap_page<ulong>) == page_size, "pmap_page<ulong> incorrect size");
static_assert(sizeof(pmap_page<ulonglong>) == page_size, "pmap_page<ulonglong> incorrect size");
//! \endcond

//! \brief A deprecated allocation system optimized for AMap page searching
//!
//! A Free Map (or fmap) page has one byte per AMap page, indicating how many consecutive
//! slots are available for allocation on that amap page. No longer used as of Outlook 2007
//! SP2, but as with PMap pages are still created for backwards compatibility.
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \deprecated
//! \sa [MS-PST] 2.2.2.7.5.1
//! \ingroup disk_pagerelated
template<typename T>
struct fmap_page : public page<T>
{
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(fmap_page<ulong>) == page_size, "fmap_page<ulong> incorrect size");
static_assert(sizeof(fmap_page<ulonglong>) == page_size, "fmap_page<ulonglong> incorrect size");
//! \endcond

//! \brief A deprecated allocation  system optimized for PMap page searching
//!
//! A Free Page Map page has one bit per PMap page, indicating if that PMap page has any
//! slots available. A bit being set indicates there is no space available in that PMap page.
//! No longer used as of Outlook 2007, but still created for backwards compatibility.
//!
//! The lack of fpmap pages was the reason for the 2GB limit of ANSI pst files (rather than
//! a more intuitive 4GB limit) - the fpmap region in the header only had enough slots to
//! "map" 2GB of space.
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \deprecated
//! \sa [MS-PST] 2.2.2.7.6.1
//! \ingroup disk_pagerelated
template<typename T>
struct fpmap_page : public page<T>
{
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(fpmap_page<ulong>) == page_size, "fpmap_page<ulong> incorrect size");
static_assert(sizeof(fpmap_page<ulonglong>) == page_size, "fpmap_page<ulonglong> incorrect size");
//! \endcond

//! \brief The location of the only DList page in the file
//! \sa [MS-PST] 2.2.2.7.4
//! \ingroup disk_pagerelated
const size_t dlist_page_location = 0x4200;

//! \brief A metapage holding information about AMap pages
//!
//! The Density List page contains a list of AMap pages in the file, in ascending
//! order of density. That is to say, the "emptiest" page is at the top. This is the
//! data backing the new allocation scheme which replaced the old pmap/fmap/fpmap
//! scheme, starting in Outlook 2007 SP2.
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \sa [MS-PST] 2.2.2.7.4.2
//! \ingroup disk_pagerelated
template<typename T>
struct dlist_page
{
    static const size_t extra_space = page<T>::page_data_size - 8;
    static const size_t max_entries = extra_space / sizeof(ulong); //!< Maximum number of entries in the dlist page

    byte flags;                       //!< Flags indicating the state of the dlist page
    byte num_entries;                 //!< Number of entries in the entries array
    union
    {
        ulong current_page;           //!< The current AMap page used for allocations
        ulong backfill_location;      //!< The current backfill marker, when backfilling
    };
    union
    {
        ulong entries[max_entries];   //!< Each entry has bits for the amap page (ordinal) and free space (slots)
        byte _ignore[extra_space];
    };
    page_trailer<T> trailer;          //!< The page trailer
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(dlist_page<ulong>) == page_size, "dlist_page<ulong> incorrect size");
static_assert(sizeof(dlist_page<ulonglong>) == page_size, "dlist_page<ulonglong> incorrect size");
//! \endcond

//! \brief The portion of the dlist entry which refers to the amap page number
//! \sa [MS-PST] 2.2.2.7.4.1
//! \ingroup disk_pagerelated
const ulong dlist_page_num_mask = 0x0000FFFF;

//! \brief The bits to shift a dlist entry to get the slots on that amap page
//! \sa [MS-PST] 2.2.2.7.4.1
//! \ingroup disk_pagerelated
const ulong dlist_slots_shift = 20;

//! \brief Get a amap page number (ordinal) from a DLIST entry
//! \param[in] entry The entry to inspect
//! \sa [MS-PST] 2.2.2.7.4.1
//! \ingroup disk_pagerelated
inline ulong dlist_get_page_num(ulong entry) { return entry & dlist_page_num_mask; } 

//! \brief Get the number of free slots from a DLIST entry
//! \param[in] entry The entry to inspect
//! \sa [MS-PST] 2.2.2.7.4.1
//! \ingroup disk_pagerelated
inline ulong dlist_get_slots(ulong entry) { return entry >> dlist_slots_shift; }

//! \brief BTree Entry
//!
//! An array of these are used on non-leaf BT Pages
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \sa [MS-PST] 2.2.2.7.7.2
//! \ingroup disk_pagerelated
template<typename T>
struct bt_entry
{
    typedef T bt_key;

    bt_key key;               //!< The key of the page in ref
    block_reference<T> ref;   //!< A reference to a lower level page
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(bt_entry<ulong>) == 12, "bt_entry<ulong> incorrect size");
static_assert(sizeof(bt_entry<ulonglong>) == 24, "bt_entry<ulonglong> incorrect size");
//! \endcond

//! \brief NBT Leaf Entry
//!
//! An array of these are used on leaf NBT pages. It describes a node.
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \sa [MS-PST] 2.2.2.7.7.4
//! \ingroup disk_pagerelated
template<typename T>
struct nbt_leaf_entry
{
    typedef T nid_index;
    typedef T block_id_disk;

    nid_index nid;       //!< The node id
    block_id_disk data;  //!< The block id of the data block
    block_id_disk sub;   //!< The block id of the subnode block
    node_id parent_nid;  //!< The parent node id
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(nbt_leaf_entry<ulong>) == 16, "nbt_leaf_entry<ulong> incorrect size");
static_assert(sizeof(nbt_leaf_entry<ulonglong>) == 32, "nbt_leaf_entry<ulonglong> incorrect size");
//! \endcond

//! \brief BBT Leaf Entry
//!
//! An array of these are used on leaf BBT pages. It describes a block.
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \sa [MS-PST] 2.2.2.7.7.3
//! \ingroup disk_pagerelated
template<typename T>
struct bbt_leaf_entry
{
    block_reference<T> ref; //!< A reference to this block on disk
    ushort size;            //!< The unaligned size of this block
    ushort ref_count;       //!< The reference count of this block
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(bbt_leaf_entry<ulong>) == 12, "bbt_leaf_entry<ulong> incorrect size");
static_assert(sizeof(bbt_leaf_entry<ulonglong>) == 24, "bbt_leaf_entry<ulonglong> incorrect size");
//! \endcond

//! \brief The fundamental page structure which forms the basis of the two BTrees
//!
//! Generally speaking, the generic form of a BTree page contains a fixed
//! array of entries, followed by metadata about those entries and the
//! page. The entry type (and entry size, and thus the max number of
//! entries) varies between NBT and BBT pages.
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \tparam EntryType The type of entry on this page. In practice either a \ref bbt_leaf_entry, \ref nbt_leaf_entry, or \ref bt_entry
//! \sa [MS-PST] 2.2.2.7.7.1
//! \ingroup disk_pagerelated
template<typename T, typename EntryType>
struct bt_page
{
    static const size_t extra_space = page<T>::page_data_size - (sizeof(T) * sizeof(byte));
    static const size_t max_entries = extra_space / sizeof(EntryType); //!< Maximum number of entries on a page
    union
    {
        EntryType entries[max_entries];
        byte _ignore[extra_space];
    };

    byte num_entries;          //!< Number of entries on this page
    byte num_entries_max;      //!< Maximum number of entries on this page
    byte entry_size;           //!< The size of each entry
    byte level;                //!< The level of this page. A level of zero indicates a leaf.

    page_trailer<T> trailer;   //!< The page trailer
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
namespace detail {
    // We can't pass types containing "," to offsetof, because it's a
    // preprocessor macro, and it doesn't really understand how to parse an
    // argument list.  So we declare some aliases in a "detail" namespace
    // where they won't pollute the header.
    typedef bt_page<ulong, bt_entry<ulong> > bt_page_ulong;
    typedef bt_page<ulonglong, bt_entry<ulonglong> > bt_page_ulonglong;
}
static_assert(offsetof(detail::bt_page_ulong, num_entries) == 496, "bt_page<ulong, bt_entry<ulong> > num_entries at incorrect offset");
static_assert(offsetof(detail::bt_page_ulonglong, num_entries) == 488, "bt_page<ulonglong, bt_entry<ulonglong> > num_entries at incorrect offset");
static_assert(offsetof(detail::bt_page_ulong, trailer) == 500, "bt_page<ulong, bt_entry<ulong> > trailer at incorrect offset");
static_assert(offsetof(detail::bt_page_ulonglong, trailer) == 496, "bt_page<ulonglong, bt_entry<ulonglong> > trailer at incorrect offset");
//! \endcond

//! \brief NBT non-leaf page
//!
//! A BTree page instance.
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \sa [MS-PST] 2.2.2.7.7.1
//! \ingroup disk_pagerelated
template<typename T>
struct nbt_nonleaf_page : public bt_page<T, bt_entry<T> >
{
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(nbt_nonleaf_page<ulong>) == page_size, "nbt_nonleaf_page<ulong> incorrect size");
static_assert(sizeof(nbt_nonleaf_page<ulonglong>) == page_size, "nbt_nonleaf_page<ulonglong> incorrect size");
//! \endcond

//! \brief BBT non-leaf page
//!
//! A BTree page instance. Note that this structure is identical to
//! a non-leaf NBT page.
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \sa [MS-PST] 2.2.2.7.7.1
//! \ingroup disk_pagerelated
template<typename T>
struct bbt_nonleaf_page : public bt_page<T, bt_entry<T> >
{
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(bbt_nonleaf_page<ulong>) == page_size, "bbt_nonleaf_page<ulong> incorrect size");
static_assert(sizeof(bbt_nonleaf_page<ulonglong>) == page_size, "bbt_nonleaf_page<ulonglong> incorrect size");
//! \endcond

//! \brief NBT leaf page
//!
//! A BTree page instance. The NBT leaf page has an array of nbt_leaf_entries
//! ordered by node id, which describe the nodes of the database.
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \sa [MS-PST] 2.2.2.7.7.1
//! \ingroup disk_pagerelated
template<typename T>
struct nbt_leaf_page : public bt_page<T, nbt_leaf_entry<T> >
{
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(nbt_leaf_page<ulong>) == page_size, "nbt_leaf_page<ulong> incorrect size");
static_assert(sizeof(nbt_leaf_page<ulonglong>) == page_size, "nbt_leaf_page<ulonglong> incorrect size");
//! \endcond

//! \brief BBT leaf page
//!
//! A BTree page instance. The BBT leaf page has an array of bbt_leaf_entries
//! ordered by block id, which describe the blocks in the database.
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \sa [MS-PST] 2.2.2.7.7.1
//! \ingroup disk_pagerelated
template<typename T>
struct bbt_leaf_page : public bt_page<T, bbt_leaf_entry<T> >
{
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(bbt_leaf_page<ulong>) == page_size, "bbt_leaf_page<ulong> incorrect size");
static_assert(sizeof(bbt_leaf_page<ulonglong>) == page_size, "bbt_leaf_page<ulonglong> incorrect size");
//! \endcond

//
// block structures
//

//! \defgroup disk_blockrelated Block Structures
//! \ingroup disk

//! \brief The maximum individual block size
//! \sa [MS-PST] 2.2.2.8
//! \ingroup disk_blockrelated
const size_t max_block_disk_size = 8 * 1024;

//! \brief The different block types.
//! \ingroup disk_blockrelated
//! \sa [MS-PST] 2.2.2.8.3
enum block_types
{
    block_type_external = 0x00, //!< An external data block
    block_type_extended = 0x01, //!< An extended block type
    block_type_sub = 0x02       //!< A subnode block type
};

//! \brief Aligns a block size to the size on disk
//!
//! Adds the block trailer and slot alignment to the data size
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \param[in] size Block size
//! \returns The aligned block size
//! \sa [MS-PST] 2.2.2.7.7.3
//! \ingroup disk_blockrelated
template<typename T>
size_t align_disk(size_t size);

//! \brief Aligns a block size to the slot size
//!
//! Round size up to the next largest multiple of \ref bytes_per_slot
//! \param[in] size Block size
//! \returns The block size, aligned to slot size
//! \sa [MS-PST] 2.2.2.7.7.3
//! \ingroup disk_blockrelated
size_t align_slot(size_t size);

//! \brief The attached bit indicates a block is referenced in memory
//! This is an implementation detail, and this bit should be ignored.
//! \sa [MS-PST] 2.2.2.2/r
const uint block_id_attached_bit = 0x1;

//! \brief The internal bit indicates a block is an \ref extended_block or a \ref subnode_block
//! \ingroup disk_blockrelated
//! \sa [MS-PST] 2.2.2.2/i
const uint block_id_internal_bit = 0x2;

//! \brief The block id counter in the header is incremented by this amount for each block
//! \sa [MS-PST] 2.2.2.6/bidNextB
//! \ingroup disk_blockrelated
const uint block_id_increment = 0x4;

//! \brief Determines if a block is external or not
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \param[in] bid The id of the block
//! \returns true if the block is external
//! \ingroup disk_blockrelated
//! \sa [MS-PST] 2.2.2.2/i
template<typename T>
bool bid_is_external(T bid) { return ((bid & block_id_internal_bit) == 0); }

//! \brief Determines if a block is internal or not
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \param[in] bid The id of the block
//! \returns true if the block is internal
//! \sa [MS-PST] 2.2.2.2/i
//! \ingroup disk_blockrelated
template<typename T>
bool bid_is_internal(T bid) { return !bid_is_external(bid); }

//! \cond empty
template<typename T>
struct block_trailer
{
} PSTSDK_MS_STRUCT;
//! \endcond

//! \brief Unicode store version of the block trailer
//!
//! Blocks are of variable size. The BBT gives the unaligned size, which is an input
//! to the \ref align_disk function to give the total size on disk. Using the total size and
//! the address, one can read the block trailer - which is always located at and aligned to
//! the end of the aligned size. The block trailer contains validation information about
//! the block.
//! \sa [MS-PST] 2.2.2.8.1
//! \ingroup disk_blockrelated
template<>
struct block_trailer<ulonglong>
{
    typedef ulonglong block_id_disk;

    ushort cb;           //!< Size of the block (unaligned)
    ushort signature;    //!< Signature of this block, as calculated by the \ref compute_signature function
    ulong crc;           //!< CRC of this block, as calculated by the \ref compute_crc function
    block_id_disk bid;   //!< The id of this block
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(block_trailer<ulonglong>) == 16, "block_trailer<ulonglong> incorrect size");
//! \endcond

//! \brief ANSI store version of the block trailer
//!
//! See the Unicode store version of the block trailer for more
//! information. Note that the bid and CRC fields are in a different order
//! in the ANSI version.
//! \sa [MS-PST] 2.2.2.8.1
//! \ingroup disk_blockrelated
template<>
struct block_trailer<ulong>
{
    typedef ulong block_id_disk;

    ushort cb;
    ushort signature;
    block_id_disk bid;
    ulong crc;
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(block_trailer<ulong>) == 12, "block_trailer<ulong> incorrect size");
//! \endcond

//! \brief External block definition
//!
//! The data contained in an external block is "encrypted" as defined by
//! the \ref crypt_method of the store. The id of an external block never 
//! has the \ref block_id_internal_bit set.
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \sa [MS-PST] 2.2.2.8.3.1
//! \ingroup disk_blockrelated
template<typename T>
struct external_block
{
    static const size_t max_size = max_block_disk_size - sizeof(block_trailer<T>);
    byte data[1];   //!< Data contained in this block
} PSTSDK_MS_STRUCT;

//! \cond empty
template<typename T>
struct extended_block
{
} PSTSDK_MS_STRUCT;
//! \endcond

//! \brief Unicode store version of the extended block
//!
//! Extended blocks are a way of extended the logical size of a block. It
//! contains references to other blocks (which themselves may be extended
//! blocks). This block can logically be thought of as the concatenation
//! of the blocks it points to. Extended blocks begin with a header. Note
//! the first byte of this header is the \ref block_type - a property
//! shared with the \ref sub_block.
//! \sa [MS-PST] 2.2.2.8.3.2.1
//! \sa [MS-PST] 2.2.2.8.3.2.2
//! \ingroup disk_blockrelated
template<>
struct extended_block<ulonglong>
{
    typedef ulonglong block_id_disk;

    static const size_t max_count = (external_block<ulonglong>::max_size - 8) / sizeof(extended_block<ulonglong>::block_id_disk);
    static const size_t max_size = external_block<ulonglong>::max_size * extended_block<ulonglong>::max_count;

    byte block_type;       //!< Always \ref block_type_extended
    byte level;            //!< If zero, this block points to external blocks. If one, this block points to level zero extended blocks.
    ushort count;          //!< Number of entries in the bid array
    ulong total_size;      //!< Total logical size of this block
    block_id_disk bid[1];  //!< Array of blocks this block points to
} PSTSDK_MS_STRUCT;

//! \brief ANSI store version of the extended block
//!
//! See the Unicode version for details. The ANSI version has a different
//! max_count calcuation.
//! \sa [MS-PST] 2.2.2.8.3.2.1
//! \sa [MS-PST] 2.2.2.8.3.2.2
//! \ingroup disk_blockrelated
template<>
struct extended_block<ulong>
{
    typedef ulong block_id_disk;

    static const size_t max_count = ((4096L) - sizeof(block_trailer<ulong>) - 8) / sizeof(extended_block<ulong>::block_id_disk);
    static const size_t max_size = external_block<ulong>::max_size * extended_block<ulong>::max_count;

    byte block_type;
    byte level;
    ushort count;
    ulong total_size;
    block_id_disk bid[1];
} PSTSDK_MS_STRUCT;

//! \brief Entries on a leaf \ref sub_block
//!
//! The leaf entry contains information about the subnode. Note there is
//! no parent field.
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \sa [MS-PST] 2.2.2.8.3.3.1.1
//! \ingroup disk_blockrelated
template<typename T>
struct sub_leaf_entry
{
    typedef T block_id_disk;

    node_id nid;        //!< Subnode id
    block_id_disk data; //!< Data block of this subnode
    block_id_disk sub;  //!< Subnode block of this subnode. Yes, subnodes can and do themselves had subnodes.
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(sub_leaf_entry<ulong>) == 12, "sub_leaf_entry<ulong> incorrect size");
static_assert(sizeof(sub_leaf_entry<ulonglong>) == 24, "sub_leaf_entry<ulonglong> incorrect size");
//! \endcond

//! \brief Entries on a nonleaf \ref sub_block
//!
//! Similar to non-leaf BT pages, nonleaf subnode blocks point to other
//! subnode blocks.
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \sa [MS-PST] 2.2.2.8.3.3.2.1
//! \ingroup disk_blockrelated
template<typename T>
struct sub_nonleaf_entry
{
    typedef T block_id_disk;

    node_id nid_key;             //!< Key of the subnode block
    block_id_disk sub_block_bid; //!< Id of the subnode block
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(sub_nonleaf_entry<ulong>) == 8, "sub_nonleaf_entry<ulong> incorrect size");
static_assert(sizeof(sub_nonleaf_entry<ulonglong>) == 16, "sub_nonleaf_entry<ulonglong> incorrect size");
//! \endcond

//! \brief Subnode Blocks
//!
//! Subnode blocks form a "private NBT" for each node. Non-leaf subnode
//! blocks point to leaf subnode blocks. Leaf subnode blocks contain the
//! subnode information.
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \tparam EntryType The type of entry on this block. Generally \ref sub_nonleaf_entry or \ref sub_leaf_entry.
//! \sa [MS-PST] 2.2.2.8.3.3
//! \ingroup disk_blockrelated
template<typename T, typename EntryType>
struct sub_block
{
    byte block_type;    //!< Always \ref block_type_sub
    byte level;         //!< One for non-leaf, zero for leaf
    ushort count;       //!< Number of entries in the entry array
    EntryType entry[1]; //!< Array of entries
} PSTSDK_MS_STRUCT;

//! \brief Subnode non-leaf block
//!
//! An instance of a \ref sub_block, containing \ref sub_nonleaf_entry
//! items in the entry array
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \sa [MS-PST] 2.2.2.8.3.3.2.2
//! \ingroup disk_blockrelated
template<typename T>
struct sub_nonleaf_block : public sub_block<T, sub_nonleaf_entry<T> >
{
} PSTSDK_MS_STRUCT;

//! \brief Subnode leaf block
//!
//! An instance of a \ref sub_block, containing \ref sub_leaf_entry items
//! in the entry array
//! \tparam T \ref ulonglong for a unicode store, \ref ulong for an ANSI store
//! \sa [MS-PST] 2.2.2.8.3.3.1.2
//! \ingroup disk_blockrelated
template<typename T>
struct sub_leaf_block : public sub_block<T, sub_leaf_entry<T> >
{
} PSTSDK_MS_STRUCT;

//
// heap structures
//

//! \defgroup disk_heaprelated Heap Structures
//! \ingroup disk

//! \brief Signature of a heap
//! \sa [MS-PST] 2.3.1.2/bSig
//! \ingroup disk_heaprelated
const byte heap_signature = 0xEC;

//! \brief Maximum allocation size in a heap
//! \ingroup disk_heaprelated
const uint heap_max_alloc_size = 3580;

//! \brief Maximum allocation size in a heap in a version 14 ANSI file
//! \ingroup disk_heaprelated
const uint heap_max_alloc_size_wver_14 = 3068;

//! \brief Different heap client signature types
//! \sa [MS-PST] 2.3.1.2/bClientSig
//! \ingroup disk_heaprelated
enum heap_client_signature
{
   heap_sig_gmp = 0x6C,  //< Internal
   heap_sig_tc = 0x7C,   //< Table context
   heap_sig_smp = 0x8C,  //< Internal
   heap_sig_hmp = 0x9C,  //< Internal
   heap_sig_ch = 0xA5,   //< \deprecated Internal
   heap_sig_chtc = 0xAC, //< \deprecated Internal
   heap_sig_bth = 0xB5,  //< BTree on Heap
   heap_sig_pc = 0xBC,   //< Property Context
};

//! brief Fill level descriptions
//! \sa [MS-PST] 2.3.1.2/rgbFillLevel
enum heap_fill_level
{
    heap_fill_empty,    //< >= 3584 bytes free
    heap_fill_1,        //< 2560 - 3583 bytes free
    heap_fill_2,        //< 2048 - 2559 bytes free
    heap_fill_3,        //< 1792 - 2047 bytes free
    heap_fill_4,        //< 1536 - 1791 bytes free
    heap_fill_5,        //< 1280 - 1535 bytes free
    heap_fill_6,        //< 1024 - 1279 bytes free
    heap_fill_7,        //< 768 - 1023 bytes free
    heap_fill_8,        //< 512 - 767 bytes free
    heap_fill_9,        //< 256 - 511 bytes free
    heap_fill_10,       //< 128 - 255 bytes free
    heap_fill_11,       //< 64 - 127 bytes free
    heap_fill_12,       //< 32 - 63 bytes free
    heap_fill_13,       //< 16 - 31 bytes free
    heap_fill_14,       //< 8 - 15 bytes free
    heap_fill_full      //< < 8 bytes free
};

//! \brief Header structure on the first heap block
//! \sa [MS-PST] 2.3.1.2
//! \ingroup disk_heaprelated
struct heap_first_header
{
    static const uint fill_level_size = 4; //!< Number of bytes in the \ref page_fill_levels map on this block

    ushort page_map_offset;  //!< Offset of the start of the \ref heap_page_map
    byte signature;          //!< Always \ref heap_signature
    byte client_signature;   //!< Client defined signature, see \ref heap_client_signature
    heap_id root_id;         //!< Root allocation. This has specific meaning to the owner of this heap.
    byte page_fill_levels[fill_level_size]; //!< Fill level of this and next seven heap blocks (4 bits each), see \ref heap_fill_level
} PSTSDK_MS_STRUCT;

//! \brief Header structure on non-first/non-fill blocks
//! \sa [MS-PST] 2.3.1.3
//! \ingroup disk_heaprelated
struct heap_page_header
{
    ushort page_map_offset; //!< offset of the start of the page map
} PSTSDK_MS_STRUCT;

//! \brief Header structure on non-first/fill blocks
//! \sa [MS-PST] 2.3.1.4
//! \ingroup disk_heaprelated
struct heap_page_fill_header
{
    static const uint fill_level_size = 64; //!< Number of bytes in the page_fill_levels map on this block

    ushort page_map_offset; //!< Offset of the start of the page map
    byte page_fill_levels[fill_level_size]; //!< Fill level of this and next 127 heap blocks
} PSTSDK_MS_STRUCT;

//! \brief Provides a map of the allocations on a heap block
//!
//! The page map gives the allocation location all of the heap allocations
//! on this heap block. The allocs array contains the start of each
//! allocation. The length of allocation N is determined simply by
//! subtracting the start of the N+1 allocation from the start of Ns
//! allocation. For this reason, the allocation array has N+1 entries,
//! where the last entry gives the end of the last allocation.
//!
//! When an allocation is freed, all subsequent allocations are slid
//! down, making the freed allocation zero in length.
//! \sa [MS-PST] 2.3.1.5
//! \ingroup disk_heaprelated
struct heap_page_map
{
    ushort num_allocs; //!< Number of allocations on this block
    ushort num_frees;  //!< Number of empty allocations on this
    ushort allocs[1];  //!< The offset of each allocation
} PSTSDK_MS_STRUCT;

//
// bth structures
//

//! \defgroup disk_bthrelated BTH Structures
//! \ingroup disk

//! \brief Describes the BTH, including the size of the keys/values and the
//! \ref heap_id of the root allocation.
//! \sa [MS-PST] 2.3.2.1
//! \ingroup disk_bthrelated
struct bth_header
{
    byte bth_signature; //!< Always \ref heap_sig_bth
    byte key_size;      //!< Key size in bytes
    byte entry_size;    //!< Entry size in bytes
    byte num_levels;    //!< Number of levels
    heap_id root;       //!< Root of the actual tree structure
} PSTSDK_MS_STRUCT;

//! \brief Entries which make up a "non-leaf" BTH allocation
//!
//! Clients must keep track of the current level as they are doing a BTH
//! lookup. Non-leaf entries, similar to how the BT pages and sub_blocks
//! work, contain the key and id of a lower level page.
//! \tparam K The key type
//! \sa [MS-PST] 2.3.2.2
//! \ingroup disk_bthrelated
#pragma pack(2)
template<typename K>
struct bth_nonleaf_entry
{
    K key;        //!< Key of the lower level page
    heap_id page; //!< Heap id of the lower level page
} PSTSDK_MS_STRUCT;
#pragma pack()
//! \cond static_asserts
static_assert(sizeof(bth_nonleaf_entry<ushort>) == 6, "bth_nonleaf_entry<ushort> incorrect size");
//! \endcond

//! \brief Entries which make up a "leaf" BTH allocation
//!
//! Simply an ordered array of key/values.
//! \tparam K The key type
//! \tparam V The value type
//! \sa [MS-PST] 2.3.2.3
//! \ingroup disk_bthrelated
template<typename K, typename V>
struct bth_leaf_entry
{
    K key;   //!< Key instance
    V value; //!< Value instance
} PSTSDK_MS_STRUCT;

//! \brief BTH node
//!
//! A BTH node is simply an array of entries, where the entry is either
//! a \ref bth_nonleaf_entry or a \ref bth_leaf_entry. Clients must keep
//! track of the current level as they are recursing down a BTH, starting
//! with the value stored in the \ref bth_header.
//!
//! The number of entries is determined simply by the size of the heap
//! allocation divided by the size of the EntryType.
//! \tparam EntryType The entry type in this BTH. In practice eitehr a \ref bth_leaf_entry or \ref bth_nonleaf_entry.
//! \sa [MS-PST] 2.3.2
//! \ingroup disk_bthrelated
template<typename EntryType>
struct bth_node
{
    EntryType entries[1]; //!< Array of entries
} PSTSDK_MS_STRUCT;

//! \brief BTH Leaf node
//!
//! Instance of a BTH node, with \ref bth_leaf_entry for entries.
//! \tparam K The key type
//! \tparam V The value type
//! \sa [MS-PST] 2.3.2.3
//! \ingroup disk_bthrelated
template<typename K, typename V>
struct bth_leaf_node : bth_node<bth_leaf_entry<K,V> >
{
} PSTSDK_MS_STRUCT;

//! \brief BTH Nonleaf node
//!
//! Instance of a BTH node, with \ref bth_nonleaf_entry for entries.
//! \tparam K The key type
//! \tparam V The value type
//! \sa [MS-PST] 2.3.2.2
//! \ingroup disk_bthrelated
template<typename K>
struct bth_nonleaf_node : bth_node<bth_nonleaf_entry<K> >
{
} PSTSDK_MS_STRUCT;

//
// pc structures
//

//! \defgroup disk_pcrelated Property Context Structures
//! \ingroup disk

//! \brief The value type of the BTH backing a pc
//! 
//! A PC is simply a node with the root_id allocation out of the heap
//! refering to a BTH header, with a key type of \ref prop_id and a value
//! type of prop_entry.
//! \sa [MS-PST] 2.3.3.3
//! \ingroup disk_pcrelated
#pragma pack(2)
struct prop_entry
{
    ushort type;     //$< Property type
    heapnode_id id;  //$< Heapnode id for variable length properties, or the value directly for fixed size property types
};
#pragma pack()
//! \cond static_asserts
static_assert(sizeof(bth_leaf_entry<ushort, prop_entry>) == 8, "bth_leaf_entry<ushort, prop_entry> incorrect size");
//! \endcond

//! \brief Sub object
//!
//! When an attachment has its data attached as a prop_type_object, the
//! PC structure points to one of these.
//! \ingroup disk_pcrelated
struct sub_object
{
    node_id nid; //$< The subnode id containing the data for the object
    ulong size;  //$< The size of the object
} PSTSDK_MS_STRUCT;

//! \brief Multi-valued, variable length property TOC
//!
//! Variable length multivalued props have this table of contents at the
//! start of the allocation - giving the number and offset of each mv
//! instance.
//! \sa [MS-PST] 2.3.3.4.2
//! \ingroup disk_pcrelated
struct mv_toc
{
    ulong count;      //$< Number of entries in the TOC
    ulong offsets[1]; //$< Array of offsets for the start of each entry
} PSTSDK_MS_STRUCT;

//
// tc structures
//

//! \defgroup disk_tcrelated Table Context Structures
//! \ingroup disk

//! \brief Indices into the size offsets array
//! \sa [MS-PST] 2.3.4.1/rgib
//! \ingroup disk_tcrelated
enum tc_offsets
{
    tc_offsets_four,    //$< Offset of the end of the four and eight byte columns
    tc_offsets_two,     //$< Offset of the end of the two byte columns
    tc_offsets_one,     //$< Offset of the end of the one byte columns
    tc_offsets_bitmap,  //$< Offset of the end of the existance bitmap
    tc_offsets_max      //$< Number of entries in the offset array
};

//! \brief Describes a column
//! \sa [MS-PST] 2.3.4.2
//! \ingroup disk_tcrelated
#pragma pack(2)
struct column_description
{
    ushort type;        //$< Column property type
    prop_id id;         //$< Column property id
    ushort offset;      //$< Offset into the row
    byte size;          //$< Width of the column
    byte bit_offset;    //$< Bit offset into the existance bitmap
};

//! \brief GUST Column description structure
//!
//! Briefly (in Outlook 2007 RTM and SP1) there was a new type of TC
//! in the store. It had a slightly different structure. This was it.
//! \deprecated
//! \ingroup disk_tcrelated
struct gust_column_description
{
    ushort type;
    prop_id id;
    ushort offset;
    byte size;
    byte unused1;
    ushort bit_offset;
    ushort unused2;
    node_id data_subnode;
};

//! \brief The root_id allocation out of the Heap of a TC node
//! \sa [MS-PST] 2.3.4.1
//! \ingroup disk_tcrelated
struct tc_header
{
    byte signature;             //$< TC signature, \ref heap_sig_tc
    byte num_columns;           //$< Number of columns in this table
    ushort size_offsets[tc_offsets_max]; // Row offset array, see \ref tc_offsets
    heap_id row_btree_id;       //$< The bth_header allocation for the row mapping btree
    heapnode_id row_matrix_id;  //$< The heapnode_id allocation for the row matrix
    byte unused[4];
    column_description columns[1]; //$< Column description array, of length num_columns
};

//! \brief GUST TC Header
//! \deprecated
//! \ingroup disk_tcrelated
struct gust_header
{
    byte signature;
    byte unused1;
    ushort size_offsets[tc_offsets_max];
    heap_id row_btree_id;
    heapnode_id row_matrix_id;
    byte unused2[4];
    ushort num_columns;
    node_id column_subnode;
    ulong unused3;
    ulong unused4;
};
#pragma pack()

//
// nameid structures
//

//! \defgroup disk_nameidrelated Name to ID Mapping Structures
//! \ingroup disk

//! \brief Defines a id to name mapping
//!
//! Found in the entry stream of the name id map node, the nameid is the 
//! fundamental structure which defines the name to id mapping.
//! \sa [MS-PST] 2.4.7.1
//! \ingroup disk_nameidrelated
struct nameid
{
    union
    {
        ulong id;               //!< id of the named prop (for named props defined by an id)
        ulong string_offset;    //!< offset into the string stream of the name of this prop
    };
    ulong index;                //!< Bits 16-31 are the index into the entry stream. Bits 1-16 are the index into the guid stream. Bit 0 indicates if this is a string or id based prop
} PSTSDK_MS_STRUCT;

//! \brief A modified nameid structure used in the hash buckets
//!
//! Similar to a \ref nameid, except instead of an id or string offset
//! it contains the hash_base of the property. This is used to determine
//! what bucket to put it in. The information in the index is used to lookup
//! the real entry in the entry stream, where a true comparision is done.
//! \sa [MS-PST] 2.4.7.5
//! \ingroup disk_nameidrelated
struct nameid_hash_entry
{
    ulong hash_base;            //!< For numeric named props, this is just the id. Hash value of string props.
    ulong index;                //!< Bits 16-31 are the index into the entry stream. Bits 1-16 are the index into the guid stream. Bit 0 indicates if this is a string or id based prop
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(nameid) == 8, "nameid incorrect size");
static_assert(sizeof(nameid_hash_entry) == 8, "nameid incorrect size");
//! \endcond

//! \brief Returns the index of a given nameid structure
//! \param[in] n the \ref nameid structure
//! \returns the index
//! \sa [MS-PST] 2.4.7.1/wPropIdx
//! \ingroup disk_nameidrelated
inline ushort nameid_get_prop_index(const nameid& n) { return (ushort)(n.index >> 16); }

//! \brief Returns the index into the guid stream of the guid of a given nameid structure
//! \param[in] n the \ref nameid structure
//! \returns the guid's index
//! \ingroup disk_nameidrelated
//! \sa [MS-PST] 2.4.7.1/wGuid
inline ushort nameid_get_guid_index(const nameid& n) { return (ushort)((ushort)n.index >> 1); }

//! \brief Returns true if the nameid structure is named by a string
//! \param[in] n the \ref nameid structure
//! \returns true if this is named by a string
//! \sa [MS-PST] 2.4.7.1/N
//! \ingroup disk_nameidrelated
inline bool nameid_is_string(const nameid& n) { return n.index & 0x1; }

//! \brief Returns the index of a given nameid_hash_entry structure
//! \param[in] n the nameid_hash_entry structure
//! \returns the index
//! \sa [MS-PST] 2.4.7.1/wPropIdx
//! \ingroup disk_nameidrelated
inline ushort nameid_get_prop_index(const nameid_hash_entry& n) { return (ushort)(n.index >> 16); }

//! \brief Returns the index into the guid stream of the guid of a given nameid_hash_entry structure
//! \param[in] n the nameid_hash_entry structure
//! \returns the guid's index
//! \sa [MS-PST] 2.4.7.1/wGuid
//! \ingroup disk_nameidrelated
inline ushort nameid_get_guid_index(const nameid_hash_entry& n) { return (ushort)((ushort)n.index >> 1); }

//! \brief Returns true if the nameid_hash_entry structure is named by a string
//! \param[in] n the nameid_hash_entry structure
//! \returns true if this is named by a string
//! \sa [MS-PST] 2.4.7.1/N
//! \ingroup disk_nameidrelated
inline bool nameid_is_string(const nameid_hash_entry& n) { return n.index & 0x1; }

} // end disk namespace
} // end pstsdk namespace


template<typename T>
inline pstsdk::ushort pstsdk::disk::compute_signature(T id, T address)
{
    T value = address ^ id;

    return (ushort(ushort(value >> 16) ^ ushort(value)));
}

inline pstsdk::ulong pstsdk::disk::compute_crc(const void * pdata, ulong cb)
{
    ulong crc = 0;
    const byte * pb = reinterpret_cast<const byte*>(pdata);

    while(cb-- > 0)
        crc = crc_table[(int)(byte)crc ^ *pb++] ^ (crc >> 8);

    return crc;
}

inline void pstsdk::disk::permute(void * pdata, ulong cb, bool encrypt)
{
    byte * pb = reinterpret_cast<byte*>(pdata);
    const byte * ptable = encrypt ? table1 : table3;
    byte b;

    while(cb-- > 0)
    {
        b = *pb;
        *pb++ = ptable[b];
    }
}

inline void pstsdk::disk::cyclic(void * pdata, ulong cb, ulong key)
{
    byte * pb = reinterpret_cast<byte*>(pdata);
    byte b;
    ushort w;

    w = (ushort)(key ^ (key >> 16));

    while (cb-- > 0)
    {
        b = *pb;
        b = (byte)(b + (byte)w);
        b = table1[b];
        b = (byte)(b + (byte)(w >> 8));
        b = table2[b];
        b = (byte)(b - (byte)(w >> 8));
        b = table3[b];
        b = (byte)(b - (byte)w);
        *pb++ = b;

        w = (ushort)(w + 1);
    }
}

template<typename T>
inline size_t pstsdk::disk::align_disk(size_t size)
{
    return align_slot(size + sizeof(block_trailer<T>));
}

inline size_t pstsdk::disk::align_slot(size_t size)
{
    return ((size + bytes_per_slot - 1) & ~(bytes_per_slot - 1));
}

#endif
