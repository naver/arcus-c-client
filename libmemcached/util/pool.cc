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


#include <libmemcached/common.h>
#include <libmemcached/memcached_util.h>
#ifdef LIBMEMCACHED_WITH_ZK_INTEGRATION
#include <libmemcached/arcus_priv.h>
#endif
#include <libmemcached/error.hpp>

#include <cassert>
#include <cerrno>
#include <pthread.h>
#include <memory>

#ifdef LIBMEMCACHED_WITH_ZK_INTEGRATION
static memcached_return_t member_update_cachelist(memcached_st *memc,
                                                  memcached_pool_st* pool);
#endif

struct memcached_pool_st
{
#ifdef POOL_MORE_CONCURRENCY
  pthread_mutex_t master_lock;
#endif
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  memcached_st *master;
#ifdef USED_MC_LIST_IN_POOL
  memcached_st *used_mc_head;
  memcached_st *used_mc_tail;
#ifdef POOL_MORE_CONCURRENCY
  memcached_st *used_bk_head;
  memcached_st *used_bk_tail;
#endif
#endif
  memcached_st **mc_pool;
#ifdef POOL_MORE_CONCURRENCY
  memcached_st **bk_pool;
#endif
  int top;
#ifdef POOL_MORE_CONCURRENCY
  int bk_top;
#endif
  uint32_t wait_count;
  const uint32_t max_size;
  uint32_t cur_size;
  bool _owns_master;
  struct timespec _timeout;

  memcached_pool_st(memcached_st *master_arg, size_t max_arg) :
    master(master_arg),
#ifdef USED_MC_LIST_IN_POOL
    used_mc_head(NULL),
    used_mc_tail(NULL),
#ifdef POOL_MORE_CONCURRENCY
    used_bk_head(NULL),
    used_bk_tail(NULL),
#endif
#endif
    mc_pool(NULL),
#ifdef POOL_MORE_CONCURRENCY
    bk_pool(NULL),
#endif
    top(-1),
#ifdef POOL_MORE_CONCURRENCY
    bk_top(-1),
#endif
    wait_count(0),
#ifdef LIBMEMCACHED_WITH_ZK_INTEGRATION
    max_size(max_arg+1), /* +1 for internal fetch */
#else
    max_size(max_arg),
#endif
    cur_size(0),
    _owns_master(false)
  {
#ifdef POOL_MORE_CONCURRENCY
    pthread_mutex_init(&master_lock, NULL);
#endif
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    _timeout.tv_sec= 1;
    _timeout.tv_nsec= 0;
  }

  const struct timespec& timeout() const
  {
    return _timeout;
  }

  bool release(memcached_st*, memcached_return_t& rc);

  memcached_st *fetch(memcached_return_t& rc);
  memcached_st *fetch(const struct timespec&, memcached_return_t& rc);

  bool init(uint32_t initial);

  ~memcached_pool_st()
  {
#ifdef USED_MC_LIST_IN_POOL
#ifdef POOL_MORE_CONCURRENCY
    while (used_bk_head)
    {
      memcached_st *mc= used_bk_head;
      used_bk_head= mc->mc_next;
      memcached_free(mc);
    }
#endif
    while (used_mc_head)
    {
      memcached_st *mc= used_mc_head;
      used_mc_head= mc->mc_next;
      memcached_free(mc);
    }
#endif
#ifdef POOL_MORE_CONCURRENCY
    for (int x= 0; x <= bk_top; ++x)
    {
      memcached_free(bk_pool[x]);
      bk_pool[x] = NULL;
    }
#endif
    for (int x= 0; x <= top; ++x)
    {
      memcached_free(mc_pool[x]);
      mc_pool[x] = NULL;
    }
#ifdef POOL_MORE_CONCURRENCY
    pthread_mutex_destroy(&master_lock);
#endif
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    delete [] mc_pool;
#ifdef POOL_MORE_CONCURRENCY
    delete [] bk_pool;
#endif
    if (_owns_master)
    {
      memcached_free(master);
    }
  }

#ifdef UPDATE_HASH_RING_OF_FETCHED_MC
  void increment_ketama_version()
  {
    ++master->configure.ketama_version;
  }

