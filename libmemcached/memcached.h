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

#ifndef __LIBMEMCACHED_MEMCACHED_H__
#define __LIBMEMCACHED_MEMCACHED_H__

#include <inttypes.h>
#include <stdlib.h>
#include <sys/types.h>


#if !defined(__cplusplus)
# include <stdbool.h>
#endif

#include <libmemcached/visibility.h>
#include <libmemcached/configure.h>
#include <libmemcached/platform.h>
#include <libmemcached/constants.h>
#include <libmemcached/return.h>
#include <libmemcached/types.h>
#include <libmemcached/string.h>
#include <libmemcached/array.h>
#include <libmemcached/error.h>
#include <libmemcached/stats.h>
#include <libhashkit/hashkit.h>

// Everything above this line must be in the order specified.
#include <libmemcached/allocators.h>
#include <libmemcached/analyze.h>
#include <libmemcached/auto.h>
#include <libmemcached/behavior.h>
#include <libmemcached/callback.h>
#include <libmemcached/delete.h>
#include <libmemcached/dump.h>
#include <libmemcached/exist.h>
#include <libmemcached/fetch.h>
#include <libmemcached/flush.h>
#include <libmemcached/flush_buffers.h>
#include <libmemcached/get.h>
#include <libmemcached/hash.h>
#include <libmemcached/namespace.h>
#include <libmemcached/options.h>
#include <libmemcached/parse.h>
#include <libmemcached/quit.h>
#include <libmemcached/result.h>
#include <libmemcached/server.h>
#include <libmemcached/server_list.h>
#ifdef ENABLE_REPLICATION
#include <libmemcached/rgroup.h>
#endif
#include <libmemcached/storage.h>
#include <libmemcached/strerror.h>
#include <libmemcached/verbosity.h>
#include <libmemcached/version.h>
#include <libmemcached/sasl.h>
#ifdef LIBMEMCACHED_WITH_ZK_INTEGRATION
#ifdef __linux__
#include <linux/limits.h>
#endif
#ifndef PATH_MAX
#define PATH_MAX        4096  /* # chars in a path name including nul */
#endif
#endif
#include <libmemcached/arcus.h>
#include <libmemcached/collection.h>
#include <libmemcached/collection_result.h>

struct memcached_st {
  /**
    @note these are static and should not change without a call to behavior.
  */
  struct {
    bool is_purging:1;
    bool is_pool_master:1;
    bool is_processing_input:1;
    bool is_time_for_rebuild:1;
  } state;

  struct {
    // Everything below here is pretty static.
    bool auto_eject_hosts:1;
    bool binary_protocol:1;
    bool buffer_requests:1;
    bool hash_with_namespace:1;
    bool no_block:1; // Don't block
    bool no_reply:1;
    bool piped:1;
    bool multi_store:1;
    bool randomize_replica_read:1;
    bool support_cas:1;
    bool tcp_nodelay:1;
    bool use_sort_hosts:1;
    bool use_udp:1;
    bool verify_key:1;
    bool tcp_keepalive:1;
#ifdef ENABLE_REPLICATION
    bool repl_enabled:1; /* internally set and used */
#endif
  } flags;

  memcached_server_distribution_t distribution;
  hashkit_st hashkit;
  struct {
    unsigned int version;
  } server_info;
  uint32_t number_of_hosts;
#ifdef ENABLE_REPLICATION
  uint32_t rgroup_ntotal;
  uint32_t server_ntotal;
  uint32_t server_navail;
  memcached_rgroup_st *rgroups;
#endif
  memcached_server_st *servers;
  memcached_server_st *last_disconnected_server;
  int32_t snd_timeout;
  int32_t rcv_timeout;
  uint32_t server_failure_limit;
  uint32_t io_msg_watermark;
  uint32_t io_bytes_watermark;
  uint32_t io_key_prefetch;
  uint32_t tcp_keepidle;
  int32_t poll_timeout;
  int32_t connect_timeout; // How long we will wait on connect() before we will timeout
  int32_t retry_timeout;
  int send_size;
  int recv_size;
  void *user_data;
#ifdef REFACTORING_ERROR_PRINT
  uint64_t mc_id;
#endif
  uint64_t query_id;
  uint32_t number_of_replicas;
  memcached_result_st result;
  memcached_coll_result_st collection_result;
  memcached_coll_smget_result_st smget_result;

