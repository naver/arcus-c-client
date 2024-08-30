#include <config.h>
#include <libtest/test.hpp>

using namespace libtest;

#include "tests/storage.h"

#define REQ_COUNT 100
#define SMALL_BUFFER_SIZE 64
#define LARGE_BUFFER_SIZE 1000 * 1000
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
  int value_buffer_size= value_pad ? LARGE_BUFFER_SIZE : SMALL_BUFFER_SIZE;
  char divider= ('z' - 'a' + 1);

  for (int i= 0; i < REQ_COUNT; i++)
  {
    char *key= (char *) malloc(SMALL_BUFFER_SIZE);
    if (key == NULL)
    {
      printf("key cannot be allocated...\n");
      return false;
    }

    char *value= (char *) malloc(value_buffer_size);
    if (value == NULL)
    {
      printf("value cannot be allocated...\n");

      safe_free(key);
      return false;
    }

    size_t key_length= snprintf(key, SMALL_BUFFER_SIZE, "MULTI:multi-store-key-%d", i);
    size_t value_length;

    if (value_pad)
    {
      value_length= snprintf(value, value_buffer_size, "multi-store-value-%d-%u-", i, (uint32_t) rand());
      char pad= ('a' + (i % divider));

      memset(value + value_length, pad, value_buffer_size - value_length - 1);
      value[value_length= value_buffer_size - 1]= 0;
    }
    else
    {
      value_length= snprintf(value, value_buffer_size, "multi-store-value-%d-%u", i, (uint32_t) rand());
    }

    req[i].key= key;
    req[i].key_length= key_length;

    req[i].value= value;
    req[i].value_length= value_length;

    req[i].expiration= EXPIRE_TIME;
    req[i].flags= (uint32_t) rand();
    req[i].cas= UINT64_MAX;
  }

  return true;
}

