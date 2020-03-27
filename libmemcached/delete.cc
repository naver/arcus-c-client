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
                                              size_t group_key_length,
                                              const char *key,
                                              size_t key_length,
                                              time_t expiration)
{
  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
  bool to_write= (ptr->flags.buffer_requests) ? false : true;
  bool no_reply= (ptr->flags.no_reply);
  int send_length;
  memcached_return_t rc;

  uint32_t server_key= memcached_generate_hash_with_redistribution(ptr, group_key, group_key_length);
  memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, server_key);

#ifdef ENABLE_REPLICATION
do_action:
#endif
  unlikely (expiration)
  {
     if ((instance->major_version == 1 &&
          instance->minor_version > 2) ||
          instance->major_version > 1)
     {
       return MEMCACHED_INVALID_ARGUMENTS;
     }

     /* ensure that we are connected, otherwise we might bump the
      * command counter before connection */
     if ((rc= memcached_connect(instance)) != MEMCACHED_SUCCESS)
     {
       WATCHPOINT_ERROR(rc);
       return rc;
     }

     if (instance->minor_version == 0)
     {
        if (no_reply or to_write == false)
        {
           /* We might get out of sync with the server if we
            * send this command to a server newer than 1.2.x..
            * disable no_reply and buffered mode.
            */
           to_write= true;
           if (no_reply)
              memcached_server_response_increment(instance);
           no_reply= false;
        }
     }
     send_length= snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                           "delete %.*s%.*s %u%s\r\n",
                           memcached_print_array(ptr->_namespace),
                           (int) key_length, key,
                           (uint32_t)expiration,
                           no_reply ? " noreply" :"" );
  }
  else
  {
    send_length= snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                          "delete %.*s%.*s%s\r\n",
                          memcached_print_array(ptr->_namespace),
                          (int)key_length, key, no_reply ? " noreply" :"");
  }

  if (send_length >= MEMCACHED_DEFAULT_COMMAND_SIZE || send_length < 0)
  {
    return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                               memcached_literal_param("snprintf(MEMCACHED_DEFAULT_COMMAND_SIZE)"));
  }

  if (ptr->flags.use_udp and to_write == false)
  {
    if (send_length > MAX_UDP_DATAGRAM_LENGTH - UDP_DATAGRAM_HEADER_LENGTH)
      return MEMCACHED_WRITE_FAILURE;

    if (send_length + instance->write_buffer_offset > MAX_UDP_DATAGRAM_LENGTH)
    {
      memcached_io_write(instance, NULL, 0, true);
    }
  }

  rc= memcached_do(instance, buffer, (size_t)send_length, to_write);
  if (rc != MEMCACHED_SUCCESS)
  {
    memcached_io_reset(instance);
    return rc;
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
      memcached_rgroup_switchover(ptr, instance);
      instance= memcached_server_instance_fetch(ptr, server_key);
      goto do_action;
    }
#endif
  }

  return rc;
}

static inline memcached_return_t binary_delete(memcached_st *ptr,
                                               const char *group_key,
                                               size_t group_key_length,
                                               const char *key,
                                               size_t key_length,
                                               time_t expiration)
{
  protocol_binary_request_delete request= {};
  bool to_write= (ptr->flags.buffer_requests) ? false : true;
  bool no_reply= (ptr->flags.no_reply);

  if (expiration) {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

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

  memcached_return_t rc= MEMCACHED_SUCCESS;
  if ((rc= memcached_vdo(instance, vector, 3, to_write)) != MEMCACHED_SUCCESS)
  {
    memcached_io_reset(instance);
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

      if (memcached_vdo(replica, vector, 3, to_write) != MEMCACHED_SUCCESS)
      {
        memcached_io_reset(replica);
      }
      else
      {
        memcached_server_response_decrement(replica);
      }
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
      memcached_rgroup_switchover(ptr, instance);
      instance= memcached_server_instance_fetch(ptr, server_key);
      goto do_action;
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

  LIBMEMCACHED_MEMCACHED_DELETE_START();
  if (ptr->flags.binary_protocol)
  {
    rc= binary_delete(ptr, group_key, group_key_length, key, key_length, expiration);
  }
  else
  {
    rc= ascii_delete(ptr, group_key, group_key_length, key, key_length, expiration);
  }

  if (rc == MEMCACHED_SUCCESS and ptr->delete_trigger)
  {
    ptr->delete_trigger(ptr, key, key_length);
  }
  LIBMEMCACHED_MEMCACHED_DELETE_END();

  return rc;
}
