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

#pragma once


#ifdef __cplusplus
extern "C" {
#endif

/* Server List Public functions */
LIBMEMCACHED_API
  void memcached_server_list_free(memcached_server_list_st ptr);

#ifdef LIBMEMCACHED_WITH_ZK_INTEGRATION
LIBMEMCACHED_API
  void memcached_server_prune(memcached_st *ptr, bool all_flag);
#endif

LIBMEMCACHED_API
  memcached_return_t memcached_server_push_with_count(memcached_st *ptr,
                                                      const memcached_server_list_st list,
                                                      uint32_t count);

LIBMEMCACHED_API
  memcached_return_t memcached_server_push(memcached_st *ptr, const memcached_server_list_st list);

#ifdef LIBMEMCACHED_WITH_ZK_INTEGRATION
LIBMEMCACHED_API
  memcached_return_t memcached_server_push_with_serverinfo(memcached_st *ptr, 
                                                           memcached_server_info_st *serverinfo, 
                                                           uint32_t servercount);
#endif

LIBMEMCACHED_API
  memcached_server_list_st memcached_server_list_append(memcached_server_list_st ptr,
                                                        const char *hostname,
                                                        in_port_t port,
                                                        memcached_return_t *error);
LIBMEMCACHED_API
  memcached_server_list_st memcached_server_list_append_with_weight(memcached_server_list_st ptr,
                                                                    const char *hostname,
                                                                    in_port_t port,
                                                                    uint32_t weight,
                                                                    memcached_return_t *error);

LIBMEMCACHED_API
  uint32_t memcached_server_list_count(const memcached_server_list_st ptr);

LIBMEMCACHED_LOCAL
  uint32_t memcached_servers_set_count(memcached_server_list_st servers, uint32_t count);

LIBMEMCACHED_LOCAL
  memcached_server_st *memcached_server_list(const memcached_st *);

LIBMEMCACHED_LOCAL
  void memcached_server_list_set(memcached_st *self, memcached_server_list_st list);

#ifdef __cplusplus
} // extern "C"
#endif
