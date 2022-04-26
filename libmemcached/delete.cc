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
#include <libmemcached/memcached/protocol_binary.h>
#include "libmemcached/arcus_priv.h"

static inline memcached_return_t ascii_delete(memcached_st *ptr,
                                              const char *group_key,
                                              const size_t group_key_length,
                                              const char *key,
                                              const size_t key_length)
{
  bool to_write= (ptr->flags.buffer_requests) ? false : true;
  bool no_reply= (ptr->flags.no_reply);

  struct libmemcached_io_vector_st vector[]=
  {
    { 7, "delete " },
    { memcached_array_size(ptr->_namespace), memcached_array_string(ptr->_namespace) },
    { key_length, key },
    { (no_reply ? memcached_literal_param_size(" noreply") : 0), " noreply" },
    { 2, "\r\n" }
  };

  uint32_t server_key= memcached_generate_hash_with_redistribution(ptr, group_key, group_key_length);
  memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, server_key);

#ifdef ENABLE_REPLICATION
do_action:
#endif
  if (ptr->flags.use_udp && to_write == false)
  {
    size_t cmd_size= 0;
    for (uint32_t x= 0; x < 5; x++)
    {
      cmd_size+= vector[x].length;
    }

    if (cmd_size > MAX_UDP_DATAGRAM_LENGTH - UDP_DATAGRAM_HEADER_LENGTH)
    {
      return memcached_set_error(*ptr, MEMCACHED_WRITE_FAILURE, MEMCACHED_AT);
    }

    if (cmd_size + instance->write_buffer_offset > MAX_UDP_DATAGRAM_LENGTH)
    {
      memcached_io_write(instance, NULL, 0, true);
    }
  }

  /* Send command header */
  memcached_return_t rc= memcached_vdo(instance, vector, 5, to_write);

  if (rc == MEMCACHED_SUCCESS)
  {
    if (to_write == false)
    {
      rc= MEMCACHED_BUFFERED;
    }
    else if (no_reply == false)
    {
      char result[MEMCACHED_DEFAULT_COMMAND_SIZE];
      rc= memcached_response(instance, result, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);

      if (rc == MEMCACHED_DELETED)
      {
        rc= MEMCACHED_SUCCESS;
      }
#ifdef ENABLE_REPLICATION
      else if (rc == MEMCACHED_SWITCHOVER or rc == MEMCACHED_REPL_SLAVE)
      {
        ZOO_LOG_INFO(("Switchover: hostname=%s port=%d error=%s",
                      instance->hostname, instance->port, memcached_strerror(ptr, rc)));
        if (memcached_rgroup_switchover(ptr, instance) == true) {
          instance= memcached_server_instance_fetch(ptr, server_key);
          goto do_action;
        }
      }
#endif
    }
  }
#ifdef MEMCACHED_VDO_ERROR_HANDLING
#else
  else
  {
    memcached_io_reset(instance);
    return rc;
  }
#endif

  return rc;
}

static inline memcached_return_t binary_delete(memcached_st *ptr,
                                               const char *group_key,
                                               size_t group_key_length,
                                               const char *key,
                                               size_t key_length)
{
  protocol_binary_request_delete request= {};
  bool to_write= (ptr->flags.buffer_requests) ? false : true;
  bool no_reply= (ptr->flags.no_reply);

  request.message.header.request.magic= PROTOCOL_BINARY_REQ;
  if (no_reply)
  {
    request.message.header.request.opcode= PROTOCOL_BINARY_CMD_DELETEQ;
  }
  else
  {
    request.message.header.request.opcode= PROTOCOL_BINARY_CMD_DELETE;
  }
  request.message.header.request.keylen= htons((uint16_t)(key_length + memcached_array_size(ptr->_namespace)));
  request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;
  request.message.header.request.bodylen= htonl((uint32_t)(key_length + memcached_array_size(ptr->_namespace)));

  struct libmemcached_io_vector_st vector[]=
  {
    { sizeof(request.bytes), request.bytes},
    { memcached_array_size(ptr->_namespace), memcached_array_string(ptr->_namespace) },
    { key_length, key },
  };

  uint32_t server_key= memcached_generate_hash_with_redistribution(ptr, group_key, group_key_length);
  memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, server_key);

