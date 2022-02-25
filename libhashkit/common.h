/* HashKit
 * Copyright (C) 2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 */

#ifndef __LIBHASHKIT_COMMON_H__
#define __LIBHASHKIT_COMMON_H__

#include <config.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <libhashkit/hashkit.h>

#ifdef __cplusplus
extern "C" {
#endif

HASHKIT_LOCAL
void md5_signature(const unsigned char *key, unsigned int length, unsigned char *result);

HASHKIT_LOCAL
int update_continuum(hashkit_st *hashkit);

#ifdef __cplusplus
}
#endif

#endif /* __LIBHASHKIT_COMMON_H__ */
