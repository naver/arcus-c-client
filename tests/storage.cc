#include <config.h>
#include <libtest/test.hpp>

using namespace libtest;

#include "tests/storage.h"

#define MSET_COUNT 5
#define BUFFER_SIZE 64
#define EXPIRE_TIME 60

static void safe_free(char *c)
{
  if (c != NULL)
  {
    free(c);
  }
}

static void safe_free_req(memcached_storage_request_st req[MSET_COUNT])
{
  for (int i= 0; i < MSET_COUNT; i++)
  {
    safe_free(req[i].key);
    safe_free(req[i].value);
  }
}

test_return_t mset_and_get_test(memcached_st *mc)
{
  memcached_storage_request_st req[MSET_COUNT];
  srand(time(NULL));

  for (int i= 0; i < MSET_COUNT; i++)
  {
    char *key= (char *)malloc(BUFFER_SIZE);
    if (not key)
    {
      printf("key cannot be allocated...\n");
      return TEST_FAILURE;
    }

    char *value= (char *)malloc(BUFFER_SIZE);
    if (not value)
    {
      printf("value cannot be allocated...\n");
      safe_free(key);
      return TEST_FAILURE;
    }

    memset(key, 0, BUFFER_SIZE);
    snprintf(key, BUFFER_SIZE, "MSET:mset-key-%d", i);

    memset(value, 0, BUFFER_SIZE);
    snprintf(value, BUFFER_SIZE, "mset-value-%d", i);

    req[i].key= key;
    req[i].key_length= strlen(key);

    req[i].value= value;
    req[i].value_length= strlen(value);

    req[i].expiration= EXPIRE_TIME;
    req[i].flags= (uint32_t) rand();
  }

  memcached_return_t results[MSET_COUNT];
  memcached_return_t rc= memcached_mset(mc, req, MSET_COUNT, results);
  printf("memcached_mset: rc is %d, %s\n", rc, memcached_strerror(mc, rc));

  if (memcached_failed(rc))
  {
    safe_free_req(req);
    return TEST_FAILURE;
  }

  for (int i= 0; i < MSET_COUNT; i++)
  {
    printf("memcached_mset: rc[%d] is %d, %s\n", i, rc, memcached_strerror(mc, results[i]));

    if (memcached_failed(results[i]))
    {
      safe_free_req(req);
      return TEST_FAILURE;
    }
  }

  for (int i= 0; i < MSET_COUNT; i++)
  {
    size_t value_length= -1;
    uint32_t flags= -1;

    char *value= memcached_get(mc, req[i].key, req[i].key_length, &value_length, &flags, &rc);
    printf("memcached_get: rc[%d] is %d, %s\n", i, rc, memcached_strerror(mc, rc));

    if (value == NULL || memcached_failed(rc))
    {
      safe_free(value);
      safe_free_req(req);
      return TEST_FAILURE;
    }

    printf("memcached_get: flags[%d] is %u\n", i, flags);
    printf("memcached_get: value[%d] is %s\n", i, value);

    if (req[i].value_length != value_length)
    {
      printf("memcached_get: value_length[%d] is not equal... stored %ld but got %ld\n", i, req[i].value_length, value_length);

      safe_free(value);
      safe_free_req(req);
      return TEST_FAILURE;
    }
    if (strcmp(req[i].value, value))
    {
      printf("memcached_get: value[%d] is not equal... stored %s but got %s\n", i, req[i].value, value);

      safe_free(value);
      safe_free_req(req);
      return TEST_FAILURE;
    }
    if (req[i].flags != flags)
    {
      printf("memcached_get: flags[%d] is not equal... stored %u but got %u\n", i, req[i].flags, flags);

      safe_free(value);
      safe_free_req(req);
      return TEST_FAILURE;
    }

    safe_free(value);
  }

  safe_free_req(req);

  return TEST_SUCCESS;
}

/*
  Test cases
*/
test_st mset_tests[] ={
  {"mset_and_get_test", true, (test_callback_fn*)mset_and_get_test },
  {0, 0, 0}
};

collection_st collection[] ={
  {"mset_tests", 0, 0, mset_tests},
  {0, 0, 0, 0}
};

#define TEST_PORT_BASE MEMCACHED_DEFAULT_PORT +10

#include "tests/libmemcached_world.h"

void get_world(Framework *world)
{
  world->collections= collection;

  world->_create= (test_callback_create_fn*)world_create;
  world->_destroy= (test_callback_destroy_fn*)world_destroy;

  world->item._startup= (test_callback_fn*)world_test_startup;
  world->item.set_pre((test_callback_fn*)world_pre_run);
  world->item.set_flush((test_callback_fn*)world_flush);
  world->item.set_post((test_callback_fn*)world_post_run);
  world->_on_error= (test_callback_error_fn*)world_on_error;

  world->collection_startup= (test_callback_fn*)world_container_startup;
  world->collection_shutdown= (test_callback_fn*)world_container_shutdown;

  world->set_runner(&default_libmemcached_runner);

  world->set_socket();
}