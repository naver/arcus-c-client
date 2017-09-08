/*
 * arcus-c-client : Arcus C client
 * Copyright 2010-2014 NAVER Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __LIBMEMCACHED_COLLECTION_H__
#define __LIBMEMCACHED_COLLECTION_H__

/**
 * Arcus collections API (b+tree, set, and list).
 */

/* Hard-coded constants */
#define MEMCACHED_COLL_MAX_EFLAGS_COUNT         100

#include "libmemcached/memcached.h"
/* eflag filter string length: 150 = (64*2)+22
 * refer to the filter format: " %u %s 0x%s %s 0x%s"
 */
#define MEMCACHED_COLL_ONE_FILTER_STR_LENGTH    150 /* 64*2 + 22 */
#define MEMCACHED_COLL_MAX_FILTER_STR_LENGTH    150 + (MEMCACHED_COLL_MAX_EFLAGS_COUNT-1)*63
/* update filter string length: 80 = 64+16
 * refer to the filter format: " %u %s 0x%s"
 */
#define MEMCACHED_COLL_UPD_FILTER_STR_LENGTH    80
#define MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH    31  /* server maximum, not client limitation */
#define MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH   MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH*2+1
#define MEMCACHED_COLL_MAX_ELEMENT_SIZE         4*1024  /* server maximum, not client limitation */
#define MEMCACHED_COLL_MAX_PIPED_CMD_SIZE       500
#define MEMCACHED_COLL_MAX_BOP_SMGET_KEY_COUNT  2000
#define MEMCACHED_COLL_MAX_BOP_SMGET_ELEM_COUNT 1000
#define MEMCACHED_COLL_MAX_PIPED_BUFFER_SIZE    10240
#define MEMCACHED_COLL_MAX_BOP_MGET_KEY_COUNT   200
#define MEMCACHED_COLL_MAX_BOP_MGET_ELEM_COUNT  50

/* Macros used within the library
 */
#define MEMCACHED_OPCODE_IS_MGET(ptr)           (ptr->last_op_code[4] == 'm')

#define ALLOCATE_ARRAY_OR_RETURN(root, ptr, type, count) \
        if (not ptr) { \
          ptr= static_cast<type*>(libmemcached_calloc(root, count, sizeof(type))); \
          if (not ptr) return MEMCACHED_MEMORY_ALLOCATION_FAILURE; \
        }

#define ALLOCATE_ARRAY_WITH_ERROR(root, ptr, type, count, error) \
        if (not ptr) { \
          ptr= static_cast<type*>(libmemcached_calloc(root, count, sizeof(type))); \
          if (not ptr) *error= MEMCACHED_MEMORY_ALLOCATION_FAILURE; \
        }

#define DEALLOCATE_ARRAY(root, ptr) \
        if (ptr) { \
          libmemcached_free(root, ptr); \
          ptr= NULL; \
        }

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  COLL_NONE=0,
  COLL_KV,
  COLL_LIST,
  COLL_SET,
  COLL_BTREE
} memcached_coll_type_t;

typedef enum {
  MEMCACHED_COLL_QUERY_LOP,
  MEMCACHED_COLL_QUERY_LOP_RANGE,
  MEMCACHED_COLL_QUERY_SOP,
  MEMCACHED_COLL_QUERY_BOP,
  MEMCACHED_COLL_QUERY_BOP_RANGE,
  MEMCACHED_COLL_QUERY_BOP_EXT,
  MEMCACHED_COLL_QUERY_BOP_EXT_RANGE,
  MEMCACHED_COLL_QUERY_UNKNOWN
} memcached_coll_sub_key_type_t;

/* order criteria of position in collection: ASC or DESC */
typedef enum {
  MEMCACHED_COLL_ORDER_ASC,
  MEMCACHED_COLL_ORDER_DESC
} memcached_coll_order_t;

LIBMEMCACHED_LOCAL
memcached_coll_type_t find_collection_type_by_opcode(const char *opcode);

struct memcached_hexadecimal_st {
  unsigned char *array;
  size_t length;

  struct {
    bool array_is_allocated:1;
  } options;
};

/**
 * Convert the numerical number in memcached_hexadecimal_st to a string.
 * @param ptr  memcached_hexadecimal_st that contains the numerical value.
 * @param buffer  string buffer. Upon success, it contains the string representation of the numerical value.
 * @param buffer_length  size of the string buffer.
 * @return string length.
 */
LIBMEMCACHED_API
size_t memcached_hexadecimal_to_str(memcached_hexadecimal_st *ptr, char *buffer, size_t buffer_length);

LIBMEMCACHED_LOCAL
int memcached_compare_two_hexadecimal(memcached_hexadecimal_st *lhs, memcached_hexadecimal_st *rhs);

LIBMEMCACHED_LOCAL
memcached_return_t memcached_conv_hex_to_str(memcached_st *ptr, memcached_hexadecimal_st *hex, char *str, size_t str_length);

LIBMEMCACHED_LOCAL
memcached_return_t memcached_conv_str_to_hex(memcached_st *ptr, char *str, size_t str_length, memcached_hexadecimal_st *hex);

typedef enum {
  OVERFLOWACTION_NONE=0,
  OVERFLOWACTION_ERROR,
  OVERFLOWACTION_HEAD_TRIM,
  OVERFLOWACTION_TAIL_TRIM,
  OVERFLOWACTION_SMALLEST_TRIM,
  OVERFLOWACTION_LARGEST_TRIM,
  OVERFLOWACTION_SMALLEST_SILENT_TRIM,
  OVERFLOWACTION_LARGEST_SILENT_TRIM
} memcached_coll_overflowaction_t;

union memcached_coll_sub_key_st{
  int32_t index;
  int32_t index_range[2];
  uint64_t bkey;
  uint64_t bkey_range[2];
  memcached_hexadecimal_st bkey_ext;
  memcached_hexadecimal_st bkey_ext_range[2];
};

/**
 * Collection item's attributes.
 */

/* All attributes in one structure. Most functions use this structure to
 * store/pass attributes.
 */
struct memcached_coll_attrs_st {
  uint32_t flags;
  int32_t expiretime;
  memcached_coll_type_t type;
  uint32_t count;
  uint32_t maxcount;
  memcached_coll_overflowaction_t overflowaction;
  bool readable;
  memcached_coll_sub_key_st minbkey;
  memcached_coll_sub_key_st maxbkey;
  memcached_coll_sub_key_st maxbkeyrange;
  uint32_t trimmed;

  struct {
    bool is_initialized:1;
    bool set_flags:1;
    bool set_expiretime:1;
    bool set_maxcount:1;
    bool set_overflowaction:1;
    bool set_readable:1;
    bool set_maxbkeyrange:1;
    memcached_coll_sub_key_type_t subkey_type;
  } options;
};

/**
 * Initialize the collection attributes structure.
 * @param attrs  collection attributes structure.
 */
LIBMEMCACHED_API
memcached_return_t memcached_coll_attrs_init(memcached_coll_attrs_st *attrs);

/**
 * Set the collection item's flags in the attributes.
 * @param attrs  collection attributes.
 * @param flags  collection item's flags.
 */
LIBMEMCACHED_API
memcached_return_t memcached_coll_attrs_set_flags(memcached_coll_attrs_st *attrs, uint32_t flags);

/**
 * Get the collection item's flags from the collection attributes.
 * @param attrs  collection attributes.
 * @return item's flags in the attributes.
 */
LIBMEMCACHED_API
uint32_t memcached_coll_attrs_get_flags(memcached_coll_attrs_st *attrs);

