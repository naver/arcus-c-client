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
#ifndef __LIBMEMCACHED_COLLECTION_RESULT_H__
#define __LIBMEMCACHED_COLLECTION_RESULT_H__

/**
 * Most collection API functions utilize "result" structures.
 * The result contains one or more values, return codes, and
 * other pieces of information from the server.
 * Use the functions listed in this header to retrieve data
 * from the result structure.
 */

/**
 * Result structure for collection operations.
 */
struct memcached_coll_result_st {
  char item_key[MEMCACHED_MAX_KEY];
  size_t key_length;

  memcached_coll_type_t type;
  uint32_t collection_flags;
  size_t collection_count;
  size_t btree_position;  /* used only in bop_find_position_with_get */
  size_t result_position; /* used only in bop_find_position_with_get */
  time_t item_expiration;
  const memcached_st *root;
  memcached_coll_sub_key_type_t sub_key_type;
  memcached_coll_sub_key_st *sub_keys;
  memcached_hexadecimal_st *eflags;
  memcached_string_st *values;
  struct {
    bool is_allocated:1;
    bool is_initialized:1;
  } options;
  /* Add result callback function */
};

/**
 * Result structure specifically for b+tree smget operations.
 */
struct memcached_coll_smget_result_st {
  const memcached_st *root;
  memcached_coll_type_t type;

  /* BKEY */
  memcached_coll_sub_key_type_t sub_key_type;
  memcached_coll_sub_key_st *sub_keys;

  memcached_hexadecimal_st *eflags;

  /* VALUE */
  size_t value_count;
  memcached_string_st *keys;
  uint32_t *flags;
  size_t *bytes;
  memcached_string_st *values;

  /* MISSED_KEYS */
  uint32_t missed_key_count;
  memcached_string_st *missed_keys;
#ifdef SUPPORT_NEW_SMGET_INTERFACE
  memcached_return_t  *missed_causes;

  /* TRIMMED_KEYS */
  uint32_t trimmed_key_count;
  memcached_string_st       *trimmed_keys;
  memcached_coll_sub_key_st *trimmed_sub_keys;
#endif

  /* OFFSET, COUNT */
  size_t offset;
  size_t count;
#ifdef SUPPORT_NEW_SMGET_INTERFACE
  /* smget mode */
  memcached_coll_smget_mode_t smgmode;
#endif

