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

#pragma once

#ifdef ENABLE_REPLICATION
#define RGROUP_NAME_LENGTH 128
#define RGROUP_MAX_REPLICA 3

struct memcached_rgroup_info {
  char                *groupname;
  struct memcached_server_info *replicas[RGROUP_MAX_REPLICA];
  uint32_t             nreplica;
  bool                 valid;
};

struct memcached_rgroup_st {
  struct {
    bool is_allocated:1;
    bool is_initialized:1;
    bool is_shutting_down:1;
    bool is_dead:1;
  } options;
  uint32_t             groupindex;
  char                 groupname[RGROUP_NAME_LENGTH];
  uint32_t             weight;
  uint32_t             nreplica;
  memcached_server_st *replicas[RGROUP_MAX_REPLICA];
  memcached_st        *root;
};

#ifdef __cplusplus
extern "C" {
#endif

/*
 * RGroup(Replica Group) Public functions
 */

/* rgroup info functions */
LIBMEMCACHED_API
  struct memcached_rgroup_info *
  memcached_rgroup_info_create(memcached_st *memc,
                               struct memcached_server_info *serverinfo,
                               uint32_t servercount,
                               uint32_t *groupcount,
                               uint32_t *validcount);
LIBMEMCACHED_API
  void
  memcached_rgroup_info_destroy(memcached_st *memc,
                                struct memcached_rgroup_info *groupinfo);

/* rgroup struct functions */
LIBMEMCACHED_API
  int
  memcached_rgroup_expand(memcached_st *memc, uint32_t rgroupcount,
                                              uint32_t servercount);
LIBMEMCACHED_API
  bool
  memcached_rgroup_update_with_groupinfo(memcached_rgroup_st *rgroup,
                        struct memcached_rgroup_info *rginfo);

LIBMEMCACHED_API
  void
  memcached_rgroup_push_with_groupinfo(memcached_st *memc,
                        struct memcached_rgroup_info *groupinfo,
                        uint32_t groupcount);
LIBMEMCACHED_API
  memcached_return_t
  memcached_rgroup_push(memcached_st *memc,
                        const memcached_rgroup_st *grouplist,
                        uint32_t groupcount);
LIBMEMCACHED_API
  void
  memcached_rgroup_switchover(memcached_st *memc, memcached_server_st *server);
LIBMEMCACHED_API
  void
  memcached_rgroup_list_free(memcached_st *memc);
LIBMEMCACHED_API
  void
  memcached_rgroup_prune(memcached_st *memc, bool all_flag);
LIBMEMCACHED_API
  void
  memcached_rgroup_sort(memcached_st *memc);
LIBMEMCACHED_API
  memcached_rgroup_st *
  memcached_rgroup_list(const memcached_st *memc);
LIBMEMCACHED_API
  uint32_t
  memcached_rgroup_count(const memcached_st *memc);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