  bool compare_ketama_version(const memcached_st *arg) const
  {
    return (arg->configure.ketama_version == ketama_version());
  }

  int32_t ketama_version() const
  {
    return master->configure.ketama_version;
  }
#endif

  void increment_version()
  {
    ++master->configure.version;
  }

  bool compare_version(const memcached_st *arg) const
  {
    return (arg->configure.version == version());
  }

  int32_t version() const
  {
    return master->configure.version;
  }
};

#ifdef USED_MC_LIST_IN_POOL
/*
 * used mc list functions
 */
static memcached_st *mc_list_get(memcached_pool_st* pool)
{
  memcached_st *mc;

#ifdef POOL_MORE_CONCURRENCY
#ifdef LIBMEMCACHED_WITH_ZK_INTEGRATION
  if ((mc= pool->used_bk_head) != NULL)
  {
    pool->used_bk_head= mc->mc_next;
    if (pool->used_bk_head == NULL) {
      pool->used_bk_tail= NULL;
    }
    (void)member_update_cachelist(mc, pool);
    return mc;
  }
#endif
#endif
  if ((mc= pool->used_mc_head) != NULL)
  {
    pool->used_mc_head= mc->mc_next;
    if (pool->used_mc_head == NULL) {
      pool->used_mc_tail= NULL;
    }
  }
  return mc;
}

static void mc_list_add(memcached_pool_st* pool, memcached_st *mc)
{
  mc->mc_next= pool->used_mc_head;
  pool->used_mc_head= mc;
  if (pool->used_mc_tail == NULL) {
    pool->used_mc_tail= mc;
  }
}

static int mc_list_behavior_set(memcached_pool_st* pool,
                                memcached_behavior_t flag,
                                uint64_t data)
{
  memcached_st *prev;
  memcached_st *curr;
  int removed_count= 0;

  prev= NULL;
  curr= pool->used_mc_head;
  while (curr)
  {
    if (memcached_success(memcached_behavior_set(curr, flag, data)))
    {
      curr->configure.version= pool->version();
      prev= curr;
      curr= curr->mc_next;
    }
    else /* failed to set behavior */
    {
      /* remove the curr mc */
      if (prev) {
        prev->mc_next= curr->mc_next;
      } else {
        pool->used_mc_head= curr->mc_next;
        if (pool->used_mc_head == NULL) {
          pool->used_mc_tail= NULL;
        }
      }
      memcached_free(curr);
      removed_count++;
      pool->cur_size--;
      /* get the curr mc again */
      if (prev) {
        curr= curr->mc_next;
      } else {
        curr= pool->used_mc_head;
      }
    }
  }
  return removed_count;
}

#endif

static memcached_st *mc_pool_get(memcached_pool_st* pool)
{
#ifdef POOL_MORE_CONCURRENCY
#ifdef LIBMEMCACHED_WITH_ZK_INTEGRATION
  if (pool->bk_top > -1)
  {
    memcached_st *mc= pool->bk_pool[pool->bk_top--];
    (void)member_update_cachelist(mc, pool);
    return mc;
  }
#endif
#endif
  if (pool->top > -1)
  {
    return pool->mc_pool[pool->top--];
  }
  return NULL;
}


/**
 * Grow the connection pool by creating a connection structure and clone the
 * original memcached handle.
 */
static bool grow_pool(memcached_pool_st* pool)
{
  assert(pool);

  memcached_st *obj;
  if (not (obj= memcached_clone(NULL, pool->master)))
  {
    return false;
  }

  pool->mc_pool[++pool->top]= obj;
  pool->cur_size++;
#ifdef UPDATE_HASH_RING_OF_FETCHED_MC
  obj->configure.ketama_version= pool->ketama_version();
#endif
  obj->configure.version= pool->version();

  return true;
}

