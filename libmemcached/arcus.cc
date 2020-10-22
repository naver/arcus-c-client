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

#include "libmemcached/common.h"
#include "libmemcached/arcus_priv.h"

#include <sys/file.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#define ARCUS_ZK_SESSION_TIMEOUT_IN_MS        15000
#define ARCUS_ZK_HEARTBEAT_INTERVAL_IN_SEC    1
#define ARCUS_ZK_RETRY_RECONNECT_DELAY_IN_MS  1000 /* 1 sec */
#define ZOO_NO_FLAGS 0

#define ARCUS_ZK_CACHE_LIST                   "/arcus/cache_list"
#define ARCUS_ZK_CLIENT_INFO_NODE             "/arcus/client_list"
#ifdef ENABLE_REPLICATION
#define ARCUS_REPL_ZK_CACHE_LIST              "/arcus_repl/cache_list"
#define ARCUS_REPL_ZK_CLIENT_INFO_NODE        "/arcus_repl/client_list"
#endif

/**
 * ARCUS
 */
static inline arcus_return_t do_arcus_connect(memcached_st *mc, memcached_pool_st *pool,
                                              const char *ensemble_list, const char *svc_code);
static inline arcus_return_t do_arcus_init(memcached_st *mc, memcached_pool_st *pool,
                                           const char *ensemble_list, const char *svc_code);

/**
 * ARCUS_PROXY
 */
static inline arcus_return_t do_arcus_proxy_create(memcached_st *mc, const char *name);
static inline arcus_return_t do_arcus_proxy_connect(memcached_st *mc, memcached_pool_st *pool, memcached_st *proxy);
static inline arcus_return_t do_arcus_proxy_close(memcached_st *mc);
static inline void           do_arcus_proxy_update_cachelist(memcached_st *mc, const struct String_vector *strings);

/**
 * ARCUS_ZK_CONNECTION
 */
static inline arcus_return_t do_arcus_zk_connect(memcached_st *mc);
static inline arcus_return_t do_arcus_zk_close(memcached_st *mc);

/**
 * ARCUS_ZK_WATCHER
 */
static inline void do_arcus_zk_watcher_global(zhandle_t *zh, int type, int state, const char *path, void *ctx_arcus);
static inline void do_arcus_zk_watcher_cachelist(zhandle_t *zh, int type, int state, const char *path, void *ctx_arcus);

/**
 * ARCUS_ZK_OPERATIONS
 */
static inline void do_arcus_zk_update_cachelist(memcached_st *mc, const struct String_vector *strings);
static inline void do_arcus_zk_update_cachelist_by_string(memcached_st *mc, char *serverlist, const size_t size);
static inline void do_arcus_zk_watch_and_update_cachelist(memcached_st *mc, bool *retry);
static inline int  do_arcus_cluster_validation_check(memcached_st *mc, arcus_st *arcus);

/**
 * ARCUS_ZK_MANAGER
 */
/* zk manager functions */
static inline arcus_return_t  do_arcus_zk_manager_start(memcached_st *mc);
static inline void            do_arcus_zk_manager_stop(memcached_st *mc);
static void                   do_arcus_zk_manager_wakeup(memcached_st *mc, bool locked);
static void                  *arcus_zk_manager_thread_main(void *arg);

/**
 * UTILITIES
 */
static inline void do_arcus_update_cachelist(memcached_st *mc,
                                             struct memcached_server_info *serverinfo,
                                             uint32_t servercount);
static inline int do_add_server_to_cachelist(struct arcus_zk_st *zkinfo, char *nodename,
                                             struct memcached_server_info *serverinfo);
#ifdef UPDATE_HASH_RING_OF_FETCHED_MC
static inline void do_update_serverlist_with_master(memcached_st *ptr, memcached_st *master);
#endif

/* async routine synchronization */
static pthread_mutex_t azk_mtx  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  azk_cond = PTHREAD_COND_INITIALIZER;
static int             azk_count;

pthread_mutex_t lock_arcus = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond_arcus = PTHREAD_COND_INITIALIZER;

/* Mutex via OS file lock */
static int proc_mutex_create(struct arcus_proc_mutex *m, const char *fname);
static int proc_mutex_lock(struct arcus_proc_mutex *m);
static int proc_mutex_unlock(struct arcus_proc_mutex *m);
static int proc_mutex_destroy(struct arcus_proc_mutex *m);

/**
 * PUBLIC API
 * Creates the Arcus Manager and ZooKeeper client.
 */
arcus_return_t arcus_connect(memcached_st *mc,
                             const char *ensemble_list,
                             const char *svc_code)
{
  return do_arcus_connect(mc, NULL, ensemble_list, svc_code);
}

/**
 * PUBLIC API
 * Clean up the Arcus manager and ZooKeeper client.
 */
arcus_return_t arcus_close(memcached_st *mc)
{
  arcus_st *arcus;
  arcus_return_t rc;

  /* Stop the ZooKeeper manager */
  do_arcus_zk_manager_stop(mc);

  /* Close the ZooKeeper client. */
  rc= do_arcus_zk_close(mc);
  if (rc != ARCUS_SUCCESS) {
    ZOO_LOG_ERROR(("Failed to close zookeeper client"));
    return rc;
  }

  pthread_mutex_lock(&lock_arcus);
  arcus= static_cast<arcus_st *>(memcached_get_server_manager(mc));
  if (arcus) {
    arcus->pool= NULL;
    free(arcus);
    arcus= NULL;
  }
  memcached_set_server_manager(mc, NULL);
  pthread_mutex_unlock(&lock_arcus);

  return ARCUS_SUCCESS;
}

arcus_return_t arcus_pool_connect(memcached_pool_st *pool,
                                  const char *ensemble_list,
                                  const char *svc_code)
{
  memcached_st *mc= memcached_pool_get_master(pool);
  arcus_st *arcus;
  arcus_return_t rc;

  arcus= static_cast<arcus_st *>(memcached_get_server_manager(mc));
  if (not arcus) {
    rc= do_arcus_connect(memcached_pool_get_master(pool), pool, ensemble_list, svc_code);
    if (rc != ARCUS_SUCCESS) {
      return rc;
    }
    pthread_mutex_lock(&lock_arcus);
    arcus= static_cast<arcus_st *>(memcached_get_server_manager(mc));
    memcached_return_t error= memcached_pool_repopulate(pool);
    pthread_mutex_unlock(&lock_arcus);
    if (error != MEMCACHED_SUCCESS) {
      ZOO_LOG_WARN(("failed to repopulate the pool!"));
    }
  } else {
    ZOO_LOG_WARN(("arcus is already initiated"));
  }

  return ARCUS_SUCCESS;
}

arcus_return_t arcus_pool_close(memcached_pool_st *pool)
{
  return arcus_close(memcached_pool_get_master(pool));
}

arcus_return_t arcus_proxy_create(memcached_st *mc,
                                  const char *ensemble_list,
                                  const char *svc_code)
{
  arcus_return_t rc;

  /* Initiate the Arcus. */
  rc= do_arcus_init(mc, NULL, ensemble_list, svc_code);
  if (rc == ARCUS_ALREADY_INITIATED) {
    return ARCUS_SUCCESS;
  }
  else if (rc != ARCUS_SUCCESS) {
    return ARCUS_ERROR; 
  }

  /* Be a proxy. */
  rc= do_arcus_proxy_create(mc, ".arcus_proxy");
  if (rc != ARCUS_SUCCESS) {
    return rc;
  }

  /* Creates a new ZooKeeper client thread. */
  rc= do_arcus_zk_connect(mc);
  if (rc != ARCUS_SUCCESS) {
    return ARCUS_ERROR;
  }

  /* Creates a new ZooKeeper Manager thread. */
  rc= do_arcus_zk_manager_start(mc);
  if (rc != ARCUS_SUCCESS) {
    return ARCUS_ERROR;
  }
  return rc;
}

