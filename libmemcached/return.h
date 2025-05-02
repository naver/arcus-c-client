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

#ifndef __LIBMEMCACHED_RETURN_H__
#define __LIBMEMCACHED_RETURN_H__

enum memcached_return_t {
  MEMCACHED_SUCCESS,
  MEMCACHED_FAILURE,
  MEMCACHED_HOST_LOOKUP_FAILURE, // getaddrinfo() and getnameinfo() only
  MEMCACHED_CONNECTION_FAILURE,
  MEMCACHED_CONNECTION_BIND_FAILURE,  // DEPRECATED, see MEMCACHED_HOST_LOOKUP_FAILURE
  MEMCACHED_WRITE_FAILURE,
  MEMCACHED_READ_FAILURE,
  MEMCACHED_UNKNOWN_READ_FAILURE,
  MEMCACHED_PROTOCOL_ERROR,
  MEMCACHED_CLIENT_ERROR,
  MEMCACHED_SERVER_ERROR,
  MEMCACHED_CONNECTION_SOCKET_CREATE_FAILURE, // DEPRECATED
  MEMCACHED_DATA_EXISTS,
  MEMCACHED_DATA_DOES_NOT_EXIST,
  MEMCACHED_NOTSTORED,
  MEMCACHED_STORED,
  MEMCACHED_NOTFOUND,
  MEMCACHED_MEMORY_ALLOCATION_FAILURE,
  MEMCACHED_PARTIAL_READ,
  MEMCACHED_SOME_ERRORS,
  MEMCACHED_NO_SERVERS,
  MEMCACHED_END,
  MEMCACHED_DELETED,
  MEMCACHED_VALUE,
  MEMCACHED_STAT,
  MEMCACHED_ITEM,
  MEMCACHED_ERRNO,
  MEMCACHED_FAIL_UNIX_SOCKET, // DEPRECATED
  MEMCACHED_NOT_SUPPORTED,
  MEMCACHED_NO_KEY_PROVIDED, /* Deprecated. Use MEMCACHED_BAD_KEY_PROVIDED! */
  MEMCACHED_FETCH_NOTFINISHED,
  MEMCACHED_TIMEOUT,
  MEMCACHED_BUFFERED,
  MEMCACHED_BAD_KEY_PROVIDED,
  MEMCACHED_INVALID_HOST_PROTOCOL,
  MEMCACHED_SERVER_MARKED_DEAD,
  MEMCACHED_UNKNOWN_STAT_KEY,
  MEMCACHED_E2BIG,
  MEMCACHED_INVALID_ARGUMENTS,
  MEMCACHED_KEY_TOO_BIG,
  MEMCACHED_AUTH_PROBLEM,
  MEMCACHED_AUTH_FAILURE,
  MEMCACHED_AUTH_CONTINUE,
  MEMCACHED_PARSE_ERROR,
  MEMCACHED_PARSE_USER_ERROR,
  MEMCACHED_DEPRECATED,
  MEMCACHED_IN_PROGRESS,
  MEMCACHED_SERVER_TEMPORARILY_DISABLED,
  MEMCACHED_ATTR,
  MEMCACHED_ATTR_ERROR_NOT_FOUND,
  MEMCACHED_ATTR_ERROR_BAD_VALUE,
  MEMCACHED_COUNT,
  MEMCACHED_ATTR_MISMATCH,
  MEMCACHED_BKEY_MISMATCH,
  MEMCACHED_EFLAG_MISMATCH,
  MEMCACHED_UNREADABLE,
  MEMCACHED_TRIMMED,
  MEMCACHED_DUPLICATED,
  MEMCACHED_DUPLICATED_TRIMMED,
  MEMCACHED_RESPONSE,
  MEMCACHED_UPDATED,
  MEMCACHED_CREATED,
  MEMCACHED_NOTHING_TO_UPDATE,
  MEMCACHED_EXISTS,
  MEMCACHED_ALL_EXIST,
  MEMCACHED_SOME_EXIST,
  MEMCACHED_ALL_NOT_EXIST,
  MEMCACHED_ALL_SUCCESS,
  MEMCACHED_SOME_SUCCESS,
  MEMCACHED_ALL_FAILURE,
  MEMCACHED_CREATED_STORED,
  MEMCACHED_ELEMENT_EXISTS,
  MEMCACHED_NOTFOUND_ELEMENT,
  MEMCACHED_OUT_OF_RANGE,
  MEMCACHED_OVERFLOWED,
  MEMCACHED_TYPE_MISMATCH,
  MEMCACHED_EXIST,
  MEMCACHED_NOT_EXIST,
  MEMCACHED_DELETED_DROPPED,
  MEMCACHED_PIPE_ERROR_COMMAND_OVERFLOW,
  MEMCACHED_PIPE_ERROR_MEMORY_OVERFLOW,
  MEMCACHED_PIPE_ERROR_BAD_ERROR,
  MEMCACHED_REPLACED,
  MEMCACHED_POSITION,
  MEMCACHED_INVALID_COMMAND,
  MEMCACHED_UNDEFINED_RESPONSE,
#ifdef ENABLE_REPLICATION
  MEMCACHED_SWITCHOVER,
  MEMCACHED_REPL_SLAVE,
#endif
  MEMCACHED_MAXIMUM_RETURN /* Always add new error code before */
};

#ifndef __cplusplus
typedef enum memcached_return_t memcached_return_t;
#endif

static inline bool memcached_success(memcached_return_t rc)
{
  return (rc == MEMCACHED_BUFFERED ||
          rc == MEMCACHED_DELETED ||
          rc == MEMCACHED_END ||
          rc == MEMCACHED_ITEM ||
          rc == MEMCACHED_STAT ||
          rc == MEMCACHED_STORED ||
          rc == MEMCACHED_SUCCESS ||
          rc == MEMCACHED_VALUE);
}

static inline bool memcached_failed(memcached_return_t rc)
{
  return (rc != MEMCACHED_SUCCESS &&
          rc != MEMCACHED_END &&
          rc != MEMCACHED_STORED &&
          rc != MEMCACHED_STAT &&
          rc != MEMCACHED_DELETED &&
          rc != MEMCACHED_BUFFERED &&
          rc != MEMCACHED_VALUE);
}

static inline bool memcached_fatal(memcached_return_t rc)
{
  return (rc != MEMCACHED_SUCCESS &&
          rc != MEMCACHED_END &&
          rc != MEMCACHED_STORED &&
          rc != MEMCACHED_STAT &&
          rc != MEMCACHED_DELETED &&
          rc != MEMCACHED_BUFFERED &&
          rc != MEMCACHED_VALUE);
}

#define memcached_continue(__memcached_return_t) ((__memcached_return_t) == MEMCACHED_IN_PROGRESS)

#endif /* __LIBMEMCACHED_RETURN_H__ */