bool memcached_pool_st::init(uint32_t initial)
{
  mc_pool= new (std::nothrow) memcached_st *[max_size];
  if (not mc_pool)
    return false;
#ifdef POOL_MORE_CONCURRENCY
  bk_pool= new (std::nothrow) memcached_st *[max_size];
  if (not bk_pool)
  {
    delete [] mc_pool;
    return false;
  }
#endif

  /*
    Try to create the initial size of the pool. An allocation failure at
    this time is not fatal..
  */
  for (unsigned int x= 0; x < initial; ++x)
  {
    if (grow_pool(this) == false)
    {
      break;
    }
  }

  return true;
}


memcached_pool_st *memcached_pool_create(memcached_st* master, uint32_t initial, uint32_t max)
{
  if (initial == 0 or max == 0 or (initial > max))
  {
    return NULL;
  }

  memcached_pool_st *object= new (std::nothrow) memcached_pool_st(master, max);
  if (object == NULL)
  {
    return NULL;
  }

  /*
    Try to create the initial size of the pool. An allocation failure at
    this time is not fatal..
  */
  if (not object->init(initial))
  {
    delete object;
    return NULL;
  }

  master->state.is_pool_master= true;
  return object;
}

memcached_pool_st * memcached_pool(const char *option_string, size_t option_string_length)
{
  memcached_st *memc= memcached(option_string, option_string_length);

  if (memc == NULL)
  {
    return NULL;
  }

  memcached_pool_st *self= memcached_pool_create(memc, memc->configure.initial_pool_size, memc->configure.max_pool_size);
  if (self == NULL)
  {
    memcached_free(memc);
    return NULL;
  }

  self->_owns_master= true;

  return self;
}

memcached_st*  memcached_pool_destroy(memcached_pool_st* pool)
{
  if (pool == NULL)
  {
    return NULL;
  }

  // Legacy that we return the original structure
  memcached_st *ret= NULL;
  if (pool->_owns_master)
  { }
  else
  {
    ret= pool->master;
  }

  if (pool->master != NULL) {
    pool->master->state.is_pool_master= false;
  }

  delete pool;

  return ret;
}

memcached_st* memcached_pool_st::fetch(memcached_return_t& rc)
{
  static struct timespec relative_time= { 0, 0 };
  return fetch(relative_time, rc);
}

memcached_st* memcached_pool_st::fetch(const struct timespec& relative_time, memcached_return_t& rc)
{
  rc= MEMCACHED_SUCCESS;

  if (pthread_mutex_lock(&mutex))
  {
    rc= MEMCACHED_IN_PROGRESS;
    return NULL;
  }

  memcached_st *ret= NULL;
  do
  {
#ifdef USED_MC_LIST_IN_POOL
    ret= mc_list_get(this);
    if (ret != NULL)
    {
      break;
    }
#endif
    ret= mc_pool_get(this);
    if (ret != NULL)
    {
      break;
    }

    if (cur_size < max_size)
    {
      if (grow_pool(this) == false)
      {
        rc= MEMCACHED_MEMORY_ALLOCATION_FAILURE;
        break;
      }
    }
    else /* cur_size == max_size */
    {
      if (relative_time.tv_sec == 0 and relative_time.tv_nsec == 0)
      {
        rc= MEMCACHED_NOTFOUND;
        break;
      }

      struct timespec time_to_wait= {0, 0};
      time_to_wait.tv_sec= time(NULL) +relative_time.tv_sec;
      time_to_wait.tv_nsec= relative_time.tv_nsec;

      int thread_ret;
      wait_count++;
      thread_ret= pthread_cond_timedwait(&cond, &mutex, &time_to_wait);
      wait_count--;
      if (thread_ret != 0)
      {
        if (thread_ret == ETIMEDOUT)
        {
          rc= MEMCACHED_TIMEOUT;
        }
        else
        {
          errno= thread_ret;
          rc= MEMCACHED_ERRNO;
        }
        break;
      }
    }
  } while (ret == NULL);

  pthread_mutex_unlock(&mutex);

  return ret;
}

