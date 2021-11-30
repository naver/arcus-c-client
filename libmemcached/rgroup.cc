/*
 * arcus-c-client : Arcus C client
 * Copyright 2017 JaM2in Co., Ltd.
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
 *  Copyright (C) 2006-2010 Brian Aker All rights reserved.
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

#ifdef ENABLE_REPLICATION
#include "libmemcached/arcus_priv.h"

/*
 * rgroup info functions
 */

struct memcached_rgroup_info *
memcached_rgroup_info_create(memcached_st *memc,
                             struct memcached_server_info *serverinfo,
                             uint32_t servercount,
                             uint32_t *groupcount,
                             uint32_t *validcount)
{
  struct memcached_rgroup_info *ginfo;
  struct memcached_rgroup_info *groupinfo;
  uint32_t i, j, k;
  uint32_t total_ngroup= 0;
  uint32_t valid_ngroup= 0;

  groupinfo= (struct memcached_rgroup_info*)libmemcached_malloc(memc,
                     (sizeof(struct memcached_rgroup_info) * servercount));
  if (not groupinfo) {
    return NULL;
  }
  memset(groupinfo, 0, (sizeof(struct memcached_rgroup_info) * servercount));

  for (i = 0; i < servercount; i++)
  {
    /* find the replica group */
    for (j = 0; j < total_ngroup; j++) {
      if (strcmp(serverinfo[i].groupname, groupinfo[j].groupname) == 0)
        break;
    }
    if (j >= total_ngroup) { /* NOT found */
      /* make a new replica group info */
      ginfo = &groupinfo[total_ngroup];
      ginfo->groupname = serverinfo[i].groupname;
      ginfo->replicas[serverinfo[i].master ? 0 : 1] = &serverinfo[i];
      ginfo->nreplica = 1;
      ginfo->valid= true;
      total_ngroup++;
      valid_ngroup++;
      continue;
    }
    /* Found */
    /* check validity and add server info */
    ginfo= &groupinfo[j];
    do {
      if (ginfo->valid != true) { /* invalid replica group */
        break;
      }
      if (serverinfo[i].master) { /* master */
        if (ginfo->replicas[0] != NULL) { /* dual master */
          ginfo->valid= false;
          valid_ngroup--; break;
        }
        for (k = 1; k <= ginfo->nreplica; k++) {
          if (strcmp(serverinfo[i].hostname, ginfo->replicas[k]->hostname) == 0
              and    serverinfo[i].port ==   ginfo->replicas[k]->port)
            break; /* the same <hostname, port> */
        }
        if (k <= ginfo->nreplica) { /* duplicate server */
          ginfo->valid= false;
          valid_ngroup--; break;
        }
        ginfo->replicas[0] = &serverinfo[i];
        ginfo->nreplica += 1;
      } else { /* slave */
        k = ginfo->nreplica + (ginfo->replicas[0] ? 0 : 1);
        if (k >= RGROUP_MAX_REPLICA) { /* too many replicas */
          ginfo->valid= false;
          valid_ngroup--; break;
        }
        ginfo->replicas[k] = &serverinfo[i];
        ginfo->nreplica += 1;
      }
    } while(0);
  }

  /* Check if the master exists in each replica group.
   * If the master does not exist, the replica group is invalid.
   */
  for (j = 0; j < total_ngroup; j++) {
    if (groupinfo[j].replicas[0] == NULL) { /* no master */
      groupinfo[j].valid= false;
      valid_ngroup--;
    }
  }

  *groupcount= total_ngroup;
  *validcount= valid_ngroup;
  return groupinfo;
}

void
memcached_rgroup_info_destroy(memcached_st *memc,
                              struct memcached_rgroup_info *groupinfo)
{
  libmemcached_free(memc, groupinfo);
}

/*
 * memcached rgroup server management
 */

static memcached_server_st *
do_rgroup_server_alloc(memcached_st *memc)
{
  memcached_server_st *server= NULL;

  if (memc->servers) {
    server= memc->servers;
    memc->servers= server->next;
    memc->server_navail--;
  }
  return server;
}

static void
do_rgroup_server_free(memcached_st *memc, memcached_server_st *server)
{
  assert(server != NULL);
  server->next= memc->servers;
  memc->servers= server;
  memc->server_navail++;
}

