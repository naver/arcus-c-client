#include <config.h>
#include <libtest/test.hpp>

using namespace libtest;

#include "tests/storage.h"

#define REQ_COUNT MAX_KEYS_FOR_MULTI_STORE_OPERATION
#define KEY_BUFFER_SIZE 64
#define VALUE_BUFFER_SIZE 1000 * 1000
#define EXPIRE_TIME 60

static inline void safe_free(void *ptr)
{
  if (ptr != NULL)
  {
    free(ptr);
  }
}

static inline void safe_free_req(memcached_storage_request_st req[REQ_COUNT])
{
  if (req == NULL)
  {
    return;
  }

  for (int i= 0; i < REQ_COUNT; i++)
  {
    safe_free(req[i].key);
    safe_free(req[i].value);
  }
}

static bool create_req(memcached_storage_request_st req[REQ_COUNT], bool value_pad)
{
  char divider= ('z' - 'a' + 1);

  for (int i= 0; i < REQ_COUNT; i++)
  {
    char *key= (char *) malloc(KEY_BUFFER_SIZE);
    if (key == NULL)
    {
      printf("key cannot be allocated...\n");
      return false;
    }

    char *value= (char *) malloc(VALUE_BUFFER_SIZE);
    if (value == NULL)
    {
      printf("value cannot be allocated...\n");

      safe_free(key);
      return false;
    }

    size_t key_length= snprintf(key, KEY_BUFFER_SIZE, "MULTI:multi-store-key-%d", i);
    size_t value_length;

    if (value_pad)
    {
      value_length= snprintf(value, VALUE_BUFFER_SIZE, "multi-store-value-%d-%u-", i, (uint32_t) rand());
      char pad= ('a' + (i % divider));

      memset(value + value_length, pad, VALUE_BUFFER_SIZE - value_length - 1);
      value[value_length= VALUE_BUFFER_SIZE - 1]= 0;
    }
    else
    {
      value_length= snprintf(value, VALUE_BUFFER_SIZE, "multi-store-value-%d-%u", i, (uint32_t) rand());
    }

    req[i].key= key;
    req[i].key_length= key_length;

    req[i].value= value;
    req[i].value_length= value_length;

    req[i].expiration= EXPIRE_TIME;
    req[i].flags= (uint32_t) rand();
  }

  return true;
}

static bool check_results(memcached_st *mc, memcached_return_t *results, char *op_name)
{
  for (int i= 0; i < REQ_COUNT; i++)
  {
    printf("memcached_%s: rc[%d] is %d, %s\n", op_name, i, results[i], memcached_strerror(mc, results[i]));

    if (memcached_failed(results[i]))
    {
      return false;
    }
  }

  return true;
}

static bool do_get_and_check(memcached_st *mc, memcached_storage_request_st req[REQ_COUNT])
{
  memcached_return_t rc= memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_NOREPLY, 0);
  if (memcached_failed(rc))
  {
    printf("memcached_behavior_set: rc is %d, %s\n", rc, memcached_strerror(mc, rc));
    return false;
  }

  for (int i= 0; i < REQ_COUNT; i++)
  {
    size_t value_length= -1;
    uint32_t flags= -1;
    bool success;

    char *value= memcached_get(mc, req[i].key, req[i].key_length, &value_length, &flags, &rc);
    printf("memcached_get: rc[%d] is %d, %s / ", i, rc, memcached_strerror(mc, rc));

    if (value == NULL || memcached_failed(rc))
    {
      printf("failed to get key %s\n", req[i].key);
      success= false;
    }
    else if (req[i].value_length != value_length)
    {
      printf("value_length[%d] is not equal... stored %ld but got %ld\n", i, req[i].value_length, value_length);
      success= false;
    }
    else if (strcmp(req[i].value, value))
    {
      printf("value[%d] is not equal... stored %s but got %s\n", i, req[i].value, value);
      success= false;
    }
    else if (req[i].flags != flags)
    {
      printf("flags[%d] is not equal... stored %u but got %u\n", i, req[i].flags, flags);
      success= false;
    }
    else
    {
      printf("value and flags check for key %s is successful\n", req[i].key);
      success= true;
    }

    safe_free(value);
    if (success == false)
    {
      return false;
    }
  }

  return true;
}

static bool do_mset_and_get(memcached_st *mc)
{
  memcached_storage_request_st req[REQ_COUNT];
  memcached_return_t results[REQ_COUNT];
  memcached_return_t rc;
  bool test_success;

  if (create_req(req, true) == false)
  {
    test_success= false;
    goto do_return;
  }

  rc= memcached_mset(mc, req, REQ_COUNT, results);
  printf("memcached_mset: rc is %d, %s\n", rc, memcached_strerror(mc, rc));

  if (memcached_failed(rc))
  {
    test_success= false;
  }
  else if (check_results(mc, results, (char *) "mset") == false)
  {
    test_success= false;
  }
  else
  {
    test_success= do_get_and_check(mc, req);
  }

do_return:
  safe_free_req(req);
  return test_success;
}

test_return_t mset_and_get_test(memcached_st *mc)
{
  memcached_return_t rc;
  srand(time(NULL));

  for (uint64_t i= 0; i < 2; i++)
  {
    if (memcached_failed(rc= memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_NOREPLY, i % 2)))
    {
      printf("memcached_behavior_set: rc is %d, %s\n", rc, memcached_strerror(mc, rc));
      return TEST_FAILURE;
    }
    else if (do_mset_and_get(mc) == false)
    {
      return TEST_FAILURE;
    }
  }

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

static server_startup_st *single_server= NULL;

static void *single_server_world_create(server_startup_st& servers, test_return_t& error)
{
  single_server= new server_startup_st(servers);
  if (single_server == NULL)
  {
    printf("failed to allocate new server_startup_st object.\n");

    error= TEST_MEMORY_ALLOCATION_FAILURE;
    return NULL;
  }

  single_server->set_count(1);
  return world_create(*single_server, error);
}

static bool single_server_world_destroy(void *object)
{
  if (single_server != NULL)
  {
    delete single_server;
  }
  return world_destroy(object);
}

void get_world(Framework *world)
{
  world->collections= collection;

  world->_create= (test_callback_create_fn*)single_server_world_create;
  world->_destroy= (test_callback_destroy_fn*)single_server_world_destroy;

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