arcus_return_t arcus_proxy_connect(memcached_st *mc,
                                   memcached_pool_st *pool,
                                   memcached_st *proxy)
{
  arcus_return_t rc;

  /* Initiate the Arcus. */
  rc= do_arcus_init(mc, NULL, NULL, NULL);
  if (rc == ARCUS_ALREADY_INITIATED) {
    return ARCUS_SUCCESS;
  }
  else if (rc != ARCUS_SUCCESS) {
    return ARCUS_ERROR; 
  }

  arcus_set_log_stream(mc, proxy->logfile);
  rc= do_arcus_proxy_connect(mc, pool, proxy);
  arcus_server_check_for_update(mc);

  return rc;
}

arcus_return_t arcus_proxy_close(memcached_st *mc)
{
  arcus_return_t rc;

  rc= do_arcus_proxy_close(mc);
  rc= arcus_close(mc);

  return rc;
}

static inline arcus_return_t do_arcus_connect(memcached_st *mc,
                                              memcached_pool_st *pool,
                                              const char *ensemble_list,
                                              const char *svc_code)
{
  arcus_return_t rc;

  /* Initiate the Arcus. */
  rc= do_arcus_init(mc, pool, ensemble_list, svc_code);
  if (rc == ARCUS_ALREADY_INITIATED) {
    return ARCUS_SUCCESS;
  }
  else if (rc != ARCUS_SUCCESS) {
    return ARCUS_ERROR; 
  }

  /* Creates a new ZooKeeper client thread. */
  rc= do_arcus_zk_connect(mc);
  if (rc != ARCUS_SUCCESS) {
    return ARCUS_ERROR;
  }

  /* Creates a new ZooKeeper Manager thread. */
  rc= do_arcus_zk_manager_start(mc);
  if (rc != ARCUS_SUCCESS) {
    return ARCUS_ERROR;
  }
  return rc;
}

static inline arcus_return_t do_arcus_proxy_create(memcached_st *mc,
                                                   const char *name)
{
  void *mapped_addr;
  arcus_st *arcus= static_cast<arcus_st *>(memcached_get_server_manager(mc));

  strncpy(arcus->proxy.name, name, 256);
  arcus->is_proxy= true;

  /* Mmap */
#ifdef TARGET_OS_LINUX
  mapped_addr= mmap(NULL, ARCUS_MAX_PROXY_FILE_LENGTH, PROT_WRITE | PROT_READ,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
#else
  mapped_addr= mmap(NULL, ARCUS_MAX_PROXY_FILE_LENGTH, PROT_WRITE | PROT_READ,
                    MAP_SHARED, -1, 0);
#endif
  if (mapped_addr == MAP_FAILED) {
    ZOO_LOG_ERROR(("cannot map the proxy file"));
    return ARCUS_ERROR;
  }

  /* With the proxy data. */
  arcus->proxy.data= (arcus_proxy_data_st *)mapped_addr;
  arcus->proxy.data->version = 0;

  /* Initialize a mutex to protect the proxy data. */
  arcus->proxy.data->mutex.fd = -1;
  arcus->proxy.data->mutex.locked = 0;
  arcus->proxy.data->mutex.fname = NULL;

  char ap_lock_fname[64];
  snprintf(ap_lock_fname, 64, ".arcus_proxy_lock.%d", getpid());

  if (0 != proc_mutex_create(&arcus->proxy.data->mutex, ap_lock_fname)) {
    ZOO_LOG_ERROR(("Cannot create the proxy lock file. You might have to"
            " delete the lock file manually. file=%s error=%s(%d)",
            ap_lock_fname, strerror(errno), errno));
    return ARCUS_ERROR;
  }

  return ARCUS_SUCCESS;
}

static inline arcus_return_t do_arcus_proxy_connect(memcached_st *mc,
                                                    memcached_pool_st *pool,
                                                    memcached_st *proxy)
{
  arcus_st *arcus= static_cast<arcus_st *>(memcached_get_server_manager(mc));
  arcus_st *proxy_arcus= static_cast<arcus_st *>(memcached_get_server_manager(proxy));

  strncpy(arcus->proxy.name, proxy_arcus->proxy.name, 256);
  arcus->proxy.data= proxy_arcus->proxy.data;
  arcus->pool = pool;
#ifdef ENABLE_REPLICATION
  arcus->zk.is_repl_enabled= proxy_arcus->zk.is_repl_enabled;
  mc->flags.repl_enabled= arcus->zk.is_repl_enabled;
  if (arcus->pool) {
    memcached_st *master = memcached_pool_get_master(arcus->pool);
    if (master != mc) {
      master->flags.repl_enabled=  arcus->zk.is_repl_enabled;
    }
  }
#endif

  return ARCUS_SUCCESS;
}

static inline arcus_return_t do_arcus_proxy_close(memcached_st *mc)
{
  arcus_st *arcus = static_cast<arcus_st *>(memcached_get_server_manager(mc));

  /* if my role is proxy. */
  if (arcus->is_proxy) {
    arcus->proxy.data->version= 0;
    arcus->is_proxy= false;
    if (0 != proc_mutex_destroy(&arcus->proxy.data->mutex)) {
      ZOO_LOG_ERROR(("Failed to free the mutex. error=%s(%d)",
                    strerror(errno), errno));
    }
  }

  memset(arcus->proxy.name, 0, sizeof(arcus->proxy.name));
  arcus->proxy.current_version= 0;
  arcus->proxy.data= NULL;
  return ARCUS_SUCCESS;
}

/**
 * PUBLIC API
 * Returns a error string for the given error type.
 */
const char *arcus_strerror(arcus_return_t rc)
{
  switch (rc)
  {
  case ARCUS_SUCCESS:
    return "ARCUS_SUCCESS";
  case ARCUS_ERROR:
    return "ARCUS_ERROR";
  case ARCUS_AUTH_FAILED:
    return "ARCUS_AUTH_FAILED";
  case ARCUS_ALREADY_INITIATED:
    return "arcus is already initiated";
  default:
    return "unknown error";
  }
}

/**
 * PUBLIC API
 * Set a log file
 */
void arcus_set_log_stream(memcached_st *mc,
                          FILE *logfile)
{
  zoo_set_log_stream(logfile);
  mc->logfile= logfile;
}

/**
 * Initialize the Arcus.
 */
static inline arcus_return_t do_arcus_init(memcached_st *mc,
                                           memcached_pool_st *pool,
                                           const char *ensemble_list,
                                           const char *svc_code)
{
  arcus_st *arcus;
  arcus_return_t rc= ARCUS_SUCCESS;

  zoo_set_debug_level(ZOO_LOG_LEVEL_WARN);

  /* Set memcached flags (Arcus default) */
  mc->flags.use_sort_hosts= false;
  memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_NO_BLOCK,        1);
  memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_TCP_NODELAY,     1);
  memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED, 1);
  memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_DISTRIBUTION,    MEMCACHED_DISTRIBUTION_CONSISTENT_KETAMA_SPY);

  memcached_behavior_set_key_hash(mc, MEMCACHED_HASH_MD5);

  pthread_mutex_lock(&lock_arcus);
  do {
    arcus= static_cast<arcus_st *>(memcached_get_server_manager(mc));
    if (arcus) {
      ZOO_LOG_WARN(("Your memcached already has a Arcus server manager"));
      rc= ARCUS_ALREADY_INITIATED; break;
    }

    /* Create a new Arcus client */
    arcus= static_cast<arcus_st *>(libmemcached_malloc(mc, sizeof(arcus_st)));
    if (not arcus) {
      ZOO_LOG_ERROR(("cannot allocate the arcus"));
      rc= ARCUS_ERROR; break;
    }

    memset(arcus, 0, sizeof(arcus_st));

    arcus->zk.port= 0;
    arcus->zk.session_timeout= ARCUS_ZK_SESSION_TIMEOUT_IN_MS;
    arcus->zk.maxbytes= 0;
    arcus->zk.last_rc= !ZOK;
    arcus->zk.conn_result= ARCUS_SUCCESS;
    arcus->zk.is_initializing= true;

    arcus->pool = pool;
    arcus->is_proxy= false;

    /* Set ZooKeeper parameters */
    snprintf(arcus->zk.ensemble_list, sizeof(arcus->zk.ensemble_list),
             "%s", (ensemble_list)?ensemble_list:"");
    snprintf(arcus->zk.svc_code, sizeof(arcus->zk.svc_code),
             "%s", (svc_code)?svc_code:"");
    snprintf(arcus->zk.path, sizeof(arcus->zk.path),
             "%s/%s", ARCUS_ZK_CACHE_LIST, arcus->zk.svc_code);

    /* Set the Arcus to memcached as a server manager.  */
    memcached_set_server_manager(mc, (void *)arcus);

    /* clear zk_manager structure */
    memset(&arcus->zk_mgr.request, 0, sizeof(struct arcus_zk_request_st));
    pthread_mutex_init(&arcus->zk_mgr.lock, NULL);
    pthread_cond_init(&arcus->zk_mgr.cond, NULL);
    arcus->zk_mgr.notification= false;
    arcus->zk_mgr.reqstop= false;
    arcus->zk_mgr.running= false;
  } while(0);
  pthread_mutex_unlock(&lock_arcus);

  return rc;
}

