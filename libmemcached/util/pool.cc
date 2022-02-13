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

struct memcached_pool_st
{
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  memcached_st *master;
  memcached_st **mc_pool;
  int top;
  const uint32_t max_size;
  uint32_t cur_size;
  bool _owns_master;
  struct timespec _timeout;

  memcached_pool_st(memcached_st *master_arg, size_t max_arg) :
    master(master_arg),
    mc_pool(NULL),
    top(-1),
    max_size(max_arg),
    cur_size(0),
    _owns_master(false)
  {
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
    for (int x= 0; x <= top; ++x)
    {
      memcached_free(mc_pool[x]);
      mc_pool[x] = NULL;
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    delete [] mc_pool;
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
#ifdef UPDATE_HASH_RING_OF_FETCHED_MC
    return (arg->configure.version == version() && compare_ketama_version(arg));
#else
    return (arg->configure.version == version());
#endif
  }

  int32_t version() const
  {
    return master->configure.version;
  }
};


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
    if (top > -1)
    {
      ret= mc_pool[top--];
    }
    else if (cur_size < max_size)
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
      if ((thread_ret= pthread_cond_timedwait(&cond, &mutex, &time_to_wait)) != 0)
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

#ifdef LIBMEMCACHED_WITH_ZK_INTEGRATION
  if (ret != NULL && ret->ketama.info == NULL) {
    arcus_st *arcus= static_cast<arcus_st *>(memcached_get_server_manager(ret));
    if (arcus && arcus->pool) {
      memcached_ketama_reference(ret, this->master);
    }
  }
#endif

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

#ifdef LIBMEMCACHED_WITH_ZK_INTEGRATION
  arcus_st *arcus= static_cast<arcus_st *>(memcached_get_server_manager(released));
  if (arcus && arcus->pool) {
    memcached_ketama_release(released);
  }
#endif

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

  mc_pool[++top]= released;

  if (top == 0 and cur_size == max_size)
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

  if (pthread_mutex_lock(&pool->mutex))
  {
    return MEMCACHED_IN_PROGRESS;
  }

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

  (void)pthread_mutex_unlock(&pool->mutex);

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

  if (pthread_mutex_lock(&pool->mutex))
  {
    return MEMCACHED_IN_PROGRESS;
  }

#ifdef UPDATE_HASH_RING_OF_FETCHED_MC
  pool->increment_ketama_version();
#else
  pool->increment_version();
#endif

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

  (void)pthread_mutex_unlock(&pool->mutex);

  return MEMCACHED_SUCCESS;
}

#ifdef POOL_UPDATE_SERVERLIST
#ifdef LOCK_UPDATE_SERVERLIST
memcached_return_t memcached_pool_update_cachelist(memcached_pool_st *pool,
                                                   struct memcached_server_info *serverinfo,
                                                   uint32_t servercount, bool init)
#else
memcached_return_t memcached_pool_update_cachelist(memcached_pool_st *pool)
#endif
{
#ifdef LOCK_UPDATE_SERVERLIST
  memcached_return_t rc;
  bool serverlist_changed= false;
#endif
  if (pool == NULL) {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  (void)pthread_mutex_lock(&pool->mutex);
#ifdef LOCK_UPDATE_SERVERLIST
  rc= memcached_update_cachelist(pool->master, serverinfo, servercount,
                                 &serverlist_changed);
  if (init)
  {
#ifdef UPDATE_HASH_RING_OF_FETCHED_MC
    pool->increment_ketama_version();
#else
    pool->increment_version();
#endif

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
    rc = MEMCACHED_SUCCESS;
  }
  else if (rc == MEMCACHED_SUCCESS && serverlist_changed)
#endif
  {
#ifdef UPDATE_HASH_RING_OF_FETCHED_MC
    pool->increment_ketama_version();
#else
    pool->increment_version();
#endif

    /* update the cachelist of member mcs */
    for (int xx= 0; xx <= pool->top; ++xx)
    {
      memcached_st *mc= pool->mc_pool[xx];
      memcached_return_t error;
      error= memcached_update_cachelist_with_master(mc, pool->master);
      if (error == MEMCACHED_SUCCESS) {
#ifdef UPDATE_HASH_RING_OF_FETCHED_MC
        mc->configure.ketama_version= pool->ketama_version();
#else
        mc->configure.version= pool->version();
#endif
      }
    }
  }
  (void)pthread_mutex_unlock(&pool->mutex);
#ifdef LOCK_UPDATE_SERVERLIST
  return rc;
#else
  return MEMCACHED_SUCCESS;
#endif
}
#endif

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

