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

static memcached_return_t memcached_flush_binary(memcached_st *ptr,
                                                 time_t expiration);

static memcached_return_t memcached_flush_textual(memcached_st *ptr,
                                                  const char *prefix,
                                                  time_t expiration);

memcached_return_t memcached_flush_by_prefix(memcached_st *ptr,
                                             const char *prefix,
                                             size_t prefix_length,
                                             time_t expiration)
{
  memcached_return_t rc;

  arcus_server_check_for_update(ptr);

  if (memcached_failed(rc= initialize_query(ptr)))
  {
    return rc;
  }

  if (not prefix)
  {
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS,
           MEMCACHED_AT,
           memcached_literal_param("given prefix is null"));
  }

  if (prefix_length == 0)
  {
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS,
           MEMCACHED_AT,
           memcached_literal_param("given prefix length == 0"));
  }

  LIBMEMCACHED_MEMCACHED_FLUSH_START();
  if (ptr->flags.binary_protocol)
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS,
           MEMCACHED_AT,
           memcached_literal_param("Binary protocol is not supported yet"));
  else
    rc= memcached_flush_textual(ptr, prefix, expiration);

  LIBMEMCACHED_MEMCACHED_FLUSH_END();
  return rc;
}

memcached_return_t memcached_flush(memcached_st *ptr, time_t expiration)
{
  memcached_return_t rc;

  arcus_server_check_for_update(ptr);

  if (memcached_failed(rc= initialize_query(ptr)))
  {
    return rc;
  }

  LIBMEMCACHED_MEMCACHED_FLUSH_START();
  if (ptr->flags.binary_protocol)
    rc= memcached_flush_binary(ptr, expiration);
  else
    rc= memcached_flush_textual(ptr, NULL, expiration);

  LIBMEMCACHED_MEMCACHED_FLUSH_END();
  return rc;
}

static memcached_return_t memcached_flush_textual(memcached_st *ptr,
                                                  const char *prefix,
                                                  time_t expiration)
{
  unlikely (memcached_server_count(ptr) == 0)
    return MEMCACHED_NO_SERVERS;

  for (unsigned int x= 0; x < memcached_server_count(ptr); x++)
  {
    memcached_return_t rc;
    char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];

    bool no_reply= ptr->flags.no_reply;
    memcached_server_write_instance_st instance=
      memcached_server_instance_fetch(ptr, x);

    int send_length;
    if (expiration)
    {
      send_length= snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                            "%s%s%s%s%llu%s\r\n",
                            prefix ? "flush_prefix" : "flush_all",
                            prefix ? " " : "",
                            prefix ? prefix : "",
                            prefix ? " " : "",
                            (unsigned long long)expiration, no_reply ? " noreply" : "");
    }
    else
    {
      send_length= snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                            "%s%s%s%s%s\r\n",
                            prefix ? "flush_prefix" : "flush_all",
                            prefix ? " " : "",
                            prefix ? prefix : "",
                            prefix ? " " : "",
                            no_reply ? " noreply" : "");
    }

    if (send_length >= MEMCACHED_DEFAULT_COMMAND_SIZE || send_length < 0)
    {
      return memcached_set_error(*instance, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                                 memcached_literal_param("snprintf(MEMCACHED_DEFAULT_COMMAND_SIZE)"));
    }

    rc= memcached_do(instance, buffer, (size_t)send_length, true);

    if (rc == MEMCACHED_SUCCESS && !no_reply)
    {
      char result[MEMCACHED_DEFAULT_COMMAND_SIZE];
      (void)memcached_response(instance, result, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);
    }
  }

  return MEMCACHED_SUCCESS;
}

static memcached_return_t memcached_flush_binary(memcached_st *ptr,
                                                 time_t expiration)
{
  protocol_binary_request_flush request= {};

  unlikely (memcached_server_count(ptr) == 0)
    return MEMCACHED_NO_SERVERS;

  request.message.header.request.magic= (uint8_t)PROTOCOL_BINARY_REQ;
  request.message.header.request.opcode= PROTOCOL_BINARY_CMD_FLUSH;
  request.message.header.request.extlen= 4;
  request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;
  request.message.header.request.bodylen= htonl(request.message.header.request.extlen);
  request.message.body.expiration= htonl((uint32_t) expiration);

  for (uint32_t x= 0; x < memcached_server_count(ptr); x++)
  {
    memcached_server_write_instance_st instance=
      memcached_server_instance_fetch(ptr, x);

    if (ptr->flags.no_reply)
    {
      request.message.header.request.opcode= PROTOCOL_BINARY_CMD_FLUSHQ;
    }
    else
    {
      request.message.header.request.opcode= PROTOCOL_BINARY_CMD_FLUSH;
    }

    if (memcached_do(instance, request.bytes, sizeof(request.bytes), true) != MEMCACHED_SUCCESS)
    {
      memcached_io_reset(instance);
      return MEMCACHED_WRITE_FAILURE;
    }
  }

  for (uint32_t x= 0; x < memcached_server_count(ptr); x++)
  {
    memcached_server_write_instance_st instance=
      memcached_server_instance_fetch(ptr, x);

    if (memcached_server_response_count(instance) > 0)
      (void)memcached_response(instance, NULL, 0, NULL);
  }

  return MEMCACHED_SUCCESS;
}