static inline int do_add_client_info(arcus_st *arcus)
{
  int result;
  char path[250];
  char hostname[256];
  struct hostent *host = NULL;
  time_t timer;
  struct tm *ti;

  timer = time(NULL);
  ti = localtime(&timer);
  if (gethostname(hostname, sizeof(hostname)) < 0) {
    memcpy(hostname, "NULL", 5);
  } else {
    host = (struct hostent *) gethostbyname(hostname);
  }

  /* create the ephemeral znode
   * "/arcus or arcus_repl/client_list/{service_code}/
   *  {client hostname}_{ip address}_{pool count}_c_{client version}_{YYYYMMDDHHIISS}_{zk session id}"
   * it means administrator has to create the {service_code} node before using.
   */
  char* client_info_znode = (char*)ARCUS_ZK_CLIENT_INFO_NODE;
#ifdef ENABLE_REPLICATION
  if (arcus->zk.is_repl_enabled) {
    client_info_znode = (char*)ARCUS_REPL_ZK_CLIENT_INFO_NODE;
  }
#endif
  snprintf(path, sizeof(path), "%s/%s/%.*s_%s_%u_c_%s_%d%02d%02d%02d%02d%02d_%llx",
                              client_info_znode,
                              arcus->zk.svc_code,
                              50, hostname,
                              (host == NULL ? "NULL" : inet_ntoa(*((struct in_addr *)host->h_addr))),
                              (unsigned int)get_memcached_pool_size(arcus->pool),
                              ARCUS_VERSION_STRING,
                              ti->tm_year+1900, ti->tm_mon+1, ti->tm_mday, ti->tm_hour, ti->tm_min, ti->tm_sec,
                              (long long) zoo_client_id(arcus->zk.handle)->client_id);

  result = zoo_exists(arcus->zk.handle, path, 0, NULL);
  if (result == ZNONODE) {
    result = zoo_create (arcus->zk.handle, path, NULL, 0, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, path, sizeof(path));
  } else if (result == ZOK) {
    int nops= 2;
    zoo_op_t ops[2];
    zoo_op_result_t results[2];
    zoo_delete_op_init(&ops[0], path, -1);
    zoo_create_op_init(&ops[1], path, NULL, 0, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, path, sizeof(path));

    result = zoo_multi (arcus->zk.handle, nops, ops, results);
    if (result == ZNONODE) {
      result = zoo_create (arcus->zk.handle, path, NULL, 0, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, path, sizeof(path));
    }
  }
  if (result != ZOK) {
    ZOO_LOG_ERROR(("the znode of client info can not create"));
    return -1;
  }
  return 0;
}

/* mutex for async operations */
static void inc_count(int delta)
{
  pthread_mutex_lock(&azk_mtx);
  azk_count += delta;
  pthread_cond_broadcast(&azk_cond);
  pthread_mutex_unlock(&azk_mtx);
}

static void clear_count(void)
{
  pthread_mutex_lock(&azk_mtx);
  azk_count= 0;
  pthread_mutex_unlock(&azk_mtx);
}

static int wait_count(int timeout)
{
    struct timeval  tv;
    struct timespec ts;
    int rc= 0;
    pthread_mutex_lock(&azk_mtx);
    while (azk_count > 0 && rc == 0) {
        if (timeout == 0) {
            rc= pthread_cond_wait(&azk_cond, &azk_mtx);
        } else {
            gettimeofday(&tv, NULL);
            ts.tv_sec = tv.tv_sec;
            ts.tv_nsec= tv.tv_usec*1000;
            ts.tv_sec += timeout/1000; /* timeout: msec */
            rc= pthread_cond_timedwait(&azk_cond, &azk_mtx, &ts);
        }
    }
    pthread_mutex_unlock(&azk_mtx);
    return rc;
}

/**
 * Creates a new ZooKeeper client thread.
 */
static inline arcus_return_t do_arcus_zk_connect(memcached_st *mc)
{
  arcus_st *arcus;
  arcus_return_t rc= ARCUS_SUCCESS;

  inc_count(1);

  pthread_mutex_lock(&lock_arcus);
  do {
    arcus= static_cast<arcus_st *>(memcached_get_server_manager(mc));
    if (not arcus) {
      rc= ARCUS_ERROR; break;
    }
    if (arcus->zk.handle) {
      ZOO_LOG_WARN(("Arcus already has a zookeeper client"));
      rc= ARCUS_ERROR; break;
    }
    if (not arcus->zk.ensemble_list) {
      ZOO_LOG_ERROR(("Ensemble list cannot be null"));
      rc= ARCUS_ERROR; break;
    }

    ZOO_LOG_WARN(("Initiating zookeeper client"));

    /* Connect to ZooKeeper ensemble. */
    arcus->zk.handle= zookeeper_init(arcus->zk.ensemble_list,
                                     do_arcus_zk_watcher_global,
                                     arcus->zk.session_timeout,
                                     &(arcus->zk.myid),
                                     (void *)mc,
                                     ZOO_NO_FLAGS);
    if (not arcus->zk.handle) {
      ZOO_LOG_ERROR(("zookeeper_init() failed, reason=%s, zookeeper=%s",
                    strerror(errno), arcus->zk.ensemble_list));
      rc= ARCUS_ERROR; break;
    }
  } while(0);
  pthread_mutex_unlock(&lock_arcus);

  if (rc != ARCUS_SUCCESS) {
    return ARCUS_ERROR;
  }

  /* Wait for zk connected
   * We need to wait until ZOO_CONNECTED_STATE
   */
  do {
    if (wait_count(arcus->zk.session_timeout) != 0) {
      ZOO_LOG_ERROR(("ZooKeeper connection failed until the session timeout."));
      inc_count(-1);
      rc= ARCUS_ERROR; break;
    }

    pthread_mutex_lock(&lock_arcus);
    if (arcus->zk.conn_result != ARCUS_SUCCESS) {
      ZOO_LOG_ERROR(("ZooKeeper connection failed, result=%d", arcus->zk.conn_result));
      pthread_mutex_unlock(&lock_arcus);
      rc= ARCUS_ERROR; break;
    }

    if (do_arcus_cluster_validation_check(mc, arcus) < 0) {
      pthread_mutex_unlock(&lock_arcus);
      rc= ARCUS_ERROR; break;
    }
    pthread_mutex_unlock(&lock_arcus);

    if (do_add_client_info(arcus) < 0) {
      rc= ARCUS_ERROR; break;
    }
  } while(0);

  if (rc != ARCUS_SUCCESS) {
      pthread_mutex_lock(&lock_arcus);
      zookeeper_close(arcus->zk.handle);
      arcus->zk.handle= NULL;
      pthread_mutex_unlock(&lock_arcus);
  }
  return rc;
}