bool memcached_pool_st::release(memcached_st *released, memcached_return_t& rc)
{
  rc= MEMCACHED_SUCCESS;
  if (released == NULL)
  {
    rc= MEMCACHED_INVALID_ARGUMENTS;
    return false;
  }

  if (pthread_mutex_lock(&mutex))
  {
    rc= MEMCACHED_IN_PROGRESS;
    return false;
  }

  /*
    Someone updated the behavior on the object, so we clone a new memcached_st with the new settings. If we fail to clone, we keep the old one around.
  */
  if (compare_version(released) == false)
  {
    memcached_st *memc;
    if ((memc= memcached_clone(NULL, master)))
    {
      memcached_free(released);
      released= memc;
    }
  }
#ifdef UPDATE_HASH_RING_OF_FETCHED_MC
#ifdef LIBMEMCACHED_WITH_ZK_INTEGRATION
  else if (compare_ketama_version(released) == false)
  {
    (void)member_update_cachelist(released, this);
  }
#endif
#endif

#ifdef USED_MC_LIST_IN_POOL
  mc_list_add(this, released);
#else
  mc_pool[++top]= released;
#endif

  if (wait_count > 0)
  {
    /* we might have people waiting for a connection.. wake them up :-) */
    pthread_cond_broadcast(&cond);
  }

  (void)pthread_mutex_unlock(&mutex);

  return true;
}

memcached_st* memcached_pool_fetch(memcached_pool_st* pool, struct timespec* relative_time, memcached_return_t* rc)
{
  if (pool == NULL)
  {
    return NULL;
  }

  memcached_return_t unused;
  if (rc == NULL)
  {
    rc= &unused;
  }

  if (relative_time == NULL)
  {
    return pool->fetch(*rc);
  }

  return pool->fetch(*relative_time, *rc);
}

memcached_st* memcached_pool_pop(memcached_pool_st* pool,
                                 bool block,
                                 memcached_return_t *rc)
{
  if (pool == NULL)
  {
    return NULL;
  }

  memcached_return_t unused;
  if (rc == NULL)
  {
    rc= &unused;
  }

  memcached_st *memc;
  if (block)
  {
    memc= pool->fetch(pool->timeout(), *rc);
  }
  else
  {
    memc= pool->fetch(*rc);
  }

  return memc;
}

memcached_return_t memcached_pool_release(memcached_pool_st* pool, memcached_st *released)
{
  if (pool == NULL)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  memcached_return_t rc;

  (void) pool->release(released, rc);

  return rc;
}

memcached_return_t memcached_pool_push(memcached_pool_st* pool, memcached_st *released)
{
  return memcached_pool_release(pool, released);
}


