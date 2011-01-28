//! \file
//! \brief Primitive structures defined by MS-PST and MAPI
//! \author Terry Mahaffey
//! \ingroup util

//! \defgroup primitive Primitive Types
//! \ingroup util

#ifndef PSTSDK_UTIL_PRIMITIVES_H
#define PSTSDK_UTIL_PRIMITIVES_H

#include <boost/config.hpp>
#include <boost/cstdint.hpp>

//
// Global compiler hacks
//

#ifdef BOOST_NO_RVALUE_REFERENCES
#ifndef SUPPRESS_CPLUSPLUS0X_MESSAGES
#pragma message("C++0x rvalue references not supported; consider updating your compiler")
#endif
#endif

#ifdef BOOST_NO_STATIC_ASSERT
#include <boost/static_assert.hpp>
#define static_assert(x,y) BOOST_STATIC_ASSERT(x)
#endif

//! \brief Global Validation Settings
//!
//! You may optionally define one of the following values before including any pstsdk header:
//! - PSTSDK_VALIDATION_LEVEL_NONE, no validation - except some type checks
//! - PSTSDK_VALIDATION_LEVEL_WEAK, involves fast checks such as signature matching, param validation, etc
//! - PSTSDK_VALIDATION_LEVEL_FULL, includes all weak checks plus crc validation and any other "expensive" checks
//!
//! Weak validation is the default.
//! \ingroup primitive
#ifndef PSTSDK_VALIDATION_LEVEL_NONE
#define PSTSDK_VALIDATION_LEVEL_WEAK
#endif

#ifdef PSTSDK_VALIDATION_LEVEL_FULL
// full validation also implies weak validation
#define PSTSDK_VALIDATION_LEVEL_WEAK
#endif

// Many of the low-level data structures in pstsdk rely on being laid out
// in the fashion preferred by Visual C++.  For example, these structures
// need to align ulonglong to the nearest 8 bytes.  Under GCC, we can force
// this layout behavior on Mac and Linux systems by appending an
// appropriate attribute declaration to the structure.  We're also tried
// '#pragma ms_struct on', but it fails on at least some Linux systems.
#ifdef __GNUC__
#define PSTSDK_MS_STRUCT __attribute__((ms_struct))
#else
#define PSTSDK_MS_STRUCT
#endif