/**
 * Destroy given ZooKeeper client thread.
 */
static inline arcus_return_t do_arcus_zk_close(memcached_st *mc)
{
  arcus_st *arcus;
  arcus_return_t rc= ARCUS_SUCCESS;

  clear_count();

  pthread_mutex_lock(&lock_arcus);
  do {
    arcus= static_cast<arcus_st *>(memcached_get_server_manager(mc));
    if (not arcus) {
      rc= ARCUS_ERROR; break;
    }

    /* Delete the (expired) session. */
    arcus->zk.myid.client_id= 0;

    /* Clear connect result */
    arcus->zk.conn_result= ARCUS_SUCCESS;

    /* Close the ZooKeeper handle. */
    if (arcus->zk.handle) {
      ZOO_LOG_WARN(("Closing zookeeper client"));
      int zrc= zookeeper_close(arcus->zk.handle);
      arcus->zk.handle= NULL;
      if (zrc != ZOK) {
        ZOO_LOG_WARN(("zookeeper_close() failed, reason=%s, ensemble=%s",
                     zerror(zrc), arcus->zk.ensemble_list));
        rc= ARCUS_ERROR; break;
      }
    }
  } while(0);
  pthread_mutex_unlock(&lock_arcus);

  return rc;
}

/**
 * PUBLIC API
 * Check for cache server updates.
 */
void arcus_server_check_for_update(memcached_st *ptr)
{
  arcus_st *arcus;
  size_t size;
  uint32_t version;

  arcus= static_cast<arcus_st *>(memcached_get_server_manager(ptr));
#ifdef UPDATE_HASH_RING_OF_FETCHED_MC
  if (not arcus) {
    return;
  }

  if (arcus->proxy.data)
  {
    if (arcus->proxy.data->version != arcus->proxy.current_version)
    {
      proc_mutex_lock(&arcus->proxy.data->mutex);
      {
        version= arcus->proxy.data->version;
        size= arcus->proxy.data->size;

        if (arcus->pool) {
          memcached_st *master = memcached_pool_get_master(arcus->pool);
          // update the master just once
          if (master && master->configure.ketama_version == ptr->configure.ketama_version) {
            do_arcus_zk_update_cachelist_by_string(master, arcus->proxy.data->serverlist, size);
          }
        }

        do_arcus_zk_update_cachelist_by_string(ptr, arcus->proxy.data->serverlist, size);
        arcus->proxy.current_version= version;
      }
      proc_mutex_unlock(&arcus->proxy.data->mutex);
    }
  }
  else
  {
    if (arcus->pool) {
      memcached_st *master = memcached_pool_get_master(arcus->pool);
      if (master && master->configure.ketama_version != ptr->configure.ketama_version) {
        /* master's cache list was changed, update my server list. */
        pthread_mutex_lock(&lock_arcus);
        {
          memcached_pool_lock(arcus->pool);
          do_update_serverlist_with_master(ptr, master);
          memcached_pool_unlock(arcus->pool);
        }
        pthread_mutex_unlock(&lock_arcus);
      }
    }
  }
#else
  if (not arcus or not arcus->proxy.data) {
    return;
  }

  if (arcus->proxy.data->version != arcus->proxy.current_version)
  {
    proc_mutex_lock(&arcus->proxy.data->mutex);
    {
      version= arcus->proxy.data->version;
      size= arcus->proxy.data->size;

      if (arcus->pool) {
        memcached_st *master = memcached_pool_get_master(arcus->pool);
        // update the master just once
        if (master && master->configure.version == ptr->configure.version) {
          do_arcus_zk_update_cachelist_by_string(master, arcus->proxy.data->serverlist, size);
        }
      }

      do_arcus_zk_update_cachelist_by_string(ptr, arcus->proxy.data->serverlist, size);
      arcus->proxy.current_version= version;
    }
    proc_mutex_unlock(&arcus->proxy.data->mutex);
  }
#endif
}

static inline int do_arcus_cluster_validation_check(memcached_st *mc, arcus_st *arcus)
{
  int zkrc;
  struct Stat stat;

#ifdef ENABLE_REPLICATION
  /* Check /arcus_repl/cache_list/{svc} first
   * If it exists, the service code belongs to a repl cluster.
   */
  snprintf(arcus->zk.path, sizeof(arcus->zk.path),
    "%s/%s", ARCUS_REPL_ZK_CACHE_LIST, arcus->zk.svc_code);
  zkrc= zoo_exists(arcus->zk.handle, arcus->zk.path, 0, &stat);
  if (zkrc == ZOK) {
    ZOO_LOG_WARN(("Detected Arcus replication cluster. %s exits", arcus->zk.path));
    arcus->zk.is_repl_enabled= true;
    mc->flags.repl_enabled= true;
    return 0;
  }
#endif

  snprintf(arcus->zk.path, sizeof(arcus->zk.path),
    "%s/%s", ARCUS_ZK_CACHE_LIST, arcus->zk.svc_code);
  zkrc= zoo_exists(arcus->zk.handle, arcus->zk.path, 0, &stat);
  if (zkrc == ZOK) {
    ZOO_LOG_WARN(("Detected Arcus cluster. %s exits", arcus->zk.path));
    arcus->zk.is_repl_enabled= false;
    mc->flags.repl_enabled= false;
    return 0;
  }
  else {
    ZOO_LOG_ERROR(("zoo_exists failed while trying to"
      " determine Arcus version. path=%s reason=%s, zookeeper=%s",
      arcus->zk.path, zerror(zkrc), arcus->zk.ensemble_list));
    return -1;
  }
}

/**
 * Add a server to the cache server list with a given host:port string.
 * @note servers parameter must be freed.
 */
static inline int do_add_server_to_cachelist(struct arcus_zk_st *zkinfo, char *nodename,
                                             struct memcached_server_info *serverinfo)
{
  int c= 0;
  char *hostport= nodename;
  char *buffer;
  char *word;
  char seps[]= ":-";

  if (not zkinfo) {
    ZOO_LOG_ERROR(("arcus zk is null"));
    return -1;
  }

#ifdef ENABLE_REPLICATION
  if (zkinfo->is_repl_enabled) {
    for (word= strtok_r(nodename, "^", &buffer);
         word;
         word= strtok_r(NULL,     "^", &buffer), c++)
    {
      if (c == 0) /* groupname */
      {
        serverinfo->groupname= word;
      }
      else if (c == 1) /* role : M or S */
      {
        if (strlen(word) != 1 || (word[0] != 'M' && word[0] != 'S'))
          break; /* invalid znode name */
        serverinfo->master= (word[0] == 'M' ? true : false);
      }
      else /* hostport */
      {
        hostport= word;
        break;
      }
    }
    if (c < 2) {
      return -1; /* invalid znode name */
    }
    c= 0;
  } else {
    serverinfo->groupname= NULL;
    serverinfo->master= false;
  }
#endif