static void
do_rgroup_server_insert(memcached_rgroup_st *rgroup, int sindex,
                        const char *hostname, in_port_t port)
{
  assert(rgroup->nreplica < RGROUP_MAX_REPLICA);
  assert(sindex >= -1 && sindex <= (int)rgroup->nreplica);
  memcached_server_st *server;

  /* create a new memcached server */
  memcached_string_t _hostname= { memcached_string_make_from_cstr(hostname) };
  server= do_rgroup_server_alloc(rgroup->root);
  assert(server != NULL);
  server= __server_create_with(rgroup->root, server, _hostname, port, 0,
                               MEMCACHED_CONNECTION_TCP);
  /* set group index */
  server->groupindex= (int32_t)rgroup->groupindex;

  /* attach the new memcached server */
  //pthread_mutex_lock(&rgroup_lock);
  if (sindex == -1 || sindex == (int)rgroup->nreplica) {
    /* append */
    rgroup->replicas[rgroup->nreplica]= server;
  } else { /* sindex >= 0 && sindex < (int)rgroup->nreplica */
    /* insert */
    for (int i= (int)(rgroup->nreplica-1); i >= sindex; i--) {
      rgroup->replicas[i+1]= rgroup->replicas[i];
    }
    rgroup->replicas[sindex]= server;
  }
  rgroup->nreplica += 1;
  //pthread_mutex_unlock(&rgroup_lock);
}

static void
do_rgroup_server_remove(memcached_rgroup_st *rgroup, int sindex)
{
  assert(sindex >= 0 && sindex < (int)rgroup->nreplica);
  memcached_server_st *server;

  /* detach the memcached server */
  //pthread_mutex_lock(&rgroup_lock);
  server = rgroup->replicas[sindex];
  if (sindex == (int)(rgroup->nreplica-1)) {
    rgroup->replicas[sindex]= NULL;
  } else { /* sindex < (int)(rgroup->nreplica-1) */
    for (int i= sindex+1; i < (int)rgroup->nreplica; i++) {
      rgroup->replicas[i-1]= rgroup->replicas[i];
    }
    rgroup->replicas[rgroup->nreplica-1]= NULL;
  }
  rgroup->nreplica -= 1;
  //pthread_mutex_unlock(&rgroup_lock);

  /* free the memcached server */
  __server_free(server);
  do_rgroup_server_free(rgroup->root, server);
}

static void
do_rgroup_server_replace(memcached_rgroup_st *rgroup, int sindex,
                         const char *hostname, in_port_t port)
{
  assert(sindex >= 0 && sindex < (int)rgroup->nreplica);
  memcached_server_st *new_server;
  memcached_server_st *old_server;

  /* create a new memcached server */
  memcached_string_t _hostname= { memcached_string_make_from_cstr(hostname) };
  new_server= do_rgroup_server_alloc(rgroup->root);
  assert(new_server != NULL);
  new_server= __server_create_with(rgroup->root, new_server, _hostname, port, 0,
                                   MEMCACHED_CONNECTION_TCP);
  /* set group index */
  new_server->groupindex= (int32_t)rgroup->groupindex;

  /* replace the memcached server */
  //pthread_mutex_lock(&rgroup_lock);
  old_server = rgroup->replicas[sindex];
  rgroup->replicas[sindex] = new_server;
  //pthread_mutex_unlock(&rgroup_lock);

  /* free the old memcached server */
  __server_free(old_server);
  do_rgroup_server_free(rgroup->root, old_server);
}

static void
do_rgroup_server_switchover(memcached_rgroup_st *rgroup, int sindex)
{
  /* sindex(slave replica index) must satisfy the below condition. */
  assert(sindex > 0 && sindex < (int)rgroup->nreplica);
  memcached_server_st *server;

  //pthread_mutex_lock(&rgroup_lock);
  server = rgroup->replicas[0];
  rgroup->replicas[0] = rgroup->replicas[sindex];
  rgroup->replicas[sindex] = server;
  //pthread_mutex_unlock(&rgroup_lock);
}

/*
 * memcached rgroup sructure management
 */

static memcached_rgroup_st *
do_rgroup_create(memcached_rgroup_st *self, const memcached_st *memc)
{
  if (not self) {
    self= (memcached_rgroup_st*)libmemcached_malloc(memc, sizeof(memcached_rgroup_st));
    if (not self) {
      return NULL; /*  MEMCACHED_MEMORY_ALLOCATION_FAILURE */
    }
    self->options.is_allocated= true;
  } else {
    self->options.is_allocated= false;
  }
  self->options.is_initialized= true;
  return self;
}

