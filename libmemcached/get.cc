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
#include "libmemcached/arcus_priv.h"

static inline bool mget_command_is_supported(memcached_st *ptr, memcached_server_write_instance_st instance)
{
  /* mgets */
  if (ptr->flags.support_cas)
  {
    if (instance->major_version != UINT8_MAX && instance->minor_version != UINT8_MAX)
    {
      if (instance->is_enterprise)
      {
        /* >= 0.9.0-E */
        if (instance->major_version > 0 || (instance->major_version == 0 && instance->minor_version >= 9))
        {
          return true;
        }
      }
      else
      {
        /* >= 1.13.0 */
        if (instance->major_version > 1 || (instance->major_version == 1 && instance->minor_version >= 13))
        {
          return true;
        }
      }
    }
  }
  /* mget */
  else
  {
    if (instance->major_version != UINT8_MAX && instance->minor_version != UINT8_MAX)
    {
      if (instance->is_enterprise)
      {
        /* >= 0.7.0-E */
        if (instance->major_version > 0 || (instance->major_version == 0 && instance->minor_version >= 7))
        {
          return true;
        }
      }
      else
      {
        /* >= 1.11.0 */
        if (instance->major_version > 1 || (instance->major_version == 1 && instance->minor_version >= 11))
        {
          return true;
        }
      }
    }
  }
  return false;
}

static memcached_return_t before_get_query(memcached_st *ptr,
                                           const char *group_key,
                                           const size_t group_key_length,
                                           const char * const *keys,
                                           const size_t *key_length,
                                           size_t number_of_keys)
{
  memcached_return_t rc= initialize_query(ptr);
  if (memcached_failed(rc))
  {
    return rc;
  }

  if (ptr->flags.use_udp)
  {
    return memcached_set_error(*ptr, MEMCACHED_NOT_SUPPORTED, MEMCACHED_AT);
  }

  if (number_of_keys == 0)
  {
    return memcached_set_error(*ptr, MEMCACHED_NOTFOUND, MEMCACHED_AT,
                               memcached_literal_param("number_of_keys was zero"));
  }

  if (memcached_failed(memcached_key_test(*ptr, keys, key_length, number_of_keys)))
  {
    return memcached_set_error(*ptr, MEMCACHED_BAD_KEY_PROVIDED, MEMCACHED_AT,
                               memcached_literal_param("A bad key was provided"));
  }

  if (group_key and group_key_length)
  {
    if (memcached_failed(memcached_key_test(*ptr, (const char * const *)&group_key, &group_key_length, 1)))
    {
      return memcached_set_error(*ptr, MEMCACHED_BAD_KEY_PROVIDED, MEMCACHED_AT,
                                 memcached_literal_param("A bad group key was provided."));
    }
  }

  /*
    Here is where we pay for the non-block API. We need to remove any data sitting
    in the queue before we start our get.

    It might be optimum to bounce the connection if count > some number.
  */
  for (uint32_t x= 0; x < memcached_server_count(ptr); x++)
  {
    memcached_server_write_instance_st instance=
      memcached_server_instance_fetch(ptr, x);

    if (memcached_server_response_count(instance))
    {
      char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];

      if (ptr->flags.no_block)
        (void)memcached_io_write(instance, NULL, 0, true);

      while(memcached_server_response_count(instance))
        (void)memcached_response(instance, buffer, sizeof(buffer), &ptr->result);
    }
  }
  return MEMCACHED_SUCCESS;
}

