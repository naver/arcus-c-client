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
/*
*/

#include "common.h"
#include "libmemcached/arcus_priv.h"

static const char *memcached_stat_keys[] = {
  "pid",
  "uptime",
  "time",
  "version",
  "pointer_size",
  "rusage_user",
  "rusage_system",
  "curr_items",
  "total_items",
  "bytes",
  "curr_connections",
  "total_connections",
  "connection_structures",
  "cmd_get",
  "cmd_set",
  "get_hits",
  "get_misses",
  "evictions",
  "bytes_read",
  "bytes_written",
  "limit_maxbytes",
  "threads",
  NULL
};

struct local_context
{
  memcached_stat_fn func;
  void *context;
  const char *args;

  local_context(memcached_stat_fn func_arg,
                void *context_arg,
                const char *args_arg) :
    func(func_arg),
    context(context_arg),
    args(args_arg)
  { }
};


static memcached_return_t set_data(memcached_stat_st *memc_stat, char *key, char *value)
{

  if (strlen(key) < 1)
  {
    WATCHPOINT_STRING(key);
    return MEMCACHED_UNKNOWN_STAT_KEY;
  }
  else if (not strcmp("pid", key))
  {
    int64_t temp= strtoll(value, (char **)NULL, 10);

    if (temp <= INT32_MAX and ( sizeof(pid_t) == sizeof(int32_t) ))
    {
      memc_stat->pid= temp;
    }
    else if (temp > -1)
    {
      memc_stat->pid= temp;
    }
    else
    {
      // If we got a value less then -1 then something went wrong in the
      // protocol
    }
  }
  else if (not strcmp("uptime", key))
  {
    memc_stat->uptime= strtoul(value, (char **)NULL, 10);
  }
  else if (not strcmp("time", key))
  {
    memc_stat->time= strtoul(value, (char **)NULL, 10);
  }
  else if (not strcmp("version", key))
  {
    memcpy(memc_stat->version, value, strlen(value));
    memc_stat->version[strlen(value)]= 0;
  }
  else if (not strcmp("pointer_size", key))
  {
    memc_stat->pointer_size= strtoul(value, (char **)NULL, 10);
  }
  else if (not strcmp("rusage_user", key))
  {
    char *walk_ptr;
    for (walk_ptr= value; (!ispunct(*walk_ptr)); walk_ptr++) {};
    *walk_ptr= 0;
    walk_ptr++;
    memc_stat->rusage_user_seconds= strtoul(value, (char **)NULL, 10);
    memc_stat->rusage_user_microseconds= strtoul(walk_ptr, (char **)NULL, 10);
  }
  else if (not strcmp("rusage_system", key))
  {
    char *walk_ptr;
    for (walk_ptr= value; (!ispunct(*walk_ptr)); walk_ptr++) {};
    *walk_ptr= 0;
    walk_ptr++;
    memc_stat->rusage_system_seconds= strtoul(value, (char **)NULL, 10);
    memc_stat->rusage_system_microseconds= strtoul(walk_ptr, (char **)NULL, 10);
  }
  else if (not strcmp("curr_items", key))
  {
    memc_stat->curr_items= strtoul(value, (char **)NULL, 10);
  }
  else if (not strcmp("total_items", key))
  {
    memc_stat->total_items= strtoul(value, (char **)NULL, 10);
  }
  else if (not strcmp("bytes_read", key))
  {
    memc_stat->bytes_read= strtoull(value, (char **)NULL, 10);
  }
  else if (not strcmp("bytes_written", key))
  {
    memc_stat->bytes_written= strtoull(value, (char **)NULL, 10);
  }
  else if (not strcmp("bytes", key))
  {
    memc_stat->bytes= strtoull(value, (char **)NULL, 10);
  }
  else if (not strcmp("curr_connections", key))
  {
    memc_stat->curr_connections= strtoull(value, (char **)NULL, 10);
  }
  else if (not strcmp("total_connections", key))
  {
    memc_stat->total_connections= strtoull(value, (char **)NULL, 10);
  }
  else if (not strcmp("connection_structures", key))
  {
    memc_stat->connection_structures= strtoul(value, (char **)NULL, 10);
  }
  else if (not strcmp("cmd_get", key))
  {
    memc_stat->cmd_get= strtoull(value, (char **)NULL, 10);
  }
  else if (not strcmp("cmd_set", key))
  {
    memc_stat->cmd_set= strtoull(value, (char **)NULL, 10);
  }
  else if (not strcmp("get_hits", key))
  {
    memc_stat->get_hits= strtoull(value, (char **)NULL, 10);
  }
  else if (not strcmp("get_misses", key))
  {
    memc_stat->get_misses= strtoull(value, (char **)NULL, 10);
  }
  else if (not strcmp("evictions", key))
  {
    memc_stat->evictions= strtoull(value, (char **)NULL, 10);
  }
  else if (not strcmp("limit_maxbytes", key))
  {
    memc_stat->limit_maxbytes= strtoull(value, (char **)NULL, 10);
  }
  else if (not strcmp("threads", key))
  {
    memc_stat->threads= strtoul(value, (char **)NULL, 10);
  }
  else if (not (strcmp("delete_misses", key) == 0 or /* New stats in the 1.3 beta */
                strcmp("delete_hits", key) == 0 or /* Just swallow them for now.. */
                strcmp("incr_misses", key) == 0 or
                strcmp("incr_hits", key) == 0 or
                strcmp("decr_misses", key) == 0 or
                strcmp("decr_hits", key) == 0 or
                strcmp("cas_misses", key) == 0 or
                strcmp("cas_hits", key) == 0 or
                strcmp("cas_badval", key) == 0 or
                strcmp("cmd_flush", key) == 0 or
                strcmp("accepting_conns", key) == 0 or
                strcmp("listen_disabled_num", key) == 0 or
                strcmp("conn_yields", key) == 0 or
                strcmp("auth_cmds", key) == 0 or
                strcmp("auth_errors", key) == 0 or
                strcmp("reclaimed", key) == 0))
  {
    WATCHPOINT_STRING(key);
    /* return MEMCACHED_UNKNOWN_STAT_KEY; */
    return MEMCACHED_SUCCESS;
  }

  return MEMCACHED_SUCCESS;
}