static bool check_results(memcached_st *mc, memcached_return_t *results, memcached_return_t expected, char *op_name)
{
  for (int i= 0; i < REQ_COUNT; i++)
  {
    printf("memcached_%s: rc[%d] is %d, %s\n", op_name, i, results[i], memcached_strerror(mc, results[i]));

    if (results[i] != expected)
    {
      printf("memcached_%s: expected %d, %s / got %d, %s\n",
             op_name, expected, memcached_strerror(mc, expected),
             results[i], memcached_strerror(mc, results[i]));

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
  memset(req, 0, sizeof(req));

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
  else if (check_results(mc, results, MEMCACHED_SUCCESS, (char *) "mset") == false)
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

static bool do_madd_and_get(memcached_st *mc, bool is_noreply)
{
  memcached_storage_request_st req[REQ_COUNT];
  memset(req, 0, sizeof(req));

  memcached_return_t results[REQ_COUNT];
  memcached_return_t rc;
  bool test_success;

  if (create_req(req, true) == false)
  {
    test_success= false;
    goto do_return;
  }

  rc= memcached_madd(mc, req, REQ_COUNT, results);
  printf("memcached_madd: rc is %d, %s\n", rc, memcached_strerror(mc, rc));

  if (memcached_failed(rc))
  {
    test_success= false;
    goto do_return;
  }
  else if (check_results(mc, results, MEMCACHED_SUCCESS, (char *) "madd") == false)
  {
    test_success= false;
    goto do_return;
  }
  else if (do_get_and_check(mc, req) == false)
  {
    test_success= false;
    goto do_return;
  }
  else if (is_noreply == true)
  {
    test_success= true;
    goto do_return;
  }

  safe_free_req(req);
  if (create_req(req, false) == false) // expects fail, so do not pad.
  {
    test_success= false;
    goto do_return;
  }

  rc= memcached_madd(mc, req, REQ_COUNT, results);
  printf("memcached_madd after memcached_madd: rc is %d, %s\n", rc, memcached_strerror(mc, rc));

  if (memcached_success(rc))
  {
    printf("weird. madd after madd is successful.\n");
    test_success= false;
  }
  else if (check_results(mc, results, MEMCACHED_NOTSTORED, (char *) "madd") == false)
  {
    test_success= false;
  }
  else
  {
    test_success= true;
  }

do_return:
  safe_free_req(req);
  return test_success;
}

static bool do_mreplace_and_get(memcached_st *mc, bool is_noreply)
{
  memcached_storage_request_st req[REQ_COUNT];
  memset(req, 0, sizeof(req));

  memcached_return_t results[REQ_COUNT];
  memcached_return_t rc;
  bool test_success;

  if (create_req(req, true) == false)
  {
    test_success= false;
    goto do_return;
  }

  if (is_noreply == false)
  {
    rc= memcached_mreplace(mc, req, REQ_COUNT, results);
    printf("memcached_mreplace before memcached_mset: rc is %d, %s\n", rc, memcached_strerror(mc, rc));

    if (memcached_success(rc))
    {
      printf("weird. mreplace before mset is successful.\n");

      test_success= false;
      goto do_return;
    }
    else if (check_results(mc, results, MEMCACHED_NOTSTORED, (char *) "mreplace") == false)
    {
      test_success= false;
      goto do_return;
    }
  }

  rc= memcached_mset(mc, req, REQ_COUNT, results);
  printf("memcached_mset: rc is %d, %s\n", rc, memcached_strerror(mc, rc));

  if (memcached_failed(rc))
  {
    test_success= false;
    goto do_return;
  }
  else if (check_results(mc, results, MEMCACHED_SUCCESS, (char *) "mset") == false)
  {
    test_success= false;
    goto do_return;
  }

  safe_free_req(req);
  if (create_req(req, false) == false) // expects different value, so do not pad.
  {
    test_success= false;
    goto do_return;
  }

  rc= memcached_mreplace(mc, req, REQ_COUNT, results);
  printf("memcached_mreplace: rc is %d, %s\n", rc, memcached_strerror(mc, rc));

  if (memcached_failed(rc))
  {
    test_success= false;
  }
  else if (check_results(mc, results, MEMCACHED_SUCCESS, (char *) "mreplace") == false)
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

static bool do_mprepend_and_get(memcached_st *mc, bool is_noreply)
{
  memcached_storage_request_st set_req[REQ_COUNT];
  memcached_storage_request_st prepend_req[REQ_COUNT];

  memset(set_req, 0, sizeof(set_req));
  memset(prepend_req, 0, sizeof(prepend_req));

  memcached_return_t results[REQ_COUNT];
  memcached_return_t rc;
  bool test_success;

  if (create_req(set_req, false) == false)
  {
    test_success= false;
    goto do_return;
  }

  if (is_noreply == false)
  {
    rc= memcached_mprepend(mc, set_req, REQ_COUNT, results);
    printf("memcached_mprepend before memcached_mset: rc is %d, %s\n", rc, memcached_strerror(mc, rc));

    if (memcached_success(rc))
    {
      printf("weird. mprepend before mset is successful.\n");

      test_success= false;
      goto do_return;
    }
    else if (check_results(mc, results, MEMCACHED_NOTSTORED, (char *) "mprepend") == false)
    {
      test_success= false;
      goto do_return;
    }
  }

  rc= memcached_mset(mc, set_req, REQ_COUNT, results);
  printf("memcached_mset: rc is %d, %s\n", rc, memcached_strerror(mc, rc));

  if (memcached_failed(rc))
  {
    test_success= false;
    goto do_return;
  }
  else if (check_results(mc, results, MEMCACHED_SUCCESS, (char *) "mset") == false)
  {
    test_success= false;
    goto do_return;
  }

  if (create_req(prepend_req, false) == false)
  {
    test_success= false;
    goto do_return;
  }

  rc= memcached_mprepend(mc, prepend_req, REQ_COUNT, results);
  printf("memcached_mprepend: rc is %d, %s\n", rc, memcached_strerror(mc, rc));

  if (memcached_failed(rc))
  {
    test_success= false;
    goto do_return;
  }
  else if (check_results(mc, results, MEMCACHED_SUCCESS, (char *) "mprepend") == false)
  {
    test_success= false;
    goto do_return;
  }

  for (int i= 0; i < REQ_COUNT; i++)
  {
    size_t new_value_length= prepend_req[i].value_length + set_req[i].value_length;
    void *temp= realloc(prepend_req[i].value, new_value_length + 1);

    if (temp == NULL)
    {
      test_success= false;
      goto do_return;
    }

    prepend_req[i].value= (char *) temp;
    prepend_req[i].value= strcat(prepend_req[i].value, set_req[i].value);
    prepend_req[i].value_length= new_value_length;
    prepend_req[i].flags= set_req[i].flags;
  }

  test_success= do_get_and_check(mc, prepend_req);

do_return:
  safe_free_req(set_req);
  safe_free_req(prepend_req);
  return test_success;
}

static bool do_mappend_and_get(memcached_st *mc, bool is_noreply)
{
  memcached_storage_request_st set_req[REQ_COUNT];
  memcached_storage_request_st append_req[REQ_COUNT];

  memset(set_req, 0, sizeof(set_req));
  memset(append_req, 0, sizeof(append_req));

  memcached_return_t results[REQ_COUNT];
  memcached_return_t rc;
  bool test_success;

  if (create_req(set_req, false) == false)
  {
    test_success= false;
    goto do_return;
  }

  if (is_noreply == false)
  {
    rc= memcached_mappend(mc, set_req, REQ_COUNT, results);
    printf("memcached_mappend before memcached_mset: rc is %d, %s\n", rc, memcached_strerror(mc, rc));

    if (memcached_success(rc))
    {
      printf("weird. mappend before mset is successful.\n");

      test_success= false;
      goto do_return;
    }
    else if (check_results(mc, results, MEMCACHED_NOTSTORED, (char *) "mappend") == false)
    {
      test_success= false;
      goto do_return;
    }
  }

  rc= memcached_mset(mc, set_req, REQ_COUNT, results);
  printf("memcached_mset: rc is %d, %s\n", rc, memcached_strerror(mc, rc));

  if (memcached_failed(rc))
  {
    test_success= false;
    goto do_return;
  }
  else if (check_results(mc, results, MEMCACHED_SUCCESS, (char *) "mset") == false)
  {
    test_success= false;
    goto do_return;
  }

  if (create_req(append_req, false) == false)
  {
    test_success= false;
    goto do_return;
  }

  rc= memcached_mappend(mc, append_req, REQ_COUNT, results);
  printf("memcached_mappend: rc is %d, %s\n", rc, memcached_strerror(mc, rc));

  if (memcached_failed(rc))
  {
    test_success= false;
    goto do_return;
  }
  else if (check_results(mc, results, MEMCACHED_SUCCESS, (char *) "mappend") == false)
  {
    test_success= false;
    goto do_return;
  }

  for (int i= 0; i < REQ_COUNT; i++)
  {
    size_t new_value_length= set_req[i].value_length + append_req[i].value_length;
    void *temp= realloc(set_req[i].value, new_value_length + 1);

    if (temp == NULL)
    {
      test_success= false;
      goto do_return;
    }

    set_req[i].value= (char *) temp;
    set_req[i].value= strcat(set_req[i].value, append_req[i].value);
    set_req[i].value_length= new_value_length;
  }

  test_success= do_get_and_check(mc, set_req);

do_return:
  safe_free_req(set_req);
  safe_free_req(append_req);
  return test_success;
}

static bool do_mcas_and_get(memcached_st *mc, bool is_noreply)
{
  memcached_storage_request_st set_req[REQ_COUNT];
  memcached_storage_request_st cas_req[REQ_COUNT];

  memset(set_req, 0, sizeof(set_req));
  memset(cas_req, 0, sizeof(cas_req));

  char *keys[REQ_COUNT]= { NULL };
  size_t key_length[REQ_COUNT]= { 0 };

  memcached_return_t results[REQ_COUNT];
  memcached_return_t rc;
  bool test_success;

  if (create_req(set_req, true) == false)
  {
    test_success= false;
    goto do_return;
  }

  if (is_noreply == false)
  {
    rc= memcached_mcas(mc, set_req, REQ_COUNT, results);
    printf("memcached_mcas before memcached_mset: rc is %d, %s\n", rc, memcached_strerror(mc, rc));

    if (memcached_success(rc))
    {
      printf("weird. mcas before mset is successful.\n");

      test_success= false;
      goto do_return;
    }
    else if (check_results(mc, results, MEMCACHED_NOTFOUND, (char *) "mcas") == false)
    {
      test_success= false;
      goto do_return;
    }
  }

  rc= memcached_mset(mc, set_req, REQ_COUNT, results);
  printf("memcached_mset: rc is %d, %s\n", rc, memcached_strerror(mc, rc));

  if (memcached_failed(rc))
  {
    test_success= false;
    goto do_return;
  }
  else if (check_results(mc, results, MEMCACHED_SUCCESS, (char *) "mset") == false)
  {
    test_success= false;
    goto do_return;
  }

  if (create_req(cas_req, false) == false) // expects different value, so do not pad.
  {
    test_success= false;
    goto do_return;
  }

  if (is_noreply == false)
  {
    rc= memcached_mcas(mc, set_req, REQ_COUNT, results);
    printf("memcached_mcas before memcached_mget: rc is %d, %s\n", rc, memcached_strerror(mc, rc));

    if (memcached_success(rc))
    {
      printf("weird. mcas before mget is successful.\n");

      test_success= false;
      goto do_return;
    }
    else if (check_results(mc, results, MEMCACHED_DATA_EXISTS, (char *) "mcas") == false)
    {
      test_success= false;
      goto do_return;
    }
  }

  for (int i= 0; i < REQ_COUNT; i++) {
    keys[i]= cas_req[i].key;
    key_length[i]= cas_req[i].key_length;
  }

  if (memcached_failed(rc= memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_SUPPORT_CAS, 1)))
  {
    printf("memcached_behavior_set: rc is %d, %s\n", rc, memcached_strerror(mc, rc));

    test_success= false;
    goto do_return;
  }
  else if (memcached_failed(rc= memcached_mget(mc, keys, key_length, REQ_COUNT)))
  {
    printf("memcached_mget: rc is %d, %s\n", rc, memcached_strerror(mc, rc));

    test_success= false;
    goto do_return;
  }
  else if (memcached_failed(rc= memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_SUPPORT_CAS, 0)))
  {
    printf("memcached_behavior_set: rc is %d, %s\n", rc, memcached_strerror(mc, rc));

    test_success= false;
    goto do_return;
  }

  for (int i= 0; i < REQ_COUNT; i++)
  {
    memcached_result_st result;
    memset(&result, 0, sizeof(result));
    memcached_fetch_result(mc, &result, &rc);

    if (memcached_failed(rc))
    {
      printf("memcached_fetch_result: rc[%d] is %d, %s\n", i, rc, memcached_strerror(mc, rc));

      test_success= false;
      goto do_return;
    }

    cas_req[i].cas= result.item_cas;
  }

  rc= memcached_mcas(mc, cas_req, REQ_COUNT, results);
  printf("memcached_mcas: rc is %d, %s\n", rc, memcached_strerror(mc, rc));

  if (memcached_failed(rc))
  {
    test_success= false;
    goto do_return;
  }
  else if (check_results(mc, results, MEMCACHED_SUCCESS, (char *) "mcas") == false)
  {
    test_success= false;
    goto do_return;
  }

  test_success= do_get_and_check(mc, cas_req);

do_return:
  safe_free_req(set_req);
  safe_free_req(cas_req);
  return test_success;
}

test_return_t mset_and_get_test(memcached_st *mc)
{
  memcached_return_t rc;
  srand(time(NULL));

  for (uint64_t i= 0; i < 2; i++)
  {
    if (memcached_failed(rc= memcached_flush(mc, 0)))
    {
      printf("memcached_flush: rc is %d, %s\n", rc, memcached_strerror(mc, rc));
      return TEST_FAILURE;
    }
    if (memcached_failed(rc= memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_NOREPLY, i % 2)))
    {
      printf("memcached_behavior_set: rc is %d, %s\n", rc, memcached_strerror(mc, rc));
      return TEST_FAILURE;
    }
    if (do_mset_and_get(mc) == false)
    {
      return TEST_FAILURE;
    }
  }

  return TEST_SUCCESS;
}

test_return_t madd_and_get_test(memcached_st *mc)
{
  memcached_return_t rc;
  srand(time(NULL));

  for (uint64_t i= 0; i < 2; i++)
  {
    bool is_noreply= i % 2;

    if (memcached_failed(rc= memcached_flush(mc, 0)))
    {
      printf("memcached_flush: rc is %d, %s\n", rc, memcached_strerror(mc, rc));
      return TEST_FAILURE;
    }
    if (memcached_failed(rc= memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_NOREPLY, is_noreply)))
    {
      printf("memcached_behavior_set: rc is %d, %s\n", rc, memcached_strerror(mc, rc));
      return TEST_FAILURE;
    }
    if (do_madd_and_get(mc, is_noreply) == false)
    {
      return TEST_FAILURE;
    }
  }

  return TEST_SUCCESS;
}