static memcached_return_t ascii_get_by_key(memcached_st *ptr,
                                           uint32_t master_server_key,
                                           bool is_group_key_set,
                                           const char *key,
                                           const size_t key_length)
{
  memcached_return_t rc;
  memcached_server_write_instance_st instance;
  uint32_t server_key;

  if (is_group_key_set)
  {
    server_key= master_server_key;
  }
  else
  {
    server_key= memcached_generate_hash_with_redistribution(ptr, key, key_length);
  }
  instance= memcached_server_instance_fetch(ptr, server_key);

  const char *command= (ptr->flags.support_cas ? "gets " : "get ");
  struct libmemcached_io_vector_st vector[]=
  {
    { strlen(command), command },
    { memcached_array_size(ptr->_namespace), memcached_array_string(ptr->_namespace) },
    { key_length, key },
    { 2, "\r\n" }
  };

  rc= memcached_vdo(instance, vector, 4, true);
#ifdef MEMCACHED_VDO_ERROR_HANDLING
  if (rc != MEMCACHED_SUCCESS) {
#else
  if (memcached_failed(rc))
  {
    memcached_io_reset(instance);
#endif
    memcached_set_error(*ptr, rc, MEMCACHED_AT);
  }
  return rc;
}

static memcached_return_t simple_binary_mget(memcached_st *ptr,
                                             uint32_t master_server_key,
                                             bool is_group_key_set,
                                             const char * const *keys,
                                             const size_t *key_length,
                                             size_t number_of_keys, bool mget_mode)
{
  memcached_return_t rc= MEMCACHED_NOTFOUND;

  bool flush= (number_of_keys == 1);

  arcus_server_check_for_update(ptr);

  /*
    If a server fails we warn about errors and start all over with sending keys
    to the server.
  */
  for (uint32_t x= 0; x < number_of_keys; ++x)
  {
    uint32_t server_key;

    if (is_group_key_set)
    {
      server_key= master_server_key;
    }
    else
    {
      server_key= memcached_generate_hash_with_redistribution(ptr, keys[x], key_length[x]);
    }

    memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, server_key);

    if (memcached_server_response_count(instance) == 0)
    {
      rc= memcached_connect(instance);
      if (memcached_failed(rc))
      {
        continue;
      }
    }

    protocol_binary_request_getk request= { }; //= {.bytes= {0}};
    request.message.header.request.magic= PROTOCOL_BINARY_REQ;
    if (mget_mode)
      request.message.header.request.opcode= PROTOCOL_BINARY_CMD_GETKQ;
    else
      request.message.header.request.opcode= PROTOCOL_BINARY_CMD_GETK;

    memcached_return_t vk;
    vk= memcached_validate_key_length(key_length[x],
                                      ptr->flags.binary_protocol);
    unlikely (vk != MEMCACHED_SUCCESS)
    {
      if (x > 0)
      {
        memcached_io_reset(instance);
      }

      return vk;
    }

    request.message.header.request.keylen= htons((uint16_t)(key_length[x] + memcached_array_size(ptr->_namespace)));
    request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;
    request.message.header.request.bodylen= htonl((uint32_t)( key_length[x] + memcached_array_size(ptr->_namespace)));

    struct libmemcached_io_vector_st vector[]=
    {
      { sizeof(request.bytes), request.bytes },
      { memcached_array_size(ptr->_namespace), memcached_array_string(ptr->_namespace) },
      { key_length[x], keys[x] }
    };

    if (memcached_io_writev(instance, vector, 3, flush) == -1)
    {
      memcached_server_response_reset(instance);
      rc= MEMCACHED_SOME_ERRORS;
      continue;
    }

    /* We just want one pending response per server */
    memcached_server_response_reset(instance);
    memcached_server_response_increment(instance);
    if ((x > 0 && x == ptr->io_key_prefetch) && memcached_flush_buffers(ptr) != MEMCACHED_SUCCESS)
    {
      rc= MEMCACHED_SOME_ERRORS;
    }
  }

  if (mget_mode)
  {
    /*
      Send a noop command to flush the buffers
    */
    protocol_binary_request_noop request= {}; //= {.bytes= {0}};
    request.message.header.request.magic= PROTOCOL_BINARY_REQ;
    request.message.header.request.opcode= PROTOCOL_BINARY_CMD_NOOP;
    request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;

    for (uint32_t x= 0; x < memcached_server_count(ptr); ++x)
    {
      memcached_server_write_instance_st instance=
        memcached_server_instance_fetch(ptr, x);

      if (memcached_server_response_count(instance))
      {
        if (memcached_io_write(instance, NULL, 0, true) == -1)
        {
          memcached_server_response_reset(instance);
          memcached_io_reset(instance);
          rc= MEMCACHED_SOME_ERRORS;
        }

        if (memcached_io_write(instance, request.bytes,
                               sizeof(request.bytes), true) == -1)
        {
          memcached_server_response_reset(instance);
          memcached_io_reset(instance);
          rc= MEMCACHED_SOME_ERRORS;
        }
      }
    }
  }

  return rc;
}

