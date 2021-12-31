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
/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Libmemcached library
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  Copyright (C) 2006-2009 Brian Aker All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *      * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the
 *  distribution.
 *
 *      * The names of its contributors may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <libmemcached/common.h>

char *memcached_fetch(memcached_st *ptr, char *key, size_t *key_length,
                      size_t *value_length,
                      uint32_t *flags,
                      memcached_return_t *error)
{
  memcached_result_st *result_buffer= &ptr->result;
  memcached_return_t unused;
  if (not error)
    error= &unused;


  unlikely (ptr->flags.use_udp)
  {
    if (value_length)
      *value_length= 0;

    if (key_length)
      *key_length= 0;

    if (flags)
      *flags= 0;

    if (key)
      *key= 0;

    *error= MEMCACHED_NOT_SUPPORTED;
    return NULL;
  }

  result_buffer= memcached_fetch_result(ptr, result_buffer, error);
  if (result_buffer == NULL or memcached_failed(*error))
  {
    WATCHPOINT_ASSERT(result_buffer == NULL);
    if (value_length)
      *value_length= 0;

    if (key_length)
      *key_length= 0;

    if (flags)
      *flags= 0;

    if (key)
      *key= 0;

    return NULL;
  }

  if (value_length)
  {
    *value_length= memcached_string_length(&result_buffer->value);
  }

  if (key)
  {
    if (result_buffer->key_length > MEMCACHED_MAX_KEY)
    {
      *error= MEMCACHED_KEY_TOO_BIG;
      if (value_length)
        *value_length= 0;

    if (key_length)
      *key_length= 0;

    if (flags)
      *flags= 0;

    if (key)
      *key= 0;

      return NULL;
    }
    strncpy(key, result_buffer->item_key, MEMCACHED_MAX_KEY-1); // For the binary protocol we will cut off the key :(
    if (key_length)
      *key_length= result_buffer->key_length;
  }

  if (flags)
    *flags= result_buffer->item_flags;

  return memcached_string_take_value(&result_buffer->value);
}

memcached_result_st *memcached_fetch_result(memcached_st *ptr,
                                            memcached_result_st *result,
                                            memcached_return_t *error)
{
  memcached_return_t unused;
  if (not error)
    error= &unused;

  if (not ptr)
  {
    *error= MEMCACHED_INVALID_ARGUMENTS;
    return NULL;
  }

  if (ptr->flags.use_udp)
  {
    *error= MEMCACHED_NOT_SUPPORTED;
    return NULL;
  }

  if (not result)
  {
    // If we have already initialized (ie it is in use) our internal, we
    // create one.
    if (memcached_is_initialized(&ptr->result))
    {
      if (not (result= memcached_result_create(ptr, NULL)))
      {
        *error= MEMCACHED_MEMORY_ALLOCATION_FAILURE;
        return NULL;
      }
    }
    else
    {
      result= memcached_result_create(ptr, &ptr->result);
    }
  }

  *error= MEMCACHED_MAXIMUM_RETURN; // We use this to see if we ever go into the loop
  memcached_server_st *server;
  bool connection_failures= false;
  while ((server= memcached_io_get_readable_server(ptr)))
  {
    char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE + MEMCACHED_MAX_KEY];
    *error= memcached_response(server, buffer, sizeof(buffer), result);

    if (*error == MEMCACHED_IN_PROGRESS)
    {
      continue;
    }
    else if (*error == MEMCACHED_CONNECTION_FAILURE)
    {
      connection_failures= true;
      continue;
    }
    else if (*error == MEMCACHED_SUCCESS)
    {
      result->count++;
      return result;
    }
    else if (*error == MEMCACHED_END)
    {
      memcached_server_response_reset(server);
    }
    else if (*error != MEMCACHED_NOTFOUND)
    {
      break;
    }
  }
  memcached_set_last_response_code(ptr, *error);

  if (*error == MEMCACHED_NOTFOUND and result->count)
  {
    *error= MEMCACHED_END;
  }
  else if (*error == MEMCACHED_MAXIMUM_RETURN and result->count)
  {
    *error= MEMCACHED_END;
  }
  else if (*error == MEMCACHED_MAXIMUM_RETURN) // while() loop was never entered
  {
    *error= MEMCACHED_NOTFOUND;
  }
  else if (connection_failures)
  {
    /* If we have a connection failure to some servers, the caller may
     * wish to treat that differently to getting a definitive NOT_FOUND
     * from all servers, so return MEMCACHED_CONNECTION_FAILURE to allow that.
     */
    *error= MEMCACHED_CONNECTION_FAILURE;
  }
  else if (result->count == 0)
  {
    *error= MEMCACHED_NOTFOUND;
  }

  /* We have completed reading data */
  if (memcached_is_allocated(result))
  {
    memcached_result_free(result);
  }
  else
  {
    result->count= 0;
    memcached_string_reset(&result->value);
  }

  return NULL;
}

