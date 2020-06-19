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
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#include "libmemcached/memcached.h"

static const char *zkadmin_addr = "127.0.0.1:2181";
static const char *service_code = "test";

#define NUMBER_OF_THREADS       10
#define STAT_INTERVAL_IN_SEC    2

static volatile bool run = true;
int number_of_threads = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* PERFORMANCE STATISTICS */
uint64_t global_stat_total_success[NUMBER_OF_THREADS];
uint64_t global_stat_total_failure[NUMBER_OF_THREADS];
uint64_t global_stat_success[NUMBER_OF_THREADS];
uint64_t global_stat_failure[NUMBER_OF_THREADS];

memcached_st *global_mc = NULL;

static void *
my_statistics_thread(void *ctx __attribute__((unused)))
{
  struct timeval timeval;
  struct tm *current_time;
  char time_string[256];

  int i;
  int cnt = 0;

  while (run) {
    uint64_t stat_total = 0;
    uint64_t stat_subsum = 0;
    uint64_t stat_success = 0;
    uint64_t stat_failure = 0;

    // TIME
    gettimeofday(&timeval, 0);
    current_time = localtime(&timeval.tv_sec);

    snprintf(time_string, sizeof(time_string), "%4d-%02d-%02d %02d:%02d:%02d",
      current_time->tm_year + 1900, current_time->tm_mon + 1, current_time->tm_mday,
      current_time->tm_hour, current_time->tm_min, current_time->tm_sec);

    for (i=0; i<NUMBER_OF_THREADS; i++) {
      stat_subsum += (global_stat_success[i] + global_stat_failure[i]);
      stat_success += global_stat_success[i];
      stat_failure += global_stat_failure[i];

      global_stat_total_success[i] += global_stat_success[i];
      global_stat_total_failure[i] += global_stat_failure[i];
      global_stat_success[i] = 0;
      global_stat_failure[i] = 0;

      stat_total += (global_stat_total_success[i] + global_stat_failure[i]);
    }

    if (cnt++ % 20 == 0) {
      fprintf(stdout, "TIME\t\t\tTOTAL\t\tSUCCESS\t\tFAILURE\t\tTPS\n");
      fprintf(stdout, "------------------------------------------------------------------------------------\n");
    }

    fprintf(stdout, "%s\t%llu\t\t%llu\t\t%llu\t\t%lf\n",
      time_string, (unsigned long long)stat_total, (unsigned long long)stat_success,
      (unsigned long long)stat_failure, (stat_subsum/(double)STAT_INTERVAL_IN_SEC));

    sleep(STAT_INTERVAL_IN_SEC);
  }

  return NULL;
}

static void
sig_handler(int sig __attribute__((unused)))
{
  run = false;
}

static void
release_resources(memcached_st *mc, memcached_pool_st *pool)
{
  arcus_pool_close(pool);

  if (pool) {
    memcached_pool_destroy(pool);
  }

  if (mc) {
    memcached_free(mc);
  }
}

static void
sample_sop(memcached_st *mc)
{
  uint32_t flags= 10;
  uint32_t exptime= 600;
  uint32_t maxcount= MEMCACHED_COLL_MAX_PIPED_CMD_SIZE - 1;
  uint32_t i;

  memcached_return_t rc;

  memcached_coll_create_attrs_st attributes;
  memcached_coll_create_attrs_init(&attributes, flags, exptime, maxcount);

  char **values = (char **)malloc(sizeof(char *) * MEMCACHED_COLL_MAX_PIPED_CMD_SIZE);
  size_t values_length[MEMCACHED_COLL_MAX_PIPED_CMD_SIZE];

  memcached_sop_create(mc, "set:a_set", 9, &attributes);

  for (i=0; i<maxcount; i++)
  {
    values[i]= (char *)malloc(sizeof(char) * 15);
    values_length[i]= snprintf(values[i], 15, "value%d", i);

    //if ((i % 10) == 0)
    //  continue;

    //rc= memcached_sop_insert(mc, "set:b_set", 9, values[i], values_length[i], &create_op);
    //test_true_got(rc == MEMCACHED_SUCCESS || rc == MEMCACHED_BUFFERED, memcached_strerror(NULL, rc));
  }
  // 1. NOT_FOUND
  rc= memcached_sop_exist(mc, "set:b_set", 9, "value", 5);
  //test_true_got(rc == MEMCACHED_NOTFOUND, memcached_strerror(NULL, rc));

  // 3. PIPED EXIST
  memcached_return_t responses[MEMCACHED_COLL_MAX_PIPED_CMD_SIZE];
  memcached_return_t piped_rc;
  rc= memcached_sop_piped_exist(mc, "set:b_set", 9,
                                maxcount, (const char * const*)values, values_length,
                                responses, &piped_rc);
  fprintf(stderr, "DEBUG : %s, %s\n", memcached_strerror(NULL, rc), memcached_strerror(NULL, piped_rc));
  //test_true_got(rc == MEMCACHED_SUCCESS, memcached_strerror(NULL, rc));
  //test_true_got(piped_rc == MEMCACHED_SOME_EXIST, memcached_strerror(NULL, piped_rc));

  for (i=0; i<maxcount; i++)
  {
    free(values[i]);
  }
  free(values);
}

