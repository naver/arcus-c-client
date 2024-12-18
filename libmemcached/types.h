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


#ifndef __LIBMEMCACHED_TYPES_H__
#define __LIBMEMCACHED_TYPES_H__

typedef struct memcached_st memcached_st;
typedef struct memcached_stat_st memcached_stat_st;
typedef struct memcached_analysis_st memcached_analysis_st;
typedef struct memcached_result_st memcached_result_st;
typedef struct memcached_array_st memcached_array_st;
typedef struct memcached_storage_request_st memcached_storage_request_st;
typedef struct memcached_error_t memcached_error_t;

// All of the flavors of memcache_server_st
typedef struct memcached_server_st memcached_server_st;
typedef const struct memcached_server_st *memcached_server_instance_st;
typedef struct memcached_server_st *memcached_server_list_st;
#ifdef LIBMEMCACHED_WITH_ZK_INTEGRATION
typedef struct memcached_server_info_st memcached_server_info_st;
#ifdef ENABLE_REPLICATION
typedef struct memcached_rgroup_st memcached_rgroup_st;
typedef struct memcached_rgroup_info_st memcached_rgroup_info_st;
#endif
#endif

typedef struct memcached_callback_st memcached_callback_st;

// The following two structures are internal, and never exposed to users.
typedef struct memcached_string_st memcached_string_st;
typedef struct memcached_string_t memcached_string_t;
typedef struct memcached_continuum_item_st memcached_continuum_item_st;
typedef struct memcached_ketama_info_st memcached_ketama_info_st;

typedef struct memcached_coll_create_attrs_st memcached_coll_create_attrs_st;
typedef struct memcached_coll_result_st memcached_coll_result_st;
typedef struct memcached_coll_attrs_st memcached_coll_attrs_st;
typedef union memcached_coll_sub_key_st memcached_coll_sub_key_st;
typedef struct memcached_coll_query_st memcached_coll_query_st;
typedef struct memcached_coll_query_st memcached_bop_query_st;
typedef struct memcached_hexadecimal_st memcached_hexadecimal_st;
typedef struct memcached_mkey_st memcached_mkey_st;
typedef struct memcached_coll_eflag_filter_st memcached_coll_eflag_filter_st;
typedef struct memcached_coll_eflag_update_st memcached_coll_eflag_update_st;
typedef struct memcached_coll_eflag_update_st memcached_coll_update_filter_st;
typedef struct memcached_coll_smget_result_st memcached_coll_smget_result_st;

#ifdef __cplusplus
extern "C" {
#endif

typedef memcached_return_t (*memcached_clone_fn)(memcached_st *destination, const memcached_st *source);
typedef memcached_return_t (*memcached_cleanup_fn)(const memcached_st *ptr);

/**
  Memory allocation functions.
*/
typedef void (*memcached_free_fn)(const memcached_st *ptr, void *mem, void *context);
typedef void *(*memcached_malloc_fn)(const memcached_st *ptr, const size_t size, void *context);
typedef void *(*memcached_realloc_fn)(const memcached_st *ptr, void *mem, const size_t size, void *context);
typedef void *(*memcached_calloc_fn)(const memcached_st *ptr, size_t nelem, const size_t elsize, void *context);


typedef memcached_return_t (*memcached_execute_fn)(const memcached_st *ptr, memcached_result_st *result, void *context);
typedef memcached_return_t (*memcached_server_fn)(const memcached_st *ptr, memcached_server_instance_st server, void *context);
typedef memcached_return_t (*memcached_stat_fn)(memcached_server_instance_st server,
                                                const char *key, size_t key_length,
                                                const char *value, size_t value_length,
                                                void *context);

/**
  Trigger functions.
*/
typedef memcached_return_t (*memcached_trigger_key_fn)(const memcached_st *ptr,
                                                       const char *key, size_t key_length,
                                                       memcached_result_st *result);
typedef memcached_return_t (*memcached_trigger_delete_key_fn)(const memcached_st *ptr,
                                                              const char *key, size_t key_length);

typedef memcached_return_t (*memcached_dump_fn)(const memcached_st *ptr,
                                                const char *key,
                                                size_t key_length,
                                                void *context);

#ifdef __cplusplus
}
#endif

/**
  @note The following definitions are just here for backwards compatibility.
*/
typedef memcached_return_t memcached_return;
typedef memcached_server_distribution_t memcached_server_distribution;
typedef memcached_behavior_t memcached_behavior;
typedef memcached_callback_t memcached_callback;
typedef memcached_hash_t memcached_hash;
typedef memcached_connection_t memcached_connection;
typedef memcached_clone_fn memcached_clone_func;
typedef memcached_cleanup_fn memcached_cleanup_func;
typedef memcached_execute_fn memcached_execute_function;
typedef memcached_server_fn memcached_server_function;
typedef memcached_trigger_key_fn memcached_trigger_key;
typedef memcached_trigger_delete_key_fn memcached_trigger_delete_key;
typedef memcached_dump_fn memcached_dump_func;

#endif /* __LIBMEMCACHED_TYPES_H__ */