  char pipe_buffer[MEMCACHED_COLL_MAX_PIPED_BUFFER_SIZE];
  size_t pipe_buffer_pos;
  memcached_return_t *pipe_responses;
  size_t pipe_responses_length;
  memcached_return_t pipe_return_code;

  struct {
    bool weighted;
    time_t next_distribution_rebuild;
    memcached_ketama_info_st *info;
  } ketama;

  struct memcached_virtual_bucket_t *virtual_bucket;

  struct memcached_allocator_t allocators;

  memcached_clone_fn on_clone;
  memcached_cleanup_fn on_cleanup;
  memcached_trigger_key_fn get_key_failure;
  memcached_trigger_delete_key_fn delete_trigger;
  memcached_callback_st *callbacks;
  struct memcached_sasl_st sasl;
  struct memcached_error_t *error_messages;
#ifdef REFACTORING_ERROR_PRINT
  char  *error_msg_buffer;
  size_t error_msg_buffer_size;
#endif
  struct memcached_array_st *_namespace;
  struct {
    uint32_t initial_pool_size;
    uint32_t max_pool_size;
#ifdef UPDATE_HASH_RING_OF_FETCHED_MC
    int32_t ketama_version;
#endif
    int32_t version; // This is used by pool and others to determine if the memcached_st is out of date.
    struct memcached_array_st *filename;
  } configure;
  struct {
    bool is_allocated:1;
  } options;
  const char *last_op_code;
  memcached_return_t last_response_code;
  struct memcached_st *mc_next; /* used in mc pool */
#ifdef LIBMEMCACHED_WITH_ZK_INTEGRATION
  void *server_manager;
  FILE *logfile;
#endif
};

#ifdef __cplusplus
extern "C" {
#endif

LIBMEMCACHED_API
void memcached_servers_reset(memcached_st *ptr);

LIBMEMCACHED_API
memcached_st *memcached_create(memcached_st *ptr);

LIBMEMCACHED_API
memcached_st *memcached(const char *string, size_t string_length);

LIBMEMCACHED_API
void memcached_free(memcached_st *ptr);

LIBMEMCACHED_API
memcached_return_t memcached_reset(memcached_st *ptr);

LIBMEMCACHED_API
void memcached_reset_last_disconnected_server(memcached_st *ptr);

LIBMEMCACHED_API
memcached_st *memcached_clone(memcached_st *clone, const memcached_st *ptr);

LIBMEMCACHED_API
void *memcached_get_user_data(const memcached_st *ptr);

LIBMEMCACHED_API
void *memcached_set_user_data(memcached_st *ptr, void *data);

LIBMEMCACHED_API
memcached_return_t memcached_push(memcached_st *destination, const memcached_st *source);

LIBMEMCACHED_API
memcached_server_instance_st memcached_server_instance_by_position(const memcached_st *ptr, uint32_t server_key);

LIBMEMCACHED_API
uint32_t memcached_server_count(const memcached_st *);

LIBMEMCACHED_API
uint64_t memcached_query_id(const memcached_st *);

LIBMEMCACHED_API
memcached_return_t memcached_get_last_response_code(memcached_st *ptr);

LIBMEMCACHED_API
void memcached_set_last_response_code(memcached_st *ptr, memcached_return_t rc);

#ifdef LIBMEMCACHED_WITH_ZK_INTEGRATION
LIBMEMCACHED_API
void *memcached_get_server_manager(memcached_st *ptr);

LIBMEMCACHED_API
void memcached_set_server_manager(memcached_st *ptr, void *server_manager);

LIBMEMCACHED_API
memcached_return_t memcached_update_cachelist(memcached_st *ptr,
                                              memcached_server_info_st *serverinfo,
                                              uint32_t servercount, bool *serverlist_changed);

LIBMEMCACHED_API
memcached_return_t memcached_update_cachelist_with_master(memcached_st *ptr, memcached_st *master);
#endif

LIBMEMCACHED_API
void memcached_ketama_set(memcached_st *ptr, memcached_ketama_info_st *info);

LIBMEMCACHED_API
void memcached_ketama_reference(memcached_st *ptr, memcached_st *master);

LIBMEMCACHED_API
void memcached_ketama_release(memcached_st *ptr);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __LIBMEMCACHED_MEMCACHED_H__ */