static void
sample_sop_piped_exist(memcached_st *mc, int id, uint32_t userid)
{
#define MANY_PIPED_COUNT MEMCACHED_COLL_MAX_PIPED_CMD_SIZE * 2
  char key[256];
  snprintf(key, 100, "test:set:id%d_%d", id, userid);

  uint32_t flags= 10;
  uint32_t exptime= 600;
  uint32_t maxcount= MANY_PIPED_COUNT;
  uint32_t i;

  memcached_coll_create_attrs_st attributes;
  memcached_coll_create_attrs_init(&attributes, flags, exptime, 10000);
  
  memcached_return_t rc;
  memcached_return_t piped_rc;
  memcached_return_t results[MANY_PIPED_COUNT];
  
  char **values = (char **)malloc(sizeof(char *) * MANY_PIPED_COUNT);
  size_t valuelengths[MANY_PIPED_COUNT];
  
  for (i=0; i<maxcount; i++)
  { 
    values[i]= (char *)malloc(sizeof(char) * 15);
    valuelengths[i]= snprintf(values[i], 15, "%u", i);
    //valuelengths[i]= snprintf(values[i], 15, "value%llu", (unsigned long long)i);
  }
  
  memcached_sop_create(mc, key, strlen(key), &attributes);
  memcached_sop_insert(mc, key, strlen(key), "0", 1, NULL);
  memcached_sop_insert(mc, key, strlen(key), "499", 3, NULL);
/*
  rc= memcached_sop_piped_insert(mc, key, strlen(key),
                                 MANY_PIPED_COUNT,
                                 values, valuelengths,
                                 &attributes, results, &piped_rc);

  if (rc != MEMCACHED_SUCCESS && piped_rc != MEMCACHED_ALL_SUCCESS)
    fprintf(stderr, "sop_piped_insert failed, reason=%s\n", memcached_strerror(mc, rc));
*/

  struct timeval s_time, e_time;
  gettimeofday(&s_time, NULL);

  rc= memcached_sop_piped_exist(mc, key, strlen(key),
                                maxcount, (const char * const*)values, valuelengths,
                                results, &piped_rc);

  gettimeofday(&e_time, NULL);
  double elapsed_ms = ((e_time.tv_sec - s_time.tv_sec) * 1000.0)
                    + ((e_time.tv_usec - s_time.tv_usec) / 1000.0);
  
  if (rc != MEMCACHED_SUCCESS && piped_rc != MEMCACHED_ALL_EXIST)
    fprintf(stderr, "sop_piped_exist failed, reason=%s\n", memcached_strerror(NULL, rc));
  else
    fprintf(stderr, "elapsed time : %lf ms\n", elapsed_ms);
  
  for (i=0; i<maxcount; i++)
  { 
    free((void*)values[i]);
  }

  free((void*)values);
}

#define SAMPLE_SOP        0
#define SAMPLE_SOP_PIPED_EXIST 1

static void *
my_application_thread(void *ctx_pool)
{
  memcached_pool_st *pool = (memcached_pool_st *)ctx_pool;
  int id;
  uint32_t userid = 0;
  memcached_st *mc;
  memcached_return rc;

  pthread_mutex_lock(&mutex);
  id = number_of_threads++;
  pthread_mutex_unlock(&mutex);

  while (userid++ < 10) {
    mc = memcached_pool_pop(pool, true, &rc);

    if (mc != NULL) {
      if (SAMPLE_SOP)
        sample_sop(mc);
      if (SAMPLE_SOP_PIPED_EXIST)
        sample_sop_piped_exist(mc, id, userid);
    }
    else {
      fprintf(stderr, "Failed to get the memcached instance from pool!\n");
      ++global_stat_failure[id];
    }

    if (memcached_pool_push(pool, mc) != MEMCACHED_SUCCESS) {
      fprintf(stderr, "Failed to release the memcached instance!\n");
    }

    sleep(1);
  }

  return NULL;
}

int
main(int argc __attribute__((unused)), char** argv __attribute__((unused)))
{
  int x;
  int min_thread = 5;
  int max_thread = NUMBER_OF_THREADS;
  pthread_t tid[NUMBER_OF_THREADS];
  pthread_t tstat;

  memcached_pool_st *pool = NULL;

  global_mc = memcached_create(NULL);
  pool = memcached_pool_create(global_mc, min_thread, max_thread);

  if (!pool) {
    fprintf(stderr, "memcached_pool_create failed\n");
    return 1;
  }

  arcus_return_t arc = arcus_pool_connect(pool, zkadmin_addr, service_code);

  if (arc != ARCUS_SUCCESS) {
    fprintf(stderr, "arcus_connect() failed, reason=%s\n", arcus_strerror(arc));
    memcached_pool_push(pool, global_mc);
    return 1;
  }

  signal(SIGINT, sig_handler);

  pthread_create(&tstat, NULL, my_statistics_thread, NULL);

  for (x=0; x<NUMBER_OF_THREADS; x++) {
    pthread_create(&tid[x], NULL, my_application_thread, pool);
  }

  for (x=0; x<NUMBER_OF_THREADS; x++) {
    pthread_join(tid[x], NULL);
  }

  release_resources(global_mc, pool);

  return 1;
}