/**
 * Set the collection item's expiration time in the attributes.
 * @param attrs  collection attributes.
 * @param expiretime  collection item's expiration time. Refer to memcached's expiration time.
 */
LIBMEMCACHED_API
memcached_return_t memcached_coll_attrs_set_expiretime(memcached_coll_attrs_st *attrs, int32_t expiretime);

/**
 * Get the collection item's expiration time from the collection attributes.
 * @param attrs  collection attributes.
 * @return item's expiration time in the attributes.
 */
LIBMEMCACHED_API
int32_t memcached_coll_attrs_get_expiretime(memcached_coll_attrs_st *attrs);

/**
 * Set the collection item's maxcount (maximum element count) in the attributes.
 * @param attrs  collection attributes.
 * @param maxcount  collection item's maxcount.
 */
LIBMEMCACHED_API
memcached_return_t memcached_coll_attrs_set_maxcount(memcached_coll_attrs_st *attrs, uint32_t maxcount);

/**
 * Get the collection item's maxcount from the collection attributes.
 * @param attrs  collection attributes.
 * @return item's maxcount in the attributes.
 */
LIBMEMCACHED_API
uint32_t memcached_coll_attrs_get_maxcount(memcached_coll_attrs_st *attrs);

/**
 * Set the collection item's readable to "on" in the attributes.
 * @param attrs  collection attributes.
 */
LIBMEMCACHED_API
memcached_return_t memcached_coll_attrs_set_readable_on(memcached_coll_attrs_st *attrs);

/**
 * Get the collection item's readable from the collection attributes.
 * @param attrs  collection attributes.
 * @return item's readable attribute.
 */
LIBMEMCACHED_API
bool memcached_coll_attrs_is_readable(memcached_coll_attrs_st *attrs);

/**
 * Set the b+tree item's maxbkeyrange (maximum bkey range) in the attributes.
 * @param attrs  collection attributes.
 * @param maxbkeyrange  b+tree item's maximum bkey range.
 */
LIBMEMCACHED_API
memcached_return_t memcached_coll_attrs_set_maxbkeyrange(memcached_coll_attrs_st *attrs, uint32_t maxbkeyrange);

/**
 * Get the b+tree item's maximum bkey range from the collection attributes.
 * @param attrs  collection attributes.
 * @return b+tree's maximum bkey range.
 */
LIBMEMCACHED_API
uint32_t memcached_coll_attrs_get_maxbkeyrange(memcached_coll_attrs_st *attrs);

/**
 * Get the b+tree item's minbkey (minimum bkey) from the collection attributes.
 * @param attrs  collection attributes.
 * @return b+tree item's minimum bkey.
 */
LIBMEMCACHED_API
uint32_t memcached_coll_attrs_get_minbkey(memcached_coll_attrs_st *attrs);

/**
 * Get the b+tree item's maxbkey (maximum bkey) from the collection attributes.
 * @param attrs  collection attributes.
 * @return b+tree item's maximum bkey.
 */
LIBMEMCACHED_API
uint32_t memcached_coll_attrs_get_maxbkey(memcached_coll_attrs_st *attrs);

/**
 * Get the b+tree item's byte-array type minimum bkey from the collection attributes.
 * @param attrs  collection attributes.
 * @param bkey  buffer to hold the bkey, should be at least MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH bytes.
 * @param size  actual size of the bkey (the number of bytes), filled by the function.
 */
LIBMEMCACHED_API
memcached_return_t memcached_coll_attrs_get_minbkey_by_byte(memcached_coll_attrs_st *attrs, unsigned char **bkey, size_t *size);

/**
 * Get the b+tree item's byte-array type maximum bkey from the collection attributes.
 * @param attrs  collection attributes.
 * @param bkey  buffer to hold the bkey, should be at least MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH bytes.
 * @param size  actual size of the bkey (the number of bytes), filled by the function.
 */
LIBMEMCACHED_API
memcached_return_t memcached_coll_attrs_get_maxbkey_by_byte(memcached_coll_attrs_st *attrs, unsigned char **bkey, size_t *size);

/**
 * Set the b+tree item's byte-array type maximum bkey range in the attributes.
 * @param attrs  collection attributes.
 * @param maxbkeyrange  buffer holding the bkey range.
 * @param maxbkeyrange_size  size of the range, should be at most MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH bytes.
 */
LIBMEMCACHED_API
memcached_return_t memcached_coll_attrs_set_maxbkeyrange_by_byte(memcached_coll_attrs_st *attrs, unsigned char *maxbkeyrange, size_t maxbkeyrange_size);

/**
 * Get the b+tree item's byte-array type maximum bkey range from the collection attributes.
 * @param attrs  collection attributes.
 * @param maxbkeyrange  buffer to hold the bkey, should be at least MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH bytes.
 * @param maxbkeyrange_size  actual size of the bkey range (the number of bytes), filled by the function.
 */
LIBMEMCACHED_API
memcached_return_t memcached_coll_attrs_get_maxbkeyrange_by_byte(memcached_coll_attrs_st *attrs, unsigned char **maxbkeyrange, size_t *maxbkeyrange_size);

/**
 * Set the collection item's overflow action in the attributes.
 * @param attrs  collection attributes.
 * @param overflowaction  overflow action (OVERFLOWACTION_).
 */
LIBMEMCACHED_API
memcached_return_t memcached_coll_attrs_set_overflowaction(memcached_coll_attrs_st *attrs, memcached_coll_overflowaction_t overflowaction);

/**
 * Get the collection item's overflow action from the collection attributes.
 * @param attrs  collection attributes.
 * @return overflow action.
 */
LIBMEMCACHED_API
memcached_coll_overflowaction_t memcached_coll_attrs_get_overflowaction(memcached_coll_attrs_st *attrs);

/**
 * Get the collection item's trimmed attribute.
 * @param attrs  collection attributes.
 * @retval 0  not trimmed.
 * @retval 1  trimmed.
 */
LIBMEMCACHED_API
uint32_t memcached_coll_attrs_get_trimmed(memcached_coll_attrs_st *attrs);

/**
 * Attributes used when creating a collection item.
 * These are a subset of all attributes.
 */
struct memcached_coll_create_attrs_st {
  uint32_t flags;
  int32_t expiretime;
  uint32_t maxcount;
  memcached_coll_overflowaction_t overflowaction;
  bool is_unreadable;

  struct {
    bool is_initialized:1;
    bool set_flags:1;
    bool set_expiretime:1;
    bool set_maxcount:1;
    bool set_overflowaction:1;
    bool set_readable:1;
  } options;
};

/**
 * Initialize the attributes used for creating a collection item (collection create attributes).
 * @param attrs  collection create attributes.
 * @param flags  item's flags.
 * @param exptime  item's expiration time.
 * @param maxcount  item's maximum element count.
 */
LIBMEMCACHED_API
memcached_return_t memcached_coll_create_attrs_init(memcached_coll_create_attrs_st *attrs,
                                                    uint32_t flags, int32_t exptime, uint32_t maxcount);

/**
 * Set the collection item's flags in the collection create attributes.
 * @param attrs  collection create attributes.
 * @param flags  item's flags.
 */
LIBMEMCACHED_API
memcached_return_t memcached_coll_create_attrs_set_flags(memcached_coll_create_attrs_st *attrs, uint32_t flags);

/**
 * Set the collection item's expiration time in the collection create attributes.
 * @param attrs  collection create attributes.
 * @param exptime  item's expiration time.
 */
LIBMEMCACHED_API
memcached_return_t memcached_coll_create_attrs_set_expiretime(memcached_coll_create_attrs_st *attrs, int32_t exptime);

