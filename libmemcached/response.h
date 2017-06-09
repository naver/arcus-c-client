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

#pragma once

#define KEY_MAX_LENGTH                  MEMCACHED_MAX_KEY + 1
#define FLAGS_MAX_LENGTH                10
#define BYTE_ARRAY_BKEY_MAX_LENGTH      MEMCACHED_COLL_MAX_BKEY_EXT_LENGTH*2+3
#define BKEY_MAX_LENGTH                 10
#define EFLAG_MAX_LENGTH                MEMCACHED_COLL_MAX_BKEY_EXT_LENGTH*2+3
#define BYTES_MAX_LENGTH                10

#ifdef __cplusplus
extern "C" {
#endif

/* Read a single response from the server */
LIBMEMCACHED_LOCAL
memcached_return_t memcached_read_one_response(memcached_server_write_instance_st ptr,
                                               char *buffer, size_t buffer_length,
                                               memcached_result_st *result);

LIBMEMCACHED_LOCAL
memcached_return_t memcached_response(memcached_server_write_instance_st ptr,
                                      char *buffer, size_t buffer_length,
                                      memcached_result_st *result);

LIBMEMCACHED_LOCAL
memcached_return_t memcached_read_one_coll_response(memcached_server_write_instance_st ptr,
                                                    char *buffer, size_t buffer_length,
                                                    memcached_coll_result_st *result);

LIBMEMCACHED_LOCAL
memcached_return_t memcached_coll_response(memcached_server_write_instance_st ptr,
                                           char *buffer, size_t buffer_length,
                                           memcached_coll_result_st *result);

LIBMEMCACHED_LOCAL
 void              memcached_add_coll_pipe_return_code(memcached_server_write_instance_st ptr,
                                                       memcached_return_t return_code);

LIBMEMCACHED_LOCAL
memcached_return_t memcached_read_one_coll_smget_response(memcached_server_write_instance_st ptr,
                                                          char *buffer, size_t buffer_length,
                                                          memcached_coll_smget_result_st *result);

LIBMEMCACHED_LOCAL
memcached_return_t memcached_coll_smget_response(memcached_server_write_instance_st ptr,
                                                 char *buffer, size_t buffer_length,
                                                 memcached_coll_smget_result_st *result);

#ifdef __cplusplus
}
#endif
