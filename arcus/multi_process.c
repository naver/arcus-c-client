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
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/wait.h>

#include "libmemcached/memcached.h"

static const char *zkadmin_addr = "127.0.0.1:2181";
static const char *service_code = "test";

#define NUM_OF_CHILDREN 10
#define NUM_OF_PIPED_ITEMS 100
#define NUM_OF_USERS 10

#define SAMPLE_PIPE  1
#define SAMPLE_MGET  0
#define SAMPLE_MGET2 0

#define USE_MC_POOL 1

static inline void process_child(memcached_st *proxy_mc)
{
  fprintf(stderr, "[pid:%d] begin : child_process\n", getpid());

#ifdef USE_MC_POOL
  memcached_st *mc;
  memcached_st *master_mc = memcached_create(NULL);
  memcached_pool_st *pool = memcached_pool_create(master_mc, NUM_OF_CHILDREN/2, NUM_OF_CHILDREN);
  arcus_proxy_connect(master_mc, pool, proxy_mc);
#else
  memcached_st *mc = memcached_create(NULL);
  arcus_proxy_connect(mc, NULL, proxy_mc);
#endif

  memcached_return_t rc;
  int userid, i;
  int pid = getpid();
  sleep(1);

  for (userid=0; userid<NUM_OF_USERS; userid++) {

#ifdef USE_MC_POOL
      mc = memcached_pool_fetch(pool, NULL, &rc);
#endif

    if (SAMPLE_PIPE) {
      fprintf(stderr, "test piped_insert()\n");

      // key
      char key[256];
      snprintf(key, sizeof(key), "ntalk_userid:btree:id%d_%d", pid, userid);

      // bkeys
      uint64_t bkey = 0;
      uint64_t bkeys[NUM_OF_PIPED_ITEMS];

      // eflags
      uint32_t eflag = 1001;
      const unsigned char **eflags = malloc(sizeof(unsigned char *) * NUM_OF_PIPED_ITEMS);
      size_t eflags_length[NUM_OF_PIPED_ITEMS];

      // values
      uint32_t value = 5000;
      const char **values = malloc(sizeof(char *) * NUM_OF_PIPED_ITEMS);
      size_t values_length[NUM_OF_PIPED_ITEMS];

      for (i=0; i<NUM_OF_PIPED_ITEMS; i++)
      {
        //queries[i] = memcached_bop_query_create(mc, NULL, bkey);
        bkeys[i] = bkey;
        eflags[i] = (unsigned char *)&eflag;
        eflags_length[i] = sizeof(eflags[i]);
        values[i] = (char *)&value;
        values_length[i] = sizeof(values[i]);

        bkey++;
        value++;
      }

      // create options
      memcached_coll_create_attrs_st attributes;
      memcached_coll_create_attrs_init(&attributes, 0, 600, 1000);

      // results
      memcached_return_t errors[NUM_OF_PIPED_ITEMS];
      memset(errors, 0, NUM_OF_PIPED_ITEMS);

      // do piped insert
      memcached_return_t piped_rc;
      memcached_bop_piped_insert(mc, key, strlen(key),
        NUM_OF_PIPED_ITEMS, bkeys, eflags, eflags_length, values, values_length,
        &attributes, errors, &piped_rc);

      uint32_t maxbkeyrange = 100000;

      memcached_coll_attrs_st attrs;
      memcached_coll_attrs_init(&attrs);
      memcached_coll_attrs_set_maxbkeyrange(&attrs, maxbkeyrange);

      rc = memcached_set_attrs(mc, key, strlen(key), &attrs);
    
      rc = memcached_get_attrs(mc, key, strlen(key), &attrs);
      maxbkeyrange = memcached_coll_attrs_get_maxbkeyrange(&attrs);

      free(eflags);
      free(values);

      for (i=0; i<NUM_OF_PIPED_ITEMS; i++)
      {
        fprintf(stderr, "[%d] insert result : %s\n", i, memcached_strerror(mc, errors[i]));
      }

      /* BOP UPDATE */
      uint32_t bkey_to_update = 0;
      uint32_t value_to_update = 9999;

      // update filter
      uint32_t fvalue = 2001;
      memcached_coll_update_filter_st update_filter;
      memcached_coll_update_filter_init(&update_filter, (unsigned char *)&fvalue, sizeof(fvalue));
      //memcached_bop_update_filter_set_bitwise(&update_filter, fwhere, MEMCACHED_COLL_BITWISE_AND);

      rc= memcached_bop_update(mc, key, strlen(key),
        bkey_to_update, &update_filter,
        (const char *)&value_to_update, sizeof(value_to_update));

      if (rc != MEMCACHED_SUCCESS)
      {
        fprintf(stderr, "memcached_bop_update failed : %s\n", memcached_strerror(mc, rc));
      }
    }

    if (SAMPLE_MGET) {
      fprintf(stderr, "test mget()\n");
      const char *keys[] = {
        "sinf:deletetest0",
        "sinf:deletetest1",
        "sinf:deletetest2",
        "sinf:deletetest3",
        "sinf:deletetest4",
        "sinf:deletetest5",
        "sinf:deletetest6",
        "sinf:deletetest7",
        "sinf:deletetest8",
        "sinf:deletetest9",
        "sinf:deletetest10",
        "sinf:deletetest11",
        "sinf:deletetest12",
        "sinf:deletetest13",
        "sinf:deletetest14",
        "sinf:deletetest15",
        "sinf:deletetest16",
        "sinf:deletetest17",
        "sinf:deletetest18",
        "sinf:deletetest19"
      };
      size_t lengths[] = {
        20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
        21, 21, 21, 21, 21, 21, 21, 21, 21, 21};

      char *string;
      size_t string_length;
      uint32_t flags;

      rc = memcached_mget(mc, keys, lengths, 20);

      char key[MEMCACHED_MAX_KEY];
      size_t key_length;

      for (i=0; i<20; i++)
      {
        string = memcached_fetch(mc, key, &key_length, &string_length, &flags, &rc);
        key[key_length] = '\0';
        fprintf(stderr, "fetch[%d] key=%s, value=%s, rc = %s\n",
                i, key, string, memcached_strerror(mc, rc));
        free(string);
      }
      fprintf(stderr, "\n");
    }

    if (SAMPLE_MGET2) {
      fprintf(stderr, "test mget()\n");
      const char *keys[] = {"foo1", "foo2", "foo3", "foo4", "foo5"};
      size_t lengths[] = { 4, 4, 4, 4, 4 };

      for (i=0; i<5; i++)
      {
        rc = memcached_set(mc, keys[i], lengths[i], keys[i], lengths[i], 600, 0);
        if (i == 1 || i == 2 || i == 4)
          rc = memcached_delete(mc, keys[i], lengths[i], 0);
      }

      char *string;
      size_t string_length;
      uint32_t flags;

      rc = memcached_mget(mc, keys, lengths, 5);

      char key[MEMCACHED_MAX_KEY];
      size_t key_length;

      for (i=0; i<5; i++)
      {
        string = memcached_fetch(mc, key, &key_length, &string_length, &flags, &rc);
        fprintf(stderr, "fetch[%d] key=%s, value=%s, rc = %s\n",
                i, key, string, memcached_strerror(mc, rc));
        free(string);
      }
      fprintf(stderr, "\n");
    }

#ifdef USE_MC_POOL
    rc = memcached_pool_release(pool, mc);
#endif
    sleep(1);
  }

  fprintf(stderr, "[pid:%d] end : child_process\n", getpid());
#ifdef USE_MC_POOL
  arcus_proxy_close(master_mc);
  memcached_pool_destroy(pool);
  memcached_free(master_mc);
#else
  arcus_proxy_close(mc);
#endif
}

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
  memcached_st *proxy_mc;
  arcus_return_t rc; 
  int i;

  proxy_mc = memcached_create(NULL);
  rc = arcus_proxy_create(proxy_mc, zkadmin_addr, service_code);

  if (rc != ARCUS_SUCCESS) {
    goto RELEASE;
  }

  pid_t pid;

  for (i=0; i<NUM_OF_CHILDREN; i++) {
    pid = fork();

    switch (pid) {
      case 0:
        process_child(proxy_mc);
        exit(EXIT_SUCCESS);
      case -1: 
        perror("fork error");
        exit(EXIT_FAILURE);
      default:
        break;
    }   
  }

  //siginfo_t info;
  //waitid(P_ALL, 0, &info, WEXITED | WSTOPPED | WCONTINUED);
  sleep(20);

RELEASE:
  arcus_proxy_close(proxy_mc);
  memcached_free(proxy_mc);

  return EXIT_SUCCESS;
}

