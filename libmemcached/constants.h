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


#ifndef __LIBMEMCACHED_CONSTANTS_H__
#define __LIBMEMCACHED_CONSTANTS_H__

#ifdef LIBMEMCACHED_WITH_ZK_INTEGRATION
#define ENABLE_REPLICATION 1
#endif

#define UPDATE_HASH_RING_OF_FETCHED_MC 1
#define IMMEDIATELY_RECONNECT 1
#define REFACTORING_ERROR_PRINT 1
#define BOP_ARITHMETIC_INITIAL 1
#define POOL_UPDATE_SERVERLIST 1
#define POOL_MORE_CONCURRENCY 1
#define KETAMA_HASH_COLLSION 1

/* Public defines */
#define MEMCACHED_DEFAULT_PORT 11211
#define MEMCACHED_MAX_KEY 4001 /* (4000 + 1) We add one to have it null terminated */
#define MEMCACHED_MAX_BUFFER 8196
#define MEMCACHED_MAX_HOST_SORT_LENGTH 86 /* Used for Ketama */
#define MEMCACHED_POINTS_PER_SERVER 160
#define MEMCACHED_CONTINUUM_SIZE MEMCACHED_POINTS_PER_SERVER*100 /* This would then set max hosts to 100 */
#define MEMCACHED_STRIDE 4
/* poll timeout (or operation timeout): 700ms
 * It avoids the occurence of operation timeout
 * even if two packet retransmissions exist in linux.
 */
#define MEMCACHED_DEFAULT_TIMEOUT 700
#define MEMCACHED_DEFAULT_CONNECT_TIMEOUT 1000
#define MEMCACHED_CONTINUUM_ADDITION 10 /* How many extra slots we should build for in the continuum */
#define MEMCACHED_PREFIX_KEY_MAX_SIZE 128
#define MEMCACHED_EXPIRATION_NOT_ADD 0xffffffffU
#define MEMCACHED_VERSION_STRING_LENGTH 24
#define MEMCACHED_MAXIMUM_INTEGER_DISPLAY_LENGTH 20
#define MEMCACHED_SERVER_FAILURE_LIMIT 5
#define MEMCACHED_SERVER_FAILURE_RETRY_TIMEOUT 1

#define MAX_SERVERS_FOR_MULTI_KEY_OPERATION 1000

enum memcached_server_distribution_t {
  MEMCACHED_DISTRIBUTION_MODULA,
  MEMCACHED_DISTRIBUTION_CONSISTENT,
  MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA,
  MEMCACHED_DISTRIBUTION_RANDOM,
  MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA_SPY,
  MEMCACHED_DISTRIBUTION_VIRTUAL_BUCKET,
  MEMCACHED_DISTRIBUTION_CONSISTENT_MAX
};

#ifndef __cplusplus
typedef enum memcached_server_distribution_t memcached_server_distribution_t;
#endif