test_return_t mreplace_and_get_test(memcached_st *mc)
{
  memcached_return_t rc;
  srand(time(NULL));

  for (uint64_t i= 0; i < 2; i++)
  {
    bool is_noreply= i % 2;

    if (memcached_failed(rc= memcached_flush(mc, 0)))
    {
      printf("memcached_flush: rc is %d, %s\n", rc, memcached_strerror(mc, rc));
      return TEST_FAILURE;
    }
    if (memcached_failed(rc= memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_NOREPLY, is_noreply)))
    {
      printf("memcached_behavior_set: rc is %d, %s\n", rc, memcached_strerror(mc, rc));
      return TEST_FAILURE;
    }
    if (do_mreplace_and_get(mc, is_noreply) == false)
    {
      return TEST_FAILURE;
    }
  }

  return TEST_SUCCESS;
}

test_return_t mprepend_and_get_test(memcached_st *mc)
{
  memcached_return_t rc;
  srand(time(NULL));

  for (uint64_t i= 0; i < 2; i++)
  {
    bool is_noreply= i % 2;

    if (memcached_failed(rc= memcached_flush(mc, 0)))
    {
      printf("memcached_flush: rc is %d, %s\n", rc, memcached_strerror(mc, rc));
      return TEST_FAILURE;
    }
    if (memcached_failed(rc= memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_NOREPLY, is_noreply)))
    {
      printf("memcached_behavior_set: rc is %d, %s\n", rc, memcached_strerror(mc, rc));
      return TEST_FAILURE;
    }
    if (do_mprepend_and_get(mc, is_noreply) == false)
    {
      return TEST_FAILURE;
    }
  }

  return TEST_SUCCESS;
}

