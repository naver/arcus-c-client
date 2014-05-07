#include <stdlib.h>
#include <stdio.h>

#include "libmemcached/memcached.h"

/* No error checking/handling to minimize clutter. */

int
main(int argc, char *argv[])
{
  memcached_st *mc;
  memcached_coll_create_attrs_st attr;
  const char *key = "this_is_key";
  size_t key_length = strlen(key);
  const uint64_t bkey = 123; /* b+tree element's key */
  const char *value = "helloworld";
  memcached_coll_result_st result;

  /* Create the memcached object */
  if (NULL == (mc = memcached_create(NULL)))
    return -1;

  /* Add the server's address */
  if (MEMCACHED_SUCCESS != memcached_server_add(mc, "127.0.0.1", 11211))
    return -1;

  /* Create a b+tree key and then insert an element in one call. */
  memcached_coll_create_attrs_init(&attr, 20 /* flags */, 100 /* exptime */,
    4000 /* maxcount */);
  if (MEMCACHED_SUCCESS != memcached_bop_insert(mc, key, key_length,
      bkey,
      NULL /* eflag */, 0 /* eflag length */,
      (const char*)value, (size_t)strlen(value)+1 /* include NULL */,
      &attr /* automatically create the b+tree key */))
    return -1;
  printf("Created a b+tree key and inserted an element.\n");

  /* Get the element */
  if (NULL == memcached_coll_result_create(mc, &result))
    return -1;
  if (MEMCACHED_SUCCESS != memcached_bop_get(mc, key, key_length, bkey,
      NULL /* no eflags filters */,
      false /* do not delete the element */,
      false /* do not delete the empty key */,
      &result))
    return -1;
  
  /* Print */
  printf("Retrieved the element. value=%s\n",
    memcached_coll_result_get_value(&result, 0));
  memcached_coll_result_free(&result);
  
  return 0;
}