  /* expected = <IP>:<PORT>-<HOSTNAME>
   *      e.g.  10.64.179.212:11212-localhost */
  for (word= strtok_r(hostport, seps, &buffer);
       word;
       word= strtok_r(NULL,     seps, &buffer), c++)
  {
    if (c == 0) /* HOST */
    {
      serverinfo->hostname= word;
    }
    else if (c == 1) /* PORT */
    {
      serverinfo->port= atoi(word);
    }
    else
    {
      break;
    }
  }
  if (c < 1 || serverinfo->port == 0) {
    return -1; /* invalid znode name */
  }
  serverinfo->exist= false;
  return 0;
}

static inline void do_arcus_proxy_update_cachelist(memcached_st *mc,
                                                   const struct String_vector *strings)
{
  arcus_st *arcus= static_cast<arcus_st *>(memcached_get_server_manager(mc));

  /* Lock the data mutex */
  proc_mutex_lock(&arcus->proxy.data->mutex);
  {
    if (strings->count) {
      /* Convert String_vector to comma-seperated string */
      size_t write_length= 0;
      for (int i= 0; i< strings->count; i++) {
        strcpy(arcus->proxy.data->serverlist + write_length, strings->data[i]);
        write_length+= strlen(strings->data[i]);
        arcus->proxy.data->serverlist[write_length++]= ',';
      }
      arcus->proxy.data->serverlist[write_length-1]= '\0';
      arcus->proxy.data->size = strings->count;
    } else { /* Empty string */
      arcus->proxy.data->serverlist[0]= '\0';
      arcus->proxy.data->size= 0;
    }
    /* Increase the data version */
    arcus->proxy.data->version++;
    ZOO_LOG_WARN(("proxy : data updated (version=%d) : %s",
                 arcus->proxy.data->version,
                 (arcus->proxy.data->size == 0) ? "NO CACHE SERVERS"
                                                : arcus->proxy.data->serverlist));
  }
  /* Unlock the data mutex */
  proc_mutex_unlock(&arcus->proxy.data->mutex);

  unlikely (arcus->zk.is_initializing)
  {
    arcus->zk.is_initializing= false;
    pthread_cond_broadcast(&cond_arcus);
  }
}

#ifdef ENABLE_REPLICATION
static inline memcached_return_t
__do_arcus_update_grouplist(memcached_st *mc,
                            struct memcached_server_info *serverinfo,
                            uint32_t servercount, bool *serverlist_changed)
{
  struct memcached_rgroup_info *groupinfo;
  uint32_t groupcount= 0;
  uint32_t validcount= 0;
  uint32_t x, y;
  bool prune_flag= false;

  if (servercount == 0) {
    if (memcached_server_count(mc) == 0) {
      return MEMCACHED_SUCCESS;
    }
    memcached_rgroup_prune(mc, true); /* prune all rgroups */
    *serverlist_changed= true;
    return run_distribution(mc);
  }

  ZOO_LOG_INFO(("__do_arcus_update_grouplist: count=%u\n", servercount));
  for (x= 0; x < servercount; x++) {
    ZOO_LOG_INFO(("server[%u] groupname=%s %s hostname=%s port=%u\n",
            x, serverinfo[x].groupname, 
            serverinfo[x].master ? "master" : "slave",
            serverinfo[x].hostname, 
            serverinfo[x].port));
  }

  groupinfo= memcached_rgroup_info_create(mc, serverinfo, servercount,
                                          &groupcount, &validcount);
  if (not groupinfo) {
    return memcached_set_error(*mc, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                               memcached_literal_param("Failed to create rgroup_info"));
  }

  if (memcached_rgroup_expand(mc, groupcount, servercount) != 0) {
    return memcached_set_error(*mc, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                               memcached_literal_param("Failed to expand rgroup space"));
  }

  for (x= 0; x< memcached_server_count(mc); x++) {
    /* find replica group */
    for (y= 0; y< groupcount; y++) {
      if (strcmp(mc->rgroups[x].groupname, groupinfo[y].groupname) == 0)
        break;
    }
    if (y < groupcount) { /* Found */
      if (groupinfo[y].valid) {
        if (memcached_rgroup_update_with_groupinfo(&mc->rgroups[x], &groupinfo[y]) == true) {
            *serverlist_changed= true;
        }
        groupinfo[y].valid= false;
        validcount--;
      }
    } else { /* Not found */
      mc->rgroups[x].options.is_dead= true;
      prune_flag= true;
    }
  }

  if (prune_flag) { /* prune old dead replica groups */
    memcached_rgroup_prune(mc, false); /* prune dead rgroups only */
  }
  if (validcount > 0) { /* push new valid replica groups */
    memcached_rgroup_push_with_groupinfo(mc, groupinfo, groupcount);
  }
  memcached_rgroup_info_destroy(mc, groupinfo);

  if (prune_flag or validcount > 0) {
    *serverlist_changed= true;
    return run_distribution(mc);
  } else {
    return MEMCACHED_SUCCESS;
  }
}
#endif

static inline memcached_return_t
__do_arcus_update_cachelist(memcached_st *mc,
                            struct memcached_server_info *serverinfo,
                            uint32_t servercount, bool *serverlist_changed)
{
  memcached_return_t error= MEMCACHED_SUCCESS;
  uint32_t x, y;
  memcached_server_st *servers= NULL;
  memcached_server_st *new_hosts;
  bool prune_flag= false;

  if (servercount == 0)
  {
    /* If there's no available servers, delete all managed servers. */
    if (memcached_server_count(mc) == 0) {
      return MEMCACHED_SUCCESS;
    }
    memcached_server_prune(mc, true); /* prune all servers */
    *serverlist_changed = true;
    return run_distribution(mc);
  }

  for (x= 0; x< memcached_server_count(mc); x++)
  {
    for (y= 0; y< servercount; y++) {
      if (serverinfo[y].exist)
        continue;

      if (strcmp(mc->servers[x].hostname, serverinfo[y].hostname) == 0 and
          mc->servers[x].port == serverinfo[y].port) {
         serverinfo[y].exist= true; break;
      }
    }
    if (y == servercount) { /* NOT found */
      mc->servers[x].options.is_dead= true;
      prune_flag= true;
    }
  }
  for (x= 0; x< servercount; x++)
  {
    if (serverinfo[x].exist == false) {
      new_hosts= memcached_server_list_append(servers, serverinfo[x].hostname,
                                              serverinfo[x].port, &error);
      if (new_hosts == NULL)
        break;
      servers= new_hosts;
    }
  }

  if (error == MEMCACHED_SUCCESS) {
    if (servers) {
      if (prune_flag) {
        memcached_server_prune(mc, false); /* prune dead servers only */
      }
      error= memcached_server_push(mc, servers);
    } else {
      if (prune_flag) {
        memcached_server_prune(mc, false); /* prune dead servers only */
        error= run_distribution(mc);
      }
    }
    if (servers or prune_flag) {
      *serverlist_changed = true;
    }
  } else { /* memcached_server_list_append FAIL */
    if (prune_flag) {
      for (x= 0; x< memcached_server_count(mc); x++) {
        if (mc->servers[x].options.is_dead == true)
          mc->servers[x].options.is_dead= false;
      }
    }
  }
  if (servers) {
    memcached_server_list_free(servers);
  }
  return error;
}