/**
 * Set the collection item's maximum element count in the collection create attributes.
 * @param attrs  collection create attributes.
 * @param maxcount  item's maximum element count.
 */
LIBMEMCACHED_API
memcached_return_t memcached_coll_create_attrs_set_maxcount(memcached_coll_create_attrs_st *attrs, uint32_t maxcount);

/**
 * Set the collection item's overflow action in the collection create attributes.
 * @param attrs  collection create attributes.
 * @param overflowaction  overflow action (OVERFLOWACTION_).
 */
LIBMEMCACHED_API
memcached_return_t memcached_coll_create_attrs_set_overflowaction(memcached_coll_create_attrs_st *attrs,
                                                                  memcached_coll_overflowaction_t overflowaction);

/**
 * Set the collection item's readable attribute to "on" or "off" in the collection create attributes.
 * @param attrs  collection create attributes.
 * @param is_unreadable  if true, set readable to be "off", otherwise "on".
 */
LIBMEMCACHED_API
memcached_return_t memcached_coll_create_attrs_set_unreadable(memcached_coll_create_attrs_st *attrs, bool is_unreadable);

/**
 * Common query parameters in a structure.  Some collection commands utilize
 * many parameters.  This structure is just a container for them.
 */
struct memcached_coll_query_st {
  memcached_coll_sub_key_type_t type;
  memcached_coll_sub_key_st sub_key;

  const char *value;
  size_t value_length;

  size_t offset;
  size_t count;

  memcached_coll_eflag_filter_st *eflag_filter;

  struct {
    bool is_initialized:1;
  } options;
};

/**
 * Initialize the query structure for a single list element.
 * @param ptr  query structure for collection items.
 * @param list_index  index of the list element.
 */
LIBMEMCACHED_API
memcached_return_t memcached_lop_query_init(memcached_coll_query_st *ptr,
                                            const int32_t list_index);

/**
 * Initialize the query structure for a range of list elements.
 * @param ptr  query structure for collection items.
 * @param list_index_from  first index of the list element range.
 * @param list_index_to  last index of the list element range.
 */
LIBMEMCACHED_API
memcached_return_t memcached_lop_range_query_init(memcached_coll_query_st *ptr,
                                                  const int32_t list_index_from, const int32_t list_index_to);

/**
 * Initialize the query structure for elements in the set item.
 * @param ptr  query structure for collection items.
 * @param count  number of elements.
 */
LIBMEMCACHED_API
memcached_return_t memcached_sop_query_init(memcached_coll_query_st *ptr,
                                            size_t count);

/**
 * Initialize the query structure for a set element using the element value.
 * @param ptr  query structure for collection items.
 * @param value  buffer holding the element's value.
 * @param value_length  length of the element's value.
 */
LIBMEMCACHED_API
memcached_return_t memcached_sop_value_query_init(memcached_coll_query_st *ptr,
                                                  const char *value, size_t value_length);

/**
 * Initialize the query structure for a b+tree element.
 * @param ptr  query structure for collection items.
 * @param bkey  element's bkey.
 * @param eflag_filter  optional eflag filter, maybe NULL.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_query_init(memcached_bop_query_st *ptr, const uint64_t bkey,
                                            memcached_coll_eflag_filter_st *eflag_filter);

/**
 * Initialize the query structure for a range of b+tree elements.
 * @param ptr  query structure for collection items.
 * @param bkey_from  first bkey in the range.
 * @param bkey_to  last bkey in the range.
 * @param eflag_filter  optional eflag filter, maybe NULL.
 * @param offset  start offset within the range.
 * @param count  number of elements.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_range_query_init(memcached_bop_query_st *ptr,
                                                  const uint64_t bkey_from, const uint64_t bkey_to,
                                                  memcached_coll_eflag_filter_st *eflag_filter,
                                                  size_t offset, size_t count);

/**
 * Initialize the query structure for a b+tree element using byte-array type bkey.
 * @param ptr  query structure for collection items.
 * @param bkey  byte-array bkey.
 * @param bkey_length  bkey length (number of bytes).
 * @param eflag_filter  optional eflag filter. maybe NULL.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_ext_query_init(memcached_bop_query_st *ptr,
                                                const unsigned char *bkey, const size_t bkey_length,
                                                memcached_coll_eflag_filter_st *eflag_filter);

/**
 * Initialize the query structure for a range of b+tree elements using byte-array type bkey.
 * @param ptr  query structure for collection items.
 * @param bkey_from  start byte-array bkey in the range.
 * @param bkey_from_length  start bkey's length (number of bytes).
 * @param bkey_to  last byte-array bkey in the range.
 * @param bkey_to_length  last bkey's length (number of bytes).
 * @param eflag_filter  optional eflag filter. maybe NULL.
 * @param offset  start offset within the range.
 * @param count  number of elements.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_ext_range_query_init(memcached_bop_query_st *ptr,
                                                      const unsigned char *bkey_from, size_t bkey_from_length,
                                                      const unsigned char *bkey_to, size_t bkey_to_length,
                                                      memcached_coll_eflag_filter_st *eflag_filter,
                                                      size_t offset, size_t count);

/**
 * B+tree item support eflag (element flag).  It is an arbitrary byte array
 * associated with the element.  B+tree commands can use eflag as well as
 * bkey when searching for elements.  The application can tell the memcached
 * server to perform certain logical or bitwise operations on eflag and
 * to select only those elements that meet the given condition.  This is
 * called "filtering".  Operators and associated data are called eflag filter.
 *
 * For details, refer to the Arcus memcached protocol manual.
 */

typedef enum {
  MEMCACHED_COLL_BITWISE_AND,
  MEMCACHED_COLL_BITWISE_OR,
  MEMCACHED_COLL_BITWISE_XOR
} memcached_coll_bitwise_t;

typedef enum {
  MEMCACHED_COLL_COMP_EQ,
  MEMCACHED_COLL_COMP_NE,
  MEMCACHED_COLL_COMP_LT,
  MEMCACHED_COLL_COMP_LE,
  MEMCACHED_COLL_COMP_GT,
  MEMCACHED_COLL_COMP_GE
} memcached_coll_comp_t;

struct memcached_coll_eflag_filter_st {
  size_t fwhere;
  size_t flength;

  struct {
    memcached_coll_bitwise_t op;
    memcached_hexadecimal_st foperand;
  } bitwise;

  struct {
    memcached_coll_comp_t op;
    memcached_hexadecimal_st fvalue[MEMCACHED_COLL_MAX_EFLAGS_COUNT];
    size_t count;
  } comp;

  struct {
    bool is_initialized:1;
    bool is_bitwised:1;
  } options;
};

/**
 * Initialize the b+tree eflag filter.
 * For details, refer to the Arcus memcached protocol manual.
 * @param ptr  eflag filter structure.
 * @param fwhere  eflag offset where the filter performs the comparison.
 * @param fvalue  value to use in the comparison.
 * @param fvalue_length  value length (number of bytes).
 * @param comp_op  comparison operator.
 */
LIBMEMCACHED_API
memcached_return_t memcached_coll_eflag_filter_init(memcached_coll_eflag_filter_st *ptr,
                                                    const size_t fwhere,
                                                    const unsigned char *fvalue, const size_t fvalue_length,
                                                    memcached_coll_comp_t comp_op);