memcached_return_t memcached_fetch_execute(memcached_st *ptr,
                                           memcached_execute_fn *callback,
                                           void *context,
                                           uint32_t number_of_callbacks)
{
  memcached_result_st *result= &ptr->result;
  memcached_return_t rc;
  bool some_errors= false;

  while ((result= memcached_fetch_result(ptr, result, &rc)))
  {
    if (memcached_failed(rc) and rc == MEMCACHED_NOTFOUND)
    {
      continue;
    }
    else if (memcached_failed(rc))
    {
      memcached_set_error(*ptr, rc, MEMCACHED_AT);
      some_errors= true;
      continue;
    }

    for (uint32_t x= 0; x < number_of_callbacks; x++)
    {
      memcached_return_t ret= (*callback[x])(ptr, result, context);
      if (memcached_failed(ret))
      {
        some_errors= true;
        memcached_set_error(*ptr, ret, MEMCACHED_AT);
        break;
      }
    }
  }

  if (some_errors)
  {
    return MEMCACHED_SOME_ERRORS;
  }

  // If we were able to run all keys without issue we return
  // MEMCACHED_SUCCESS
  if (memcached_success(rc))
  {
    return MEMCACHED_SUCCESS;
  }

  return rc;
}

memcached_coll_result_st *
memcached_coll_fetch_result(memcached_st *ptr,
                            memcached_coll_result_st *result,
                            memcached_return_t *error)
{
  memcached_return_t unused;
  if (not error)
    error= &unused;

  bool result_was_allocated_internally= false;

  if (not result)
  {
    // If we have already initialized (ie it is in use) our internal,
    // we create one.
    if (memcached_is_initialized(&ptr->collection_result))
    {
      if (not (result= memcached_coll_result_create(ptr, NULL)))
      {
        *error= MEMCACHED_MEMORY_ALLOCATION_FAILURE;
        return NULL;
      }
    }
    else
    {
      result= memcached_coll_result_create(ptr, &ptr->collection_result);
      result_was_allocated_internally= true;
    }
  }
  else if (not memcached_is_initialized(result))
  {
    memcached_coll_result_create(ptr, result);
  }

  // Set the collection type
  result->type= find_collection_type_by_opcode(ptr->last_op_code);

  *error= MEMCACHED_MAXIMUM_RETURN; // We use this to see if we ever go into the loop
  memcached_server_st *server;
  while ((server= memcached_io_get_readable_server(ptr)))
  {
    char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE + MEMCACHED_MAX_KEY];
    *error= memcached_coll_response(server, buffer, sizeof(buffer), result);

    if (*error == MEMCACHED_IN_PROGRESS)
    {
      continue;
    }
    else if (*error == MEMCACHED_SUCCESS or *error == MEMCACHED_TRIMMED or
             *error == MEMCACHED_DELETED or *error == MEMCACHED_DELETED_DROPPED)
    {
      return result;
    }
    else if (*error == MEMCACHED_END)
    {
      memcached_server_response_reset(server);
    }
    else if (MEMCACHED_OPCODE_IS_MGET(ptr))
    {
      return result;
    }
    else if (*error != MEMCACHED_NOTFOUND         and
             *error != MEMCACHED_NOTFOUND_ELEMENT )
    {
      break;
    }
  }
  memcached_set_last_response_code(ptr, *error);

  if (result->collection_count and *error == MEMCACHED_NOTFOUND)
  {
    *error= MEMCACHED_END;
  }
  else if (result->collection_count and *error == MEMCACHED_NOTFOUND_ELEMENT)
  {
    *error= MEMCACHED_END;
  }
  else if (result->collection_count and *error == MEMCACHED_OUT_OF_RANGE)
  {
    *error= MEMCACHED_END;
  }
  else if (result->collection_count and *error == MEMCACHED_MAXIMUM_RETURN)
  {
    *error= MEMCACHED_END;
  }
  else if (*error == MEMCACHED_MAXIMUM_RETURN) // while() loop was never entered
  {
    *error= MEMCACHED_NOTFOUND;
  }
  else if (*error == MEMCACHED_SUCCESS)
  {
    *error= MEMCACHED_END;
  }
  else if (*error != MEMCACHED_END              and
           *error != MEMCACHED_UNREADABLE       and
           *error != MEMCACHED_OUT_OF_RANGE     and
           *error != MEMCACHED_NOTFOUND_ELEMENT and
#ifdef ENABLE_REPLICATION /* coll get with delete */
           *error != MEMCACHED_SWITCHOVER       and
           *error != MEMCACHED_REPL_SLAVE       and
#endif
           *error != MEMCACHED_TYPE_MISMATCH    and
           *error != MEMCACHED_BKEY_MISMATCH    )
  {
    *error= MEMCACHED_NOTFOUND;
  }

  /* We have completed reading data */
  if (result_was_allocated_internally)
  {
    memcached_coll_result_free(result);
  }
  else
  {
    memcached_coll_result_reset(result);
  }

  return NULL;
}

