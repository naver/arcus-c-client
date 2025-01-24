/*
 * arcus-c-client : Arcus C client
 * Copyright 2017 JaM2in Co., Ltd.
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
#ifdef LIBMEMCACHED_WITH_ZK_INTEGRATION
#include "libmemcached/arcus_priv.h"
#endif

static memcached_return_t ascii_exist(memcached_st *memc,
                                      const char *group_key,
                                      size_t group_key_length,
                                      const char *key,
                                      size_t key_length)
{
#ifdef LIBMEMCACHED_WITH_ZK_INTEGRATION
  struct libmemcached_io_vector_st vector[]=
  {
    { sizeof("getattr ") -1, "getattr " },
    { memcached_array_size(memc->_namespace), memcached_array_string(memc->_namespace) },
    { key_length, key },
    { 2, "\r\n" }
  };
#else
  struct libmemcached_io_vector_st vector[]=
  {
    { sizeof("add ") -1, "add " },
    { memcached_array_size(memc->_namespace), memcached_array_string(memc->_namespace) },
    { key_length, key },
    { sizeof(" 0") -1, " 0" },
    { sizeof(" 2678400") -1, " 2678400" },
    { sizeof(" 0") -1, " 0" },
    { 2, "\r\n" },
    { 2, "\r\n" }
  };
#endif

  uint32_t server_key= memcached_generate_hash_with_redistribution(memc, group_key, group_key_length);
  memcached_server_write_instance_st instance= memcached_server_instance_fetch(memc, server_key);

  /* Send command header */
#ifdef LIBMEMCACHED_WITH_ZK_INTEGRATION
  memcached_return_t rc=  memcached_vdo(instance, vector, 4, true);
#else
  memcached_return_t rc=  memcached_vdo(instance, vector, 8, true);
#endif
  if (rc == MEMCACHED_SUCCESS)
  {
    char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
#ifdef LIBMEMCACHED_WITH_ZK_INTEGRATION
    while ((rc= memcached_coll_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL)) == MEMCACHED_ATTR);
    if (rc == MEMCACHED_END)
      rc= MEMCACHED_SUCCESS;
#else
    rc= memcached_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);

    if (rc == MEMCACHED_NOTSTORED)
      rc= MEMCACHED_SUCCESS;

    if (rc == MEMCACHED_STORED)
      rc= MEMCACHED_NOTFOUND;
#endif
  }

  return rc;
}

static memcached_return_t binary_exist(memcached_st *memc,
                                       const char *group_key,
                                       size_t group_key_length,
                                       const char *key,
                                       size_t key_length)
{
  protocol_binary_request_set request= {};
  size_t send_length= sizeof(request.bytes);

  request.message.header.request.magic= PROTOCOL_BINARY_REQ;
  request.message.header.request.opcode= PROTOCOL_BINARY_CMD_ADD;
  request.message.header.request.keylen= htons((uint16_t)(key_length + memcached_array_size(memc->_namespace)));
  request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;
  request.message.header.request.extlen= 8;
  request.message.body.flags= 0;
  request.message.body.expiration= htonl(2678400);

  request.message.header.request.bodylen= htonl((uint32_t) (key_length
                                                            +memcached_array_size(memc->_namespace)
                                                            +request.message.header.request.extlen));

  struct libmemcached_io_vector_st vector[]=
  {
    { send_length, request.bytes },
    { memcached_array_size(memc->_namespace), memcached_array_string(memc->_namespace) },
    { key_length, key }
  };

  uint32_t server_key= memcached_generate_hash_with_redistribution(memc, group_key, group_key_length);
  memcached_server_write_instance_st instance= memcached_server_instance_fetch(memc, server_key);

  /* write the header */
  memcached_return_t rc= memcached_vdo(instance, vector, 3, true);
  if (rc != MEMCACHED_SUCCESS) {
    return rc;
  }

  rc= memcached_response(instance, NULL, 0, NULL);

  if (rc == MEMCACHED_SUCCESS)
    rc= MEMCACHED_NOTFOUND;

  if (rc == MEMCACHED_DATA_EXISTS)
    rc= MEMCACHED_SUCCESS;

  return rc;
}

memcached_return_t memcached_exist(memcached_st *memc, const char *key, size_t key_length)
{
  return memcached_exist_by_key(memc, key, key_length, key, key_length);
}

memcached_return_t memcached_exist_by_key(memcached_st *memc,
                                          const char *group_key, size_t group_key_length,
                                          const char *key, size_t key_length)
{
#ifdef LIBMEMCACHED_WITH_ZK_INTEGRATION
  arcus_server_check_for_update(memc);
#endif
  memcached_return_t rc;
  if (memcached_failed(rc= initialize_query(memc)))
  {
    return rc;
  }

  if (memcached_failed(rc= memcached_validate_key_length(key_length, memc->flags.binary_protocol)))
  {
    return rc;
  }

  if (memcached_failed(rc= memcached_key_test(*memc, (const char **)&key, &key_length, 1)))
  {
    return rc;
  }

  if (memc->flags.use_udp)
  {
    return MEMCACHED_NOT_SUPPORTED;
  }

  if (memc->flags.binary_protocol)
  {
    rc= binary_exist(memc, group_key, group_key_length, key, key_length);
  }
  else
  {
    rc= ascii_exist(memc, group_key, group_key_length, key, key_length);
  }

  return rc;
}
