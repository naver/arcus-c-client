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

#ifndef __LIBMEMCACHED_ALLOCATORS_H__
#define __LIBMEMCACHED_ALLOCATORS_H__

struct memcached_allocator_t {
  memcached_calloc_fn calloc;
  memcached_free_fn free;
  memcached_malloc_fn malloc;
  memcached_realloc_fn realloc;
  void *context;
};

#ifdef __cplusplus
extern "C" {
#endif

LIBMEMCACHED_API
memcached_return_t memcached_set_memory_allocators(memcached_st *ptr,
                                                   memcached_malloc_fn mem_malloc,
                                                   memcached_free_fn mem_free,
                                                   memcached_realloc_fn mem_realloc,
                                                   memcached_calloc_fn mem_calloc,
                                                   void *context);

LIBMEMCACHED_API
void memcached_get_memory_allocators(const memcached_st *ptr,
                                     memcached_malloc_fn *mem_malloc,
                                     memcached_free_fn *mem_free,
                                     memcached_realloc_fn *mem_realloc,
                                     memcached_calloc_fn *mem_calloc);

LIBMEMCACHED_API
void *memcached_get_memory_allocators_context(const memcached_st *ptr);

LIBMEMCACHED_LOCAL
void _libmemcached_free(const memcached_st *ptr, void *mem, void *context);

LIBMEMCACHED_LOCAL
void *_libmemcached_malloc(const memcached_st *ptr, const size_t size, void *context);

LIBMEMCACHED_LOCAL
void *_libmemcached_realloc(const memcached_st *ptr, void *mem, const size_t size, void *context);

LIBMEMCACHED_LOCAL
void *_libmemcached_calloc(const memcached_st *ptr, size_t nelem, size_t size, void *context);

LIBMEMCACHED_LOCAL
struct memcached_allocator_t memcached_allocators_return_default(void);

#ifdef __cplusplus
}
#endif

#endif /* __LIBMEMCACHED_ALLOCATORS_H__ */