static void
do_rgroup_init(memcached_rgroup_st *self, memcached_st *root,
              const memcached_string_t& groupname,
              uint32_t groupindex, uint32_t weight)
{
  //self->options.is_initialized
  //self->options.is_allocated
  self->options.is_shutting_down= false;
  self->options.is_dead= false;

  self->root= root;
  assert(groupname.size < RGROUP_NAME_LENGTH);
  memcpy(self->groupname, groupname.c_str, groupname.size);
  self->groupname[groupname.size]= 0;
  self->groupindex= groupindex;
  self->weight= weight ? weight : 1; // 1 is the default weight value

  self->nreplica= 0;
  for (int i= 0; i < RGROUP_MAX_REPLICA; i++)
    self->replicas[i]= NULL;
}

static void
do_rgroup_index_set(memcached_rgroup_st *self, uint32_t groupindex)
{
  if (self->groupindex != groupindex)
  {
    self->groupindex= groupindex;
    for (uint32_t x= 0; x < self->nreplica; x++)
      self->replicas[x]->groupindex= groupindex;
  }
}

static void
do_rgroup_destroy(memcached_rgroup_st *self, memcached_st *memc)
{
  while (self->nreplica > 0) {
    do_rgroup_server_remove(self, self->nreplica-1);
  }
  if (memcached_is_allocated(self)) {
    libmemcached_free(memc, self);
  } else {
    self->options.is_initialized= false;
  }
}

static int
do_rgroup_compare(const void *p1, const void *p2)
{
  memcached_rgroup_st *a= (memcached_rgroup_st *)p1;
  memcached_rgroup_st *b= (memcached_rgroup_st *)p2;

  return strcmp(a->groupname, b->groupname);
}

/*
 * rgroup struct functions
 */

int
memcached_rgroup_expand(memcached_st *memc, uint32_t rgroupcount,
                                            uint32_t servercount)
{
  memcached_rgroup_st *new_rgroup;
  memcached_server_st *new_server;

  /* expand rgroup space */
  if (rgroupcount > memc->rgroup_ntotal) {
    uint32_t new_count= memc->rgroup_ntotal + 10;
    while (rgroupcount > new_count) {
      new_count += 10;
    }
    new_rgroup= static_cast<memcached_rgroup_st*>(libmemcached_realloc(memc, memc->rgroups,
                                                  (sizeof(memcached_rgroup_st)*new_count)));
    if (not new_rgroup) {
      return -1;
    }
    memc->rgroups= new_rgroup;
    memc->rgroup_ntotal= new_count;
  }

  /* expand server space */
  while ((servercount+1) > memc->server_ntotal) { /* one more surplus */
    /* One more server needed for replacing a server of a rgroup.
     * See memcached_rgroup_server_replace()
     */
    new_server= (memcached_server_st*)libmemcached_malloc(memc, sizeof(memcached_server_st));
    if (not new_server) {
      return -1;
    }
    new_server->next= memc->servers;
    memc->servers= new_server;
    memc->server_ntotal++;
    memc->server_navail++;
  }

  return 0;
}

#define RGROUP_SERVER_IS_SAME(g1, n1, g2, n2) \
  (strcmp((g1)->replicas[n1]->hostname, (g2)->replicas[n2]->hostname) == 0 \
   and (g1)->replicas[n1]->port == (g2)->replicas[n2]->port)

bool
memcached_rgroup_update_with_groupinfo(memcached_rgroup_st *rgroup,
                                       struct memcached_rgroup_info *rginfo)
{
  /* rgroup : old rgroup struct */
  /* rginfo : new rgroup info */

