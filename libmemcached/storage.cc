/*
 * arcus-c-client : Arcus C client
 * Copyright 2010-2014 NAVER Corp.
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
/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Storage related functions, aka set, replace,..
 *
 */

#include <libmemcached/common.h>
#include "libmemcached/arcus_priv.h"

enum memcached_storage_action_t {
  SET_OP,
  REPLACE_OP,
  ADD_OP,
  PREPEND_OP,
  APPEND_OP,
  CAS_OP
};

/* Inline this */
static inline const char *storage_op_string(memcached_storage_action_t verb)
{
  switch (verb)
  {
  case REPLACE_OP:
    return "replace ";

  case ADD_OP:
    return "add ";

  case PREPEND_OP:
    return "prepend ";

  case APPEND_OP:
    return "append ";

  case CAS_OP:
    return "cas ";

  case SET_OP:
    break;
  }

  return "set ";
}

static inline uint8_t get_com_code(memcached_storage_action_t verb, bool noreply)
{
  /* 0 isn't a value we want, but GCC 4.2 seems to think ret can otherwise
   * be used uninitialized in this function. FAIL */
  uint8_t ret= 0;

  if (noreply)
    switch (verb)
    {
    case SET_OP:
      ret=PROTOCOL_BINARY_CMD_SETQ;
      break;
    case ADD_OP:
      ret=PROTOCOL_BINARY_CMD_ADDQ;
      break;
    case CAS_OP: /* FALLTHROUGH */
    case REPLACE_OP:
      ret=PROTOCOL_BINARY_CMD_REPLACEQ;
      break;
    case APPEND_OP:
      ret=PROTOCOL_BINARY_CMD_APPENDQ;
      break;
    case PREPEND_OP:
      ret=PROTOCOL_BINARY_CMD_PREPENDQ;
      break;
    default:
      WATCHPOINT_ASSERT(verb);
      break;
    }
  else
    switch (verb)
    {
    case SET_OP:
      ret=PROTOCOL_BINARY_CMD_SET;
      break;
    case ADD_OP:
      ret=PROTOCOL_BINARY_CMD_ADD;
      break;
    case CAS_OP: /* FALLTHROUGH */
    case REPLACE_OP:
      ret=PROTOCOL_BINARY_CMD_REPLACE;
      break;
    case APPEND_OP:
      ret=PROTOCOL_BINARY_CMD_APPEND;
      break;
    case PREPEND_OP:
      ret=PROTOCOL_BINARY_CMD_PREPEND;
      break;
    default:
      WATCHPOINT_ASSERT(verb);
      break;
    }

  return ret;
}

static memcached_return_t memcached_send_binary(memcached_st *ptr,
                                                uint32_t server_key,
                                                const char *key,
                                                size_t key_length,
                                                const char *value,
                                                size_t value_length,
                                                time_t expiration,
                                                uint32_t flags,
                                                uint64_t cas,
                                                memcached_storage_action_t verb)
{
  bool flush;
  protocol_binary_request_set request= {};
  size_t send_length= sizeof(request.bytes);

  bool noreply= ptr->flags.no_reply;

  request.message.header.request.magic= PROTOCOL_BINARY_REQ;
  request.message.header.request.opcode= get_com_code(verb, noreply);
  request.message.header.request.keylen= htons((uint16_t)(key_length + memcached_array_size(ptr->_namespace)));
  request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;
  if (verb == APPEND_OP || verb == PREPEND_OP)
  {
    send_length -= 8; /* append & prepend does not contain extras! */
  }
  else
  {
    request.message.header.request.extlen= 8;
    request.message.body.flags= htonl(flags);
    request.message.body.expiration= htonl((uint32_t)expiration);
  }

  request.message.header.request.bodylen= htonl((uint32_t) (key_length + memcached_array_size(ptr->_namespace) + value_length +
                                                            request.message.header.request.extlen));

  if (cas)
  {
    request.message.header.request.cas= memcached_htonll(cas);
  }

  struct libmemcached_io_vector_st vector[]=
  {
    { send_length, request.bytes },
    { memcached_array_size(ptr->_namespace), memcached_array_string(ptr->_namespace) },
    { key_length, key },
    { value_length, value }
  };

  flush= (bool) ((ptr->flags.buffer_requests && verb == SET_OP) ? 0 : 1);
  memcached_server_write_instance_st server= memcached_server_instance_fetch(ptr, server_key);

#ifdef ENABLE_REPLICATION
do_action:
#endif
  WATCHPOINT_SET(server->io_wait_count.read= 0);
  WATCHPOINT_SET(server->io_wait_count.write= 0);

  if (ptr->flags.use_udp && ! flush)
  {
    size_t cmd_size= send_length + key_length + value_length;

    if (cmd_size > MAX_UDP_DATAGRAM_LENGTH - UDP_DATAGRAM_HEADER_LENGTH)
    {
      return MEMCACHED_WRITE_FAILURE;
    }
    if (cmd_size + server->write_buffer_offset > MAX_UDP_DATAGRAM_LENGTH)
    {
      memcached_io_write(server, NULL, 0, true);
    }
  }

  /* write the header */
  memcached_return_t rc= memcached_vdo(server, vector, 4, flush);
  if (rc != MEMCACHED_SUCCESS) {
    return rc;
  }

  if (verb == SET_OP && ptr->number_of_replicas > 0)
  {
    request.message.header.request.opcode= PROTOCOL_BINARY_CMD_SETQ;
    WATCHPOINT_STRING("replicating");

    for (uint32_t x= 0; x < ptr->number_of_replicas; x++)
    {
      memcached_server_write_instance_st instance;

      ++server_key;
      if (server_key == memcached_server_count(ptr))
        server_key= 0;

      instance= memcached_server_instance_fetch(ptr, server_key);

      if (memcached_vdo(instance, vector, 4, false) == MEMCACHED_SUCCESS) {
        memcached_server_response_decrement(instance);
      }
    }
  }

  if (flush == false)
  {
    return MEMCACHED_BUFFERED;
  }

  if (noreply)
  {
    return MEMCACHED_SUCCESS;
  }

  rc= memcached_response(server, NULL, 0, NULL);
#ifdef ENABLE_REPLICATION
  if (rc == MEMCACHED_SWITCHOVER or rc == MEMCACHED_REPL_SLAVE)
  {
    ZOO_LOG_INFO(("Switchover: hostname=%s port=%d error=%s",
                  server->hostname, server->port, memcached_strerror(ptr, rc)));
    if (memcached_rgroup_switchover(ptr, server) == true) {
      server= memcached_server_instance_fetch(ptr, server_key);
      goto do_action;
    }
  }
#endif
  return rc;
}