/**
 * Initialize the b+tree eflag filter using multiple comparison values.
 * For details, refer to the Arcus memcached protocol manual.
 * @param ptr  eflag filter structure.
 * @param fwhere  eflag offset where the filter performs the comparison.
 * @param fvalues  array of values to use in the comparison.
 * @param fvalue_length  value length (number of bytes).
 * @param fvalue_count  number of values in the value array.
 * @param comp_op  comparison operator.
 */
LIBMEMCACHED_API
memcached_return_t memcached_coll_eflags_filter_init(memcached_coll_eflag_filter_st *ptr,
                                                    const size_t fwhere,
                                                    const unsigned char *fvalues,
                                                    const size_t fvalue_length,
                                                    const size_t fvalue_count,
                                                    memcached_coll_comp_t comp_op);

/**
 * Initialize the b+tree eflag filter for bitwise operation.
 * For details, refer to the Arcus memcached protocol manual.
 * @param ptr  eflag filter structure.
 * @param foperand  operand value.
 * @param foperand_length  operand length (number of bytes).
 * @param bitwise_op  bitwise operator.
 */
LIBMEMCACHED_API
memcached_return_t memcached_coll_eflag_filter_set_bitwise(memcached_coll_eflag_filter_st *ptr,
                                                           const unsigned char *foperand, const size_t foperand_length,
                                                           memcached_coll_bitwise_t bitwise_op);

/**
 * B+tree eflag filter used for certain update commands.
 * This is just for convinience.
 */
struct memcached_coll_update_filter_st {
  size_t fwhere;
  size_t flength;

  struct {
    memcached_coll_bitwise_t op;
    memcached_hexadecimal_st foperand;
  } bitwise;

  struct {
    memcached_coll_comp_t op;
    memcached_hexadecimal_st fvalue;
  } comp;

  struct {
    bool is_initialized:1;
    bool is_bitwised:1;
  } options;
};

/**
 * Initialize the b+tree eflag filter used for update commands.
 * For details, refer to the Arcus memcached protocol manual.
 * @param ptr  filter structure.
 * @param fvalue  comparison value.
 * @param fvalue_length  value length (number of bytes).
 */
LIBMEMCACHED_API
memcached_return_t memcached_coll_update_filter_init(memcached_coll_update_filter_st *ptr,
                                                     const unsigned char *fvalue, const size_t fvalue_length);

/**
 * Initialize the b+tree eflag filter used for update commands.
 * For details, refer to the Arcus memcached protocol manual.
 * @param ptr  filter structure.
 * @param fwhere  eflag offset.
 * @param bitwise_op  bitwise operator.
 */
LIBMEMCACHED_API
memcached_return_t memcached_coll_update_filter_set_bitwise(memcached_coll_update_filter_st *ptr,
                                                            const size_t fwhere, memcached_coll_bitwise_t bitwise_op);

/* COLLECTION APIs */

/**
 * Attributes API.
 */

/**
 * Set the collection item's attributes.
 * @param ptr  memcached handle.
 * @param key  item key.
 * @param key_length  key length (number of bytes).
 * @param attrs  attributes.
 */
LIBMEMCACHED_API
memcached_return_t memcached_set_attrs(memcached_st *ptr,
                                       const char *key, size_t key_length,
                                       memcached_coll_attrs_st *attrs);

/**
 * Get the collection item's attributes.
 * @param ptr  memcached handle.
 * @param key  item key.
 * @param key_length  key length (number of bytes).
 * @param attrs  attributes, filled upon successful return.
 */
LIBMEMCACHED_API
memcached_return_t memcached_get_attrs(memcached_st *ptr,
                                       const char *key, size_t key_length,
                                       memcached_coll_attrs_st *attrs);

/**
 * List API.
 */

/**
 * Insert an element into the list item.
 * Optionally create the item if it does not exist.
 * @param ptr  memcached handle.
 * @param key  list item's key.
 * @param key_length  key length (number of bytes).
 * @param list_index  element index.
 * @param value  buffer holding the element value.
 * @param value_length  length of the element value (number of bytes).
 * @param attrs  if not NULL, create the item using the attributes if the item does not exist.
 */
LIBMEMCACHED_API
memcached_return_t memcached_lop_insert(memcached_st *ptr, const char *key, size_t key_length,
                                        const int32_t list_index,
                                        const char *value, size_t value_length,
                                        memcached_coll_create_attrs_st *attributes);

/**
 * Delete an element from the list item.
 * Optionally delete the item if it becomes empty.
 * @param ptr  memcached handle.
 * @param key  list item's key.
 * @param key_length  key length (number of bytes).
 * @param list_index  element index.
 * @param drop_if_empty  if true, delete the empty list item.
 */
LIBMEMCACHED_API
memcached_return_t memcached_lop_delete(memcached_st *ptr, const char *key, size_t key_length,
                                        const int32_t list_index, bool drop_if_empty);

/**
 * Delete a range of elements from the list item.
 * Optionally delete the item if it becomes empty.
 * @param ptr  memcached handle.
 * @param key  list item's key.
 * @param key_length  key length (number of bytes).
 * @param from  first index in the range.
 * @param to  last index in the range.
 * @param drop_if_empty  if true, delete the empty list item.
 */
LIBMEMCACHED_API
memcached_return_t memcached_lop_delete_by_range(memcached_st *ptr, const char *key, size_t key_length,
                                                 const int32_t from, const int32_t to, bool drop_if_empty);

/**
 * Fetch an element from the list item.
 * Optionally delete the element and delete the item if it becomes empty.
 * @param ptr  memcached handle.
 * @param key  list item's key.
 * @param key_length  key length (number of bytes).
 * @param list_index  element index.
 * @param with_delete  if true, delete the element.
 * @param drop_if_empty  if true, delete the empty list item.
 * @param result  result structure, filled upon return.
 */
LIBMEMCACHED_API
memcached_return_t memcached_lop_get(memcached_st *ptr, const char *key, size_t key_length,
                                     const int32_t list_index, bool with_delete, bool drop_if_empty,
                                     memcached_coll_result_st *result);

/**
 * Fetch a range of elements from the list item.
 * Optionally delete the elements and delete the item if it becomes empty.
 * @param ptr  memcached handle.
 * @param key  list item's key.
 * @param key_length  key length (number of bytes).
 * @param from  first index in the range.
 * @param to  last index in the range.
 * @param with_delete  if true, delete the elements.
 * @param drop_if_empty  if true, delete the empty list item.
 * @param result  result structure, filled upon return.
 */
LIBMEMCACHED_API
memcached_return_t memcached_lop_get_by_range(memcached_st *ptr, const char *key, size_t key_length,
                                              const int32_t from, const int32_t to,
                                              bool with_delete, bool drop_if_empty,
                                              memcached_coll_result_st *result);

/**
 * Set API.
 */

/**
 * Insert an element into the set item.
 * Optionally create the item if it does not exist.
 * @param ptr  memcached handle.
 * @param key  set item's key.
 * @param key_length  key length (number of bytes).
 * @param value  buffer holding the element value.
 * @param value_length  length of the element value (number of bytes).
 * @param attrs  if not NULL, create the item using the attributes if the item does not exist.
 */
LIBMEMCACHED_API
memcached_return_t memcached_sop_insert(memcached_st *ptr, const char *key, size_t key_length,
                                        const char *value, size_t value_length,
                                        memcached_coll_create_attrs_st *attributes);

/**
 * Delete an element from the set item.
 * Optionally delete the item if it becomes empty.
 * @param ptr  memcached handle.
 * @param key  set item's key.
 * @param key_length  key length (number of bytes).
 * @param value  buffer holding the element value.
 * @param value_length  length of the element value (number of bytes).
 * @param drop_if_empty  if true, delete the empty list item.
 */