static memcached_return_t replication_binary_mget(memcached_st *ptr,
                                                  uint32_t* hash,
                                                  bool* dead_servers,
                                                  const char *const *keys,
                                                  const size_t *key_length,
                                                  size_t number_of_keys)
{
  memcached_return_t rc= MEMCACHED_NOTFOUND;
  uint32_t start= 0;
  uint64_t randomize_read= memcached_behavior_get(ptr, MEMCACHED_BEHAVIOR_RANDOMIZE_REPLICA_READ);

  arcus_server_check_for_update(ptr);

  if (randomize_read)
    start= (uint32_t)random() % (uint32_t)(ptr->number_of_replicas + 1);

  /* Loop for each replica */
  for (uint32_t replica= 0; replica <= ptr->number_of_replicas; ++replica)
  {
    bool success= true;

    for (uint32_t x= 0; x < number_of_keys; ++x)
    {
      if (hash[x] == memcached_server_count(ptr))
        continue; /* Already successfully sent */

      uint32_t server= hash[x] + replica;

      /* In case of randomized reads */
      if (randomize_read && ((server + start) <= (hash[x] + ptr->number_of_replicas)))
        server += start;

      while (server >= memcached_server_count(ptr))
      {
        server -= memcached_server_count(ptr);
      }

      if (dead_servers[server])
      {
        continue;
      }

      memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, server);

      if (memcached_server_response_count(instance) == 0)
      {
        rc= memcached_connect(instance);

        if (memcached_failed(rc))
        {
          memcached_io_reset(instance);
          dead_servers[server]= true;
          success= false;
          continue;
        }
      }

      protocol_binary_request_getk request= {};
      request.message.header.request.magic= PROTOCOL_BINARY_REQ;
      request.message.header.request.opcode= PROTOCOL_BINARY_CMD_GETK;
      request.message.header.request.keylen= htons((uint16_t)(key_length[x] + memcached_array_size(ptr->_namespace)));
      request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;
      request.message.header.request.bodylen= htonl((uint32_t)(key_length[x] + memcached_array_size(ptr->_namespace)));

      /*
       * We need to disable buffering to actually know that the request was
       * successfully sent to the server (so that we should expect a result
       * back). It would be nice to do this in buffered mode, but then it
       * would be complex to handle all error situations if we got to send
       * some of the messages, and then we failed on writing out some others
       * and we used the callback interface from memcached_mget_execute so
       * that we might have processed some of the responses etc. For now,
       * just make sure we work _correctly_
     */
      struct libmemcached_io_vector_st vector[]=
      {
        { sizeof(request.bytes), request.bytes },
        { memcached_array_size(ptr->_namespace), memcached_array_string(ptr->_namespace) },
        { key_length[x], keys[x] }
      };

      if (memcached_io_writev(instance, vector, 3, true) == -1)
      {
        memcached_io_reset(instance);
        dead_servers[server]= true;
        success= false;
        continue;
      }

      memcached_server_response_increment(instance);
      hash[x]= memcached_server_count(ptr);
    }

    if (success)
      break;
  }

  return rc;
}

static memcached_return_t binary_mget_by_key(memcached_st *ptr,
                                             uint32_t master_server_key,
                                             bool is_group_key_set,
                                             const char * const *keys,
                                             const size_t *key_length,
                                             size_t number_of_keys,
                                             bool mget_mode)
{
  if (ptr->number_of_replicas == 0)
  {
    return simple_binary_mget(ptr, master_server_key, is_group_key_set,
                              keys, key_length, number_of_keys, mget_mode);
  }

  uint32_t* hash= static_cast<uint32_t*>(libmemcached_malloc(ptr, sizeof(uint32_t) * number_of_keys));
  bool* dead_servers= static_cast<bool*>(libmemcached_calloc(ptr, memcached_server_count(ptr), sizeof(bool)));

  if (hash == NULL || dead_servers == NULL)
  {
    libmemcached_free(ptr, hash);
    libmemcached_free(ptr, dead_servers);
    return MEMCACHED_MEMORY_ALLOCATION_FAILURE;
  }

  if (is_group_key_set)
  {
    for (size_t x= 0; x < number_of_keys; x++)
    {
      hash[x]= master_server_key;
    }
  }
  else
  {
    for (size_t x= 0; x < number_of_keys; x++)
    {
      hash[x]= memcached_generate_hash_with_redistribution(ptr, keys[x], key_length[x]);
    }
  }

  memcached_return_t rc= replication_binary_mget(ptr, hash, dead_servers, keys,
                                                 key_length, number_of_keys);

  WATCHPOINT_IFERROR(rc);
  libmemcached_free(ptr, hash);
  libmemcached_free(ptr, dead_servers);

  return MEMCACHED_SUCCESS;
}

/*
  What happens if no servers exist?
*/

