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
    strncpy(key, result_buffer->item_key, result_buffer->key_length); // For the binary protocol we will cut off the key :(
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
  while ((server= memcached_io_get_readable_server(ptr)))
  {
    char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
    *error= memcached_response(server, buffer, sizeof(buffer), result);

    if (*error == MEMCACHED_IN_PROGRESS)
    {
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
  else if (*error == MEMCACHED_SUCCESS)
  {
    *error= MEMCACHED_END;
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
    char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
    *error= memcached_coll_response(server, buffer, sizeof(buffer), result);
    memcached_set_last_response_code(ptr, *error);

    if (*error == MEMCACHED_IN_PROGRESS)
    {
      continue;
    }
    else if (*error == MEMCACHED_SUCCESS or
             *error == MEMCACHED_DELETED or *error == MEMCACHED_DELETED_DROPPED or
             *error == MEMCACHED_TRIMMED or *error == MEMCACHED_DELETED_TRIMMED )
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
merge_results(memcached_coll_smget_result_st **results,
              memcached_return_t *responses, size_t num_results,
              memcached_coll_smget_result_st *merged)
{
  memcached_return_t rc= MEMCACHED_END;
  size_t result_idx[256];
  memset(result_idx, 0, 256*sizeof(size_t));

  int merged_count= 0;
  int last_merged_index= 0;

  /* 1. Merge bkeys */
  for (size_t i=0; i<merged->value_count; i++)
  {
    // find a smallest value.
    int smallest_result_idx= 0;
    bool found_smallest_element= false;
    for (size_t j=0; j<num_results; j++)
    {
      if (results[j]->value_count == 0)
      {
        // no more elements in this result.
        if (not found_smallest_element)
        {
          smallest_result_idx++;
        }
        continue;
      }

      if (MEMCACHED_COLL_QUERY_BOP_EXT       == merged->sub_key_type or
          MEMCACHED_COLL_QUERY_BOP_EXT_RANGE == merged->sub_key_type )
      {
        memcached_hexadecimal_st *smallest_bkey= &results[smallest_result_idx]->sub_keys[result_idx[smallest_result_idx]].bkey_ext;
        memcached_hexadecimal_st *current_bkey= &results[j]->sub_keys[result_idx[j]].bkey_ext;

        if (memcached_is_descending(merged))
        {
          if (memcached_compare_two_hexadecimal(smallest_bkey, current_bkey) == -1)
          {
            smallest_result_idx= j;
          }
        }
        else
        {
          if (memcached_compare_two_hexadecimal(smallest_bkey, current_bkey) > -1)
          {
            smallest_result_idx= j;
          }
        }

        found_smallest_element= true;
      }
      else
      {
        uint64_t smallest_bkey= results[smallest_result_idx]->sub_keys[result_idx[smallest_result_idx]].bkey;
        uint64_t current_bkey= results[j]->sub_keys[result_idx[j]].bkey;

        if (memcached_is_descending(merged))
        {
          if (smallest_bkey < current_bkey)
          {
            smallest_result_idx= j;
          }
        }
        else
        {
          if (smallest_bkey >= current_bkey)
          {
            smallest_result_idx= j;
          }
        }

        found_smallest_element= true;
      }
    }

    // merge or free
    if (i >= merged->offset and i < merged->offset+merged->count)
    {
      // merge
      merged->keys  [merged_count]= results[smallest_result_idx]->keys  [result_idx[smallest_result_idx]];
      merged->values[merged_count]= results[smallest_result_idx]->values[result_idx[smallest_result_idx]];
      merged->flags [merged_count]= results[smallest_result_idx]->flags [result_idx[smallest_result_idx]];
      merged->eflags[merged_count]= results[smallest_result_idx]->eflags[result_idx[smallest_result_idx]];
      merged->bytes [merged_count]= results[smallest_result_idx]->bytes [result_idx[smallest_result_idx]];

      if (MEMCACHED_COLL_QUERY_BOP_EXT       == results[smallest_result_idx]->sub_key_type or
          MEMCACHED_COLL_QUERY_BOP_EXT_RANGE == results[smallest_result_idx]->sub_key_type )
      {
        merged->sub_keys[merged_count].bkey_ext= results[smallest_result_idx]->sub_keys[result_idx[smallest_result_idx]].bkey_ext;

        if (merged_count > 0 and
            memcached_compare_two_hexadecimal(&merged->sub_keys[merged_count  ].bkey_ext,
                                              &merged->sub_keys[merged_count-1].bkey_ext) == 0)
        {
          rc= MEMCACHED_DUPLICATED;
        }
      }
      else
      {
        merged->sub_keys[merged_count].bkey = results[smallest_result_idx]->sub_keys[result_idx[smallest_result_idx]].bkey;

        if (merged_count > 0 and
            merged->sub_keys[merged_count].bkey == merged->sub_keys[merged_count -1].bkey)
        {
          rc= MEMCACHED_DUPLICATED;
        }
      }

      merged_count++;
      last_merged_index= smallest_result_idx;
    }
    else
    {
      // free
      memcached_string_free(&results[smallest_result_idx]->keys[result_idx[smallest_result_idx]]);
      memcached_string_free(&results[smallest_result_idx]->values[result_idx[smallest_result_idx]]);
      libmemcached_free(results[smallest_result_idx]->root,
                        results[smallest_result_idx]->eflags[result_idx[smallest_result_idx]].array);

      if (MEMCACHED_COLL_QUERY_BOP_EXT       == results[smallest_result_idx]->sub_key_type or
          MEMCACHED_COLL_QUERY_BOP_EXT_RANGE == results[smallest_result_idx]->sub_key_type )
      {
        libmemcached_free(results[smallest_result_idx]->root,
                          results[smallest_result_idx]->sub_keys[result_idx[smallest_result_idx]].bkey_ext.array);
      }
    }

    // if there are no more elements in this result.
    if (++result_idx[smallest_result_idx] >= results[smallest_result_idx]->value_count)
    {
      results[smallest_result_idx]->value_count = 0;
    }
  }

  // set the count
  merged->value_count= merged_count;

  // determine the response message
  if (results[last_merged_index] and results[last_merged_index]->value_count == 0 and
      (MEMCACHED_TRIMMED == responses[last_merged_index] or
       MEMCACHED_DUPLICATED_TRIMMED == responses[last_merged_index]))
  {
    if (MEMCACHED_DUPLICATED == rc)
    {
      rc= MEMCACHED_DUPLICATED_TRIMMED;
    }
    else
    {
      rc= MEMCACHED_TRIMMED;
    }
  }

  /* 2. Merge missed keys */

  memset(result_idx, 0, 256*sizeof(size_t));

  for (size_t i=0; i<merged->missed_key_count; i++)
  {
    int smallest_result_idx = 0;
    bool found_smallest_element = false;
    for (size_t j=0; j<num_results; j++)
    {
      if (results[j]->missed_key_count == 0)
      {
        // no elements in this result.
        if (not found_smallest_element)
        {
          smallest_result_idx++;
        }
        continue;
      }

      const char *smallest_missed_key= memcached_string_value(&results[smallest_result_idx]->missed_keys[result_idx[smallest_result_idx]]);
      const char *current_missed_key= memcached_string_value(&results[j]->missed_keys[result_idx[j]]);

      size_t smallest_length= memcached_string_length(&results[smallest_result_idx]->missed_keys[result_idx[smallest_result_idx]]);
      size_t current_length= memcached_string_length(&results[j]->missed_keys[result_idx[j]]);

      if ((smallest_length > current_length) or
          (smallest_length == current_length and strcmp(smallest_missed_key, current_missed_key) >= 0))
      {
        smallest_result_idx = j;
      }

      found_smallest_element = true;
    }

    merged->missed_keys[i]= results[smallest_result_idx]->missed_keys[result_idx[smallest_result_idx]];

    // no more elements in this result.
    if (++result_idx[smallest_result_idx] >= results[smallest_result_idx]->missed_key_count)
    {
      results[smallest_result_idx]->missed_key_count = 0;
    }
  }

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
    else /* other failures */
    {
      memcached_coll_smget_result_free(each_result);
      break;
    }

    /* On MEMCACHED_END or something. */
    result->value_count+= each_result->value_count;
    result->missed_key_count+= each_result->missed_key_count;

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
      if (*error == MEMCACHED_MEMORY_ALLOCATION_FAILURE) {
        break;
      }
    }

    memcached_return_t response= merge_results(results_on_each_server, responses_on_each_server, server_idx, result);
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