LIBMEMCACHED_API
memcached_return_t memcached_sop_delete(memcached_st *ptr, const char *key, size_t key_length,
                                        const char *value, size_t value_length, bool drop_if_empty);

/**
 * Check the existence of an element in the set item.
 * @param ptr  memcached handle.
 * @param key  set item's key.
 * @param key_length  key length (number of bytes).
 * @param value  buffer holding the element value.
 * @param value_length  length of the element value (number of bytes).
 */
LIBMEMCACHED_API
memcached_return_t memcached_sop_exist(memcached_st *ptr, const char *key, size_t key_length,
                                       const char *value, size_t value_length);

/**
 * Fetch a number of elements from the set item.
 * Optionally delete the elements and the item if it becomes empty.
 * @param ptr  memcached handle.
 * @param key  set item's key.
 * @param key_length  key length (number of bytes).
 * @param count  number of elements to fetch.
 * @param with_delete  if true, delete the elements.
 * @param drop_if_empty  if true, delete the empty list item.
 * @param result  result structure, filled upon return.
 */
LIBMEMCACHED_API
memcached_return_t memcached_sop_get(memcached_st *ptr, const char *key, size_t key_length,
                                     size_t count, bool with_delete, bool drop_if_empty,
                                     memcached_coll_result_st *result);

/**
 * B+tree API.
 */

/**
 * Insert an element into the b+tree item.
 * Optionally create the item if it does not exist.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param bkey  element bkey.
 * @param eflag  optional eflag, maybe NULL.
 * @param eflag_length  eflag length (number of bytes).
 * @param value  buffer holding the element value.
 * @param value_length  length of the element value (number of bytes).
 * @param attrs  if not NULL, create the item using the attributes if the item does not exist.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_insert(memcached_st *ptr, const char *key, size_t key_length,
                                        const uint64_t bkey,
                                        const unsigned char *eflag, size_t eflag_length,
                                        const char *value, size_t value_length,
                                        memcached_coll_create_attrs_st *attrs);

/**
 * Insert an element into the b+tree item using byte-array bkey.
 * Optionally create the item if it does not exist.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param bkey  byte-array bkey.
 * @param bkey_length  bkey length (number of bytes).
 * @param eflag  optional eflag, maybe NULL.
 * @param eflag_length  eflag length (number of bytes).
 * @param value  buffer holding the element value.
 * @param value_length  length of the element value (number of bytes).
 * @param attrs  if not NULL, create the item using the attributes if the item does not exist.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_ext_insert(memcached_st *ptr, const char *key, size_t key_length,
                                            const unsigned char *bkey, size_t bkey_length,
                                            const unsigned char *eflag, size_t eflag_length,
                                            const char *value, size_t value_length,
                                            memcached_coll_create_attrs_st *attrs);

/**
 * Upsert (update the element if it exists, insert otherwise) an element into the b+tree item.
 * Optionally create the item if it does not exist.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param bkey  element bkey.
 * @param eflag  optional eflag, maybe NULL.
 * @param eflag_length  eflag length (number of bytes).
 * @param value  buffer holding the element value.
 * @param value_length  length of the element value (number of bytes).
 * @param attrs  if not NULL, create the item using the attributes if the item does not exist.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_upsert(memcached_st *ptr, const char *key, size_t key_length,
                                        const uint64_t bkey,
                                        const unsigned char *eflag, size_t eflag_length,
                                        const char *value, size_t value_length,
                                        memcached_coll_create_attrs_st *attrs);

/**
 * Upsert (update the element if it exists, insert otherwise) an element into the b+tree item using byte-array bkey.
 * Optionally create the item if it does not exist.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param bkey  byte-array bkey.
 * @param bkey_length  bkey length (number of bytes).
 * @param eflag  optional eflag, maybe NULL.
 * @param eflag_length  eflag length (number of bytes).
 * @param value  buffer holding the element value.
 * @param value_length  length of the element value (number of bytes).
 * @param attrs  if not NULL, create the item using the attributes if the item does not exist.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_ext_upsert(memcached_st *ptr, const char *key, size_t key_length,
                                            const unsigned char *bkey, size_t bkey_length,
                                            const unsigned char *eflag, size_t eflag_length,
                                            const char *value, size_t value_length,
                                            memcached_coll_create_attrs_st *attrs);

/**
 * Update the existing element in the b+tree item.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param bkey  element bkey.
 * @param update_filter  optional eflag filter, maybe NULL.
 * @param value  buffer holding the element value.
 * @param value_length  length of the element value (number of bytes).
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_update(memcached_st *ptr, const char *key, size_t key_length,
                                        const uint64_t bkey,
                                        memcached_coll_update_filter_st *update_filter,
                                        const char *value, size_t value_length);

/**
 * Update the existing element in the b+tree item using byte-array bkey.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param bkey  byte-array bkey.
 * @param bkey_length  bkey length (number of bytes).
 * @param update_filter  optional eflag filter, maybe NULL.
 * @param value  buffer holding the element value.
 * @param value_length  length of the element value (number of bytes).
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_ext_update(memcached_st *ptr, const char *key, size_t key_length,
                                            const unsigned char *bkey, size_t bkey_length,
                                            memcached_coll_update_filter_st *update_filter,
                                            const char *value, size_t value_length);

/**
 * Delete an element from the b+tree item.
 * Optionally delete the item if it becomes empty.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param bkey  element bkey.
 * @param eflag_filter  optional eflag filter, maybe NULL.
 * @param drop_if_empty  if true, delete the empty list item.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_delete(memcached_st *ptr, const char *key, size_t key_length,
                                        const uint64_t bkey, memcached_coll_eflag_filter_st *eflag_filter,
                                        bool drop_if_empty);

/**
 * Delete a range of elements from the b+tree item.
 * Optionally delete the item if it becomes empty.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param from  first bkey in the range.
 * @param to  last bkey in the range.
 * @param eflag_filter  optional eflag filter, maybe NULL.
 * @param count  number of elements to delete.
 * @param drop_if_empty  if true, delete the empty list item.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_delete_by_range(memcached_st *ptr, const char *key, size_t key_length,
                                                 const uint64_t from, const uint64_t to,
                                                 memcached_coll_eflag_filter_st *eflag_filter, size_t count,
                                                 bool drop_if_empty);

/**
 * Delete an element from the b+tree item using byte-array bkey.
 * Optionally delete the item if it becomes empty.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param bkey  byte-array bkey.
 * @param bkey_length  bkey length (number of bytes).
 * @param eflag_filter  optional eflag filter, maybe NULL.
 * @param drop_if_empty  if true, delete the empty list item.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_ext_delete(memcached_st *ptr, const char *key, size_t key_length,
                                            const unsigned char *bkey, size_t bkey_length,
                                            memcached_coll_eflag_filter_st *eflag_filter,
                                            bool drop_if_empty);

/**
 * Delete a range of elements from the b+tree item using byte-array bkey.
 * Optionally delete the item if it becomes empty.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param from  first byte-array bkey in the range.
 * @param from_length  length of the first byte-array bkey (number of bytes).
 * @param to  last byte-array bkey in the range.
 * @param to_length  length of the first byte-array bkey (number of bytes).
 * @param eflag_filter  optional eflag filter, maybe NULL.
 * @param count  number of elements to delete.
 * @param drop_if_empty  if true, delete the empty list item.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_ext_delete_by_range(memcached_st *ptr, const char *key, size_t key_length,
                                                     const unsigned char *from, size_t from_length,
                                                     const unsigned char *to, size_t to_length,
                                                     memcached_coll_eflag_filter_st *eflag_filter,
                                                     size_t count, bool drop_if_empty);

/**
 * Increment the value of an existing element in the b+tree item.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param bkey  element bkey.
 * @param delta  increment amount.
 * @param value  incremented value, filled upon return.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_incr(memcached_st *ptr, const char *key, size_t key_length,
                                      const uint64_t bkey, const uint64_t delta, uint64_t *value);

/**
 * Increment the value of an existing element in the b+tree item using byte-array bkey.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param bkey  byte-array bkey.
 * @param bkey_length  bkey length (number of bytes).
 * @param delta  increment amount.
 * @param value  incremented value, filled upon return.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_ext_incr(memcached_st *ptr, const char *key, size_t key_length,
                                          const unsigned char *bkey, size_t bkey_length,
                                          const uint64_t delta, uint64_t *value);

/**
 * Decrement the value of an existing element in the b+tree item.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param bkey  element bkey.
 * @param delta  decrement amount.
 * @param value  decremented value, filled upon return.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_decr(memcached_st *ptr, const char *key, size_t key_length,
                                      const uint64_t bkey, const uint64_t delta, uint64_t *value);

/**
 * Decrement the value of an existing element in the b+tree item using byte-array bkey.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param bkey  byte-array bkey.
 * @param bkey_length  bkey length (number of bytes).
 * @param delta  decrement amount.
 * @param value  decremented value, filled upon return.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_ext_decr(memcached_st *ptr, const char *key, size_t key_length,
                                          const unsigned char *bkey, size_t bkey_length,
                                          const uint64_t delta, uint64_t *value);

/**
 * Fetch an element from the b+tree item.
 * Optionally delete the element and the item if it becomes empty.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param bkey  element bkey.
 * @param eflag_filter  optional eflag filter, maybe NULL.
 * @param with_delete  if true, delete the elements.
 * @param drop_if_empty  if true, delete the empty list item.
 * @param result  result structure, filled upon return.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_get(memcached_st *ptr, const char *key, size_t key_length,
                                     const uint64_t bkey,
                                     memcached_coll_eflag_filter_st *eflag_filter,
                                     bool with_delete, bool drop_if_empty,
                                     memcached_coll_result_st *result);

/**
 * Fetch a range of elements from the b+tree item.
 * Optionally delete the elements and the item if it becomes empty.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param from  start bkey in the range.
 * @param to  last bkey in the range.
 * @param eflag_filter  optional eflag filter, maybe NULL.
 * @param offset  start offset within the range.
 * @param count  number of elements.
 * @param with_delete  if true, delete the elements.
 * @param drop_if_empty  if true, delete the empty list item.
 * @param result  result structure, filled upon return.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_get_by_range(memcached_st *ptr, const char *key, size_t key_length,
                                              const uint64_t from, const uint64_t to,
                                              memcached_coll_eflag_filter_st *eflag_filter,
                                              const size_t offset, const size_t count,
                                              bool with_delete, bool drop_if_empty,
                                              memcached_coll_result_st *result);

/**
 * Fetch an element from the b+tree item using byte-array bkey.
 * Optionally delete the element and the item if it becomes empty.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param bkey  byte-array bkey.
 * @param bkey_length  bkey length (number of bytes).
 * @param eflag_filter  optional eflag filter, maybe NULL.
 * @param with_delete  if true, delete the elements.
 * @param drop_if_empty  if true, delete the empty list item.
 * @param result  result structure, filled upon return.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_ext_get(memcached_st *ptr, const char *key, size_t key_length,
                                         const unsigned char *bkey, size_t bkey_length,
                                         memcached_coll_eflag_filter_st *eflag_filter,
                                         bool with_delete, bool drop_if_empty,
                                         memcached_coll_result_st *result);

/**
 * Fetch a range of elements from the b+tree item using byte-array bkey.
 * Optionally delete the elements and the item if it becomes empty.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param from  first byte-array bkey in the range.
 * @param from_length  length of the first byte-array bkey (number of bytes).
 * @param to  last byte-array bkey in the range.
 * @param to_length  length of the first byte-array bkey (number of bytes).
 * @param eflag_filter  optional eflag filter, maybe NULL.
 * @param offset  start offset within the range.
 * @param count  number of elements.
 * @param with_delete  if true, delete the elements.
 * @param drop_if_empty  if true, delete the empty list item.
 * @param result  result structure, filled upon return.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_ext_get_by_range(memcached_st *ptr, const char *key, size_t key_length,
                                                  const unsigned char *from, size_t from_length,
                                                  const unsigned char *to, size_t to_length,
                                                  memcached_coll_eflag_filter_st *eflag_filter,
                                                  const size_t offset, const size_t count,
                                                  bool with_delete, bool drop_if_empty,
                                                  memcached_coll_result_st *result);

/**
 * Fetch an element from the b+tree item using the query structure.
 * Optionally delete the element and the item if it becomes empty.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param query  query structure containing bkey and other fields necessary for the fetch operation.
 * @param with_delete  if true, delete the elements.
 * @param drop_if_empty  if true, delete the empty list item.
 * @param result  result structure, filled upon return.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_get_by_query(memcached_st *ptr, const char *key, size_t key_length,
                                              memcached_bop_query_st *query,
                                              bool with_delete, bool drop_if_empty,
                                              memcached_coll_result_st *result);

/**
 * Fetch elements from multiple b+tree items (mget or multi-get).
 * This function sends requests to memcached servers. To retrieve elements, call memcached_coll_fetch_result.
 * @param ptr  memcached handle.
 * @param keys  array of item keys.
 * @param key_length  array of item key lengths (number of bytes).
 * @param number_of_keys  number of keys in the key array.
 * @param query  query structure containing bkey and other optional fields.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_mget(memcached_st *ptr,
                                      const char * const *keys, const size_t *key_length,
                                      size_t number_of_keys,
                                      memcached_coll_query_st *query);

/**
 * Retrieve elements previously requested by mget.
 * This function sends requests. To retrieve elements, call memcached_coll_fetch_result.
 * @param ptr  memcached handle.
 * @param result  optional result structure, if NULL, the function allocates one.
 * @param error  error code.
 * @return result structure, filled with elements.
 */