static memcached_return_t
merge_smget_results(memcached_coll_smget_result_st **results,
                    memcached_return_t *responses, size_t num_results,
                    memcached_coll_smget_result_st *merged)
{
  memcached_return_t rc= MEMCACHED_END;
  size_t result_idx[256];
  memset(result_idx, 0, 256*sizeof(size_t));

  memcached_coll_sub_key_st *prev_sub_key = NULL;
  memcached_coll_sub_key_st *curr_sub_key;

  size_t merged_count= 0;
  size_t found_count= 0;
  bool bkey_trimmed= false;
  bool byte_array_bkey= (merged->sub_key_type == MEMCACHED_COLL_QUERY_BOP_EXT or
                         merged->sub_key_type == MEMCACHED_COLL_QUERY_BOP_EXT_RANGE)
                      ? true : false;

#if 0 /* FOR DEBUGGING */
  fprintf(stderr, "merged: offset=%ld count=%ld value_count=%ld, num_results=%ld\n",
                   merged->offset, merged->count, merged->value_count, num_results);
  for (size_t i=0; i<num_results; i++) {
    fprintf(stderr, "results[%ld]->value_count=%ld, response=%s\n",
            i, results[i]->value_count,
            responses[i] == MEMCACHED_END ? "MEMCACHED_END" :
            responses[i] == MEMCACHED_TRIMMED ? "MEMCACHED_TRIMMED" :
            responses[i] == MEMCACHED_DUPLICATED ? "MEMCACHED_DUPLICATED" :
            responses[i] == MEMCACHED_DUPLICATED_TRIMMED ? "MEMCACHED_DUPLICATED_TRIMMED" : "UNKNOWN");
    for (size_t j=0; j<results[i]->value_count; j++) {
      fprintf(stderr, "bkey[%llu] key[%s]\n",
                      (unsigned long long)memcached_coll_smget_result_get_bkey(results[i], j),
                      (char*)memcached_coll_smget_result_get_key(results[i], j));
    }
  }
#endif

  /* 1. Merge bkeys */
  for (size_t i=0; i<merged->value_count; i++)
  {
    // find a smallest value.
    int smallest_idx= -1;
    int comp_result;
    for (size_t j=0; j<num_results; j++)
    {
      if (result_idx[j] >= results[j]->value_count) {
        continue;
      }
      if (smallest_idx == -1) {
        smallest_idx= j;
        continue;
      }

      if (byte_array_bkey) {
        memcached_hexadecimal_st *smallest_bkey= &results[smallest_idx]->sub_keys[result_idx[smallest_idx]].bkey_ext;
        memcached_hexadecimal_st *current_bkey= &results[j]->sub_keys[result_idx[j]].bkey_ext;
        comp_result= memcached_compare_two_hexadecimal(smallest_bkey, current_bkey);
      } else {
        uint64_t smallest_bkey= results[smallest_idx]->sub_keys[result_idx[smallest_idx]].bkey;
        uint64_t current_bkey= results[j]->sub_keys[result_idx[j]].bkey;
        comp_result= (smallest_bkey == current_bkey) ? 0 : ((smallest_bkey < current_bkey) ? -1 : 1);
      }
      if (comp_result == 0) { /* compare key string */
        const char *smallest_key= memcached_string_value(&results[smallest_idx]->keys[result_idx[smallest_idx]]);
        const char *current_key= memcached_string_value(&results[j]->keys[result_idx[j]]);
        comp_result= strcmp(smallest_key, current_key);
      }

      if (memcached_is_descending(merged)) {
        if (comp_result < 0)
          smallest_idx= j;
      } else {
        if (comp_result > 0)
          smallest_idx= j;
      }
    }

    bool bkey_duplicated= false;
    curr_sub_key= &results[smallest_idx]->sub_keys[result_idx[smallest_idx]];
    if (prev_sub_key != NULL) {
      if (byte_array_bkey) {
        if (memcached_compare_two_hexadecimal(&prev_sub_key->bkey_ext, &curr_sub_key->bkey_ext) == 0)
           bkey_duplicated= true;
      } else {
        if (prev_sub_key->bkey == curr_sub_key->bkey)
           bkey_duplicated= true;
      }
    }
    prev_sub_key= curr_sub_key;

    if (bkey_duplicated)
    {
      if (merged->smgmode == MEMCACHED_COLL_SMGET_UNIQUE) {
        /* if there are no more elements in this result. */
        if (++result_idx[smallest_idx] >= results[smallest_idx]->value_count) {
          if ((merged->smgmode == MEMCACHED_COLL_SMGET_NONE) &&
              (responses[smallest_idx] == MEMCACHED_TRIMMED or
               responses[smallest_idx] == MEMCACHED_DUPLICATED_TRIMMED)) {
              bkey_trimmed= true;
          }
        }
        continue;
      }
    }
    else
    {
      if (bkey_trimmed) {
        break; /* stop smget */
      }
    }

    // merge or free
    if (found_count >= merged->offset and found_count < merged->offset+merged->count)
    {
      if (merged_count > 0 && bkey_duplicated) {
        if (rc == MEMCACHED_END)
          rc= MEMCACHED_DUPLICATED;
      }

      // merge
      if (byte_array_bkey)
        merged->sub_keys[merged_count].bkey_ext= curr_sub_key->bkey_ext;
      else
        merged->sub_keys[merged_count].bkey= curr_sub_key->bkey;

      merged->keys  [merged_count]= results[smallest_idx]->keys  [result_idx[smallest_idx]];
      merged->values[merged_count]= results[smallest_idx]->values[result_idx[smallest_idx]];
      merged->flags [merged_count]= results[smallest_idx]->flags [result_idx[smallest_idx]];
      merged->eflags[merged_count]= results[smallest_idx]->eflags[result_idx[smallest_idx]];
      merged->bytes [merged_count]= results[smallest_idx]->bytes [result_idx[smallest_idx]];
      merged_count++;
    }
    else
    {
      // free
      memcached_string_free(&results[smallest_idx]->keys[result_idx[smallest_idx]]);
      memcached_string_free(&results[smallest_idx]->values[result_idx[smallest_idx]]);
      libmemcached_free(results[smallest_idx]->root,
                        results[smallest_idx]->eflags[result_idx[smallest_idx]].array);
      if (byte_array_bkey) {
        libmemcached_free(results[smallest_idx]->root,
                          results[smallest_idx]->sub_keys[result_idx[smallest_idx]].bkey_ext.array);
      }
    }
    found_count++;

    /* if there are no more elements in this result. */
    if (++result_idx[smallest_idx] >= results[smallest_idx]->value_count)
    {
      if ((merged->smgmode == MEMCACHED_COLL_SMGET_NONE) &&
          (responses[smallest_idx] == MEMCACHED_TRIMMED or
           responses[smallest_idx] == MEMCACHED_DUPLICATED_TRIMMED))
      {
          bkey_trimmed= true;
      }
    }

    if (merged_count >= merged->count) {
      break; /* the end */
    }
  }

  for (size_t j=0; j<num_results; j++)
  {
    for (size_t x=result_idx[j]; x<results[j]->value_count; x++)
    {
      // free
      memcached_string_free(&results[j]->keys[x]);
      memcached_string_free(&results[j]->values[x]);
      libmemcached_free(results[j]->root, results[j]->eflags[x].array);
      if (byte_array_bkey) {
        libmemcached_free(results[j]->root, results[j]->sub_keys[x].bkey_ext.array);
      }
    }
    results[j]->value_count= 0;
  }

  if (bkey_trimmed && merged_count < merged->count)
  {
    if (rc == MEMCACHED_END) {
      rc= MEMCACHED_TRIMMED;
    } else if (rc == MEMCACHED_DUPLICATED) {
      rc= MEMCACHED_DUPLICATED_TRIMMED;
    }
  }

  // set the count
  merged->value_count= merged_count;

  /* 2. Merge missed keys */

  /* no sorting needed */
  merged_count= 0;
  for (size_t j=0; j<num_results; j++)
  {
    for (size_t x=0; x<results[j]->missed_key_count; x++)
    {
      merged->missed_keys[merged_count]= results[j]->missed_keys[x];
      if (merged->smgmode != MEMCACHED_COLL_SMGET_NONE)
        merged->missed_causes[merged_count]= results[j]->missed_causes[x];
      merged_count++;
    }
    results[j]->missed_key_count= 0;
  }

  /* 3. Merge trimmed keys */

  memset(result_idx, 0, 256*sizeof(size_t));
  merged_count= 0;
  for (size_t i=0; i<merged->trimmed_key_count; i++)
  {
    int smallest_idx= -1;
    int comp_result;
    bool should_merge= false;
    for (size_t j=0; j<num_results; j++)
    {
      if (results[j]->trimmed_key_count == 0) {
        /* no elements in this result. */
        continue;
      }
      if (smallest_idx == -1) {
        smallest_idx= j;
        continue;
      }

      prev_sub_key = &results[smallest_idx]->trimmed_sub_keys[result_idx[smallest_idx]];
      curr_sub_key = &results[j]->trimmed_sub_keys[result_idx[j]];
      if (byte_array_bkey) {
        comp_result= memcached_compare_two_hexadecimal(&prev_sub_key->bkey_ext,
                                                       &curr_sub_key->bkey_ext);
      } else {
        comp_result= (prev_sub_key->bkey == curr_sub_key->bkey) ? 0
                   : ((prev_sub_key->bkey <  curr_sub_key->bkey) ? -1 : 1);
      }
      if (comp_result == 0) { /* compare key string */
        const char *prev_key= memcached_string_value(&results[smallest_idx]->keys[result_idx[smallest_idx]]);
        const char *curr_key= memcached_string_value(&results[j]->keys[result_idx[j]]);
        comp_result= strcmp(prev_key, curr_key);
      }

      if (memcached_is_descending(merged)) {
        if (comp_result < 0)
          smallest_idx= j;
      } else {
        if (comp_result > 0)
          smallest_idx= j;
      }
    }

    if (merged->value_count < merged->count)
    {
      /* compare it with the bkey of the last found element */
      prev_sub_key = &merged->sub_keys[merged->value_count-1];
      curr_sub_key = &results[smallest_idx]->trimmed_sub_keys[result_idx[smallest_idx]];
      if (byte_array_bkey) {
        comp_result= memcached_compare_two_hexadecimal(&prev_sub_key->bkey_ext,
                                                       &curr_sub_key->bkey_ext);
      } else {
        comp_result= (prev_sub_key->bkey == curr_sub_key->bkey) ? 0
                   : (prev_sub_key->bkey <  curr_sub_key->bkey) ? -1 : 1;
      }
      if (comp_result <= 0) {
          should_merge= true;
      }
    }

    if (should_merge)
    {
      /* merge */
      if (byte_array_bkey) {
        merged->trimmed_sub_keys[merged_count].bkey_ext= curr_sub_key->bkey_ext;
      } else {
        merged->trimmed_sub_keys[merged_count].bkey= curr_sub_key->bkey;
      }
      merged->trimmed_keys[merged_count]= results[smallest_idx]->trimmed_keys[result_idx[smallest_idx]];
      merged_count++;
    }
    else /* merged->value_count == merged->count */
    {
      /* free */
      memcached_string_free(&results[smallest_idx]->trimmed_keys[result_idx[smallest_idx]]);
      if (byte_array_bkey) {
        libmemcached_free(results[smallest_idx]->root,
                          results[smallest_idx]->trimmed_sub_keys[result_idx[smallest_idx]].bkey_ext.array);
      }
    }

    /* no more elements in this result */
    if (++result_idx[smallest_idx] >= results[smallest_idx]->trimmed_key_count)
    {
      results[smallest_idx]->trimmed_key_count = 0;
    }
  }

  merged->trimmed_key_count = merged_count;

  return rc;
}