memcached_return_t memcached_pool_behavior_set(memcached_pool_st *pool,
                                               memcached_behavior_t flag,
                                               uint64_t data)
{
  if (pool == NULL)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

#ifdef POOL_MORE_CONCURRENCY
  (void)pthread_mutex_lock(&pool->master_lock);
  (void)pthread_mutex_lock(&pool->mutex);
#else
  if (pthread_mutex_lock(&pool->mutex))
  {
    return MEMCACHED_IN_PROGRESS;
  }
#endif

  /* update the master */
  memcached_return_t rc= memcached_behavior_set(pool->master, flag, data);
  if (memcached_failed(rc))
  {
    (void)pthread_mutex_unlock(&pool->mutex);
    return rc;
  }

  pool->increment_version();

  /* update the clones */
  for (int xx= 0; xx <= pool->top; ++xx)
  {
    if (memcached_success(memcached_behavior_set(pool->mc_pool[xx], flag, data)))
    {
      pool->mc_pool[xx]->configure.version= pool->version();
    }
    else
    {
      memcached_st *memc;
      if ((memc= memcached_clone(NULL, pool->master)))
      {
        memcached_free(pool->mc_pool[xx]);
        pool->mc_pool[xx]= memc;
        /* I'm not sure what to do in this case.. this would happen
          if we fail to push the server list inside the client..
          I should add a testcase for this, but I believe the following
          would work, except that you would add a hole in the pool list..
          in theory you could end up with an empty pool....
        */
      }
    }
  }
#ifdef USED_MC_LIST_IN_POOL
  int removed_count= mc_list_behavior_set(pool, flag, data);
  for (int xx= 0; xx < removed_count; ++xx)
  {
    if (grow_pool(pool) == false)
       break;
  }
#endif

  (void)pthread_mutex_unlock(&pool->mutex);
#ifdef POOL_MORE_CONCURRENCY
  (void)pthread_mutex_unlock(&pool->master_lock);
#endif

  return rc;
}

memcached_return_t memcached_pool_behavior_get(memcached_pool_st *pool,
                                               memcached_behavior_t flag,
                                               uint64_t *value)
{
  if (pool == NULL)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  if (pthread_mutex_lock(&pool->mutex))
  {
    return MEMCACHED_IN_PROGRESS;
  }

  *value= memcached_behavior_get(pool->master, flag);

  (void)pthread_mutex_unlock(&pool->mutex);

  return MEMCACHED_SUCCESS;
}

void memcached_pool_lock(memcached_pool_st* pool)
{
  (void)pthread_mutex_lock(&pool->mutex);
}

void memcached_pool_unlock(memcached_pool_st* pool)
{
  (void)pthread_mutex_unlock(&pool->mutex);
}

#ifdef LIBMEMCACHED_WITH_ZK_INTEGRATION
/*
 * static functions
 */
static memcached_return_t member_update_cachelist(memcached_st *memc,
                                                  memcached_pool_st* pool)
{
  memcached_return_t rc;

  rc= memcached_update_cachelist_with_master(memc, pool->master);
  if (rc == MEMCACHED_SUCCESS)
  {
#ifdef UPDATE_HASH_RING_OF_FETCHED_MC
    memc->configure.ketama_version= pool->ketama_version();
#else
    memc->configure.version= pool->version();
#endif
  }
  return rc;
}

#ifdef USED_MC_LIST_IN_POOL
static int mc_list_remove_all(memcached_pool_st* pool)
{
  memcached_st *mc;
  int removed_count= 0;

  while (pool->used_mc_head)
  {
    mc= pool->used_mc_head;
    pool->used_mc_head= mc->mc_next;
    memcached_free(mc);
    removed_count++;
    pool->cur_size--;
  }
  pool->used_mc_tail= NULL;
  return removed_count;
}

static void mc_list_update_cachelist(memcached_pool_st* pool)
{
  memcached_st *mc;

#ifdef POOL_MORE_CONCURRENCY
  while (pool->used_bk_head)
  {
    /* fetch the member mc from used_bk_list */
    /* mc= mc_list_get(&pool->used_bk_list); */
    mc= pool->used_bk_head;
    pool->used_bk_head= mc->mc_next;
    if (pool->used_bk_head == NULL) {
      pool->used_bk_tail= NULL;
    }
    (void)pthread_mutex_unlock(&pool->mutex);

    /* update the chachelist of member mc without pool lock */
    (void)member_update_cachelist(mc, pool);

    (void)pthread_mutex_lock(&pool->mutex);
    /* push the member mc to the tail of used_mc_list */
    /* mc_list_append(&pool->used_mc_list, mc); */
    mc->mc_next= NULL;
    if (pool->used_mc_tail) {
      pool->used_mc_tail->mc_next= mc;
    } else {
      pool->used_mc_head= mc;
    }
    pool->used_mc_tail= mc;

    if (pool->wait_count > 0) {
      /* we might have people waiting for a connection.. wake them up :-) */
      pthread_cond_broadcast(&pool->cond);
    }
  }
#else
  mc= pool->used_mc_head;
  while (mc)
  {
    (void)member_update_cachelist(mc, pool);
    mc= mc->mc_next;
  }
#endif
}
#endif