enum memcached_behavior_t {
  MEMCACHED_BEHAVIOR_NO_BLOCK,
  MEMCACHED_BEHAVIOR_TCP_NODELAY,
  MEMCACHED_BEHAVIOR_HASH,
  MEMCACHED_BEHAVIOR_SOCKET_SEND_SIZE,
  MEMCACHED_BEHAVIOR_SOCKET_RECV_SIZE,
  MEMCACHED_BEHAVIOR_CACHE_LOOKUPS,
  MEMCACHED_BEHAVIOR_SUPPORT_CAS,
  MEMCACHED_BEHAVIOR_POLL_TIMEOUT,
  MEMCACHED_BEHAVIOR_DISTRIBUTION,
  MEMCACHED_BEHAVIOR_BUFFER_REQUESTS,
  MEMCACHED_BEHAVIOR_USER_DATA,
  MEMCACHED_BEHAVIOR_SORT_HOSTS,
  MEMCACHED_BEHAVIOR_VERIFY_KEY,
  MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT,
  MEMCACHED_BEHAVIOR_RETRY_TIMEOUT,
  MEMCACHED_BEHAVIOR_KETAMA_HASH,
  MEMCACHED_BEHAVIOR_BINARY_PROTOCOL,
  MEMCACHED_BEHAVIOR_SND_TIMEOUT,
  MEMCACHED_BEHAVIOR_RCV_TIMEOUT,
  MEMCACHED_BEHAVIOR_SERVER_FAILURE_LIMIT,
  MEMCACHED_BEHAVIOR_IO_MSG_WATERMARK,
  MEMCACHED_BEHAVIOR_IO_BYTES_WATERMARK,
  MEMCACHED_BEHAVIOR_IO_KEY_PREFETCH,
  MEMCACHED_BEHAVIOR_HASH_WITH_PREFIX_KEY,
  MEMCACHED_BEHAVIOR_NOREPLY,
  MEMCACHED_BEHAVIOR_USE_UDP,
  MEMCACHED_BEHAVIOR_AUTO_EJECT_HOSTS,
  MEMCACHED_BEHAVIOR_NUMBER_OF_REPLICAS,
  MEMCACHED_BEHAVIOR_RANDOMIZE_REPLICA_READ,
  MEMCACHED_BEHAVIOR_CORK,
  MEMCACHED_BEHAVIOR_TCP_KEEPALIVE,
  MEMCACHED_BEHAVIOR_TCP_KEEPIDLE,
  MEMCACHED_BEHAVIOR_LOAD_FROM_FILE,
  MEMCACHED_BEHAVIOR_REMOVE_FAILED_SERVERS,
  MEMCACHED_BEHAVIOR_MAX
};

#ifndef __cplusplus
typedef enum memcached_behavior_t memcached_behavior_t;
#endif

enum memcached_callback_t {
  MEMCACHED_CALLBACK_PREFIX_KEY = 0,
  MEMCACHED_CALLBACK_USER_DATA = 1,
  MEMCACHED_CALLBACK_CLEANUP_FUNCTION = 2,
  MEMCACHED_CALLBACK_CLONE_FUNCTION = 3,
#ifdef MEMCACHED_ENABLE_DEPRECATED
  MEMCACHED_CALLBACK_MALLOC_FUNCTION = 4,
  MEMCACHED_CALLBACK_REALLOC_FUNCTION = 5,
  MEMCACHED_CALLBACK_FREE_FUNCTION = 6,
#endif
  MEMCACHED_CALLBACK_GET_FAILURE = 7,
  MEMCACHED_CALLBACK_DELETE_TRIGGER = 8,
  MEMCACHED_CALLBACK_MAX,
  MEMCACHED_CALLBACK_NAMESPACE= MEMCACHED_CALLBACK_PREFIX_KEY
};

#ifndef __cplusplus
typedef enum memcached_callback_t memcached_callback_t;
#endif

enum memcached_hash_t {
  MEMCACHED_HASH_DEFAULT= 0,
  MEMCACHED_HASH_MD5,
  MEMCACHED_HASH_CRC,
  MEMCACHED_HASH_FNV1_64,
  MEMCACHED_HASH_FNV1A_64,
  MEMCACHED_HASH_FNV1_32,
  MEMCACHED_HASH_FNV1A_32,
  MEMCACHED_HASH_HSIEH,
  MEMCACHED_HASH_MURMUR,
  MEMCACHED_HASH_JENKINS,
  MEMCACHED_HASH_CUSTOM,
  MEMCACHED_HASH_MAX
};

#ifndef __cplusplus
typedef enum memcached_hash_t memcached_hash_t;
#endif

enum memcached_connection_t {
  MEMCACHED_CONNECTION_TCP,
  MEMCACHED_CONNECTION_UDP,
  MEMCACHED_CONNECTION_UNIX_SOCKET
};

enum {
  MEMCACHED_CONNECTION_UNKNOWN= 0,
  MEMCACHED_CONNECTION_MAX= 0
};

#ifndef __cplusplus
typedef enum memcached_connection_t memcached_connection_t;
#endif

#endif /* __LIBMEMCACHED_CONSTANTS_H__ */