LIBMEMCACHED_API
memcached_coll_result_st *memcached_coll_fetch_result(memcached_st *ptr,
                                                      memcached_coll_result_st *result,
                                                      memcached_return_t *error);

/**
 * Count the number of b+tree elements that match the given bkey and eflag filter.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param bkey  element bkey.
 * @param eflag_filter  eflag filter.
 * @param count  number of elements, filled upon return.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_count(memcached_st *ptr,
                                       const char *key, size_t key_length,
                                       const uint64_t bkey,
                                       memcached_coll_eflag_filter_st *eflag_filter,
                                       size_t *count);

/**
 * Count the number of b+tree elements that are in the given bkey range and match the eflag filter.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param from  first bkey in the range.
 * @param last  last bkey in the range.
 * @param eflag_filter  eflag filter.
 * @param count  number of elements, filled upon return.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_count_by_range(memcached_st *ptr, const char *key, size_t key_length,
                                                const uint64_t from, const uint64_t to,
                                                memcached_coll_eflag_filter_st *eflag_filter,
                                                size_t *count);

/**
 * Count the number of b+tree elements that match the given byte-array bkey and eflag filter.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param bkey  byte-array bkey.
 * @param bkey_length  bkey length (number of bytes).
 * @param eflag_filter  eflag filter.
 * @param count  number of elements, filled upon return.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_ext_count(memcached_st *ptr, const char *key, size_t key_length,
                                           const unsigned char *bkey, size_t bkey_length,
                                           memcached_coll_eflag_filter_st *eflag_filter,
                                           size_t *count);

/**
 * Count the number of b+tree elements that are in the given byte-array bkey range and match the eflag filter.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param from  first byte-array bkey in the range.
 * @param from_length  length of the first byte-array bkey (number of bytes).
 * @param to  last byte-array bkey in the range.
 * @param to_length  length of the first byte-array bkey (number of bytes).
 * @param eflag_filter  eflag filter.
 * @param count  number of elements, filled upon return.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_ext_count_by_range(memcached_st *ptr, const char *key, size_t key_length,
                                                    const unsigned char *from, size_t from_length,
                                                    const unsigned char *to, size_t to_length,
                                                    memcached_coll_eflag_filter_st *eflag_filter,
                                                    size_t *count);

/**
 * Create an empty list item.
 * @param ptr  memcached handle.
 * @param key  list item's key.
 * @param key_length  key length (number of bytes).
 * @param attrs  item's attributes.
 */