static void mc_pool_update_cachelist(memcached_pool_st* pool)
{
#ifdef POOL_MORE_CONCURRENCY
  memcached_st *mc;

  while (pool->bk_top > -1)
  {
    /* fetch the member mc from bk_pool */
    mc= pool->bk_pool[pool->bk_top--];
    (void)pthread_mutex_unlock(&pool->mutex);

    /* update the chachelist of member mc without pool lock */
    (void)member_update_cachelist(mc, pool);

    (void)pthread_mutex_lock(&pool->mutex);
    /* push the member mc into mc_pool */
    pool->mc_pool[++pool->top]= mc;

    if (pool->wait_count > 0) {
      /* we might have people waiting for a connection.. wake them up :-) */
      pthread_cond_broadcast(&pool->cond);
    }
  }
#else
  for (int xx= 0; xx <= pool->top; ++xx)
  {
    (void)member_update_cachelist(pool->mc_pool[xx], pool);
  }
#endif
}

/**
 * Returns the pool's master
 */
memcached_st *memcached_pool_get_master(memcached_pool_st* pool)
{
  return pool->master;
}

/**
 * Repopulates the pool based on the master.
 */
memcached_return_t memcached_pool_repopulate(memcached_pool_st* pool)
{
  if (pool == NULL)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

#ifdef POOL_MORE_CONCURRENCY
  (void)pthread_mutex_lock(&pool->master_lock);
  (void)pthread_mutex_lock(&pool->mutex);
#else
  if (pthread_mutex_lock(&pool->mutex))
  {
    return MEMCACHED_IN_PROGRESS;
  }
#endif

#ifdef UPDATE_HASH_RING_OF_FETCHED_MC
  pool->increment_ketama_version();
#endif
  pool->increment_version();

  /* update the clones */
  for (int xx= 0; xx <= pool->top; ++xx)
  {
    memcached_st *memc;
    if ((memc= memcached_clone(NULL, pool->master)))
    {
      memcached_free(pool->mc_pool[xx]);
      pool->mc_pool[xx]= memc;
      /* I'm not sure what to do in this case.. this would happen
        if we fail to push the server list inside the client..
        I should add a testcase for this, but I believe the following
        would work, except that you would add a hole in the pool list..
        in theory you could end up with an empty pool....
      */
    }
  }
#ifdef USED_MC_LIST_IN_POOL
  int removed_count= mc_list_remove_all(pool);
  for (int xx= 0; xx < removed_count; ++xx)
  {
    if (grow_pool(pool) == false)
       break;
  }
#endif

  (void)pthread_mutex_unlock(&pool->mutex);
#ifdef POOL_MORE_CONCURRENCY
  (void)pthread_mutex_unlock(&pool->master_lock);
#endif

  return MEMCACHED_SUCCESS;
}