  if (rgroup->nreplica == 1)
  {
    if (rginfo->nreplica == 1) {
      if (RGROUP_SERVER_IS_SAME(rgroup, 0, rginfo, 0)) {
        /* no change: do nothing */
        return false;
      } else {
        /* replace the server */
        do_rgroup_server_replace(rgroup, 0, rginfo->replicas[0]->hostname,
                                            rginfo->replicas[0]->port);
      }
    } else { /* rginfo->nreplica >= 2 */
      if (RGROUP_SERVER_IS_SAME(rgroup, 0, rginfo, 0) != true) {
        /* replace the master */
        do_rgroup_server_replace(rgroup, 0, rginfo->replicas[0]->hostname,
                                            rginfo->replicas[0]->port);
      }
      /* add new slaves */
      for (int i= 1; i < (int)rginfo->nreplica; i++) {
        do_rgroup_server_insert(rgroup, i, rginfo->replicas[i]->hostname,
                                           rginfo->replicas[i]->port);
      }
    }
    return true;
  }
  else /* rgroup->nreplica >= 2 */
  {
    int i, j;
    bool changed = false;

    if (RGROUP_SERVER_IS_SAME(rgroup, 0, rginfo, 0) != true) {
      for (i= 1; i < (int)rgroup->nreplica; i++) {
        if (RGROUP_SERVER_IS_SAME(rgroup, i, rginfo, 0))
          break;
      }
      if (i < (int)rgroup->nreplica) { /* found */
        /* master failover : The old slave become the new master */
        do_rgroup_server_switchover(rgroup, i);
      } else {
        /* replace the master */
        do_rgroup_server_replace(rgroup, 0, rginfo->replicas[0]->hostname,
                                            rginfo->replicas[0]->port);
      }
      changed = true;
    }

    /* handle the slave nodes */
    if (rginfo->nreplica == 1) {
      /* remove old slave nodes */
      while (rgroup->nreplica > 1) {
        do_rgroup_server_remove(rgroup, rgroup->nreplica-1);
      }
      changed = true;
    } else { /* rginfo->nreplica >= 2 */
      /* remove old slaves that are disappeared */
      for (i= 1; i < (int)rgroup->nreplica; i++) {
        for (j= 1; j < (int)rginfo->nreplica; j++) {
          if (RGROUP_SERVER_IS_SAME(rgroup, i, rginfo, j))
            break;
        }
        if (j >= (int)rginfo->nreplica) { /* Not exist */
          do_rgroup_server_remove(rgroup, i);
          i -= 1; /* for adjusting "i" index */
          changed = true;
        }
      }
      /* insert new slaves that are appeared */
      for (i= 1; i < (int)rginfo->nreplica; i++) {
        for (j= 1; j < (int)rgroup->nreplica; j++) {
          if (RGROUP_SERVER_IS_SAME(rgroup, j, rginfo, i))
            break;
        }
        if (j >= (int)rgroup->nreplica) { /* Not exist */
          do_rgroup_server_insert(rgroup, -1, rginfo->replicas[i]->hostname,
                                              rginfo->replicas[i]->port);
          changed = true;
        }
      }
    }
    return changed;
  }
}

void
memcached_rgroup_push_with_groupinfo(memcached_st *memc,
                      struct memcached_rgroup_info *groupinfo,
                      uint32_t groupcount)
{
  memcached_rgroup_st *rgroup;
  uint32_t x, y;

  /* See memcached_rgroup_expand() */
  assert(groupcount <= memc->rgroup_ntotal);
  for (x= 0; x < groupcount; x++) {
    if (groupinfo[x].valid) {
      /* create rgroup */
      rgroup= do_rgroup_create(&memc->rgroups[memc->number_of_hosts], memc);
      assert(rgroup != NULL);
      memcached_string_t _groupname= { memcached_string_make_from_cstr(groupinfo[x].groupname) };
      do_rgroup_init(rgroup, memc, _groupname, memc->number_of_hosts, 0);
      /* add replicas */
      for (y= 0; y < groupinfo[x].nreplica; y++) {
        do_rgroup_server_insert(rgroup, y, groupinfo[x].replicas[y]->hostname,
                                           groupinfo[x].replicas[y]->port);
      }
      memc->number_of_hosts++;
    }
  }
}