char *memcached_stat_get_value(const memcached_st *ptr, memcached_stat_st *memc_stat,
                               const char *key, memcached_return_t *error)
{
  char buffer[SMALL_STRING_LEN];
  int length;
  char *ret;

  *error= MEMCACHED_SUCCESS;

  if (not memcmp("pid", key, sizeof("pid") -1))
  {
    length= snprintf(buffer, SMALL_STRING_LEN,"%lld", (signed long long)memc_stat->pid);
  }
  else if (not memcmp("uptime", key, sizeof("uptime") -1))
  {
    length= snprintf(buffer, SMALL_STRING_LEN,"%lu", memc_stat->uptime);
  }
  else if (not memcmp("time", key, sizeof("time") -1))
  {
    length= snprintf(buffer, SMALL_STRING_LEN,"%llu", (unsigned long long)memc_stat->time);
  }
  else if (not memcmp("version", key, sizeof("version") -1))
  {
    length= snprintf(buffer, SMALL_STRING_LEN,"%s", memc_stat->version);
  }
  else if (not memcmp("pointer_size", key, sizeof("pointer_size") -1))
  {
    length= snprintf(buffer, SMALL_STRING_LEN,"%lu", memc_stat->pointer_size);
  }
  else if (not memcmp("rusage_user", key, sizeof("rusage_user") -1))
  {
    length= snprintf(buffer, SMALL_STRING_LEN,"%lu.%lu", memc_stat->rusage_user_seconds, memc_stat->rusage_user_microseconds);
  }
  else if (not memcmp("rusage_system", key, sizeof("rusage_system") -1))
  {
    length= snprintf(buffer, SMALL_STRING_LEN,"%lu.%lu", memc_stat->rusage_system_seconds, memc_stat->rusage_system_microseconds);
  }
  else if (not memcmp("curr_items", key, sizeof("curr_items") -1))
  {
    length= snprintf(buffer, SMALL_STRING_LEN,"%lu", memc_stat->curr_items);
  }
  else if (not memcmp("total_items", key, sizeof("total_items") -1))
  {
    length= snprintf(buffer, SMALL_STRING_LEN,"%lu", memc_stat->total_items);
  }
  else if (not memcmp("curr_connections", key, sizeof("curr_connections") -1))
  {
    length= snprintf(buffer, SMALL_STRING_LEN,"%lu", memc_stat->curr_connections);
  }
  else if (not memcmp("total_connections", key, sizeof("total_connections") -1))
  {
    length= snprintf(buffer, SMALL_STRING_LEN,"%lu", memc_stat->total_connections);
  }
  else if (not memcmp("connection_structures", key, sizeof("connection_structures") -1))
  {
    length= snprintf(buffer, SMALL_STRING_LEN,"%lu", memc_stat->connection_structures);
  }
  else if (not memcmp("cmd_get", key, sizeof("cmd_get") -1))
  {
    length= snprintf(buffer, SMALL_STRING_LEN,"%llu", (unsigned long long)memc_stat->cmd_get);
  }
  else if (not memcmp("cmd_set", key, sizeof("cmd_set") -1))
  {
    length= snprintf(buffer, SMALL_STRING_LEN,"%llu", (unsigned long long)memc_stat->cmd_set);
  }
  else if (not memcmp("get_hits", key, sizeof("get_hits") -1))
  {
    length= snprintf(buffer, SMALL_STRING_LEN,"%llu", (unsigned long long)memc_stat->get_hits);
  }
  else if (not memcmp("get_misses", key, sizeof("get_misses") -1))
  {
    length= snprintf(buffer, SMALL_STRING_LEN,"%llu", (unsigned long long)memc_stat->get_misses);
  }
  else if (not memcmp("evictions", key, sizeof("evictions") -1))
  {
    length= snprintf(buffer, SMALL_STRING_LEN,"%llu", (unsigned long long)memc_stat->evictions);
  }
  else if (not memcmp("bytes_read", key, sizeof("bytes_read") -1))
  {
    length= snprintf(buffer, SMALL_STRING_LEN,"%llu", (unsigned long long)memc_stat->bytes_read);
  }
  else if (not memcmp("bytes_written", key, sizeof("bytes_written") -1))
  {
    length= snprintf(buffer, SMALL_STRING_LEN,"%llu", (unsigned long long)memc_stat->bytes_written);
  }
  else if (not memcmp("bytes", key, sizeof("bytes") -1))
  {
    length= snprintf(buffer, SMALL_STRING_LEN,"%llu", (unsigned long long)memc_stat->bytes);
  }
  else if (not memcmp("limit_maxbytes", key, sizeof("limit_maxbytes") -1))
  {
    length= snprintf(buffer, SMALL_STRING_LEN,"%llu", (unsigned long long)memc_stat->limit_maxbytes);
  }
  else if (not memcmp("threads", key, sizeof("threads") -1))
  {
    length= snprintf(buffer, SMALL_STRING_LEN,"%lu", memc_stat->threads);
  }
  else
  {
    *error= MEMCACHED_NOTFOUND;
    return NULL;
  }

  if (length >= SMALL_STRING_LEN || length < 0)
  {
    *error= MEMCACHED_FAILURE;
    return NULL;
  }

  ret= static_cast<char *>(libmemcached_malloc(ptr, (size_t) (length + 1)));
  memcpy(ret, buffer, (size_t) length);
  ret[length]= '\0';

  return ret;
}