#ifdef POOL_UPDATE_SERVERLIST
memcached_return_t memcached_pool_update_cachelist(memcached_pool_st *pool,
                                                   struct memcached_server_info *serverinfo,
                                                   uint32_t servercount, bool init)
{
  memcached_return_t rc;
  bool serverlist_changed= false;

  if (pool == NULL) {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

#ifdef POOL_MORE_CONCURRENCY
  (void)pthread_mutex_lock(&pool->master_lock);
#endif
  (void)pthread_mutex_lock(&pool->mutex);
  rc= memcached_update_cachelist(pool->master, serverinfo, servercount,
                                 &serverlist_changed);
  if (init)
  {
#ifdef UPDATE_HASH_RING_OF_FETCHED_MC
    pool->increment_ketama_version();
#endif
    pool->increment_version();

    /* clone the member mcs */
    for (int xx= 0; xx <= pool->top; ++xx)
    {
      memcached_st *memc= memcached_clone(NULL, pool->master);
      if (memc) {
        memcached_free(pool->mc_pool[xx]);
        pool->mc_pool[xx]= memc;
        /* I'm not sure what to do in this case.. this would happen
           if we fail to push the server list inside the client..
           I should add a testcase for this, but I believe the following
           would work, except that you would add a hole in the pool list..
           in theory you could end up with an empty pool....
         */
      }
    }
#ifdef USED_MC_LIST_IN_POOL
    int removed_count= mc_list_remove_all(pool);
    for (int xx= 0; xx < removed_count; ++xx)
    {
      if (grow_pool(pool) == false)
         break;
    }
#endif
    rc = MEMCACHED_SUCCESS;
  }
  else if (rc == MEMCACHED_SUCCESS && serverlist_changed)
  {
#ifdef UPDATE_HASH_RING_OF_FETCHED_MC
    pool->increment_ketama_version();
#else
    pool->increment_version();
#endif

#ifdef POOL_MORE_CONCURRENCY
    /* move member mcs of used_mc_list to used_bk_list */
    pool->used_bk_head= pool->used_mc_head;
    pool->used_bk_tail= pool->used_mc_tail;
    pool->used_mc_head= NULL;
    pool->used_mc_tail= NULL;

    /* move member mcs of mc_pool to bk_pool */
    if (pool->top > -1) {
      memcpy(pool->bk_pool, pool->mc_pool, (sizeof(void*)*(pool->top+1)));
      pool->bk_top = pool->top;
      pool->top= -1;
    }
#endif

    /* update the cachelist of member mcs */
#ifdef USED_MC_LIST_IN_POOL
    mc_list_update_cachelist(pool);
#endif
    mc_pool_update_cachelist(pool);
  }
  (void)pthread_mutex_unlock(&pool->mutex);
#ifdef POOL_MORE_CONCURRENCY
  (void)pthread_mutex_unlock(&pool->master_lock);
#endif
  return rc;
}
#endif

memcached_return_t memcached_pool_update_member(memcached_pool_st* pool, memcached_st *mc)
{
  memcached_return_t rc= MEMCACHED_SUCCESS;

  (void)pthread_mutex_lock(&pool->mutex);
#ifdef UPDATE_HASH_RING_OF_FETCHED_MC
  if (mc->configure.ketama_version != pool->ketama_version())
#else
  if (mc->configure.version != pool->version())
#endif
  {
    (void)member_update_cachelist(mc, pool);
  }
  (void)pthread_mutex_unlock(&pool->mutex);
  return rc;
}

#ifdef ENABLE_REPLICATION
memcached_return_t
memcached_pool_use_single_server(memcached_pool_st *pool,
                                 const char *host, int port)
{
  memcached_st *mc;
  memcached_return_t error;
  memcached_server_list_st list;

  // single-server list
  list = memcached_server_list_append(NULL, host, port, &error);
  if (list == NULL)
    return MEMCACHED_FAILURE;

  mc = memcached_pool_get_master(pool);

  // delete all existing servers from the master
  memcached_servers_reset(mc);

  // use the new server list
  error = memcached_server_push(mc, list);
  if (list)
    memcached_server_list_free(list);
  if (error != MEMCACHED_SUCCESS)
    return error;

  // clone the master to the whole pool
  return memcached_pool_repopulate(pool);
}
#endif

uint16_t get_memcached_pool_size(memcached_pool_st* pool)
{
  if (pool == NULL) return 0;
  return pool->max_size;
}

#endif

