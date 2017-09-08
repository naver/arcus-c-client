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
#include "common.h"

static inline void _coll_result_init(memcached_coll_result_st *self,
                                     const memcached_st *memc)
{
  self->collection_flags= 0;
  self->collection_count= 0;
  self->btree_position= 0;
  self->result_position= 0;
  self->item_expiration= 0;
  self->values= NULL;
  self->sub_key_type= MEMCACHED_COLL_QUERY_UNKNOWN;
  self->sub_keys= NULL;
  self->eflags= NULL;
  self->root= memc;
  self->options.is_initialized= true;
}

memcached_coll_result_st *memcached_coll_result_create(const memcached_st *memc,
                                                       memcached_coll_result_st *ptr)
{
  /* Saving malloc calls :) */
  if (ptr)
  {
    ptr->options.is_allocated= false;
  }
  else
  {
    ptr= static_cast<memcached_coll_result_st*>(libmemcached_malloc(memc, sizeof(memcached_coll_result_st)));

    if (not ptr) return NULL;

    ptr->options.is_allocated= true;
  }

  _coll_result_init(ptr, memc);

  return ptr;
}

void memcached_coll_result_reset(memcached_coll_result_st *ptr)
{
  if (not ptr) return;

  for (size_t i=0; i<ptr->collection_count; i++)
  {
    memcached_string_free(&ptr->values[i]);
  }

  if (ptr->values)
  {
    libmemcached_free(ptr->root, ptr->values);
    ptr->values= NULL;
  }

  if (ptr->sub_keys)
  {
    if (MEMCACHED_COLL_QUERY_BOP_EXT == ptr->sub_key_type)
    {
      for (size_t i=0; i<ptr->collection_count; i++)
      {
        libmemcached_free(ptr->root, ptr->sub_keys[i].bkey_ext.array);
        ptr->sub_keys[i].bkey_ext.array= NULL;
        ptr->sub_keys[i].bkey_ext.length= 0;
      }
      ptr->sub_key_type= MEMCACHED_COLL_QUERY_UNKNOWN;
    }
    libmemcached_free(ptr->root, ptr->sub_keys);
    ptr->sub_keys= NULL;
  }

  if (ptr->eflags)
  {
    for (size_t i=0; i<ptr->collection_count; i++)
    {
      libmemcached_free(ptr->root, ptr->eflags[i].array);
      ptr->eflags[i].array = NULL;
      ptr->eflags[i].length = 0;
    }

    libmemcached_free(ptr->root, ptr->eflags);
    ptr->eflags= NULL;
  }

  _coll_result_init(ptr, ptr->root);
}

void memcached_coll_result_free(memcached_coll_result_st *ptr)
{
  if (not ptr)                           return;
  if (not memcached_is_initialized(ptr)) return;

  memcached_coll_result_reset(ptr);

  ptr->options.is_initialized= false;

  if (memcached_is_allocated(ptr))
  {
    ptr->options.is_allocated= false;
    libmemcached_free(ptr->root, ptr);
    ptr= NULL;
  }
}

const char *memcached_coll_result_get_key(memcached_coll_result_st* result)
{
  assert_msg_with_return(result, "result is null", NULL);
  return result->key_length ? result->item_key : NULL;
}

size_t memcached_coll_result_get_key_length(memcached_coll_result_st* result)
{
  assert_msg_with_return(result, "result is null", 0);
  return result->key_length;
}

memcached_coll_type_t memcached_coll_result_get_type(memcached_coll_result_st *result)
{
  assert_msg_with_return(result, "result is null", COLL_NONE);
  return result->type;
}

size_t memcached_coll_result_get_count(memcached_coll_result_st *result)
{
  assert_msg_with_return(result, "result is null", 0);
  return result->collection_count;
}

uint32_t memcached_coll_result_get_flags(memcached_coll_result_st *result)
{
  assert_msg_with_return(result, "result is null", 0);
  return result->collection_flags;
}

size_t memcached_coll_result_get_btree_position(memcached_coll_result_st *result)
{
  assert_msg_with_return(result, "result is null", 0);
  return result->btree_position;
}