#ifdef ENABLE_REPLICATION
do_action:
#endif
  if (ptr->flags.use_udp && ! to_write)
  {
    size_t cmd_size= sizeof(request.bytes) + key_length;
    if (cmd_size > MAX_UDP_DATAGRAM_LENGTH - UDP_DATAGRAM_HEADER_LENGTH)
      return MEMCACHED_WRITE_FAILURE;

    if (cmd_size + instance->write_buffer_offset > MAX_UDP_DATAGRAM_LENGTH)
      memcached_io_write(instance, NULL, 0, true);
  }

#ifdef MEMCACHED_VDO_ERROR_HANDLING
  memcached_return_t rc= memcached_vdo(instance, vector, 3, to_write);
  if (rc != MEMCACHED_SUCCESS) {
#else
  memcached_return_t rc= MEMCACHED_SUCCESS;
  if ((rc= memcached_vdo(instance, vector, 3, to_write)) != MEMCACHED_SUCCESS)
  {
    memcached_io_reset(instance);
#endif
    return rc;
  }

  unlikely (ptr->number_of_replicas > 0)
  {
    request.message.header.request.opcode= PROTOCOL_BINARY_CMD_DELETEQ;

    for (uint32_t x= 0; x < ptr->number_of_replicas; ++x)
    {
      memcached_server_write_instance_st replica;

      ++server_key;
      if (server_key == memcached_server_count(ptr))
        server_key= 0;

      replica= memcached_server_instance_fetch(ptr, server_key);

#ifdef MEMCACHED_VDO_ERROR_HANDLING
      if (memcached_vdo(replica, vector, 3, to_write) == MEMCACHED_SUCCESS) {
        memcached_server_response_decrement(replica);
      }
#else
      if (memcached_vdo(replica, vector, 3, to_write) != MEMCACHED_SUCCESS)
      {
        memcached_io_reset(replica);
      }
      else
      {
        memcached_server_response_decrement(replica);
      }
#endif
    }
  }

  if (to_write == false)
  {
    rc= MEMCACHED_BUFFERED;
  }
  else if (no_reply == false)
  {
    char result[MEMCACHED_DEFAULT_COMMAND_SIZE];
    rc= memcached_response(instance, result, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);
    if (rc == MEMCACHED_DELETED)
    {
      rc= MEMCACHED_SUCCESS;
    }
#ifdef ENABLE_REPLICATION
    else if (rc == MEMCACHED_SWITCHOVER or rc == MEMCACHED_REPL_SLAVE)
    {
      ZOO_LOG_INFO(("Switchover: hostname=%s port=%d error=%s",
                    instance->hostname, instance->port, memcached_strerror(ptr, rc)));
      if (memcached_rgroup_switchover(ptr, instance) == true) {
        instance= memcached_server_instance_fetch(ptr, server_key);
        goto do_action;
      }
    }
#endif
  }

  return rc;
}

memcached_return_t memcached_delete(memcached_st *ptr, const char *key, size_t key_length,
                                    time_t expiration)
{
  return memcached_delete_by_key(ptr, key, key_length,
                                 key, key_length, expiration);
}

memcached_return_t memcached_delete_by_key(memcached_st *ptr,
                                           const char *group_key, size_t group_key_length,
                                           const char *key, size_t key_length,
                                           time_t expiration)
{
  arcus_server_check_for_update(ptr);

  memcached_return_t rc;
  if (memcached_failed(rc= initialize_query(ptr)))
  {
    return rc;
  }

  if (memcached_failed(rc= memcached_validate_key_length(key_length, ptr->flags.binary_protocol)))
  {
    return rc;
  }

  if (memcached_failed(rc= memcached_key_test(*ptr, (const char **)&key, &key_length, 1)))
  {
    return rc;
  }

  if (expiration)
  {
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                               memcached_literal_param("Memcached server does not allow expiration of deleted items"));
  }

  LIBMEMCACHED_MEMCACHED_DELETE_START();
  if (ptr->flags.binary_protocol)
  {
    rc= binary_delete(ptr, group_key, group_key_length, key, key_length);
  }
  else
  {
    rc= ascii_delete(ptr, group_key, group_key_length, key, key_length);
  }

  if (rc == MEMCACHED_SUCCESS and ptr->delete_trigger)
  {
    ptr->delete_trigger(ptr, key, key_length);
  }
  LIBMEMCACHED_MEMCACHED_DELETE_END();

  return rc;
}