static inline void do_arcus_update_cachelist(memcached_st *mc,
                                             struct memcached_server_info *serverinfo,
                                             uint32_t servercount)
{
  arcus_st *arcus= static_cast<arcus_st *>(memcached_get_server_manager(mc));
  memcached_return_t error= MEMCACHED_SUCCESS;
  struct timeval tv_begin, tv_end;
  int msec;

  ZOO_LOG_DEBUG(("CACHE_LIST=UPDATING, from %s, cache_servers=%d",
                 arcus->zk.ensemble_list, memcached_server_count(mc)));
  gettimeofday(&tv_begin, 0);

  /* Update the server list. */
  bool serverlist_changed= false;
#ifdef ENABLE_REPLICATION
  if (mc->flags.repl_enabled)
    error= __do_arcus_update_grouplist(mc, serverinfo, servercount, &serverlist_changed);
  else
#endif
  error= __do_arcus_update_cachelist(mc, serverinfo, servercount, &serverlist_changed);

  unlikely (arcus->zk.is_initializing)
  {
    arcus->zk.is_initializing= false;
    pthread_cond_broadcast(&cond_arcus);
  }

  /* If enabled memcached pooling, repopulate the pool. */
  if (arcus->pool && mc == memcached_pool_get_master(arcus->pool) && serverlist_changed) {
    memcached_return_t rc= memcached_pool_repopulate(arcus->pool);
    if (rc == MEMCACHED_SUCCESS) {
      ZOO_LOG_WARN(("MEMACHED_POOL=REPOPULATED"));
    } else {
      ZOO_LOG_WARN(("failed to repopulate the pool!"));
    }
  }

  gettimeofday(&tv_end, 0);
  msec= ((tv_end.tv_sec - tv_begin.tv_sec) * 1000)
      + ((tv_end.tv_usec - tv_begin.tv_usec) / 1000);

  if (error == MEMCACHED_SUCCESS) {
    ZOO_LOG_WARN(("CACHE_LIST=UPDATED, to %s, cache_servers=%d in %d ms",
                  arcus->zk.ensemble_list, mc->number_of_hosts, msec));
  } else {
    ZOO_LOG_WARN(("CACHE_LIST=UPDATE_FAIL in %d ms", msec));
  }
}

/**
 * Rebuild the memcached server list by string
 */
static inline void do_arcus_zk_update_cachelist_by_string(memcached_st *mc,
                                                          char *serverlist,
                                                          const size_t size)
{
  arcus_st *arcus = static_cast<arcus_st *>(memcached_get_server_manager(mc));
  uint32_t servercount= 0;
  struct memcached_server_info *serverinfo;
  char buffer[ARCUS_MAX_PROXY_FILE_LENGTH];

  serverinfo = static_cast<memcached_server_info *>(libmemcached_malloc(mc, sizeof(memcached_server_info)*(size+1)));
  if (not serverinfo) {
    return;
  }

  strncpy(buffer, serverlist, ARCUS_MAX_PROXY_FILE_LENGTH);

  /* Parse the cache server list strings. */
  if (size > 0) {
    char *buf= NULL;
    char *token= NULL;

    /* Comma-seperated server list */
    for (token= strtok_r(buffer, ",", &buf);
         token;
         token= strtok_r(NULL,   ",", &buf))
    {
      if (do_add_server_to_cachelist(&arcus->zk, token,
                                     &serverinfo[servercount]) == 0) {
        servercount++; /* valid znode name */
      }
    }
  }

  pthread_mutex_lock(&lock_arcus);
  do_arcus_update_cachelist(mc, serverinfo, servercount);
  pthread_mutex_unlock(&lock_arcus);

  libmemcached_free(mc, serverinfo);
}

#ifdef UPDATE_HASH_RING_OF_FETCHED_MC
static inline void do_update_serverlist_with_master(memcached_st *ptr, memcached_st *master)
{
  /* pool locked */
  uint32_t servercount= 0;
  struct memcached_server_info *serverinfo;
#ifdef ENABLE_REPLICATION
  if (ptr->flags.repl_enabled)
  {
    serverinfo= static_cast<memcached_server_info *>(libmemcached_malloc(ptr, sizeof(memcached_server_info)*(memcached_server_count(master)*2)));
    if (not serverinfo) {
      return;
    }

    memcached_rgroup_st *grouplist = master->rgroups;
    for (uint32_t i= 0; i< memcached_server_count(master); i++) {
      for (uint32_t j= 0; j< grouplist[i].nreplica; j++) {
        serverinfo[servercount].groupname = grouplist[i].groupname;
        serverinfo[servercount].hostname  = grouplist[i].replicas[j]->hostname;
        serverinfo[servercount].port      = grouplist[i].replicas[j]->port;
        serverinfo[servercount].master    = (j == 0 ? true : false);
        serverinfo[servercount].exist     = false;
        servercount++;
      }
    }
  }
  else
#endif
  {
    serverinfo= static_cast<memcached_server_info *>(libmemcached_malloc(ptr, sizeof(memcached_server_info)*(memcached_server_count(master))));
    if (not serverinfo) {
      return;
    }

    memcached_server_list_st list = master->servers;
    for (uint32_t i= 0; i< memcached_server_count(master); i++) {
#ifdef ENABLE_REPLICATION
      serverinfo[servercount].groupname = NULL;
      serverinfo[servercount].master    = false;
#endif
      serverinfo[servercount].hostname  = list[i].hostname;
      serverinfo[servercount].port      = list[i].port;
      serverinfo[servercount].exist     = false;
      servercount++; /* valid znode name */
    }
  }

  /* Update the server list. */
  memcached_return_t error= MEMCACHED_SUCCESS;
  bool serverlist_changed;
#ifdef ENABLE_REPLICATION
  if (ptr->flags.repl_enabled)
    error= __do_arcus_update_grouplist(ptr, serverinfo, servercount, &serverlist_changed);
  else
#endif
  error= __do_arcus_update_cachelist(ptr, serverinfo, servercount, &serverlist_changed);

  /* TODO: need error handling */
  if (error != MEMCACHED_SUCCESS)
  {
  }
  libmemcached_free(ptr, serverinfo);

  /* update hash ring */
  memcached_ketama_release(ptr);
  memcached_ketama_reference(ptr, master);

  /* update hash ring version */
  ptr->configure.ketama_version = master->configure.ketama_version;
}
#endif

static inline int do_arcus_zk_process_reconnect(memcached_st *mc, bool *retry)
{
  int ret= 0;
  arcus_return_t rc;
  arcus_st *arcus= static_cast<arcus_st *>(memcached_get_server_manager(mc));

  ZOO_LOG_WARN(("process reconnect to ZooKeeper"));
  do {
    rc= do_arcus_zk_close(mc);
    if (rc != ARCUS_SUCCESS)
      break;

    rc= do_arcus_zk_connect(mc);
    if (rc != ARCUS_SUCCESS)
      break;
  } while(0);

  if (rc == ARCUS_SUCCESS) {
    ZOO_LOG_WARN(("Success reconnect to ZooKeeper"));

    pthread_mutex_lock(&arcus->zk_mgr.lock);
    arcus->zk_mgr.request.update_cache_list= true;
    pthread_mutex_unlock(&arcus->zk_mgr.lock);
    *retry= true;
    ret= 0;
  } else {
    ZOO_LOG_WARN(("Failed reconnect to ZooKeeper, retry after 1sec."));
    ret= -1;
  }
  return ret;
}

/**
 * Rebuild the memcached server list.
 * This is a callback function for the zoo_aget_children(),
 * called on creation of the client or when ZOO_CHILD_EVENT is occurred.
 */