size_t memcached_coll_result_get_result_position(memcached_coll_result_st *result)
{
  assert_msg_with_return(result, "result is null", 0);
  return result->result_position;
}

uint64_t memcached_coll_result_get_bkey(memcached_coll_result_st *result, size_t index)
{
  assert_msg_with_return(result, "result is null", 0);
  assert_msg_with_return(result->type == COLL_BTREE, "not a b+tree", 0);
  return result->sub_keys[index].bkey;
}

memcached_hexadecimal_st *memcached_coll_result_get_bkey_ext(memcached_coll_result_st *result, size_t index)
{
  assert_msg_with_return(result, "result is null", NULL);
  assert_msg_with_return(result->type == COLL_BTREE, "not a b+tree", NULL);
  assert_msg_with_return(result->sub_key_type == MEMCACHED_COLL_QUERY_BOP_EXT, "not a byte array bkey", NULL);
  return &result->sub_keys[index].bkey_ext;
}

memcached_hexadecimal_st *memcached_coll_result_get_eflag(memcached_coll_result_st *result, size_t index)
{
  assert_msg_with_return(result, "result is null", NULL);
  assert_msg_with_return(result->type == COLL_BTREE, "not a b+tree", NULL);
  return (not result->eflags)? NULL : &result->eflags[index];
}

const char *memcached_coll_result_get_value(memcached_coll_result_st *result, size_t index)
{
  assert_msg_with_return(result, "result is null", NULL);
  return (not result->values)? NULL : result->values[index].string;
}

size_t memcached_coll_result_get_value_length(memcached_coll_result_st *result, size_t index)
{
  assert_msg_with_return(result, "result is null", 0);
  return (not result->values)? 0 : memcached_string_length(&result->values[index]);
}

static inline void _coll_smget_result_init(memcached_coll_smget_result_st *self,
                                           const memcached_st *memc)
{
  self->value_count= 0;
  self->missed_key_count= 0;
  self->keys= NULL;
  self->values= NULL;
  self->flags= NULL;
  self->sub_key_type= MEMCACHED_COLL_QUERY_UNKNOWN;
  self->sub_keys= NULL;
  self->eflags= NULL;
  self->bytes= NULL;
  self->missed_keys= NULL;
  self->offset= 0;
  self->count= 0;
  self->root= memc;
  self->options.is_initialized= true;
  self->options.is_descending= false;
}

memcached_coll_smget_result_st *memcached_coll_smget_result_create(const memcached_st *memc,
                                                                   memcached_coll_smget_result_st *ptr)
{
  /* Saving malloc calls :) */
  if (ptr)
  {
    ptr->options.is_allocated= false;
  }
  else
  {
    ptr= static_cast<memcached_coll_smget_result_st*>(libmemcached_malloc(memc, sizeof(memcached_coll_smget_result_st)));

    if (ptr == NULL)
      return NULL;

    ptr->options.is_allocated= true;
  }

  _coll_smget_result_init(ptr, memc);

  return ptr;
}

void memcached_coll_smget_result_reset(memcached_coll_smget_result_st *ptr)
{
  if (not ptr) return;

  size_t i;

  if (ptr->keys)
  {
    for (i=0; i<ptr->value_count; i++)
    {
      memcached_string_free(&ptr->keys[i]);
    }

    libmemcached_free(ptr->root, ptr->keys);
    ptr->keys= NULL;
  }

  if (ptr->values)
  {
    for (i=0; i<ptr->value_count; i++)
    {
      memcached_string_free(&ptr->values[i]);
    }

    libmemcached_free(ptr->root, ptr->values);
    ptr->values= NULL;
  }

  if (ptr->flags)
  {
    libmemcached_free(ptr->root, ptr->flags);
    ptr->flags= NULL;
  }

  if (ptr->sub_keys)
  {
    if (MEMCACHED_COLL_QUERY_BOP_EXT == ptr->sub_key_type or
        MEMCACHED_COLL_QUERY_BOP_EXT_RANGE == ptr->sub_key_type)
    {
      for (i=0; i<ptr->value_count; i++)
      {
        libmemcached_free(ptr->root, ptr->sub_keys[i].bkey_ext.array);
        ptr->sub_keys[i].bkey_ext.array = NULL;
        ptr->sub_keys[i].bkey_ext.length = 0;
      }
      ptr->sub_key_type= MEMCACHED_COLL_QUERY_UNKNOWN;
    }

    libmemcached_free(ptr->root, ptr->sub_keys);
    ptr->sub_keys= NULL;
  }

  if (ptr->eflags)
  {
    for (i=0; i<ptr->value_count; i++)
    {
      libmemcached_free(ptr->root, ptr->eflags[i].array);
      ptr->eflags[i].array = NULL;
      ptr->eflags[i].length = 0;
    }

    libmemcached_free(ptr->root, ptr->eflags);
    ptr->eflags= NULL;
  }

  if (ptr->bytes)
  {
    libmemcached_free(ptr->root, ptr->bytes);
    ptr->bytes= NULL;
  }

  if (ptr->missed_keys)
  {
    for (i=0; i<ptr->missed_key_count; i++) {
      memcached_string_free(&ptr->missed_keys[i]);
    }

    libmemcached_free(ptr->root, ptr->missed_keys);
    ptr->missed_keys= NULL;
  }

  _coll_smget_result_init(ptr, ptr->root);
}