char *memcached_get_by_key(memcached_st *ptr,
                           const char *group_key,
                           size_t group_key_length,
                           const char *key, size_t key_length,
                           size_t *value_length,
                           uint32_t *flags,
                           memcached_return_t *error)
{
  arcus_server_check_for_update(ptr);

  memcached_return_t unused;
  if (error == NULL)
    error= &unused;

  uint64_t query_id= 0;
  if (ptr)
  {
    query_id= ptr->query_id;
  }

  if (memcached_failed(*error= before_get_query(ptr, group_key, group_key_length,
                                                (const char * const *)&key, &key_length, 1)))
  {
    if (memcached_has_current_error(*ptr)) // Find the most accurate error
    {
      *error= memcached_last_error(ptr);
    }

    if (value_length)
    {
      *value_length= 0;
    }

    return NULL;
  }

  bool is_group_key_set= false;
  unsigned int master_server_key= (unsigned int)-1; /* 0 is a valid server id! */
  if (group_key and group_key_length)
  {
    master_server_key= memcached_generate_hash_with_redistribution(ptr, group_key, group_key_length);
    is_group_key_set= true;
  }

  /* Request the key */
  if (ptr->flags.binary_protocol)
  {
    *error= binary_mget_by_key(ptr, master_server_key, is_group_key_set,
                               (const char * const *)&key, &key_length, 1, false);
  }
  else
  {
    *error= ascii_get_by_key(ptr, master_server_key, is_group_key_set, key, key_length);
  }

  assert_msg(ptr->query_id >= query_id +1, "Programmer error, the query_id was not incremented.");
  //assert_msg(ptr->query_id == query_id +1, "Programmer error, the query_id was not incremented.");

  if (memcached_failed(*error))
  {
    if (memcached_has_current_error(*ptr)) // Find the most accurate error
    {
      *error= memcached_last_error(ptr);
    }

    if (value_length)
      *value_length= 0;

    return NULL;
  }

  char *value= memcached_fetch(ptr, NULL, NULL,
                               value_length, flags, error);
  assert_msg(ptr->query_id >= query_id +1, "Programmer error, the query_id was not incremented.");
  //assert_msg(ptr->query_id == query_id +1, "Programmer error, the query_id was not incremented.");

  /* This is for historical reasons */
  if (*error == MEMCACHED_END)
    *error= MEMCACHED_NOTFOUND;

  if (value == NULL)
  {
    if (ptr->get_key_failure && *error == MEMCACHED_NOTFOUND)
    {
      memcached_result_reset(&ptr->result);
      memcached_return_t rc= ptr->get_key_failure(ptr, key, key_length, &ptr->result);

      /* On all failure drop to returning NULL */
      if (rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED)
      {
        if (rc == MEMCACHED_BUFFERED)
        {
          uint64_t latch; /* We use latch to track the state of the original socket */
          latch= memcached_behavior_get(ptr, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS);
          if (latch == 0)
            memcached_behavior_set(ptr, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, 1);

          rc= memcached_set(ptr, key, key_length,
                            (memcached_result_value(&ptr->result)),
                            (memcached_result_length(&ptr->result)),
                            0,
                            (memcached_result_flags(&ptr->result)));

          if (rc == MEMCACHED_BUFFERED && latch == 0)
          {
            memcached_behavior_set(ptr, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, 0);
          }
        }
        else
        {
          rc= memcached_set(ptr, key, key_length,
                            (memcached_result_value(&ptr->result)),
                            (memcached_result_length(&ptr->result)),
                            0,
                            (memcached_result_flags(&ptr->result)));
        }

        if (rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED)
        {
          *error= rc;
          *value_length= memcached_result_length(&ptr->result);
          *flags= memcached_result_flags(&ptr->result);
          return memcached_string_take_value(&ptr->result.value);
        }
      }
    }
    assert_msg(ptr->query_id >= query_id +1, "Programmer error, the query_id was not incremented.");
    //assert_msg(ptr->query_id == query_id +1, "Programmer error, the query_id was not incremented.");

    return NULL;
  }

  size_t dummy_length;
  uint32_t dummy_flags;
  memcached_return_t dummy_error;

  char *dummy_value= memcached_fetch(ptr, NULL, NULL,
                                     &dummy_length, &dummy_flags,
                                     &dummy_error);
  assert_msg(dummy_value == 0, "memcached_fetch() returned additional values beyond the single get it expected");
  assert_msg(dummy_length == 0, "memcached_fetch() returned additional values beyond the single get it expected");
  assert_msg(ptr->query_id >= query_id +1, "Programmer error, the query_id was not incremented.");
  //assert_msg(ptr->query_id == query_id +1, "Programmer error, the query_id was not incremented.");

  return value;
}