static memcached_return_t binary_stats_fetch(memcached_stat_st *memc_stat,
                                             const char *args,
                                             memcached_server_write_instance_st instance,
                                             struct local_context *check)
{
  char result[MEMCACHED_DEFAULT_COMMAND_SIZE];
  protocol_binary_request_stats request= {}; // = {.bytes= {0}};
  request.message.header.request.magic= PROTOCOL_BINARY_REQ;
  request.message.header.request.opcode= PROTOCOL_BINARY_CMD_STAT;
  request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;

  if (args)
  {
    size_t len= strlen(args);

    memcached_return_t rc= memcached_validate_key_length(len, true);
    unlikely (rc != MEMCACHED_SUCCESS)
      return rc;

    request.message.header.request.keylen= htons((uint16_t)len);
    request.message.header.request.bodylen= htonl((uint32_t) len);

    struct libmemcached_io_vector_st vector[]=
    {
      { sizeof(request.bytes), request.bytes },
      { len, args }
    };

    rc = memcached_vdo(instance, vector, 2, true);
    if (rc != MEMCACHED_SUCCESS) {
      return rc;
    }
  }
  else
  {
    memcached_return_t rc = memcached_do(instance, request.bytes, sizeof(request.bytes), true);
    if (rc != MEMCACHED_SUCCESS) {
      return rc;
    }
  }