static inline void do_arcus_zk_update_cachelist(memcached_st *mc,
                                                const struct String_vector *strings)
{
  arcus_st *arcus;

  pthread_mutex_lock(&lock_arcus);
  do {
    arcus= static_cast<arcus_st *>(memcached_get_server_manager(mc));
    if (not arcus) {
      ZOO_LOG_ERROR(("arcus is null"));
      break;
    }
    if (arcus->is_proxy) {
      do_arcus_proxy_update_cachelist(mc, strings);
    }
    else {
      uint32_t servercount= 0;
      struct memcached_server_info *serverinfo;
      serverinfo= static_cast<memcached_server_info *>(libmemcached_malloc(mc, sizeof(memcached_server_info)*(strings->count+1)));
      if (not serverinfo) {
        break;
      }
      for (int i= 0; i< strings->count; i++) {
        if (do_add_server_to_cachelist(&arcus->zk, strings->data[i],
                                       &serverinfo[servercount]) == 0) {
          servercount++; /* valid znode name */
        }
      }
      do_arcus_update_cachelist(mc, serverinfo, servercount);
      libmemcached_free(mc, serverinfo);
    }
  } while(0);
  pthread_mutex_unlock(&lock_arcus);
}

static inline void do_arcus_zk_watch_and_update_cachelist(memcached_st *mc,
                                                          bool *retry)
{
  arcus_st *arcus;
  struct String_vector strings = { 0, NULL };
  int zkrc;

  arcus= static_cast<arcus_st *>(memcached_get_server_manager(mc));
  if (not arcus) {
    ZOO_LOG_ERROR(("Arcus is null"));
    return;
  }

  /* Make a new watch on Arcus cache list. */
  zkrc= zoo_wget_children(arcus->zk.handle, arcus->zk.path,
                          do_arcus_zk_watcher_cachelist,
                          (void *)mc, &strings);
  if (zkrc == ZOK) {
    /* TODO (next commit)
     * Error handling of update_chacelist process,
     * must distinguish retrys by failure and by event.
     * If it is retrys by failure, watcher should not register.
     */
    /* Update the cache server list. */
    do_arcus_zk_update_cachelist(mc, &strings);
    deallocate_String_vector(&strings);
  } else {
    ZOO_LOG_ERROR(("zoo_wget_children() failed, reason=%s, zookeeper=%s",
                  zerror(zkrc), arcus->zk.ensemble_list));
    pthread_mutex_lock(&arcus->zk_mgr.lock);
    arcus->zk_mgr.request.update_cache_list= true;
    pthread_mutex_unlock(&arcus->zk_mgr.lock);
    *retry= true;
  }
}

/**
 * ZooKeeper watcher for Arcus cache list.
 */
static inline void do_arcus_zk_watcher_cachelist(zhandle_t *zh __attribute__((unused)),
                                                 int type,
                                                 int state __attribute__((unused)),
                                                 const char *path __attribute__((unused)),
                                                 void *ctx_mc)
{
  if (type == ZOO_CHILD_EVENT) {
    ZOO_LOG_INFO(("ZOO_CHILD_EVENT from ZK cache list"));

    memcached_st *mc= static_cast<memcached_st *>(ctx_mc);
    pthread_mutex_lock(&lock_arcus);
    arcus_st *arcus= static_cast<arcus_st *>(memcached_get_server_manager(mc));
    pthread_mutex_unlock(&lock_arcus);
    if (not arcus || not arcus->zk.handle) {
      ZOO_LOG_ERROR(("arcus is null"));
      return;
    }

    pthread_mutex_lock(&arcus->zk_mgr.lock);
    arcus->zk_mgr.request.update_cache_list= true;
    do_arcus_zk_manager_wakeup(mc, true);
    pthread_mutex_unlock(&arcus->zk_mgr.lock);
  } else {
    ZOO_LOG_WARN(("Unexpected event gotten by watcher_cachelist"));
  }
}

/**
 * ZooKeeper global watcher
 */
static inline void do_arcus_zk_watcher_global(zhandle_t *zh,
                                              int type,
                                              int state,
                                              const char *path __attribute__((unused)),
                                              void *ctx_mc)
{
  memcached_st *mc= static_cast<memcached_st *>(ctx_mc);
  arcus_st *arcus;

  if (type != ZOO_SESSION_EVENT) {
    return;
  }

  pthread_mutex_lock(&lock_arcus);
  arcus= static_cast<arcus_st *>(memcached_get_server_manager(mc));
  if (not arcus || not arcus->zk.handle) {
    ZOO_LOG_ERROR(("arcus is null"));
    pthread_mutex_unlock(&lock_arcus);
    return;
  }

  if (state == ZOO_CONNECTED_STATE)
  {
    ZOO_LOG_WARN(("SESSION_STATE=CONNECTED, to %s", arcus->zk.ensemble_list));

    const clientid_t *id= zoo_client_id(zh);
    if (arcus->zk.myid.client_id == 0 or arcus->zk.myid.client_id != id->client_id) {
      ZOO_LOG_DEBUG(("Previous sessionid : 0x%llx", (long long) arcus->zk.myid.client_id));
      arcus->zk.myid= *id;
      ZOO_LOG_DEBUG(("Current sessionid  : 0x%llx", (long long) arcus->zk.myid.client_id));
    }
    pthread_mutex_unlock(&lock_arcus);

    inc_count(-1);
  }
  else if (state == ZOO_CONNECTING_STATE or state == ZOO_ASSOCIATING_STATE)
  {
    ZOO_LOG_WARN(("SESSION_STATE=CONNECTING, to %s", arcus->zk.ensemble_list));
    pthread_mutex_unlock(&lock_arcus);
  }
  else if (state == ZOO_AUTH_FAILED_STATE)
  {
    arcus->zk.conn_result= ARCUS_AUTH_FAILED;
    pthread_mutex_unlock(&lock_arcus);
    if (arcus->zk_mgr.running) {
      ZOO_LOG_WARN(("SESSION_STATE=AUTH_FAILED, create a new client after closing failed one"));
      pthread_mutex_lock(&arcus->zk_mgr.lock);
      arcus->zk_mgr.request.reconnect_process= true;
      do_arcus_zk_manager_wakeup(mc, true);
      pthread_mutex_unlock(&arcus->zk_mgr.lock);
    } else {
      ZOO_LOG_WARN(("SESSION_STATE=AUTH_FAILED, close the failed client on the first connection"));
    }
    inc_count(-1);
  }
  else if (state == ZOO_EXPIRED_SESSION_STATE)
  {
    ZOO_LOG_WARN(("SESSION_STATE=EXPIRED_SESSION, create a new client after closing expired one"));
    pthread_mutex_unlock(&lock_arcus);
    if (arcus->zk_mgr.running) {
      pthread_mutex_lock(&arcus->zk_mgr.lock);
      arcus->zk_mgr.request.reconnect_process= true;
      do_arcus_zk_manager_wakeup(mc, true);
      pthread_mutex_unlock(&arcus->zk_mgr.lock);
    }
  }
}

/**
 * ZooKeeper manager
 */