static memcached_return_t memcached_send_ascii(memcached_st *ptr,
                                               const uint32_t server_key,
                                               const char *key,
                                               const size_t key_length,
                                               const char *value,
                                               const size_t value_length,
                                               const time_t expiration,
                                               const uint32_t flags,
                                               const uint64_t cas,
                                               memcached_storage_action_t verb)
{
  char flags_buffer[MEMCACHED_MAXIMUM_INTEGER_DISPLAY_LENGTH +1];
  int flags_buffer_length= snprintf(flags_buffer, sizeof(flags_buffer), " %u", flags);
  if (size_t(flags_buffer_length) >= sizeof(flags_buffer) or flags_buffer_length < 0)
  {
    return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                               memcached_literal_param("snprintf(MEMCACHED_MAXIMUM_INTEGER_DISPLAY_LENGTH)"));
  }

  char expiration_buffer[MEMCACHED_MAXIMUM_INTEGER_DISPLAY_LENGTH +1];
  int expiration_buffer_length= snprintf(expiration_buffer, sizeof(expiration_buffer), " %lld", (long long)expiration);
  if (size_t(expiration_buffer_length) >= sizeof(expiration_buffer) or expiration_buffer_length < 0)
  {
    return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                               memcached_literal_param("snprintf(MEMCACHED_MAXIMUM_INTEGER_DISPLAY_LENGTH)"));
  }

  char value_buffer[MEMCACHED_MAXIMUM_INTEGER_DISPLAY_LENGTH +1];
  int value_buffer_length= snprintf(value_buffer, sizeof(value_buffer), " %lu", (unsigned long)value_length);
  if (size_t(value_buffer_length) >= sizeof(value_buffer) or value_buffer_length < 0)
  {
    return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                               memcached_literal_param("snprintf(MEMCACHED_MAXIMUM_INTEGER_DISPLAY_LENGTH)"));
  }

  char cas_buffer[MEMCACHED_MAXIMUM_INTEGER_DISPLAY_LENGTH +1];
  int cas_buffer_length= 0;
  if (cas)
  {
    cas_buffer_length= snprintf(cas_buffer, sizeof(cas_buffer), " %llu", (unsigned long long)cas);
    if (size_t(cas_buffer_length) >= sizeof(cas_buffer) or cas_buffer_length < 0)
    {
      return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                                 memcached_literal_param("snprintf(MEMCACHED_MAXIMUM_INTEGER_DISPLAY_LENGTH)"));
    }
  }

  struct libmemcached_io_vector_st vector[]=
  {
    { strlen(storage_op_string(verb)), storage_op_string(verb) },
    { memcached_array_size(ptr->_namespace), memcached_array_string(ptr->_namespace) },
    { key_length, key },
    { size_t(flags_buffer_length), flags_buffer },
    { size_t(expiration_buffer_length), expiration_buffer },
    { size_t(value_buffer_length), value_buffer },
    { size_t(cas_buffer_length), cas_buffer },
    { (ptr->flags.no_reply ? memcached_literal_param_size(" noreply") : 0), " noreply" },
    { 2, "\r\n" },
    { value_length, value },
    { 2, "\r\n" }
  };

  bool to_write;
  if (ptr->flags.buffer_requests && verb == SET_OP)
  {
    to_write= false;
  }
  else
  {
    to_write= true;
  }

  memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, server_key);;