test_return_t mappend_and_get_test(memcached_st *mc)
{
  memcached_return_t rc;
  srand(time(NULL));

  for (uint64_t i= 0; i < 2; i++)
  {
    bool is_noreply= i % 2;

    if (memcached_failed(rc= memcached_flush(mc, 0)))
    {
      printf("memcached_flush: rc is %d, %s\n", rc, memcached_strerror(mc, rc));
      return TEST_FAILURE;
    }
    if (memcached_failed(rc= memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_NOREPLY, is_noreply)))
    {
      printf("memcached_behavior_set: rc is %d, %s\n", rc, memcached_strerror(mc, rc));
      return TEST_FAILURE;
    }
    if (do_mappend_and_get(mc, is_noreply) == false)
    {
      return TEST_FAILURE;
    }
  }

  return TEST_SUCCESS;
}

test_return_t mcas_and_get_test(memcached_st *mc)
{
  memcached_return_t rc;
  srand(time(NULL));

  for (uint64_t i= 0; i < 2; i++)
  {
    bool is_noreply= i % 2;

    if (memcached_failed(rc= memcached_flush(mc, 0)))
    {
      printf("memcached_flush: rc is %d, %s\n", rc, memcached_strerror(mc, rc));
      return TEST_FAILURE;
    }
    if (memcached_failed(rc= memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_NOREPLY, is_noreply)))
    {
      printf("memcached_behavior_set: rc is %d, %s\n", rc, memcached_strerror(mc, rc));
      return TEST_FAILURE;
    }
    if (do_mcas_and_get(mc, is_noreply) == false)
    {
      return TEST_FAILURE;
    }
  }

  return TEST_SUCCESS;
}

/*
  Test cases
*/
test_st multi_storage_tests[] ={
  {"mset_and_get_test", true, (test_callback_fn*)mset_and_get_test },
  {"madd_and_get_test", true, (test_callback_fn*)madd_and_get_test },
  {"mreplace_and_get_test", true, (test_callback_fn*)mreplace_and_get_test },
  {"mprepend_and_get_test", true, (test_callback_fn*)mprepend_and_get_test },
  {"mappend_and_get_test", true, (test_callback_fn*)mappend_and_get_test },
  {"mcas_and_get_test", true, (test_callback_fn*)mcas_and_get_test },
  {0, 0, 0}
};

collection_st collection[] ={
  {"multi_storage_tests", 0, 0, multi_storage_tests},
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