char *memcached_get(memcached_st *ptr, const char *key,
                    size_t key_length,
                    size_t *value_length,
                    uint32_t *flags,
                    memcached_return_t *error)
{
  return memcached_get_by_key(ptr, NULL, 0, key, key_length, value_length,
                              flags, error);
}

memcached_return_t memcached_mget_by_key(memcached_st *ptr,
                                         const char *group_key,
                                         size_t group_key_length,
                                         const char * const *keys,
                                         const size_t *key_length,
                                         size_t number_of_keys)
{
  arcus_server_check_for_update(ptr);

  memcached_return_t rc;
  bool failures_occurred_in_sending= false;
  unsigned int master_server_key= (unsigned int)-1; /* 0 is a valid server id! */

  if (memcached_failed(rc= before_get_query(ptr, group_key, group_key_length,
                                            keys, key_length, number_of_keys)))
  {
    return rc;
  }

  if (memcached_server_count(ptr) > MAX_SERVERS_FOR_MULTI_KEY_OPERATION)
  {
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                               memcached_literal_param("memcached instances should be <= MAX_SERVERS_FOR_MULTI_KEY_OPERATION"));
  }

  LIBMEMCACHED_MEMCACHED_MGET_START();

  bool is_group_key_set= false;
  if (group_key and group_key_length)
  {
    master_server_key= memcached_generate_hash_with_redistribution(ptr, group_key, group_key_length);
    is_group_key_set= true;
  }

  if (ptr->flags.binary_protocol)
  {
    return binary_mget_by_key(ptr, master_server_key, is_group_key_set, keys,
                              key_length, number_of_keys, true);
  }

  /* Key-Server mapping */
  uint32_t *key_to_serverkey= NULL;
  ALLOCATE_ARRAY_OR_RETURN(ptr, key_to_serverkey, uint32_t, number_of_keys);

  for (uint32_t x= 0; x < number_of_keys; x++)
  {
    key_to_serverkey[x]= memcached_generate_hash_with_redistribution(ptr, keys[x], key_length[x]);
  }

  /* Prepare <lenkeys> and <numkeys> */
  uint32_t lenkeys[MAX_SERVERS_FOR_MULTI_KEY_OPERATION]= { 0 };
  uint16_t numkeys[MAX_SERVERS_FOR_MULTI_KEY_OPERATION]= { 0 };

  for (uint32_t x= 0; x < number_of_keys; x++)
  {
    uint32_t serverkey= key_to_serverkey[x];
    lenkeys[serverkey]+= (memcached_array_size(ptr->_namespace) + key_length[x] + 1); // +1 for the space( ).
    numkeys[serverkey]+= 1;
  }

  /*
    If a server fails we warn about errors and start all over with sending keys
    to the server.
  */
  WATCHPOINT_ASSERT(rc == MEMCACHED_SUCCESS);
  size_t hosts_connected= 0;
  bool hosts_failed[MAX_SERVERS_FOR_MULTI_KEY_OPERATION]= { false };
  int write_result= 0;
  for (uint32_t x= 0; x < number_of_keys; x++)
  {
    memcached_server_write_instance_st instance;
    uint32_t server_key;

    if (is_group_key_set)
    {
      server_key= master_server_key;
    }
    else
    {
      server_key= key_to_serverkey[x];
    }

    instance= memcached_server_instance_fetch(ptr, server_key);
    bool enable_mget= mget_command_is_supported(ptr, instance);
    if (enable_mget)
    {
      if (hosts_failed[server_key])
      {
        /* For mget command, the number of keys and length are mismatched,
         * so the remaining requests are not sent.
         */
        continue;
      }
    }

    struct libmemcached_io_vector_st vector[]=
    {
      { 0, NULL },
      { memcached_array_size(ptr->_namespace), memcached_array_string(ptr->_namespace) },
      { key_length[x], keys[x] }
    };
    size_t veclen= 3;

    if (memcached_server_response_count(instance) == 0)
    {
      rc= memcached_connect(instance);

      if (memcached_failed(rc))
      {
        if (rc != MEMCACHED_ERRNO)
        {
          memcached_set_error(*instance, rc, MEMCACHED_AT);
        }
        hosts_failed[server_key]= true;
        continue;
      }
      hosts_connected++;

      /* check enable_mget here again because the instance version
       * can be set for the first time in the memcached_connect() above.
       */
      enable_mget= mget_command_is_supported(ptr, instance);
      if (enable_mget)
      {
        const char *command= (ptr->flags.support_cas ? "mgets" : "mget");
        char command_buffer[80];
        vector[0].length= snprintf(command_buffer, 80, "%s %u %u\r\n",
                                   command,
                                   lenkeys[server_key]-1,
                                   numkeys[server_key]); // -1 for the space-less first key
        vector[0].buffer= command_buffer;

        /* Sending the request header */
        write_result= memcached_io_writev(instance, vector, veclen, false);
      }
      else
      {
        const char *command= (ptr->flags.support_cas ? "gets " : "get ");
        vector[0].length= strlen(command);
        vector[0].buffer= command;

        /* Sending the request header */
        write_result= memcached_io_writev(instance, vector, veclen, false);
      }

      if (write_result == -1)
      {
        failures_occurred_in_sending= true;
        hosts_failed[server_key]= true;
        continue;
      }

      WATCHPOINT_ASSERT(instance->cursor_active == 0);
      memcached_server_response_increment(instance);
      WATCHPOINT_ASSERT(instance->cursor_active == 1);
    }
    else
    {
      vector[0].length= 1;
      vector[0].buffer= " ";

      /* Sending space-separated key */
      write_result= memcached_io_writev(instance, vector, veclen, false);
      if (write_result == -1)
      {
        memcached_server_response_reset(instance);
        failures_occurred_in_sending= true;
        hosts_failed[server_key]= true;
        continue;
      }
    }
  }

  DEALLOCATE_ARRAY(ptr, key_to_serverkey);

  if (hosts_connected == 0)
  {
    LIBMEMCACHED_MEMCACHED_MGET_END();

    if (memcached_failed(rc))
      return rc;

    return memcached_set_error(*ptr, MEMCACHED_NO_SERVERS, MEMCACHED_AT);
  }


  /*
    Should we muddle on if some servers are dead?
  */
  bool success_happened= false;
  for (uint32_t x= 0; x < memcached_server_count(ptr); x++)
  {
    memcached_server_write_instance_st instance=
      memcached_server_instance_fetch(ptr, x);

    if (memcached_server_response_count(instance))
    {
      /* We need to do something about non-connnected hosts in the future */
      if ((memcached_io_write(instance, "\r\n", 2, true)) == -1)
      {
        failures_occurred_in_sending= true;
      }
      else
      {
        success_happened= true;
      }
    }
  }

  LIBMEMCACHED_MEMCACHED_MGET_END();

  if (failures_occurred_in_sending && success_happened)
  {
    return MEMCACHED_SOME_ERRORS;
  }

  if (success_happened)
    return MEMCACHED_SUCCESS;

  return MEMCACHED_FAILURE; // Complete failure occurred
}