memcached_return_t
memcached_rgroup_push(memcached_st *memc,
                      const memcached_rgroup_st *grouplist,
                      uint32_t groupcount)
{
  memcached_rgroup_st *rgroup;
  uint32_t x, y;

  if (not grouplist) {
    return MEMCACHED_SUCCESS;
  }

  if (memcached_rgroup_expand(memc, groupcount, (groupcount*RGROUP_MAX_REPLICA)) != 0) {
    return MEMCACHED_MEMORY_ALLOCATION_FAILURE;
  }

  for (x= 0; x < groupcount; x++) {
    /* create rgroup */
    rgroup= do_rgroup_create(&memc->rgroups[memc->number_of_hosts], memc);
    assert(rgroup != NULL);
    memcached_string_t _groupname= { memcached_string_make_from_cstr(grouplist[x].groupname) };
    do_rgroup_init(rgroup, memc, _groupname, memc->number_of_hosts, 0);

    /* add replicas */
    for (y= 0; y < grouplist[x].nreplica; y++) {
      do_rgroup_server_insert(rgroup, y, grouplist[x].replicas[y]->hostname,
                                         grouplist[x].replicas[y]->port);
    }
    if (grouplist[x].weight > 1) {
      memc->ketama.weighted= true;
    }
    memc->number_of_hosts++;
  }
  return run_distribution(memc);
}

bool
memcached_rgroup_switchover(memcached_st *memc, memcached_server_st *server)
{
  /* find rgroup */
  assert(server->groupindex >= 0);
  memcached_rgroup_st *rgroup= &memc->rgroups[server->groupindex];

  /* do switchover */
  if (server->switchover_sidx == -1) {
    int c= 0;
    char *token;
    char *buffer= NULL;
    char *hostname= NULL;
    in_port_t port= 0;

    for (token= strtok_r(server->switchover_peer, ":", &buffer);
         token;
         token= strtok_r(NULL, ":", &buffer), c++) {
      if (c == 0) { /* HOST */
        hostname= token;
      } else { /* PORT */
        port= atoi(token);
        break;
      }
    }
    if (hostname != NULL && port > 0) {
      for (c= 1; c < (int)rgroup->nreplica; c++) {
        if (strcmp(rgroup->replicas[c]->hostname, hostname) == 0 and
            rgroup->replicas[c]->port == port) {
          server->switchover_sidx= c;
          break;
        }
      }
    }
    if (server->switchover_sidx == -1) {
      /* Something is wrong. do not perform switchover */
      ZOO_LOG_WARN(("Can't find switchover peer(%s:%d)", hostname, port));
      return false;
    }
  }
  do_rgroup_server_switchover(rgroup, server->switchover_sidx);
  return true;
}

void
memcached_rgroup_list_free(memcached_st *memc)
{
  if (memc->rgroups)
  {
    for (uint32_t x= 0; x < memc->number_of_hosts; x++) {
      do_rgroup_destroy(&memc->rgroups[x], memc);
    }
    libmemcached_free(memc, memc->rgroups);
    memc->rgroups= NULL;
    memc->rgroup_ntotal= 0;

    memcached_server_st *server;
    while ((server= do_rgroup_server_alloc(memc)) != NULL) {
      libmemcached_free(memc, server);
    }
    memc->server_ntotal= 0;
    memc->number_of_hosts= 0;
  }
}

void
memcached_rgroup_prune(memcached_st *memc, bool all_flag)
{
  if (all_flag)
  {
    memcached_rgroup_list_free(memc);
  }
  else
  {
    uint32_t x, cursor;

    cursor= 0;
    for (x= 0; x < memcached_server_count(memc); x++) {
      if (memc->rgroups[x].options.is_dead) {
        /* free the dead rgroup */
        do_rgroup_destroy(&memc->rgroups[x], memc);
      } else {
        /* If this rgroup is not dead and there's free space ahead,
         * move it there.
         */
        if (cursor < x) {
          memc->rgroups[cursor]= memc->rgroups[x];
          do_rgroup_index_set(&memc->rgroups[cursor], cursor);
        }
        cursor++;
      }
    }
    if (x > 0) {
      /* Change the number of hosts */
      memc->number_of_hosts= cursor;
    }
  }
}

void
memcached_rgroup_sort(memcached_st *memc)
{
  if (memc->rgroups) {
    /* sort rgoups */
    qsort(memc->rgroups, memc->number_of_hosts,
          sizeof(memcached_rgroup_st), do_rgroup_compare);
    
    /* adjust group index */
    for (uint32_t x= 0; x < memcached_server_count(memc); x++)
      do_rgroup_index_set(&memc->rgroups[x], x);
  }
}

memcached_rgroup_st *
memcached_rgroup_list(const memcached_st *memc)
{
  if (memc) {
    return memc->rgroups;
  }
  return NULL;
}

uint32_t
memcached_rgroup_count(const memcached_st *memc)
{
  if (memc) {
    return memc->number_of_hosts;
  }
  return 0;
}
#endif