  memcached_server_response_decrement(instance);
  do
  {
    memcached_return_t rc= memcached_response(instance, result, sizeof(result), NULL);

    if (rc == MEMCACHED_END)
      break;

    unlikely (rc != MEMCACHED_SUCCESS)
    {
      return rc;
    }

    if (memc_stat)
    {
      unlikely((set_data(memc_stat, result, result + strlen(result) + 1)) == MEMCACHED_UNKNOWN_STAT_KEY)
      {
        WATCHPOINT_ERROR(MEMCACHED_UNKNOWN_STAT_KEY);
        WATCHPOINT_ASSERT(0);
      }
    }

    if (check && check->func)
    {
      size_t key_length= strlen(result);

      check->func(instance,
                  result, key_length,
                  result+key_length+1, strlen(result+key_length+1),
                  check->context);
    }
  } while (1);

  /* shit... memcached_response will decrement the counter, so I need to
   * reset it.. todo: look at this and try to find a better solution.
   */
  instance->cursor_active= 0;

  return MEMCACHED_SUCCESS;
}

static memcached_return_t ascii_stats_fetch(memcached_stat_st *memc_stat,
                                            const char *args,
                                            memcached_server_write_instance_st instance,
                                            struct local_context *check)
{
  const size_t buffer_length= MEMCACHED_DEFAULT_COMMAND_SIZE;
  char buffer[buffer_length];
  int write_length;

  if (args)
  {
    write_length= snprintf(buffer, buffer_length, "stats %s\r\n", args);
  }
  else
  {
    write_length= snprintf(buffer, buffer_length, "stats\r\n");
  }

  if ((size_t)write_length >= buffer_length || write_length < 0)
  {
    return memcached_set_error(*instance, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                               memcached_literal_param("snprintf(MEMCACHED_DEFAULT_COMMAND_SIZE)"));
  }

  memcached_return_t rc= memcached_do(instance, buffer, (size_t)write_length, true);
  if (rc == MEMCACHED_SUCCESS)
  {
    char result[MEMCACHED_DEFAULT_COMMAND_SIZE];
    while ((rc= memcached_response(instance, result, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL)) == MEMCACHED_STAT)
    {
      char *string_ptr, *end_ptr;
      char *key, *value;

      string_ptr= result;
      string_ptr+= 5; /* Move past STAT */
      for (end_ptr= string_ptr; isgraph(*end_ptr); end_ptr++) {};
      key= string_ptr;
      key[(size_t)(end_ptr-string_ptr)]= 0;

      string_ptr= end_ptr + 1;
      for (end_ptr= string_ptr; !(isspace(*end_ptr)); end_ptr++) {};
      value= string_ptr;
      value[(size_t)(end_ptr -string_ptr)]= 0;
      if (memc_stat)
      {
        unlikely((set_data(memc_stat, key, value)) == MEMCACHED_UNKNOWN_STAT_KEY)
        {
          WATCHPOINT_ERROR(MEMCACHED_UNKNOWN_STAT_KEY);
          WATCHPOINT_ASSERT(0);
        }
      }

      if (check && check->func)
      {
        check->func(instance,
                    key, strlen(key),
                    value, strlen(value),
                    check->context);
      }
    }
  }

  if (rc == MEMCACHED_END)
    return MEMCACHED_SUCCESS;
  else
    return rc;
}

