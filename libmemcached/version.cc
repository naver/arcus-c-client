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
 *  Copyright (C) 2010 Brian Aker All rights reserved.
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

const char * memcached_lib_version(void) 
{
  return LIBMEMCACHED_VERSION_STRING;
}

static inline memcached_return_t memcached_version_textual(memcached_st *ptr)
{
  libmemcached_io_vector_st vector[]=
  {
    { strlen("version\r\n"), "version\r\n" }
  };

  uint32_t success= 0;
  bool errors_happened= false;
  for (uint32_t x= 0; x < memcached_server_count(ptr); x++)
  {
    memcached_server_write_instance_st instance=
      memcached_server_instance_fetch(ptr, x);

    /* Optimization, we only fetch version once. */
    if (instance->major_version != UINT8_MAX)
    {
      continue;
    }

    memcached_return_t rrc= memcached_vdo(instance, vector, 1, true);
    if (memcached_failed(rrc))
    {
      errors_happened= true;
      (void)memcached_set_error(*instance, rrc, MEMCACHED_AT);
      memcached_io_reset(instance);
      continue;
    }
    success++;
  }

  if (success)
  {
    /* Collect the returned items */
    memcached_server_write_instance_st instance;
    while ((instance= memcached_io_get_readable_server(ptr)))
    {
      char buffer[32];
      memcached_return_t rrc= memcached_response(instance, buffer, sizeof(buffer), NULL);
      if (rrc != MEMCACHED_SUCCESS)
      {
        errors_happened= true;
      }
    }
  }

  return errors_happened ? MEMCACHED_SOME_ERRORS : MEMCACHED_SUCCESS;
}

static inline memcached_return_t memcached_version_binary(memcached_st *ptr)
{
  protocol_binary_request_version request= {};
  request.message.header.request.magic= PROTOCOL_BINARY_REQ;
  request.message.header.request.opcode= PROTOCOL_BINARY_CMD_VERSION;
  request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;

  libmemcached_io_vector_st vector[]=
  {
    { sizeof(request.bytes), request.bytes }
  };

  uint32_t success= 0;
  bool errors_happened= false;
  for (uint32_t x= 0; x < memcached_server_count(ptr); x++)
  {
    memcached_server_write_instance_st instance=
      memcached_server_instance_fetch(ptr, x);

    /* Optimization, we only fetch version once. */
    if (instance->major_version != UINT8_MAX)
    {
      continue;
    }

    memcached_return_t rrc= memcached_vdo(instance, vector, 1, true);
    if (memcached_failed(rrc))
    {
      errors_happened= true;
      (void)memcached_set_error(*instance, rrc, MEMCACHED_AT);
      memcached_io_reset(instance);
      continue;
    }
    success++;
  }

  if (success)
  {
    /* Collect the returned items */
    memcached_server_write_instance_st instance;
    while ((instance= memcached_io_get_readable_server(ptr)))
    {
      char buffer[32];
      memcached_return_t rrc= memcached_response(instance, buffer, sizeof(buffer), NULL);
      if (rrc != MEMCACHED_SUCCESS)
      {
        errors_happened= true;
      }
    }
  }

  return errors_happened ? MEMCACHED_SOME_ERRORS : MEMCACHED_SUCCESS;
}

static inline memcached_return_t version_ascii_instance(memcached_server_write_instance_st instance)
{
  memcached_return_t rc= MEMCACHED_SUCCESS;
  if (instance->major_version == UINT8_MAX)
  {
    libmemcached_io_vector_st vector[]=
    {
      { strlen("version\r\n"), "version\r\n" }
    };

    uint32_t before_active= instance->cursor_active;

    rc= memcached_vdo(instance, vector, 1, true);
    if (rc == MEMCACHED_WRITE_FAILURE)
    {
      (void)memcached_set_error(*instance, rc, MEMCACHED_AT);
      memcached_io_reset(instance);
      return rc;
    }
    /* If no_reply or piped is set, cursor_active is not increased in memcached_vdo().
     * Think calling memcached_version_instance() when connecting to a server
     * in pipe or no_reply operations.
     */
    if (before_active == instance->cursor_active)
    {
      memcached_server_response_increment(instance);
    }
    char buffer[32];
    rc= memcached_response(instance, buffer, sizeof(buffer), NULL);
  }
  return rc;
}

static inline memcached_return_t version_binary_instance(memcached_server_write_instance_st instance)
{
  memcached_return_t rc= MEMCACHED_SUCCESS;
  if (instance->major_version == UINT8_MAX)
  {
    protocol_binary_request_version request= {};
    request.message.header.request.magic= PROTOCOL_BINARY_REQ;
    request.message.header.request.opcode= PROTOCOL_BINARY_CMD_VERSION;
    request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;

    libmemcached_io_vector_st vector[]=
    {
      { sizeof(request.bytes), request.bytes }
    };

    uint32_t before_active= instance->cursor_active;

    rc= memcached_vdo(instance, vector, 1, true);
    if (rc == MEMCACHED_WRITE_FAILURE)
    {
      (void)memcached_set_error(*instance, rc, MEMCACHED_AT);
      memcached_io_reset(instance);
      return rc;
    }
    /* If no_reply or piped is set, cursor_active is not increased in memcached_vdo().
     * Think calling memcached_version_instance() when connecting to a server
     * in pipe or no_reply operations.
     */
    if (before_active == instance->cursor_active)
    {
      memcached_server_response_increment(instance);
    }
    char buffer[32];
    rc= memcached_response(instance, buffer, sizeof(buffer), NULL);
  }
  return rc;
}

memcached_return_t memcached_version(memcached_st *ptr)
{
  if (ptr)
  {
    arcus_server_check_for_update(ptr);

    memcached_return_t rc;
    if ((rc= initialize_query(ptr)) != MEMCACHED_SUCCESS)
    {
      return rc;
    }

    if (ptr->flags.use_udp)
    {
      return MEMCACHED_NOT_SUPPORTED;
    }

    if (ptr->flags.binary_protocol)
    {
      return memcached_version_binary(ptr);
    }
    return memcached_version_textual(ptr);
  }

  return MEMCACHED_INVALID_ARGUMENTS;
}

memcached_return_t memcached_version_instance(memcached_server_write_instance_st instance)
{
  if (instance && instance->root)
  {
    if (instance->root->flags.use_udp)
    {
      return MEMCACHED_NOT_SUPPORTED;
    }
    if (instance->root->flags.binary_protocol)
    {
      return version_binary_instance(instance);
    }
    return version_ascii_instance(instance);
  }

  return MEMCACHED_INVALID_ARGUMENTS;
}