#ifdef ENABLE_REPLICATION
do_action:
#endif
  WATCHPOINT_SET(instance->io_wait_count.read= 0);
  WATCHPOINT_SET(instance->io_wait_count.write= 0);

  if (ptr->flags.use_udp && ptr->flags.buffer_requests)
  {
    size_t cmd_size= 0;
    for (uint32_t x= 0; x < 11; x++)
    {
      cmd_size+= vector[x].length;
    }

    if (cmd_size > MAX_UDP_DATAGRAM_LENGTH - UDP_DATAGRAM_HEADER_LENGTH)
      return memcached_set_error(*ptr, MEMCACHED_WRITE_FAILURE, MEMCACHED_AT);

    if (cmd_size + instance->write_buffer_offset > MAX_UDP_DATAGRAM_LENGTH)
      memcached_io_write(instance, NULL, 0, true);
  }

  /* Send command header */
  memcached_return_t rc= memcached_vdo(instance, vector, 11, to_write);

  if (rc == MEMCACHED_SUCCESS)
  {
    if (ptr->flags.no_reply)
    {
      rc= (to_write == false) ? MEMCACHED_BUFFERED : MEMCACHED_SUCCESS;
    }
    else if (to_write == false)
    {
      rc= MEMCACHED_BUFFERED;
    }
    else
    {
      char result[MEMCACHED_DEFAULT_COMMAND_SIZE];
      rc= memcached_response(instance, result, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);

      if (rc == MEMCACHED_STORED)
      {
        rc= MEMCACHED_SUCCESS;
      }
#ifdef ENABLE_REPLICATION
      else if (rc == MEMCACHED_SWITCHOVER or rc == MEMCACHED_REPL_SLAVE)
      {
        ZOO_LOG_INFO(("Switchover: hostname=%s port=%d error=%s",
                      instance->hostname, instance->port, memcached_strerror(ptr, rc)));
        if (memcached_rgroup_switchover(ptr, instance) == true) {
          instance= memcached_server_instance_fetch(ptr, server_key);
          goto do_action;
        }
      }
#endif
    }
  }

  return rc;
}

static inline memcached_return_t memcached_send(memcached_st *ptr,
                                                const char *group_key, size_t group_key_length,
                                                const char *key, size_t key_length,
                                                const char *value, size_t value_length,
                                                time_t expiration,
                                                uint32_t flags,
                                                uint64_t cas,
                                                memcached_storage_action_t verb)
{
  arcus_server_check_for_update(ptr);

  memcached_return_t rc;
  if (memcached_failed(rc= initialize_query(ptr)))
  {
    return rc;
  }

  if (memcached_failed(rc= memcached_validate_key_length(key_length, ptr->flags.binary_protocol)))
  {
    return rc;
  }

  if (memcached_failed(memcached_key_test(*ptr, (const char **)&key, &key_length, 1)))
  {
    return MEMCACHED_BAD_KEY_PROVIDED;
  }

  uint32_t server_key= memcached_generate_hash_with_redistribution(ptr, group_key, group_key_length);
  if (ptr->flags.binary_protocol)
  {
    rc= memcached_send_binary(ptr, server_key,
                              key, key_length,
                              value, value_length, expiration,
                              flags, cas, verb);
  }
  else
  {
    rc= memcached_send_ascii(ptr, server_key,
                             key, key_length,
                             value, value_length, expiration,
                             flags, cas, verb);
  }

  return rc;
}


memcached_return_t memcached_set(memcached_st *ptr, const char *key, size_t key_length,
                                 const char *value, size_t value_length,
                                 time_t expiration,
                                 uint32_t flags)
{
  memcached_return_t rc;
  LIBMEMCACHED_MEMCACHED_SET_START();
  rc= memcached_send(ptr, key, key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, SET_OP);
  LIBMEMCACHED_MEMCACHED_SET_END();
  return rc;
}

memcached_return_t memcached_add(memcached_st *ptr,
                                 const char *key, size_t key_length,
                                 const char *value, size_t value_length,
                                 time_t expiration,
                                 uint32_t flags)
{
  memcached_return_t rc;
  LIBMEMCACHED_MEMCACHED_ADD_START();
  rc= memcached_send(ptr, key, key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, ADD_OP);
  LIBMEMCACHED_MEMCACHED_ADD_END();
  return rc;
}