static inline arcus_return_t do_arcus_zk_manager_start(memcached_st *mc)
{
  arcus_st *arcus;
  arcus_return_t rc= ARCUS_SUCCESS;

  /* Start arcus zk manager thread */
  pthread_mutex_lock(&lock_arcus);
  do {
    arcus= static_cast<arcus_st *>(memcached_get_server_manager(mc));
    if (not arcus || not arcus->zk.handle) {
      rc= ARCUS_ERROR; break;
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    /* zk manager thread */
    if (0 != pthread_create(&arcus->zk_mgr.tid, &attr, arcus_zk_manager_thread_main, (void *)mc)) {
      ZOO_LOG_ERROR(("Cannot create zk manager thread: %s(%d)", strerror(errno), errno));
      zookeeper_close(arcus->zk.handle);
      arcus->zk.handle= NULL;
      rc= ARCUS_ERROR; break;
    }

    pthread_mutex_lock(&arcus->zk_mgr.lock);
    arcus->zk_mgr.request.update_cache_list= true;
    do_arcus_zk_manager_wakeup(mc, true);
    pthread_mutex_unlock(&arcus->zk_mgr.lock);
  } while(0);
  pthread_mutex_unlock(&lock_arcus);

  if (rc != ARCUS_SUCCESS) {
      return rc;
  }

  ZOO_LOG_WARN(("Waiting for the cache server list..."));

  pthread_mutex_lock(&lock_arcus);
  arcus= static_cast<arcus_st *>(memcached_get_server_manager(mc));
  if (arcus && arcus->zk.is_initializing) {
    struct timeval now;
    struct timespec ts;

    /* Wait for the cache list (timed out after 5 sec.) */
    gettimeofday(&now, NULL);
    ts.tv_sec= now.tv_sec + (ARCUS_ZK_SESSION_TIMEOUT_IN_MS / 1000 / 3);
    ts.tv_nsec= now.tv_usec * 1000;

    if (pthread_cond_timedwait(&cond_arcus, &lock_arcus, &ts)) {
      ZOO_LOG_ERROR(("pthread_cond_timedwait failed. %s(%d)", strerror(errno), errno));
      rc= ARCUS_ERROR;
    }
  }
  pthread_mutex_unlock(&lock_arcus);

  if (rc == ARCUS_SUCCESS) {
    ZOO_LOG_WARN(("Done"));
  }
  return rc;
}

static inline void do_arcus_zk_manager_stop(memcached_st *mc)
{
  arcus_st *arcus;

  pthread_mutex_lock(&lock_arcus);
  arcus= static_cast<arcus_st *>(memcached_get_server_manager(mc));
  pthread_mutex_unlock(&lock_arcus);
  if (arcus && arcus->zk_mgr.running) {
    ZOO_LOG_WARN(("Wait for the arcus zookeeper manager to stop"));
    arcus->zk_mgr.reqstop= true;
    do_arcus_zk_manager_wakeup(mc, false);

    /* Do not call zookeeper_close (do_arcus_zk_close) and zoo_wget_children at
     * the same time.  Otherwise, this thread (main thread) and the watcher
     * thread (zoo_wget_children) may hang forever until the user manually
     * kills the process.  The root cause is within the zookeeper library.
     * Its handling of concurrent API calls like zoo_wget_children and
     * zookeeper_close still has holes...
     */
    while (arcus->zk_mgr.running) {
      usleep(10000); // 10ms wait
    }
  }
}

static void do_arcus_zk_manager_wakeup(memcached_st *mc, bool locked)
{
  arcus_st *arcus= static_cast<arcus_st *>(memcached_get_server_manager(mc));
  if (not arcus) {
    return;
  }

  if (!locked)
    pthread_mutex_lock(&arcus->zk_mgr.lock);
  if (!arcus->zk_mgr.notification) {
    arcus->zk_mgr.notification= true;
    pthread_cond_signal(&arcus->zk_mgr.cond);
  }
  if (!locked)
    pthread_mutex_unlock(&arcus->zk_mgr.lock);
}

static void *arcus_zk_manager_thread_main(void *arg)
{
  memcached_st *manager_mc= (memcached_st*)arg;
  arcus_st *arcus= static_cast<arcus_st *>(memcached_get_server_manager(manager_mc));
  struct arcus_zk_request_st zkreq;
  struct timeval  tv;
  struct timespec ts;
  size_t elapsed_time = 0; /* unit : millisecond */
  bool zk_retry= false;
  bool reconnect_retry= false;

  ZOO_LOG_WARN(("Running arcus zookeeper manager thread"));
  arcus->zk_mgr.running= true;
  while (!arcus->zk_mgr.reqstop)
  {
    pthread_mutex_lock(&arcus->zk_mgr.lock);
    while (!arcus->zk_mgr.notification && !arcus->zk_mgr.reqstop) {
      /* Poll if requested.
       * Otherwise, wait till the ZK watcher wakes us up.
       */
      if (zk_retry || reconnect_retry) {
        gettimeofday(&tv, NULL);
        tv.tv_usec += 100000; /* Wake up in 100 ms. Magic number. */
        if (tv.tv_usec >= 1000000) {
          tv.tv_sec += 1;
          tv.tv_usec -= 1000000;
        }
        ts.tv_sec= tv.tv_sec;
        ts.tv_nsec= tv.tv_usec * 1000;
        pthread_cond_timedwait(&arcus->zk_mgr.cond, &arcus->zk_mgr.lock, &ts);
        if (zk_retry) zk_retry= false;
        if (reconnect_retry) elapsed_time += 100; /* 100 ms */
        break;
      }
      pthread_cond_wait(&arcus->zk_mgr.cond, &arcus->zk_mgr.lock);
    }
    arcus->zk_mgr.notification= false;
    zkreq = arcus->zk_mgr.request;
    memset(&arcus->zk_mgr.request, 0, sizeof(struct arcus_zk_request_st));
    pthread_mutex_unlock(&arcus->zk_mgr.lock);

    if (arcus->zk_mgr.reqstop)
      break;

    /* Process request by order */
    /* 1. reconnect process.
     * 2. update cache list
     */
    if (zkreq.reconnect_process || reconnect_retry) {
      if (reconnect_retry) {
        if (elapsed_time < ARCUS_ZK_RETRY_RECONNECT_DELAY_IN_MS) {
          continue;
        }
        reconnect_retry= false;
        elapsed_time= 0;
      }
      if (do_arcus_zk_process_reconnect(manager_mc, &zk_retry) < 0) {
        reconnect_retry= true;
      }
    } else {
      if (zkreq.update_cache_list) {
        do_arcus_zk_watch_and_update_cachelist(manager_mc, &zk_retry);
      }
    }
  }
  ZOO_LOG_WARN(("Stop arcus zookeeper manager thread"));
  arcus->zk_mgr.running= false;

  return NULL;
}

/* flock is modified from Apache Portable Runtime */
/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
static int proc_mutex_create(struct arcus_proc_mutex *m, const char *fname)
{
  int fd;

  m->fname = strdup(fname);
  if (m->fname == NULL) {
    ZOO_LOG_ERROR(("Failed to allocate memory to store the lock file name"));
    errno = ENOMEM;
    return -1;
  }
  fd = open(fname, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR);
  if (fd == -1)
    return -1;
  m->locked = 0;
  m->fd = fd;
  return 0;
}

static int proc_mutex_lock(struct arcus_proc_mutex *m)
{
  int rc;

  if (m == NULL || m->fd < 0) {
    ZOO_LOG_ERROR(("proc_mutex_lock: invalid argument. m=%p", m));
    errno = EINVAL;
    return -1;
  }
  do {
    rc = flock(m->fd, LOCK_EX);
  } while (rc < 0 && errno == EINTR);
  if (rc < 0) {
    return -1;
  }
  m->locked = 1;
  return 0;
}

static int proc_mutex_unlock(struct arcus_proc_mutex *m)
{
  int rc;

  if (m == NULL || m->fd < 0) {
    ZOO_LOG_ERROR(("proc_mutex_lock: invalid argument. m=%p", m));
    errno = EINVAL;
    return -1;
  }
  m->locked = 0;
  do {
    rc = flock(m->fd, LOCK_UN);
  } while (rc < 0 && errno == EINTR);
  if (rc < 0) {
    return -1;
  }
  return 0;
}

static int proc_mutex_destroy(struct arcus_proc_mutex *m)
{
  if (m->locked) {
    if (0 != proc_mutex_unlock(m)) {
      ZOO_LOG_ERROR(("Failed to unlock the mutex. error=%s(%d)",
          strerror(errno), errno));
      return -1;
    }
  }
  if (m->fd >= 0) {
    close(m->fd);
    m->fd = -1;
  }
  unlink(m->fname);
  free(m->fname);
  m->fname = NULL;
  return 0;
}