void memcached_coll_smget_result_free(memcached_coll_smget_result_st *ptr)
{
  if (not ptr)                           return;
  if (not memcached_is_initialized(ptr)) return;

  memcached_coll_smget_result_reset(ptr);

  ptr->options.is_initialized= false;

  if (memcached_is_allocated(ptr))
  {
    libmemcached_free(ptr->root, ptr);
    ptr= NULL;
  }
}

memcached_coll_type_t memcached_coll_smget_result_get_type(memcached_coll_smget_result_st *result)
{
  assert_msg_with_return(result, "result is null", COLL_NONE);
  return result->type;
}

size_t memcached_coll_smget_result_get_count(memcached_coll_smget_result_st *result)
{
  assert_msg_with_return(result, "result is null", 0);
  return result->value_count;
}

const char *memcached_coll_smget_result_get_key(memcached_coll_smget_result_st *result, size_t index)
{
  assert_msg_with_return(result, "result is null", 0);
  return (not result->keys)? NULL : result->keys[index].string;
}

uint64_t memcached_coll_smget_result_get_bkey(memcached_coll_smget_result_st *result, size_t index)
{
  assert_msg_with_return(result, "result is null", 0);
  return result->sub_keys[index].bkey;
}

memcached_hexadecimal_st *memcached_coll_smget_result_get_bkey_ext(memcached_coll_smget_result_st *result, size_t index)
{
  assert_msg_with_return(result, "result is null", 0);
  return &result->sub_keys[index].bkey_ext;
}

memcached_hexadecimal_st *memcached_coll_smget_result_get_eflag(memcached_coll_smget_result_st *result, size_t index)
{
  assert_msg_with_return(result, "result is null", 0);
  return (not result->eflags)? NULL : &result->eflags[index];
}

const char *memcached_coll_smget_result_get_value(memcached_coll_smget_result_st *result, size_t index)
{
  assert_msg_with_return(result, "result is null", 0);
  return (not result->values)? NULL : result->values[index].string;
}

size_t memcached_coll_smget_result_get_value_length(memcached_coll_smget_result_st *result, size_t index)
{
  assert_msg_with_return(result, "result is null", 0);
  return (not result->values)? 0 : memcached_string_length(&result->values[index]);
}

size_t memcached_coll_smget_result_get_missed_key_count(memcached_coll_smget_result_st *result)
{
  assert_msg_with_return(result, "result is null", 0);
  return result->missed_key_count;
}

const char *memcached_coll_smget_result_get_missed_key(memcached_coll_smget_result_st *result, size_t index)
{
  assert_msg_with_return(result, "result is null", 0);
  return (not result->missed_keys)? NULL : result->missed_keys[index].string;
}

size_t memcached_coll_smget_result_get_missed_key_length(memcached_coll_smget_result_st *result, size_t index)
{
  assert_msg_with_return(result, "result is null", 0);
  return (not result->missed_keys)? 0 : result->missed_keys[index].current_size;
}