memcached_return_t memcached_replace(memcached_st *ptr,
                                     const char *key, size_t key_length,
                                     const char *value, size_t value_length,
                                     time_t expiration,
                                     uint32_t flags)
{
  memcached_return_t rc;
  LIBMEMCACHED_MEMCACHED_REPLACE_START();
  rc= memcached_send(ptr, key, key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, REPLACE_OP);
  LIBMEMCACHED_MEMCACHED_REPLACE_END();
  return rc;
}

memcached_return_t memcached_prepend(memcached_st *ptr,
                                     const char *key, size_t key_length,
                                     const char *value, size_t value_length,
                                     time_t expiration,
                                     uint32_t flags)
{
  memcached_return_t rc;
  rc= memcached_send(ptr, key, key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, PREPEND_OP);
  return rc;
}

memcached_return_t memcached_append(memcached_st *ptr,
                                    const char *key, size_t key_length,
                                    const char *value, size_t value_length,
                                    time_t expiration,
                                    uint32_t flags)
{
  memcached_return_t rc;
  rc= memcached_send(ptr, key, key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, APPEND_OP);
  return rc;
}

memcached_return_t memcached_cas(memcached_st *ptr,
                                 const char *key, size_t key_length,
                                 const char *value, size_t value_length,
                                 time_t expiration,
                                 uint32_t flags,
                                 uint64_t cas)
{
  memcached_return_t rc;
  rc= memcached_send(ptr, key, key_length,
                     key, key_length, value, value_length,
                     expiration, flags, cas, CAS_OP);
  return rc;
}

memcached_return_t memcached_set_by_key(memcached_st *ptr,
                                        const char *group_key,
                                        size_t group_key_length,
                                        const char *key, size_t key_length,
                                        const char *value, size_t value_length,
                                        time_t expiration,
                                        uint32_t flags)
{
  memcached_return_t rc;
  LIBMEMCACHED_MEMCACHED_SET_START();
  rc= memcached_send(ptr, group_key, group_key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, SET_OP);
  LIBMEMCACHED_MEMCACHED_SET_END();
  return rc;
}

memcached_return_t memcached_add_by_key(memcached_st *ptr,
                                        const char *group_key, size_t group_key_length,
                                        const char *key, size_t key_length,
                                        const char *value, size_t value_length,
                                        time_t expiration,
                                        uint32_t flags)
{
  memcached_return_t rc;
  LIBMEMCACHED_MEMCACHED_ADD_START();
  rc= memcached_send(ptr, group_key, group_key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, ADD_OP);
  LIBMEMCACHED_MEMCACHED_ADD_END();
  return rc;
}

memcached_return_t memcached_replace_by_key(memcached_st *ptr,
                                            const char *group_key, size_t group_key_length,
                                            const char *key, size_t key_length,
                                            const char *value, size_t value_length,
                                            time_t expiration,
                                            uint32_t flags)
{
  memcached_return_t rc;
  LIBMEMCACHED_MEMCACHED_REPLACE_START();
  rc= memcached_send(ptr, group_key, group_key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, REPLACE_OP);
  LIBMEMCACHED_MEMCACHED_REPLACE_END();
  return rc;
}

memcached_return_t memcached_prepend_by_key(memcached_st *ptr,
                                            const char *group_key, size_t group_key_length,
                                            const char *key, size_t key_length,
                                            const char *value, size_t value_length,
                                            time_t expiration,
                                            uint32_t flags)
{
  memcached_return_t rc;
  rc= memcached_send(ptr, group_key, group_key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, PREPEND_OP);
  return rc;
}

memcached_return_t memcached_append_by_key(memcached_st *ptr,
                                           const char *group_key, size_t group_key_length,
                                           const char *key, size_t key_length,
                                           const char *value, size_t value_length,
                                           time_t expiration,
                                           uint32_t flags)
{
  memcached_return_t rc;
  rc= memcached_send(ptr, group_key, group_key_length,
                     key, key_length, value, value_length,
                     expiration, flags, 0, APPEND_OP);
  return rc;
}

memcached_return_t memcached_cas_by_key(memcached_st *ptr,
                                        const char *group_key, size_t group_key_length,
                                        const char *key, size_t key_length,
                                        const char *value, size_t value_length,
                                        time_t expiration,
                                        uint32_t flags,
                                        uint64_t cas)
{
  memcached_return_t rc;
  rc= memcached_send(ptr, group_key, group_key_length,
                     key, key_length, value, value_length,
                     expiration, flags, cas, CAS_OP);
  return rc;
}

