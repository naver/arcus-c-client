/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  LibMemcached
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  All rights reserved.
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

#ifndef __LIBMEMCACHED_ERROR_H__
#define __LIBMEMCACHED_ERROR_H__

#ifdef __cplusplus
extern "C" {
#endif

LIBMEMCACHED_API
  const char *memcached_last_error_message(memcached_st *);

#ifdef REFACTORING_ERROR_PRINT
LIBMEMCACHED_API
  const char *memcached_detail_error_message(memcached_st *, memcached_return_t rc);
#endif

LIBMEMCACHED_API
  void memcached_error_print(const memcached_st *);

LIBMEMCACHED_API
  memcached_return_t memcached_last_error(memcached_st *);

LIBMEMCACHED_API
  int memcached_last_error_errno(memcached_st *);

LIBMEMCACHED_API
  const char *memcached_server_error(memcached_server_instance_st ptr);

LIBMEMCACHED_API
  memcached_return_t memcached_server_error_return(memcached_server_instance_st ptr);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __LIBMEMCACHED_ERROR_H__ */