memcached_return_t memcached_mget(memcached_st *ptr,
                                  const char * const *keys,
                                  const size_t *key_length,
                                  size_t number_of_keys)
{
  return memcached_mget_by_key(ptr, NULL, 0, keys, key_length, number_of_keys);
}

memcached_return_t memcached_mget_execute_by_key(memcached_st *ptr,
                                                 const char *group_key,
                                                 size_t group_key_length,
                                                 const char * const *keys,
                                                 const size_t *key_length,
                                                 size_t number_of_keys,
                                                 memcached_execute_fn *callback,
                                                 void *context,
                                                 unsigned int number_of_callbacks)
{
  if ((ptr->flags.binary_protocol) == 0)
    return MEMCACHED_NOT_SUPPORTED;

  memcached_return_t rc;
  memcached_callback_st *original_callbacks= ptr->callbacks;
  memcached_callback_st cb= {
    callback,
    context,
    number_of_callbacks
  };

  ptr->callbacks= &cb;
  rc= memcached_mget_by_key(ptr, group_key, group_key_length, keys,
                            key_length, number_of_keys);
  ptr->callbacks= original_callbacks;
  return rc;
}

memcached_return_t memcached_mget_execute(memcached_st *ptr,
                                          const char * const *keys,
                                          const size_t *key_length,
                                          size_t number_of_keys,
                                          memcached_execute_fn *callback,
                                          void *context,
                                          unsigned int number_of_callbacks)
{
  return memcached_mget_execute_by_key(ptr, NULL, 0, keys, key_length,
                                       number_of_keys, callback,
                                       context, number_of_callbacks);
}