LIBMEMCACHED_API
memcached_return_t memcached_lop_create(memcached_st *ptr, const char *key, size_t key_length,
                                        memcached_coll_create_attrs_st *attrs);

/**
 * Create an empty set item.
 * @param ptr  memcached handle.
 * @param key  set item's key.
 * @param key_length  key length (number of bytes).
 * @param attrs  item's attributes.
 */
LIBMEMCACHED_API
memcached_return_t memcached_sop_create(memcached_st *ptr, const char *key, size_t key_length,
                                        memcached_coll_create_attrs_st *attrs);

/**
 * Create an empty b+tree item.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param attrs  item's attributes.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_create(memcached_st *ptr, const char *key, size_t key_length,
                                        memcached_coll_create_attrs_st *attrs);

/**
 * Fetch and sort-merge elements from multiple b+tree items (smget or sort-merge get).
 * @param ptr  memcached handle.
 * @param keys  array of item keys.
 * @param key_length  array of item key lengths (number of bytes).
 * @param number_of_keys  number of keys in the key array.
 * @param query  query structure containing bkey and other optional fields.
 * @param result  result structure, filled upon return.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_smget(memcached_st *ptr,
                                       const char * const *keys,
                                       const size_t *key_length, size_t number_of_keys,
                                       memcached_bop_query_st *query,
                                       memcached_coll_smget_result_st *result);

/* Used within the library. Do not call this from the application. */
LIBMEMCACHED_LOCAL
memcached_coll_smget_result_st *memcached_coll_smget_fetch_result(memcached_st *ptr,
                                                                  memcached_coll_smget_result_st *result,
                                                                  memcached_return_t *error,
                                                                  memcached_coll_type_t type);

/*
 * B+Tree position API
 */

/**
 * Find the position of an element found in b+tree item
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param bkey  element bkey.
 * @param order order criteria of position, asc or desc.
 * @param position element position, filled upon return.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_find_position(memcached_st *ptr, const char *key, size_t key_length,
                                               const uint64_t bkey,
                                               memcached_coll_order_t order,
                                               size_t *position);

/**
 * Find the position of an element found in b+tree item using byte-array bkey.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param bkey  byte-array bkey.
 * @param bkey_length  bkey length (number of bytes).
 * @param order order criteria of position, asc or desc.
 * @param position element position, filled upon return.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_ext_find_position(memcached_st *ptr, const char *key, size_t key_length,
                                                   const unsigned char *bkey, size_t bkey_length,
                                                   memcached_coll_order_t order,
                                                   size_t *position);

/**
 * Get the elements of given position range from the b+tree item.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param order order criteria of position, asc or desc.
 * @param from_position beginning position
 * @param to_position end position
 * @param result  result structure, filled upon return.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_get_by_position(memcached_st *ptr,
                                                 const char *key, size_t key_length,
                                                 memcached_coll_order_t order,
                                                 size_t from_position, size_t to_position,
                                                 memcached_coll_result_st *result);

/**
 * Find the position of given bkey in the b+tree item and get elements residing near the position.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param bkey  element bkey.
 * @param order order criteria of position, asc or desc.
 * @param count number of elements to be retrieved in both direction of the found position. (0 means the element of given bkey)
 * @param result  result structure, filled upon return.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_find_position_with_get(memcached_st *ptr,
                                                 const char *key, size_t key_length,
                                                 const uint64_t bkey,
                                                 memcached_coll_order_t order, size_t count,
                                                 memcached_coll_result_st *result);

/**
 * Find the position of given byte-array bkey in the b+tree item and get elements residing near the position.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param bkey  byte-array bkey.
 * @param bkey_length  bkey length (number of bytes).
 * @param order order criteria of position, asc or desc.
 * @param count number of elements to be retrieved in both direction of the found position. (0 means the element of given bkey)
 * @param result  result structure, filled upon return.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_ext_find_position_with_get(memcached_st *ptr,
                                                 const char *key, size_t key_length,
                                                 const unsigned char *bkey, size_t bkey_length,
                                                 memcached_coll_order_t order, size_t count,
                                                 memcached_coll_result_st *result);

/**
 * Pipelined insertions.
 * When inserting many elements, it is more efficient to use pipelined API
 * than inserting one element at a time.
 *
 * Below, number_of_piped_items indicates the number of elements.
 * Element i's value is in values[i], length in values_length[i],
 * index in list_indexes[i], bkey in bkeys[i], and so on.
 *
 * These functions return one return code for each element and also the code
 * for the entire pipelined insertion operation. For details, refer to the
 * Arcus memcached protocol manual.
 */

/**
 * Insert multiple elements into the list item in a pipelined fashion.
 * @param ptr  memcached handle.
 * @param key  list item's key.
 * @param key_length  key length (number of bytes).
 * @param number_of_piped_items  number of elements to insert.
 * @param list_indexes  array of element indexes.
 * @param values  array of buffers, each holding one element's value.
 * @param values_length  array of value buffer lengths (number of bytes).
 * @param attrs  if not NULL, create the item using the attributes if the item does not exist.
 * @param results  array of return codes for individual insertion operations, one for each element.
 * @param piped_rc  return code for the whole pipelined operation.
 */
LIBMEMCACHED_API
memcached_return_t memcached_lop_piped_insert(memcached_st *ptr, const char *key, const size_t key_length,
                                              const size_t number_of_piped_items,
                                              const int32_t *list_indexes,
                                              const char * const *values, const size_t *values_length,
                                              memcached_coll_create_attrs_st *attrs,
                                              memcached_return_t *results, memcached_return_t *piped_rc);

/**
 * Insert multiple elements into the set item in a pipelined fashion.
 * @param ptr  memcached handle.
 * @param key  set item's key.
 * @param key_length  key length (number of bytes).
 * @param number_of_piped_items  number of elements to insert.
 * @param values  array of buffers, each holding one element's value.
 * @param values_length  array of value buffer lengths (number of bytes).
 * @param attrs  if not NULL, create the item using the attributes if the item does not exist.
 * @param results  array of return codes for individual insertion operations, one for each element.
 * @param piped_rc  return code for the whole pipelined operation.
 */
LIBMEMCACHED_API
memcached_return_t memcached_sop_piped_insert(memcached_st *ptr, const char *key, const size_t key_length,
                                              const size_t number_of_piped_items,
                                              const char * const *values, const size_t *values_length,
                                              memcached_coll_create_attrs_st *attrs,
                                              memcached_return_t *results, memcached_return_t *piped_rc);

