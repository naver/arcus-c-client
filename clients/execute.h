/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary:
 *
 */

#ifndef __CLIENTS_EXECUTE_H__
#define __CLIENTS_EXECUTE_H__

#include <stdio.h>

#include "libmemcached/memcached.h"
#include "clients/generator.h"

#ifdef __cplusplus
extern "C" {
#endif

unsigned int execute_set(memcached_st *memc, pairs_st *pairs, unsigned int number_of);
unsigned int execute_get(memcached_st *memc, pairs_st *pairs, unsigned int number_of);
unsigned int execute_mget(memcached_st *memc, const char * const *keys, size_t *key_length,
                          unsigned int number_of);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __CLIENTS_EXECUTE_H__ */
