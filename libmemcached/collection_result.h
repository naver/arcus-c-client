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

struct memcached_coll_result_st {
  char item_key[MEMCACHED_MAX_KEY];
  size_t key_length;

  memcached_coll_type_t type;
  uint32_t collection_flags;
  size_t collection_count;
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

  /* OFFSET, COUNT */
  size_t offset;
  size_t count;

  struct {
    bool is_allocated:1;
    bool is_initialized:1;
    bool is_descending:1;
  } options;
};

#ifdef __cplusplus
extern "C" {
#endif

/* Result Struct */
LIBMEMCACHED_API
void memcached_coll_result_free(memcached_coll_result_st *result);

LIBMEMCACHED_API
void memcached_coll_result_reset(memcached_coll_result_st *ptr);

LIBMEMCACHED_API
memcached_coll_result_st *memcached_coll_result_create(const memcached_st *ptr,
                                                       memcached_coll_result_st *result);

LIBMEMCACHED_API
const char *memcached_coll_result_get_key(memcached_coll_result_st* result);

LIBMEMCACHED_API
size_t memcached_coll_result_get_key_length(memcached_coll_result_st* result);

LIBMEMCACHED_API
memcached_coll_type_t memcached_coll_result_get_type(memcached_coll_result_st *result);

LIBMEMCACHED_API
size_t memcached_coll_result_get_count(memcached_coll_result_st *result);

LIBMEMCACHED_API
uint32_t memcached_coll_result_get_flags(memcached_coll_result_st *result);

LIBMEMCACHED_API
uint64_t memcached_coll_result_get_bkey(memcached_coll_result_st *result, size_t idx);

LIBMEMCACHED_API
memcached_hexadecimal_st *memcached_coll_result_get_bkey_ext(memcached_coll_result_st *result, size_t idx);

LIBMEMCACHED_API
memcached_hexadecimal_st *memcached_coll_result_get_eflag(memcached_coll_result_st *result, size_t idx);

LIBMEMCACHED_API
const char *memcached_coll_result_get_value(memcached_coll_result_st *result, size_t idx);

LIBMEMCACHED_API
size_t memcached_coll_result_get_value_length(memcached_coll_result_st *result, size_t idx);


LIBMEMCACHED_API
void memcached_coll_smget_result_free(memcached_coll_smget_result_st *result);

LIBMEMCACHED_API
void memcached_coll_smget_result_reset(memcached_coll_smget_result_st *ptr);

LIBMEMCACHED_API
void memcached_coll_smget_result_free_container_only(memcached_coll_smget_result_st *ptr);

LIBMEMCACHED_API
memcached_coll_type_t memcached_coll_smget_result_get_type(memcached_coll_smget_result_st *result);

LIBMEMCACHED_API
size_t memcached_coll_smget_result_get_count(memcached_coll_smget_result_st *result);

LIBMEMCACHED_API
const char *memcached_coll_smget_result_get_key(memcached_coll_smget_result_st *result, size_t idx);

LIBMEMCACHED_API
uint64_t memcached_coll_smget_result_get_bkey(memcached_coll_smget_result_st *result, size_t idx);

LIBMEMCACHED_API
memcached_hexadecimal_st *memcached_coll_smget_result_get_bkey_ext(memcached_coll_smget_result_st *result, size_t idx);

LIBMEMCACHED_API
memcached_hexadecimal_st *memcached_coll_smget_result_get_eflag(memcached_coll_smget_result_st *result, size_t idx);

LIBMEMCACHED_API
const char *memcached_coll_smget_result_get_value(memcached_coll_smget_result_st *result, size_t idx);

LIBMEMCACHED_API
size_t memcached_coll_smget_result_get_value_length(memcached_coll_smget_result_st *result, size_t idx);

LIBMEMCACHED_API
size_t memcached_coll_smget_result_get_missed_key_count(memcached_coll_smget_result_st *result);

LIBMEMCACHED_API
const char *memcached_coll_smget_result_get_missed_key(memcached_coll_smget_result_st *result, size_t idx);

LIBMEMCACHED_API
size_t memcached_coll_smget_result_get_missed_key_length(memcached_coll_smget_result_st *result, size_t idx);

LIBMEMCACHED_API
memcached_coll_smget_result_st *memcached_coll_smget_result_create(const memcached_st *ptr,
                                                                   memcached_coll_smget_result_st *result);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __LIBMEMCACHED_COLLECTION_RESULT_H__ */