/**
 * Insert multiple elements into the b+tree item in a pipelined fashion.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param number_of_piped_items  number of elements to insert.
 * @param bkeys  array of element bkey's.
 * @param eflags  array of optional eflags.
 * @param eflags_length  array of eflags lengths.
 * @param values  array of buffers, each holding one element's value.
 * @param values_length  array of value buffer lengths (number of bytes).
 * @param attrs  if not NULL, create the item using the attributes if the item does not exist.
 * @param results  array of return codes for individual insertion operations, one for each element.
 * @param piped_rc  return code for the whole pipelined operation.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_piped_insert(memcached_st *ptr, const char *key, const size_t key_length,
                                              const size_t number_of_piped_items,
                                              const uint64_t *bkeys,
                                              const unsigned char * const *eflags, const size_t *eflags_length,
                                              const char * const *values, const size_t *values_length,
                                              memcached_coll_create_attrs_st *attrs,
                                              memcached_return_t *results, memcached_return_t *piped_rc);

/**
 * Insert multiple elements into the b+tree item in a pipelined fashion, using byte-array bkey's.
 * @param ptr  memcached handle.
 * @param key  b+tree item's key.
 * @param key_length  key length (number of bytes).
 * @param number_of_piped_items  number of elements to insert.
 * @param bkeys  array of byte-array bkey's.
 * @param bkeys_length  array of bkey lengths (number of bytes).
 * @param eflags  array of optional eflags.
 * @param eflags_length  array of eflags lengths.
 * @param values  array of buffers, each holding one element's value.
 * @param values_length  array of value buffer lengths (number of bytes).
 * @param attrs  if not NULL, create the item using the attributes if the item does not exist.
 * @param results  array of return codes for individual insertion operations, one for each element.
 * @param piped_rc  return code for the whole pipelined operation.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_ext_piped_insert(memcached_st *ptr, const char *key, const size_t key_length,
                                                  const size_t number_of_piped_items,
                                                  const unsigned char * const *bkeys, const size_t *bkeys_length,
                                                  const unsigned char * const *eflags, const size_t *eflags_length,
                                                  const char * const *values, const size_t *values_length,
                                                  memcached_coll_create_attrs_st *attrs,
                                                  memcached_return_t *results, memcached_return_t *piped_rc);

/**
 * Insert a single element into multiple list items in a pipelined fashion.
 * @param ptr  memcached handle.
 * @param keys  array of the list items' keys.
 * @param key_length  array of key lengths (number of bytes).
 * @param number_of_keys  number of list items.
 * @param index  element index.
 * @param value  buffer holding the element's value.
 * @param value_length  length of the value buffer (number of bytes).
 * @param attrs  if not NULL, create the item using the attributes if the item does not exist.
 * @param results  array of return codes for individual insertion operations, one for each element.
 * @param piped_rc  return code for the whole pipelined operation.
 */
LIBMEMCACHED_API
memcached_return_t memcached_lop_piped_insert_bulk(memcached_st *ptr,
                                                   const char * const *keys,
                                                   const size_t *key_length, size_t number_of_keys,
                                                   const int32_t index,
                                                   const char *value, size_t value_length,
                                                   memcached_coll_create_attrs_st *attrs,
                                                   memcached_return_t *results,
                                                   memcached_return_t *piped_rc);

/**
 * Insert a single element into multiple set items in a pipelined fashion.
 * @param ptr  memcached handle.
 * @param keys  array of the set items' keys.
 * @param key_length  array of key lengths (number of bytes).
 * @param number_of_keys  number of list items.
 * @param value  buffer holding the element's value.
 * @param value_length  length of the value buffer (number of bytes).
 * @param attrs  if not NULL, create the item using the attributes if the item does not exist.
 * @param results  array of return codes for individual insertion operations, one for each element.
 * @param piped_rc  return code for the whole pipelined operation.
 */
LIBMEMCACHED_API
memcached_return_t memcached_sop_piped_insert_bulk(memcached_st *ptr,
                                                   const char * const *keys,
                                                   const size_t *key_length, size_t number_of_keys,
                                                   const char *value, size_t value_length,
                                                   memcached_coll_create_attrs_st *attrs,
                                                   memcached_return_t *results,
                                                   memcached_return_t *piped_rc);

/**
 * Insert a single element into multiple b+tree items in a pipelined fashion.
 * @param ptr  memcached handle.
 * @param keys  array of the b+tree items' keys.
 * @param key_length  array of key lengths (number of bytes).
 * @param number_of_keys  number of list items.
 * @param bkey  element's bkey.
 * @param eflag  optional eflag, maybe NULL.
 * @param eflag_length  eflag length (number of bytes).
 * @param value  buffer holding the element's value.
 * @param value_length  length of the value buffer (number of bytes).
 * @param attrs  if not NULL, create the item using the attributes if the item does not exist.
 * @param results  array of return codes for individual insertion operations, one for each element.
 * @param piped_rc  return code for the whole pipelined operation.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_piped_insert_bulk(memcached_st *ptr,
                                                   const char * const *keys,
                                                   const size_t *key_length, size_t number_of_keys,
                                                   const uint64_t bkey,
                                                   const unsigned char *eflag, size_t eflag_length,
                                                   const char *value, size_t value_length,
                                                   memcached_coll_create_attrs_st *attrs,
                                                   memcached_return_t *results,
                                                   memcached_return_t *piped_rc);

/**
 * Insert a single element into multiple b+tree items in a pipelined fashion, using byte-array bkey's.
 * @param ptr  memcached handle.
 * @param keys  array of the b+tree items' keys.
 * @param key_length  array of key lengths (number of bytes).
 * @param number_of_keys  number of list items.
 * @param bkey  byte-array bkey.
 * @param bkey_length  bkey length (number of bytes).
 * @param eflag  optional eflag, maybe NULL.
 * @param eflag_length  eflag length (number of bytes).
 * @param value  buffer holding the element's value.
 * @param value_length  length of the value buffer (number of bytes).
 * @param attrs  if not NULL, create the item using the attributes if the item does not exist.
 * @param results  array of return codes for individual insertion operations, one for each element.
 * @param piped_rc  return code for the whole pipelined operation.
 */
LIBMEMCACHED_API
memcached_return_t memcached_bop_ext_piped_insert_bulk(memcached_st *ptr,
                                                       const char * const *keys,
                                                       const size_t *key_length, size_t number_of_keys,
                                                       const unsigned char *bkey, size_t bkey_length,
                                                       const unsigned char *eflag, size_t eflag_length,
                                                       const char *value, size_t value_length,
                                                       memcached_coll_create_attrs_st *attrs,
                                                       memcached_return_t *results,
                                                       memcached_return_t *piped_rc);

/**
 * Check the existence of multiple set elements in a pipelined fashion.
 * @param ptr  memcached handle.
 * @param key  set item's key.
 * @param key_length  key length (number of bytes).
 * @param number_of_piped_items  number of elements to check.
 * @param values  array of buffers, each holding one element's value.
 * @param values_length  array of value buffer lengths (number of bytes).
 * @param results  array of return codes for individual elements.
 * @param piped_rc  return code for the whole pipelined operation.
 */
LIBMEMCACHED_API
memcached_return_t memcached_sop_piped_exist(memcached_st *ptr, const char *key, size_t key_length,
                                             const size_t number_of_piped_items,
                                             const char * const *values, const size_t *values_length,
                                             memcached_return_t *results, memcached_return_t *piped_rc);

#ifdef __cplusplus
}
#endif

#endif /* __LIBMEMCACHED_COLLECTION_H__ */