  struct {
    bool is_allocated:1;
    bool is_initialized:1;
    bool is_descending:1;
  } options;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Free allocated memory within the result and the result itself, if it is allocated by the library.
 * @param result  collection result structure
 */
LIBMEMCACHED_API
void memcached_coll_result_free(memcached_coll_result_st *result);

/**
 * Re-initialize the result, freeing allocated memory within the result.
 * @param result  collection result structure
 */
LIBMEMCACHED_API
void memcached_coll_result_reset(memcached_coll_result_st *result);

/**
 * Create and initialize the result.
 * @param ptr  memcached handle
 * @param result  optional caller-allocated collection result structure. The library allocates the result if NULL.
 * @return collection result structure.
 */
LIBMEMCACHED_API
memcached_coll_result_st *memcached_coll_result_create(const memcached_st *ptr,
                                                       memcached_coll_result_st *result);

/**
 * Get the collection item's key.
 * @param result  collection result structure
 * @return collection item's key.
 */
LIBMEMCACHED_API
const char *memcached_coll_result_get_key(memcached_coll_result_st* result);

/**
 * Get the collection item's key length.
 * @param result  collection result structure
 * @return collection item's key length.
 */
LIBMEMCACHED_API
size_t memcached_coll_result_get_key_length(memcached_coll_result_st* result);

/**
 * Get the collection item's type (b+tree, list, set).
 * @param result  collection result structure
 * @return collection item's type.
 */
LIBMEMCACHED_API
memcached_coll_type_t memcached_coll_result_get_type(memcached_coll_result_st *result);

/**
 * Get the number of elements in the result.
 * @param result  collection result structure
 * @return number of elements.
 */
LIBMEMCACHED_API
size_t memcached_coll_result_get_count(memcached_coll_result_st *result);

/**
 * Get the collection item's flags.
 * @param result  collection result structure
 * @return item flags.
 */
LIBMEMCACHED_API
uint32_t memcached_coll_result_get_flags(memcached_coll_result_st *result);

/**
 * Get the position of the found element in the btree.
 * @param result  collection result structure.
 * @return btree position.
 */
LIBMEMCACHED_API
size_t memcached_coll_result_get_btree_position(memcached_coll_result_st *result);

/**
 * Get the position of the found element in the result.
 * @param result  collection result structure.
 * @return result position.
 */
LIBMEMCACHED_API
size_t memcached_coll_result_get_result_position(memcached_coll_result_st *result);

/**
 * Get the b+tree element's position.
 * @param result  collection result structure
 * @param idx  element's index (0th fetched element, 1st fetched element, and so on).
 * @return element's position.
 */
LIBMEMCACHED_API
size_t memcached_coll_result_get_position(memcached_coll_result_st *result, size_t idx);

/**
 * Get the b+tree element's bkey.
 * @param result  collection result structure
 * @param idx  element's index (0th fetched element, 1st fetched element, and so on).
 * @return element's bkey.
 */
LIBMEMCACHED_API
uint64_t memcached_coll_result_get_bkey(memcached_coll_result_st *result, size_t idx);

/**
 * Get the b+tree element's byte-array type bkey.
 * @param result  collection result structure
 * @param idx  element's index (0th fetched element, 1st fetched element, and so on).
 * @return element's byte-array type bkey.
 */
LIBMEMCACHED_API
memcached_hexadecimal_st *memcached_coll_result_get_bkey_ext(memcached_coll_result_st *result, size_t idx);

/**
 * Get the b+tree element's flags (eflag).
 * @param result  collection result structure
 * @param idx  element's index (0th fetched element, 1st fetched element, and so on).
 * @return element's flags.
 */
LIBMEMCACHED_API
memcached_hexadecimal_st *memcached_coll_result_get_eflag(memcached_coll_result_st *result, size_t idx);

/**
 * Get the element's value.
 * @param result  collection result structure
 * @param idx  element's index (0th fetched element, 1st fetched element, and so on).
 * @return element's value.
 */
LIBMEMCACHED_API
const char *memcached_coll_result_get_value(memcached_coll_result_st *result, size_t idx);

/**
 * Get the element's value length.
 * @param result  collection result structure
 * @param idx  element's index (0th fetched element, 1st fetched element, and so on).
 * @return element's value length.
 */
LIBMEMCACHED_API
size_t memcached_coll_result_get_value_length(memcached_coll_result_st *result, size_t idx);


/**
 * Free allocated memory within the smget result and the result itself, if it is allocated by the library.
 * @param result  smget result structure
 */
LIBMEMCACHED_API
void memcached_coll_smget_result_free(memcached_coll_smget_result_st *result);

/**
 * Re-initialize the smget result, freeing allocated memory within the result.
 * @param result  smget result structure
 */
LIBMEMCACHED_API
void memcached_coll_smget_result_reset(memcached_coll_smget_result_st *result);

/**
 * Get the collection item's type. Currently it can only be b+tree.
 * @param result  smget result structure
 * @return collection item's type.
 */
LIBMEMCACHED_API
memcached_coll_type_t memcached_coll_smget_result_get_type(memcached_coll_smget_result_st *result);

/**
 * Get the number of elements in the result.
 * @param result  smget result structure
 * @return number of elements.
 */
LIBMEMCACHED_API
size_t memcached_coll_smget_result_get_count(memcached_coll_smget_result_st *result);

/**
 * Get the key of the b+tree item that contains the given element.
 * @param result  smget result structure
 * @param idx  element's index (0th fetched element, 1st fetched element, and so on).
 * @return b+tree item's key.
 */
LIBMEMCACHED_API
const char *memcached_coll_smget_result_get_key(memcached_coll_smget_result_st *result, size_t idx);

/**
 * Get the bkey of the given element.
 * @param result  smget result structure
 * @param idx  element's index (0th fetched element, 1st fetched element, and so on).
 * @return element's bkey.
 */
LIBMEMCACHED_API
uint64_t memcached_coll_smget_result_get_bkey(memcached_coll_smget_result_st *result, size_t idx);

/**
 * Get the byte-array type bkey of the given element.
 * @param result  smget result structure
 * @param idx  element's index (0th fetched element, 1st fetched element, and so on).
 * @return element's byte-array type bkey.
 */
LIBMEMCACHED_API
memcached_hexadecimal_st *memcached_coll_smget_result_get_bkey_ext(memcached_coll_smget_result_st *result, size_t idx);

/**
 * Get the eflag of the given element.
 * @param result  smget result structure
 * @param idx  element's index (0th fetched element, 1st fetched element, and so on).
 * @return element's eflag.
 */
LIBMEMCACHED_API
memcached_hexadecimal_st *memcached_coll_smget_result_get_eflag(memcached_coll_smget_result_st *result, size_t idx);

/**
 * Get the value of the given element.
 * @param result  smget result structure
 * @param idx  element's index (0th fetched element, 1st fetched element, and so on).
 * @return element's value.
 */
LIBMEMCACHED_API
const char *memcached_coll_smget_result_get_value(memcached_coll_smget_result_st *result, size_t idx);

/**
 * Get the value length of the given element.
 * @param result  smget result structure
 * @param idx  element's index (0th fetched element, 1st fetched element, and so on).
 * @return element's value length.
 */
LIBMEMCACHED_API
size_t memcached_coll_smget_result_get_value_length(memcached_coll_smget_result_st *result, size_t idx);

/**
 * Get the number of missed keys in the result.
 * @param result  smget result structure
 * @return number of missed keys.
 */
LIBMEMCACHED_API
size_t memcached_coll_smget_result_get_missed_key_count(memcached_coll_smget_result_st *result);

/**
 * Get the individual missed key.
 * @param result  smget result structure
 * @param idx  missed key's index (0th key, 1st key, and so on).
 * @return missed key.
 */
LIBMEMCACHED_API
const char *memcached_coll_smget_result_get_missed_key(memcached_coll_smget_result_st *result, size_t idx);

/**
 * Get the length of the individual missed key.
 * @param result  smget result structure
 * @param idx  missed key's index (0th key, 1st key, and so on).
 * @return missed key's length.
 */
LIBMEMCACHED_API
size_t memcached_coll_smget_result_get_missed_key_length(memcached_coll_smget_result_st *result, size_t idx);

#ifdef SUPPORT_NEW_SMGET_INTERFACE
/**
 * Get the missed cause of the individual missed key.
 * @param result  smget result structure
 * @param idx  missed key's index (0th key, 1st key, and so on).
 * @return missed cause of the missed key.
 */
LIBMEMCACHED_API
memcached_return_t memcached_coll_smget_result_get_missed_cause(memcached_coll_smget_result_st *result, size_t index);

/**
 * Get the number of trimmed keys in the result.
 * @param result  smget result structure
 * @return number of trimmed keys.
 */
LIBMEMCACHED_API
size_t memcached_coll_smget_result_get_trimmed_key_count(memcached_coll_smget_result_st *result);

/**
 * Get the individual trimmed key.
 * @param result  smget result structure
 * @param idx  trimmed key's index (0th key, 1st key, and so on).
 * @return trimmed key.
 */
LIBMEMCACHED_API
const char *memcached_coll_smget_result_get_trimmed_key(memcached_coll_smget_result_st *result, size_t index);

/**
 * Get the length of the individual trimmed key.
 * @param result  smget result structure
 * @param idx  trimmed key's index (0th key, 1st key, and so on).
 * @return trimmed key's length.
 */
LIBMEMCACHED_API
size_t memcached_coll_smget_result_get_trimmed_key_length(memcached_coll_smget_result_st *result, size_t index);

/**
 * Get the last bkey of the individual trimmed key.
 * @param result  smget result structure
 * @param idx  trimmed key's index (0th key, 1st key, and so on).
 * @return the last bkey of the trimmed key.
 */
LIBMEMCACHED_API
uint64_t memcached_coll_smget_result_get_trimmed_bkey(memcached_coll_smget_result_st *result, size_t index);

/**
 * Get the last byte-array bkey of the individual trimmed key.
 * @param result  smget result structure
 * @param idx  trimmed key's index (0th key, 1st key, and so on).
 * @return the last byte-array bkey of the trimmed key.
 */
LIBMEMCACHED_API
memcached_hexadecimal_st *memcached_coll_smget_result_get_trimmed_bkey_ext(memcached_coll_smget_result_st *result, size_t index);
#endif

/**
 * Create and initialize the smget result.
 * @param ptr  memcached handle
 * @param result  optional caller-allocated smget result structure. The library allocates the result if NULL.
 * @return smget result structure.
 */
LIBMEMCACHED_API
memcached_coll_smget_result_st *memcached_coll_smget_result_create(const memcached_st *ptr,
                                                                   memcached_coll_smget_result_st *result);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __LIBMEMCACHED_COLLECTION_RESULT_H__ */
