/*
 * arcus-c-client : Arcus C client
 * Copyright 2025-current JaM2in Co., Ltd.
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

#include "libmemcached/common.h"
#include "libmemcached/arcus_priv.h"

static memcached_return_t ascii_touch(memcached_st *ptr,
                                      const char *group_key,
                                      const size_t group_key_length,
                                      const char *key,
                                      size_t key_length,
                                      time_t expiration)
{
  bool to_write= (ptr->flags.buffer_requests) ? false : true;
  bool no_reply= (ptr->flags.no_reply);

  char expiration_buffer[MEMCACHED_MAXIMUM_INTEGER_DISPLAY_LENGTH + 1 + 1];
  int expiration_buffer_length = snprintf(expiration_buffer, sizeof(expiration_buffer), " %lld",
                                          (long long) expiration);
  if (size_t(expiration_buffer_length) >= sizeof(expiration_buffer)
      or expiration_buffer_length < 0)
  {
    return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                               memcached_literal_param("snprintf(MEMCACHED_MAXIMUM_INTEGER_DISPLAY_LENGTH)"));
  }

  struct libmemcached_io_vector_st vector[]=
  {
    { 6, "touch " },
    { memcached_array_size(ptr->_namespace), memcached_array_string(ptr->_namespace) },
    { key_length, key },
    { (size_t)expiration_buffer_length, expiration_buffer },
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
    for (uint32_t x= 0; x < 6; x++)
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

  memcached_return_t rc= memcached_vdo(instance, vector, 6, to_write);
  if (rc != MEMCACHED_SUCCESS)
  {
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
#ifdef ENABLE_REPLICATION
    if (rc == MEMCACHED_SWITCHOVER or rc == MEMCACHED_REPL_SLAVE)
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

memcached_return_t memcached_touch(memcached_st *ptr, const char *key, size_t key_length,
                                   time_t expiration)
{
  return memcached_touch_by_key(ptr, key, key_length, key, key_length, expiration);
}

memcached_return_t memcached_touch_by_key(memcached_st *ptr,
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

  if (memcached_failed(rc= memcached_key_test(*ptr, (const char **) &key, &key_length, 1)))
  {
    return rc;
  }

  LIBMEMCACHED_MEMCACHED_TOUCH_START();
  if (ptr->flags.binary_protocol)
  {
    rc= MEMCACHED_NOT_SUPPORTED;
  }
  else
  {
    rc= ascii_touch(ptr, group_key, group_key_length, key, key_length, expiration);
  }
  LIBMEMCACHED_MEMCACHED_TOUCH_END();

  return rc;
}
