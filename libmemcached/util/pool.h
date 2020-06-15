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


#include <libmemcached/memcached.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

struct memcached_pool_st;
typedef struct memcached_pool_st memcached_pool_st;

LIBMEMCACHED_API
memcached_pool_st *memcached_pool_create(memcached_st* mmc, uint32_t initial, uint32_t max);

LIBMEMCACHED_API
memcached_pool_st *memcached_pool(const char *option_string, size_t option_string_length);

LIBMEMCACHED_API
memcached_st* memcached_pool_destroy(memcached_pool_st* pool);

LIBMEMCACHED_API
memcached_st* memcached_pool_pop(memcached_pool_st* pool,
                                 bool block,
                                 memcached_return_t* rc);
LIBMEMCACHED_API
memcached_return_t memcached_pool_push(memcached_pool_st* pool,
                                       memcached_st* mmc);
LIBMEMCACHED_API
memcached_return_t memcached_pool_release(memcached_pool_st* pool, memcached_st* mmc);

LIBMEMCACHED_API
memcached_st* memcached_pool_fetch(memcached_pool_st*, struct timespec* relative_time, memcached_return_t* rc);

LIBMEMCACHED_API
memcached_return_t memcached_pool_behavior_set(memcached_pool_st *ptr,
                                               memcached_behavior_t flag,
                                               uint64_t data);
LIBMEMCACHED_API
memcached_return_t memcached_pool_behavior_get(memcached_pool_st *ptr,
                                               memcached_behavior_t flag,
                                               uint64_t *value);

LIBMEMCACHED_API
void memcached_pool_lock(memcached_pool_st *ptr);

LIBMEMCACHED_API
void memcached_pool_unlock(memcached_pool_st *ptr);

#ifdef LIBMEMCACHED_WITH_ZK_INTEGRATION
/**
 * Utility functions for the Arcus ZooKeeper client to manipulate the pool.
 */

/**
 * Get the master memcached client of the pool.
 * @param pool  memcached pool handle.
 * @return handle of the master memcached client.
 */
LIBMEMCACHED_API
memcached_st *memcached_pool_get_master(memcached_pool_st* pool);

/**
 * Re-build the pool with new memcached clients.
 * @param pool  memcached pool handle.
 */
LIBMEMCACHED_API
memcached_return_t memcached_pool_repopulate(memcached_pool_st* pool);

#ifdef ENABLE_REPLICATION
memcached_return_t memcached_pool_use_single_server(memcached_pool_st *pool,
                                                    const char *host,
                                                    int port);
#endif

/**
 * Get the size (number of clients) of the pool.
 * @param pool  memcached pool handle.
 * @return the number of clients in the pool (pool size).
 */
uint16_t get_memcached_pool_size(memcached_pool_st* pool);
#endif

#ifdef __cplusplus
} // extern "C"
#endif