memcached_coll_smget_result_st *
memcached_coll_smget_fetch_result(memcached_st *ptr,
                                  memcached_coll_smget_result_st *result,
                                  memcached_return_t *error,
                                  memcached_coll_type_t type)
{
  memcached_return_t unused;
  if (not error)
    error= &unused;

  assert(result != NULL);

  /* 1. Fetch results from the requested servers */
  memcached_coll_smget_result_st **results_on_each_server= NULL;
  memcached_return_t *responses_on_each_server= NULL;

  *error= MEMCACHED_SUCCESS;
  ALLOCATE_ARRAY_WITH_ERROR(ptr, results_on_each_server,   memcached_coll_smget_result_st *, memcached_server_count(ptr), error);
  ALLOCATE_ARRAY_WITH_ERROR(ptr, responses_on_each_server, memcached_return_t,               memcached_server_count(ptr), error);
  if (*error == MEMCACHED_MEMORY_ALLOCATION_FAILURE)
  {
    libmemcached_free(ptr, results_on_each_server);
    libmemcached_free(ptr, responses_on_each_server);
    return NULL;
  }

  memcached_coll_smget_result_st *each_result= NULL;
  size_t server_idx= 0;

  *error= MEMCACHED_MAXIMUM_RETURN; // We use this to see if we ever go into the loop
  memcached_return_t smget_error= MEMCACHED_SUCCESS;
  memcached_server_st *server = NULL; // Avoid compiler warning (-Wuninitialized)
  bool stay_on_server = false;
  while (stay_on_server || (server= memcached_io_get_readable_server(ptr)) != NULL)
  {
    stay_on_server = false;
    if (each_result == NULL)
    {
      each_result= memcached_coll_smget_result_create(ptr, NULL);

      if (not each_result)
      {
        *error= MEMCACHED_MEMORY_ALLOCATION_FAILURE;
        break;
      }
    }

    char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
    *error= memcached_coll_smget_response(server, buffer, sizeof(buffer), each_result);

    if (*error == MEMCACHED_IN_PROGRESS)
    {
      /* continue; */
      /* [ARCUS-237]
       * If we loop here, we may end up overwriting each_result with another
       * server's response.  This error is really a timeout.
       * So, bail out.
       */
      memcached_coll_smget_result_free(each_result);
      break;
    }
    else if (*error == MEMCACHED_SUCCESS)
    {
      /* [ARCUS-237]
       * We have processed VALUE and/or MISSED_KEYS.
       * Stay on this server and run smget_response again until we process END.
       * Otherwise, get_readable_server may return a different server.
       * Since we use the same each_result in smget_response, the function
       * may overflow various buffers (flags, eflags, and so on).
       *
       * Example:
       * 1. Call get_readable_server and get server1.
       * 2. Allocate smget_result (each_result).
       * 3. smget_response processes "VALUE 10 ..."
       *    Among other things, it uses ALLOCATE_ARRAY to allocate flags,
       *    eflags, and so on.
       * 4. We see MEMCACHED_SUCCESS.
       * 5. Call get_readable_server.
       *    Usually it returns the same server as (1).  That is, server1.
       *    But, it may return another server if the read buffer is empty.
       *    Suppose it returns server2.
       * 6. Call smget_response with the same each_result as (2).
       * 7. smget_response processes "VALUE 20 ..."
       * 8. ALLOCATE_ARRAY does not allocate new buffers.  Instead, we use the
       *    previously allocated buffers (flags, eflags, and so on).
       * 9. Since "VALUE 20 ..." has more values than "VALUE 10 ...",
       *    the code overruns the buffers, causing segmentation fault.
       */
      stay_on_server = true;
      continue;
    }
    else if (*error == MEMCACHED_END                or
             *error == MEMCACHED_DUPLICATED         or
             *error == MEMCACHED_DUPLICATED_TRIMMED or
             *error == MEMCACHED_TRIMMED            )
    {
      memcached_server_response_reset(server);
    }
    else if (*error == MEMCACHED_OUT_OF_RANGE  or
             *error == MEMCACHED_TYPE_MISMATCH or
             *error == MEMCACHED_BKEY_MISMATCH or
             *error == MEMCACHED_ATTR_MISMATCH)
    {
      memcached_coll_smget_result_free(each_result);
      if (smget_error == MEMCACHED_SUCCESS) {
        smget_error= *error;
      }
      each_result= NULL;
      continue;
    }
    else /* other failures */
    {
      memcached_coll_smget_result_free(each_result);
      break;
    }

    /* On MEMCACHED_END or something. */
    result->value_count+= each_result->value_count;
    result->missed_key_count+= each_result->missed_key_count;
    result->trimmed_key_count+= each_result->trimmed_key_count;

    responses_on_each_server[server_idx]= *error;
    results_on_each_server[server_idx]= each_result;
    server_idx++;
    each_result= NULL;
  }

  if (*error == MEMCACHED_MAXIMUM_RETURN)
  {
    *error= MEMCACHED_NOTFOUND;
  }

  while (*error == MEMCACHED_END                or
         *error == MEMCACHED_DUPLICATED         or
         *error == MEMCACHED_DUPLICATED_TRIMMED or
         *error == MEMCACHED_TRIMMED            )
  {
    if (smget_error != MEMCACHED_SUCCESS)
    {
      *error= smget_error;
      break;
    }

    /* Prepare the result */
    result->type= type;

    if (result->value_count > 0)
    {
      ALLOCATE_ARRAY_WITH_ERROR(result->root, result->keys,     memcached_string_st,       result->value_count, error);
      ALLOCATE_ARRAY_WITH_ERROR(result->root, result->values,   memcached_string_st,       result->value_count, error);
      ALLOCATE_ARRAY_WITH_ERROR(result->root, result->flags,    uint32_t,                  result->value_count, error);
      ALLOCATE_ARRAY_WITH_ERROR(result->root, result->sub_keys, memcached_coll_sub_key_st, result->value_count, error);
      ALLOCATE_ARRAY_WITH_ERROR(result->root, result->eflags,   memcached_hexadecimal_st,  result->value_count, error);
      ALLOCATE_ARRAY_WITH_ERROR(result->root, result->bytes,    size_t,                    result->value_count, error);
      if (*error == MEMCACHED_MEMORY_ALLOCATION_FAILURE) {
        break;
      }
    }
    if (result->missed_key_count > 0)
    {
      ALLOCATE_ARRAY_WITH_ERROR(result->root, result->missed_keys, memcached_string_st, result->missed_key_count, error);
      ALLOCATE_ARRAY_WITH_ERROR(result->root, result->missed_causes, memcached_return_t, result->missed_key_count, error);
      if (*error == MEMCACHED_MEMORY_ALLOCATION_FAILURE) {
        break;
      }
    }
    if (result->trimmed_key_count > 0)
    {
      ALLOCATE_ARRAY_WITH_ERROR(result->root, result->trimmed_keys,     memcached_string_st,       result->trimmed_key_count, error);
      ALLOCATE_ARRAY_WITH_ERROR(result->root, result->trimmed_sub_keys, memcached_coll_sub_key_st, result->trimmed_key_count, error);
      if (*error == MEMCACHED_MEMORY_ALLOCATION_FAILURE) {
        break;
      }
    }

    memcached_return_t response= merge_smget_results(results_on_each_server, responses_on_each_server, server_idx, result);
    memcached_set_last_response_code(ptr, response);
    *error = MEMCACHED_SUCCESS;
    break;
  }

  /* Free the intermediate results. */
  for (size_t x=0; x<server_idx; x++)
  {
    memcached_coll_smget_result_free(results_on_each_server[x]);
  }
  libmemcached_free(result->root, results_on_each_server);
  libmemcached_free(result->root, responses_on_each_server);

  /* Return the result */
  if (*error == MEMCACHED_SUCCESS)
  {
    return result;
  }
  else
  {
    if (memcached_is_allocated(result))
    {
      memcached_coll_smget_result_free(result);
    }
    else
    {
      memcached_coll_smget_result_reset(result);
    }
    memcached_set_last_response_code(ptr, *error);
    return NULL;
  }
}