namespace pstsdk
{

/*! \addtogroup primitive
 * @{
 */
typedef boost::uint32_t uint;
typedef boost::uint32_t ulong;
typedef boost::int32_t slong;
typedef boost::uint64_t ulonglong;
typedef boost::int64_t slonglong;
typedef boost::uint8_t byte;
typedef boost::uint16_t ushort;
/*! @} */

//! \cond static_asserts
static_assert(sizeof(byte) == 1, "pstsdk::byte unexpected size");
static_assert(sizeof(ushort) == 2, "pstsdk::ushort unexpected size");
static_assert(sizeof(uint) == 4, "pstsdk::uint unexpected size");
static_assert(sizeof(ulonglong) == 8, "pstsdk::ulonglong unexpected size");
//! \endcond

/*! \addtogroup primitive
 * @{
 */
typedef ulong node_id;
typedef ulonglong block_id;
typedef block_id page_id;

typedef ulong heap_id;
typedef ulong heapnode_id;

typedef ushort prop_id;

typedef ulong row_id;
/*! @} */

//! \brief Tag structure used to indicate a copy constructed class should be
//! an alias (shallow copy) rather than a deep copy
//!
//! When you copy construct an object with the alias tag, changes to the 
//! alias object also are reflected in the original object. When you copy
//! construct without an alias tag, you have two unique instances of the object
//! and changes in either object are not reflected in the other.
//!
//! In either case both objects refer to the same physical item on disk - the
//! alias tag only affects the in memory behavior.
//!
//! \ingroup primitive
struct alias_tag { };

//
// node id
//

//! \brief Different node types found in a PST file
//! \sa [MS-PST] 2.2.2.1/nidType
//! \ingroup primitive
enum nid_type
{
    nid_type_none = 0x00,
    nid_type_internal = 0x01,
    nid_type_folder = 0x02,
    nid_type_search_folder = 0x03,
    nid_type_message = 0x04,
    nid_type_attachment = 0x05,
    nid_type_search_update_queue = 0x06,
    nid_type_search_criteria_object = 0x07,
    nid_type_associated_message = 0x08,
    nid_type_storage = 0x09,
    nid_type_contents_table_index = 0x0A,
    nid_type_receive_folder_table = 0x0B,
    nid_type_outgoing_queue_table = 0x0C,
    nid_type_hierarchy_table = 0x0D,
    nid_type_contents_table = 0x0E,
    nid_type_associated_contents_table = 0x0F,
    nid_type_search_contents_table = 0x10,
    nid_type_attachment_table = 0x11,
    nid_type_recipient_table = 0x12,
    nid_type_search_table_index = 0x13,
    nid_type_contents_smp = 0x14,
    nid_type_associated_contents_smp = 0x15,
    nid_type_change_history_table = 0x16,
    nid_type_tombstone_table = 0x17,
    nid_type_tombstone_date_table = 0x18,
    nid_type_lrep_dups_table = 0x19,
    nid_type_folder_path_tombstone_table = 0x1A,
    nid_type_ltp = 0x1F,
    nid_type_max = 0x20
};

//! \brief The portion of a node_id reserved for the type
//! \sa [MS-PST] 2.2.2.1/nidType
//! \ingroup primitive
const ulong nid_type_mask = 0x1FL;

//! \brief Construct a node_id (NID) from a node type and index
//! \sa [MS-PST] 2.2.2.1
//! \ingroup primitive
#define make_nid(nid_type,nid_index) (((nid_type)&nid_type_mask)|((nid_index) << 5))

//! \brief Construct a folders node_id for an OST file
//! \ingroup primitive
#define make_prv_pub_nid(nid_index) (make_nid(nid_type_folder, nid_index_prv_pub_base + (nid_index)))

//! \brief The predefined nodes in a PST/OST file
//! \sa [MS-PST] 2.4.1
//! \ingroup primitive
enum predefined_nid
{
    nid_message_store = make_nid(nid_type_internal, 0x1),   //!< The property bag for this file
    nid_name_id_map = make_nid(nid_type_internal, 0x3),     //!< Contains the named prop mappings
    nid_normal_folder_template = make_nid(nid_type_folder, 0x6),
    nid_search_folder_template = make_nid(nid_type_search_folder, 0x7),
    nid_root_folder = make_nid(nid_type_folder, 0x9),       //!< Root folder of the store
    nid_search_management_queue = make_nid(nid_type_internal, 0xF),
    nid_search_activity_list = make_nid(nid_type_internal, 0x10),
    nid_search_domain_alternative = make_nid(nid_type_internal, 0x12),
    nid_search_domain_object = make_nid(nid_type_internal, 0x13),
    nid_search_gatherer_queue = make_nid(nid_type_internal, 0x14),
    nid_search_gatherer_descriptor = make_nid(nid_type_internal, 0x15),
    nid_table_rebuild_queue = make_nid(nid_type_internal, 0x17),
    nid_junk_mail_pihsl = make_nid(nid_type_internal, 0x18),
    nid_search_gatherer_folder_queue = make_nid(nid_type_internal, 0x19),
    nid_tc_sub_props = make_nid(nid_type_internal, 0x27),
    nid_index_template = 0x30,
    nid_hierarchy_table_template = make_nid(nid_type_hierarchy_table, nid_index_template),
    nid_contents_table_template = make_nid(nid_type_contents_table, nid_index_template),
    nid_associated_contents_table_template = make_nid(nid_type_associated_contents_table, nid_index_template),
    nid_search_contents_table_template = make_nid(nid_type_search_contents_table, nid_index_template),
    nid_smp_template = make_nid(nid_type_contents_smp, nid_index_template),
    nid_tombstone_table_template = make_nid(nid_type_tombstone_table, nid_index_template),
    nid_lrep_dups_table_template = make_nid(nid_type_lrep_dups_table, nid_index_template),
    nid_receive_folders = make_nid(nid_type_receive_folder_table, 0x31),
    nid_outgoing_queue = make_nid(nid_type_outgoing_queue_table, 0x32),
    nid_attachment_table = make_nid(nid_type_attachment_table, 0x33),
    nid_recipient_table = make_nid(nid_type_recipient_table, 0x34),
    nid_change_history_table = make_nid(nid_type_change_history_table, 0x35),
    nid_tombstone_table = make_nid(nid_type_tombstone_table, 0x36),
    nid_tombstone_date_table = make_nid(nid_type_tombstone_date_table, 0x37),
    nid_all_message_search_folder = make_nid(nid_type_search_folder, 0x39), //!< \deprecated The GUST
    nid_all_message_search_contents = make_nid(nid_type_search_contents_table, 0x39),
    nid_lrep_gmp = make_nid(nid_type_internal, 0x40),
    nid_lrep_folders_smp = make_nid(nid_type_internal, 0x41),
    nid_lrep_folders_table = make_nid(nid_type_internal, 0x42),
    nid_folder_path_tombstone_table = make_nid(nid_type_internal, 0x43),
    nid_hst_hmp = make_nid(nid_type_internal, 0x60),
    nid_index_prv_pub_base = 0x100,
    nid_pub_root_folder = make_prv_pub_nid(0),
    nid_prv_root_folder = make_prv_pub_nid(5),
    nid_criterr_notification = make_nid(nid_type_internal, 0x3FD),
    nid_object_notification = make_nid(nid_type_internal, 0x3FE),
    nid_newemail_notification = make_nid(nid_type_internal, 0x3FF),
    nid_extended_notification = make_nid(nid_type_internal, 0x400),
    nid_indexing_notification = make_nid(nid_type_internal, 0x401)
};

//! \brief Get a node type from a node id
//! \param[in] id The node id
//! \returns The node type
//! \sa [MS-PST] 2.2.2.1/nidType
//! \ingroup primitive
inline nid_type get_nid_type(node_id id)
    { return (nid_type)(id & nid_type_mask); }

//! \brief Get a node index from a node id
//! \param[in] id The node id
//! \returns The node index
//! \sa [MS-PST] 2.2.2.1/nidIndex
//! \ingroup primitive
inline ulong get_nid_index(node_id id)
    { return id >> 5; }

//
// heap id
//

//! \brief Get the heap page from the heap id
//! \param[in] id The heap id
//! \returns The heap page
//! \sa [MS-PST] 2.3.1.1/hidBlockIndex
//! \ingroup primitive
inline ulong get_heap_page(heap_id id)
    { return (id >> 16); } 

//! \brief Get the index from the heap id
//! \param[in] id The heap id
//! \returns The index
//! \sa [MS-PST] 2.3.1.1/hidIndex
//! \ingroup primitive
inline ulong get_heap_index(heap_id id)
    { return (((id >> 5) - 1) & 0x7FF); }

//! \brief Create a heap_id from a page and an index
//! \param[in] page The page
//! \param[in] index The index
//! \sa [MS-PST 2.3.1.1
//! \ingroup primitive
inline heap_id make_heap_id(ulong page, ulong index)
    { return (heap_id)((page << 16) | ((index + 1) << 5)); }

//
// heapnode id
//

//! \brief Inspects a heapnode_id (also known as a HNID) to determine if
//! it is a heap_id (HID)
//! \param[in] id The heapnode_id
//! \returns true if this is a heap_id
//! \sa [MS-PST] 2.3.3.2
//! \ingroup primitive
inline bool is_heap_id(heapnode_id id)
    { return (get_nid_type(id) == nid_type_none); }

//! \brief Inspects a heapnode_id (also known as a HNID) to determine if
//! it is a node_id (NID)
//! \param[in] id The heapnode_id
//! \returns true if this is a node_id of a subnode
//! \sa [MS-PST] 2.3.3.2
//! \ingroup primitive
inline bool is_subnode_id(heapnode_id id)
    { return (get_nid_type(id) != nid_type_none); }

//
// properties
//

//! \brief The different property types as defined by MAPI
//! \sa [MS-OXCDATA] 2.12.1
//! \ingroup primitive
enum prop_type
{
    prop_type_unspecified = 0,
    prop_type_null = 1,
    prop_type_short = 2,
    prop_type_mv_short = 4098,
    prop_type_long = 3,
    prop_type_mv_long = 4099,
    prop_type_float = 4,
    prop_type_mv_float = 4100,
    prop_type_double = 5,
    prop_type_mv_double = 4101,
    prop_type_currency = 6,
    prop_type_mv_currency = 4102,
    prop_type_apptime = 7,          //!< VT_DATE
    prop_type_mv_apptime = 4103,
    prop_type_error = 10,
    prop_type_boolean = 11,
    prop_type_object = 13,
    prop_type_longlong = 20,
    prop_type_mv_longlong = 4116,
    prop_type_string = 30,
    prop_type_mv_string = 4126,
    prop_type_wstring = 31,
    prop_type_mv_wstring = 4127,
    prop_type_systime = 64,         //!< Win32 FILETIME
    prop_type_mv_systime = 4160,
    prop_type_guid = 72,
    prop_type_mv_guid = 4168,
    prop_type_binary = 258,
    prop_type_mv_binary = 4354,
};

//
// mapi recipient type
//

//! \brief The different recipient types as defined by MAPI
//! \ingroup primitive
enum recipient_type
{
    mapi_to = 1,
    mapi_cc = 2,
    mapi_bcc = 3
};

// 
// message specific values
//

//! \brief A sentinel byte which indicates the message subject contains a prefix
//! \sa [MS-PST] 2.5.3.1.1.1
//! \ingroup primitive
const byte message_subject_prefix_lead_byte = 0x01;

//
// Win32 GUID
//

//! \brief A Win32 GUID structure
//! \ingroup primitive
struct guid
{
    ulong data1;
    short data2;
    short data3;
    byte data4[8];
} PSTSDK_MS_STRUCT;
//! \cond static_asserts
static_assert(sizeof(guid) == 16, "guid incorrect size");
//! \endcond

//! \brief The NULL guid
//! \ingroup primitive
const guid ps_none = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };

//! \brief The PS_MAPI guid
//! \sa [MS-OXPROPS] 1.3.2
//! \ingroup primitive
const guid ps_mapi = { 0x20328, 0, 0, { 0xc0, 0, 0, 0, 0, 0, 0, 0x46 } };

//! \brief The PS_PUBLIC_STRINGS guid
//! \sa [MS-OXPROPS] 1.3.2
//! \ingroup primitive
const guid ps_public_strings = { 0x20329, 0, 0, { 0xc0, 0, 0, 0, 0, 0, 0, 0x46 } };

} // end pstsdk namespace
#endif