memcached_stat_st *memcached_stat(memcached_st *self, char *args, memcached_return_t *error)
{
  memcached_return_t unused;
  if (error == NULL)
  {
    error= &unused;
  }

  memcached_return_t rc;

  arcus_server_check_for_update(self);

  if (memcached_failed(rc= initialize_query(self)))
  {
    *error= rc;

    return NULL;
  }

  WATCHPOINT_ASSERT(error);

  if (self->flags.use_udp)
  {
    *error= memcached_set_error(*self, MEMCACHED_NOT_SUPPORTED, MEMCACHED_AT);

    return NULL;
  }

  memcached_stat_st *stats= static_cast<memcached_stat_st *>(libmemcached_calloc(self, memcached_server_count(self), sizeof(memcached_stat_st)));

  if (not stats)
  {
    *error= MEMCACHED_MEMORY_ALLOCATION_FAILURE;

    return NULL;
  }

  WATCHPOINT_ASSERT(rc == MEMCACHED_SUCCESS);
  rc= MEMCACHED_SUCCESS;
  for (uint32_t x= 0; x < memcached_server_count(self); x++)
  {
    memcached_return_t temp_return;
    memcached_server_write_instance_st instance;
    memcached_stat_st *stat_instance;

    stat_instance= stats +x;

    stat_instance->pid= -1;
    stat_instance->root= self;

    instance= memcached_server_instance_fetch(self, x);

    if (self->flags.binary_protocol)
    {
      temp_return= binary_stats_fetch(stat_instance, args, instance, NULL);
    }
    else
    {
      temp_return= ascii_stats_fetch(stat_instance, args, instance, NULL);
    }

    if (memcached_failed(temp_return))
    {
      rc= MEMCACHED_SOME_ERRORS;
    }
  }

  *error= rc;

  return stats;
}

memcached_return_t memcached_stat_servername(memcached_stat_st *memc_stat, char *args,
                                             const char *hostname, in_port_t port)
{
  memcached_st memc;
  memcached_server_write_instance_st instance;

  memset(memc_stat, 0, sizeof(memcached_stat_st));

  memcached_st *memc_ptr= memcached_create(&memc);
  if (not memc_ptr)
    return MEMCACHED_MEMORY_ALLOCATION_FAILURE;

  memcached_server_add(&memc, hostname, port);

  memcached_return_t rc;
  if ((rc= initialize_query(memc_ptr)) != MEMCACHED_SUCCESS)
  {
    return rc;
  }

  instance= memcached_server_instance_fetch(memc_ptr, 0);

  if (memc.flags.binary_protocol)
  {
    rc= binary_stats_fetch(memc_stat, args, instance, NULL);
  }
  else
  {
    rc= ascii_stats_fetch(memc_stat, args, instance, NULL);
  }

  memcached_free(&memc);

  return rc;
}

/*
  We make a copy of the keys since at some point in the not so distant future
  we will add support for "found" keys.
*/
char ** memcached_stat_get_keys(memcached_st *ptr,
                                memcached_stat_st *,
                                memcached_return_t *error)
{
  if (not ptr)
    return NULL;

  char **list= static_cast<char **>(libmemcached_malloc(ptr, sizeof(memcached_stat_keys)));
  if (not list)
  {
    *error= memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT);
    return NULL;
  }

  memcpy(list, memcached_stat_keys, sizeof(memcached_stat_keys));

  *error= MEMCACHED_SUCCESS;

  return list;
}

void memcached_stat_free(const memcached_st *, memcached_stat_st *memc_stat)
{
  WATCHPOINT_ASSERT(memc_stat); // Be polite, but when debugging catch this as an error
  if (not memc_stat)
  {
    return;
  }

  if (memc_stat->root)
  {
    libmemcached_free(memc_stat->root, memc_stat);
    return;
  }

  libmemcached_free(NULL, memc_stat);
}

static memcached_return_t call_stat_fn(memcached_st *ptr,
                                       memcached_server_write_instance_st instance,
                                       void *context)
{
  memcached_return_t rc;
  struct local_context *check= (struct local_context *)context;

  if (ptr->flags.binary_protocol)
  {
    rc= binary_stats_fetch(NULL, check->args, instance, check);
  }
  else
  {
    rc= ascii_stats_fetch(NULL, check->args, instance, check);
  }

  return rc;
}

memcached_return_t memcached_stat_execute(memcached_st *memc, const char *args,  memcached_stat_fn func, void *context)
{
  memcached_version(memc);

 struct local_context check(func, context, args);

 return memcached_server_execute(memc, call_stat_fn, (void *)&check);
}
