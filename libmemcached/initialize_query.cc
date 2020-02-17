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

memcached_return_t initialize_query(memcached_st *self)
{
  if (not self)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  self->query_id++;

  if (self->state.is_time_for_rebuild)
  {
    memcached_reset(self);
  }

  if (memcached_server_count(self) == 0)
  {
    return memcached_set_error(*self, MEMCACHED_NO_SERVERS, MEMCACHED_AT);
  }

  return MEMCACHED_SUCCESS;
}

memcached_return_t initialize_const_query(const memcached_st *self)
{
  if (not self)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  if (memcached_server_count(self) == 0)
  {
    return MEMCACHED_NO_SERVERS;
  }

  return MEMCACHED_SUCCESS;
}

memcached_return_t before_query(memcached_st *self,
                                const char * const *keys,
                                const size_t *key_length,
                                size_t number_of_keys)
{
  memcached_return_t rc;

  if (memcached_failed(rc= initialize_query(self)))
  {
    return rc;
  }

  unlikely (self->flags.use_udp || self->flags.binary_protocol)
  {
    return memcached_set_error(*self, MEMCACHED_NOT_SUPPORTED, MEMCACHED_AT, memcached_literal_param("Binary protocol is not supported"));
  }

  if (memcached_failed(memcached_key_test(*self, keys, key_length, number_of_keys)))
  {
    return memcached_set_error(*self, MEMCACHED_BAD_KEY_PROVIDED, MEMCACHED_AT, memcached_literal_param("A bad key value was provided"));
  }

  /*
    Here is where we pay for the non-block API. We need to remove any data sitting
    in the queue before we start our get.

    It might be optimum to bounce the connection if count > some number.
   */
  for (uint32_t x= 0; x < memcached_server_count(self); x++)
  {
    memcached_server_write_instance_st instance=
      memcached_server_instance_fetch(self, x);

    if (memcached_server_response_count(instance))
    {
      char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];

      if (self->flags.no_block)
      {
        (void)memcached_io_write(instance, NULL, 0, true);
      }

      while (memcached_server_response_count(instance))
      {
        (void)memcached_coll_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, &self->collection_result);
      }
    }
  }

  return MEMCACHED_SUCCESS;
}
