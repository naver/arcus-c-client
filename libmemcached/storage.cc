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

#ifdef ENABLE_REPLICATION
static inline memcached_return_t switchover_state_to_return_t(enum memcached_server_switchover_state_t s)
{
  if (s == MEMCACHED_SERVER_SWITCHOVER_NEEDED)
  {
    return MEMCACHED_SWITCHOVER;
  }
  else if (s == MEMCACHED_SERVER_SWITCHOVER_NEEDED_REPL_SLAVE)
  {
    return MEMCACHED_REPL_SLAVE;
  }
  return MEMCACHED_MAXIMUM_RETURN;
}

static inline enum memcached_server_switchover_state_t return_t_to_switchover_state(memcached_return_t rc)
{
  if (rc == MEMCACHED_SWITCHOVER)
  {
    return MEMCACHED_SERVER_SWITCHOVER_NEEDED;
  }
  else if (rc == MEMCACHED_REPL_SLAVE)
  {
    return MEMCACHED_SERVER_SWITCHOVER_NEEDED_REPL_SLAVE;
  }
  return MEMCACHED_SERVER_SWITCHOVER_FAILED;
}
#endif

static inline bool is_no_reply(memcached_st *ptr)
{
  return ptr->flags.bulked == false and ptr->flags.no_reply == true;
}

static memcached_return_t before_bulk_storage(memcached_st *ptr,
                                              const char *group_key, size_t group_key_length,
                                              const memcached_storage_request_st *req,
                                              const size_t number_of_req)
{
  memcached_return_t rc= initialize_query(ptr);
  if (memcached_failed(rc))
  {
    return rc;
  }

  if (ptr->flags.use_udp)
  {
    return memcached_set_error(*ptr, MEMCACHED_NOT_SUPPORTED, MEMCACHED_AT);
  }
  if (req == NULL)
  {
    return memcached_set_error(*ptr, MEMCACHED_NOTFOUND, MEMCACHED_AT,
                               memcached_literal_param("req were null"));
  }
  if (number_of_req == 0)
  {
    return memcached_set_error(*ptr, MEMCACHED_NOTFOUND, MEMCACHED_AT,
                               memcached_literal_param("number_of_req were zero"));
  }
  if (group_key and group_key_length)
  {
    if (memcached_failed(memcached_key_test(*ptr, (const char * const *) &group_key, &group_key_length, 1)))
    {
      return memcached_set_error(*ptr, MEMCACHED_BAD_KEY_PROVIDED, MEMCACHED_AT,
                                 memcached_literal_param("A bad group key was provided."));
    }
  }

  /*
    Here is where we pay for the non-block API. We need to remove any data sitting
    in the queue before we start our store operations.

    It might be optimum to bounce the connection if count > some number.
  */
  memcached_server_write_instance_st instance= NULL;
  for (uint32_t x= 0; x < memcached_server_count(ptr); x++)
  {
    instance= memcached_server_instance_fetch(ptr, x);
    if (memcached_server_response_count(instance))
    {
      if (ptr->flags.no_block)
      {
        (void)memcached_io_write(instance, NULL, 0, true);
      }

      char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
      while(memcached_server_response_count(instance))
      {
        (void)memcached_response(instance, buffer, sizeof(buffer), &ptr->result);
      }
    }
  }

  return MEMCACHED_SUCCESS;
}

static memcached_return_t memcached_recv_from_server(memcached_st *ptr,
                                                     const memcached_server_write_instance_st instance)
{
  if (instance == NULL)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  char result[MEMCACHED_DEFAULT_COMMAND_SIZE];
  if (ptr->flags.bulked == false)
  {
    return memcached_response(instance, result, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);
  }
  return memcached_read_one_response(instance, result, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);
}

static memcached_return_t memcached_send_to_binary_server(memcached_st *ptr,
                                                          const uint32_t server_key,
                                                          const memcached_server_write_instance_st instance,
                                                          const memcached_storage_request_st req,
                                                          const memcached_storage_action_t verb)
{
  if (instance == NULL)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  protocol_binary_request_set request= {};
  size_t send_length= sizeof(request.bytes);

  request.message.header.request.magic= PROTOCOL_BINARY_REQ;
  request.message.header.request.opcode= get_com_code(verb, is_no_reply(ptr));
  request.message.header.request.keylen= htons((uint16_t)(req.key_length + memcached_array_size(ptr->_namespace)));
  request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;
  if (verb == APPEND_OP || verb == PREPEND_OP)
  {
    send_length -= 8; /* append & prepend does not contain extras! */
  }
  else
  {
    request.message.header.request.extlen= 8;
    request.message.body.flags= htonl(req.flags);
    request.message.body.expiration= htonl((uint32_t) req.expiration);
  }

  request.message.header.request.bodylen= htonl((uint32_t) (req.key_length + memcached_array_size(ptr->_namespace) + req.value_length +
                                                            request.message.header.request.extlen));

  if (verb == CAS_OP)
  {
    request.message.header.request.cas= memcached_htonll(req.cas);
  }

  struct libmemcached_io_vector_st vector[]= {
    { send_length, request.bytes },
    { memcached_array_size(ptr->_namespace), memcached_array_string(ptr->_namespace) },
    { req.key_length, req.key },
    { req.value_length, req.value }
  };

  bool flush= ptr->flags.bulked == true or ptr->flags.buffer_requests == false or verb != SET_OP;

  WATCHPOINT_SET(instance->io_wait_count.read= 0);
  WATCHPOINT_SET(instance->io_wait_count.write= 0);

  if (ptr->flags.use_udp && !flush)
  {
    size_t cmd_size= send_length + req.key_length + req.value_length;

    if (cmd_size > MAX_UDP_DATAGRAM_LENGTH - UDP_DATAGRAM_HEADER_LENGTH)
    {
      return MEMCACHED_WRITE_FAILURE;
    }
    if (cmd_size + instance->write_buffer_offset > MAX_UDP_DATAGRAM_LENGTH)
    {
      memcached_io_write(instance, NULL, 0, true);
    }
  }

  /* write the header */
  memcached_return_t rc= memcached_vdo(instance, vector, 4, flush);
  if (rc != MEMCACHED_SUCCESS)
  {
    return rc;
  }

  if (verb == SET_OP && ptr->number_of_replicas > 0)
  {
    request.message.header.request.opcode= PROTOCOL_BINARY_CMD_SETQ;
    WATCHPOINT_STRING("replicating");

    memcached_server_write_instance_st replica_instance= NULL;
    uint32_t replica_server_key= server_key;

    for (uint32_t x= 0; x < ptr->number_of_replicas; x++)
    {
      replica_server_key++;
      if (replica_server_key == memcached_server_count(ptr))
      {
        replica_server_key= 0;
      }

      replica_instance= memcached_server_instance_fetch(ptr, replica_server_key);
      if (memcached_vdo(replica_instance, vector, 4, false) == MEMCACHED_SUCCESS)
      {
        memcached_server_response_decrement(replica_instance);
      }
    }
  }

  if (flush == false)
  {
    return MEMCACHED_BUFFERED;
  }
  return rc;
}

static memcached_return_t memcached_storage_to_binary_server(memcached_st *ptr,
                                                             const uint32_t server_key,
                                                             const memcached_storage_request_st req,
                                                             const memcached_storage_action_t verb)
{
  memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, server_key);

#ifdef ENABLE_REPLICATION
do_action:
#endif
  memcached_return_t rc= memcached_send_to_binary_server(ptr, server_key, instance, req, verb);
  if (memcached_failed(rc) or rc == MEMCACHED_BUFFERED)
  {
    return rc;
  }

  if (is_no_reply(ptr))
  {
    return rc;
  }

  rc= memcached_recv_from_server(ptr, instance);
#ifdef ENABLE_REPLICATION
  if (rc == MEMCACHED_SWITCHOVER or rc == MEMCACHED_REPL_SLAVE)
  {
    ZOO_LOG_INFO(("Switchover: hostname=%s port=%d error=%s",
                  instance->hostname, instance->port, memcached_strerror(ptr, rc)));
    if (memcached_rgroup_switchover(ptr, instance) == true)
    {
      instance= memcached_server_instance_fetch(ptr, server_key);
      goto do_action;
    }
  }
#endif

  return rc;
}

static memcached_return_t memcached_send_to_ascii_server(memcached_st *ptr,
                                                         const memcached_server_write_instance_st instance,
                                                         const memcached_storage_request_st req,
                                                         const memcached_storage_action_t verb)
{
  if (instance == NULL)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  char flags_buffer[MEMCACHED_MAXIMUM_INTEGER_DISPLAY_LENGTH +1];
  int flags_buffer_length= snprintf(flags_buffer, sizeof(flags_buffer), " %u", req.flags);
  if (size_t(flags_buffer_length) >= sizeof(flags_buffer) or flags_buffer_length < 0)
  {
    return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                               memcached_literal_param("snprintf(MEMCACHED_MAXIMUM_INTEGER_DISPLAY_LENGTH)"));
  }

  char expiration_buffer[MEMCACHED_MAXIMUM_INTEGER_DISPLAY_LENGTH +1];
  int expiration_buffer_length= snprintf(expiration_buffer, sizeof(expiration_buffer), " %lld", (long long) req.expiration);
  if (size_t(expiration_buffer_length) >= sizeof(expiration_buffer) or expiration_buffer_length < 0)
  {
    return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                               memcached_literal_param("snprintf(MEMCACHED_MAXIMUM_INTEGER_DISPLAY_LENGTH)"));
  }

  char value_buffer[MEMCACHED_MAXIMUM_INTEGER_DISPLAY_LENGTH +1];
  int value_buffer_length= snprintf(value_buffer, sizeof(value_buffer), " %lu", (unsigned long) req.value_length);
  if (size_t(value_buffer_length) >= sizeof(value_buffer) or value_buffer_length < 0)
  {
    return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                               memcached_literal_param("snprintf(MEMCACHED_MAXIMUM_INTEGER_DISPLAY_LENGTH)"));
  }

  char cas_buffer[MEMCACHED_MAXIMUM_INTEGER_DISPLAY_LENGTH +1];
  int cas_buffer_length= 0;
  if (verb == CAS_OP)
  {
    cas_buffer_length= snprintf(cas_buffer, sizeof(cas_buffer), " %llu", (unsigned long long) req.cas);
    if (size_t(cas_buffer_length) >= sizeof(cas_buffer) or cas_buffer_length < 0)
    {
      return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                                 memcached_literal_param("snprintf(MEMCACHED_MAXIMUM_INTEGER_DISPLAY_LENGTH)"));
    }
  }

  struct libmemcached_io_vector_st vector[]= {
    { strlen(storage_op_string(verb)), storage_op_string(verb) },
    { memcached_array_size(ptr->_namespace), memcached_array_string(ptr->_namespace) },
    { req.key_length, req.key },
    { size_t(flags_buffer_length), flags_buffer },
    { size_t(expiration_buffer_length), expiration_buffer },
    { size_t(value_buffer_length), value_buffer },
    { size_t(cas_buffer_length), cas_buffer },
    { (is_no_reply(ptr) ? memcached_literal_param_size(" noreply") : 0), " noreply" },
    { 2, "\r\n" },
    { req.value_length, req.value },
    { 2, "\r\n" }
  };

  bool to_write= ptr->flags.bulked == true or ptr->flags.buffer_requests == false or verb != SET_OP;

  WATCHPOINT_SET(instance->io_wait_count.read= 0);
  WATCHPOINT_SET(instance->io_wait_count.write= 0);

  if (ptr->flags.use_udp && !to_write)
  {
    size_t cmd_size= 0;
    for (uint32_t x= 0; x < 11; x++)
    {
      cmd_size+= vector[x].length;
    }

    if (cmd_size > MAX_UDP_DATAGRAM_LENGTH - UDP_DATAGRAM_HEADER_LENGTH)
    {
      return memcached_set_error(*ptr, MEMCACHED_WRITE_FAILURE, MEMCACHED_AT);
    }
    if (cmd_size + instance->write_buffer_offset > MAX_UDP_DATAGRAM_LENGTH)
    {
      memcached_io_write(instance, NULL, 0, true);
    }
  }

  /* Send command header */
  memcached_return_t rc= memcached_vdo(instance, vector, 11, to_write);
  if (rc == MEMCACHED_SUCCESS && to_write == false)
  {
    return MEMCACHED_BUFFERED;
  }
  return rc;
}

static memcached_return_t memcached_storage_to_ascii_server(memcached_st *ptr,
                                                            const uint32_t server_key,
                                                            const memcached_storage_request_st req,
                                                            const memcached_storage_action_t verb)
{
  memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, server_key);

#ifdef ENABLE_REPLICATION
do_action:
#endif
  memcached_return_t rc= memcached_send_to_ascii_server(ptr, instance, req, verb);
  if (memcached_failed(rc) or rc == MEMCACHED_BUFFERED)
  {
    return rc;
  }

  if (is_no_reply(ptr))
  {
    return rc;
  }

  rc= memcached_recv_from_server(ptr, instance);
#ifdef ENABLE_REPLICATION
  if (rc == MEMCACHED_SWITCHOVER or rc == MEMCACHED_REPL_SLAVE)
  {
    ZOO_LOG_INFO(("Switchover: hostname=%s port=%d error=%s",
                  instance->hostname, instance->port, memcached_strerror(ptr, rc)));
    if (memcached_rgroup_switchover(ptr, instance) == true)
    {
      instance= memcached_server_instance_fetch(ptr, server_key);
      goto do_action;
    }
  }
#endif

  return rc;
}

static memcached_return_t memcached_storage(memcached_st *ptr,
                                            const char *group_key,
                                            const size_t group_key_length,
                                            const memcached_storage_request_st req,
                                            const memcached_storage_action_t verb)
{
  if (ptr == NULL)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  arcus_server_check_for_update(ptr);

  memcached_return_t rc= initialize_query(ptr);
  if (memcached_failed(rc))
  {
    return rc;
  }

  if (verb == CAS_OP and req.cas == 0)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }
  if (memcached_failed(rc= memcached_validate_key_length(req.key_length, ptr->flags.binary_protocol)))
  {
    return rc;
  }
  if (memcached_failed(memcached_key_test(*ptr, (const char **) &req.key, &req.key_length, 1)))
  {
    return MEMCACHED_BAD_KEY_PROVIDED;
  }
  if (memcached_failed(memcached_key_test(*ptr, (const char **) &group_key, &group_key_length, 1)))
  {
    return memcached_set_error(*ptr, MEMCACHED_BAD_KEY_PROVIDED, MEMCACHED_AT,
                               memcached_literal_param("A bad group key was provided."));
  }

  uint32_t server_key= memcached_generate_hash_with_redistribution(ptr, group_key, group_key_length);
  if (ptr->flags.binary_protocol)
  {
    rc= memcached_storage_to_binary_server(ptr, server_key, req, verb);
  }
  else
  {
    rc= memcached_storage_to_ascii_server(ptr, server_key, req, verb);
  }

  if (rc == MEMCACHED_STORED)
  {
    return MEMCACHED_SUCCESS;
  }
  return rc;
}

static memcached_return_t memcached_bulk_storage(memcached_st *ptr,
                                                 const char *group_key,
                                                 const size_t group_key_length,
                                                 const memcached_storage_request_st *req,
                                                 const size_t number_of_req,
                                                 const memcached_storage_action_t verb,
                                                 memcached_return_t *rc_arr_ret)
{
  if (ptr == NULL)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  arcus_server_check_for_update(ptr);

  if (req == NULL or number_of_req == 0 or rc_arr_ret == NULL)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  bool group_key_present= (group_key != NULL and group_key_length > 0);
  if (group_key_present and memcached_failed(memcached_key_test(*ptr, (const char **) &group_key, &group_key_length, 1)))
  {
    return memcached_set_error(*ptr, MEMCACHED_BAD_KEY_PROVIDED, MEMCACHED_AT,
                               memcached_literal_param("A bad group key was provided."));
  }

  memcached_return_t rc= before_bulk_storage(ptr, group_key, group_key_length, req, number_of_req);
  if (memcached_failed(rc))
  {
    return rc;
  }

  bool failure_occurred= false;
  bool success_occurred= false;

#define BUILD_RETURN_VALUE() (!success_occurred ? MEMCACHED_FAILURE \
                                                : (failure_occurred ? MEMCACHED_SOME_ERRORS \
                                                                    : MEMCACHED_SUCCESS))

  size_t min_index= -1;
  size_t max_index= -1;
  bool send_needed= false;

  for (size_t i= 0; i < number_of_req; i++)
  {
    if (verb == CAS_OP and req[i].cas == 0)
    {
      failure_occurred= true;
      rc_arr_ret[i]= MEMCACHED_INVALID_ARGUMENTS;
    }
    else if (memcached_failed(rc= memcached_validate_key_length(req[i].key_length, ptr->flags.binary_protocol)) or
             memcached_failed(rc= memcached_key_test(*ptr, &(req[i].key), &(req[i].key_length), 1)))
    {
      failure_occurred= true;
      rc_arr_ret[i]= rc;
    }
    else
    {
      rc_arr_ret[i]= MEMCACHED_MAXIMUM_RETURN;

      if (send_needed == false)
      {
        send_needed= true;
        min_index= i;
      }
      max_index= i;
    }
  }

  if (send_needed== false)
  {
    return MEMCACHED_FAILURE;
  }

  uint32_t server_key= -1;
  memcached_server_write_instance_st instance= NULL;

#define FETCH_WRITE_INSTANCE_IF_NEEDED() {                                                        \
  if (group_key_present == false)                                                                 \
  {                                                                                               \
    server_key= memcached_generate_hash_with_redistribution(ptr, req[i].key, req[i].key_length);  \
    instance= memcached_server_instance_fetch(ptr, server_key);                                   \
  }                                                                                               \
}

  if (group_key_present == true)
  {
    server_key= memcached_generate_hash_with_redistribution(ptr, group_key, group_key_length);
    instance= memcached_server_instance_fetch(ptr, server_key);
  }

  bool sent_ever= false;
  size_t min_sent_index= -1;
  size_t max_sent_index= -1;

#define FOR_INDEX(from, to) for (size_t i= from; i <= to; i++)

#ifdef ENABLE_REPLICATION
do_action:
#endif

  memcached_server_write_instance_st failed_instances= NULL;
  ptr->flags.bulked= true;

  FOR_INDEX(min_index, max_index)
  {
    if (rc_arr_ret[i] != MEMCACHED_MAXIMUM_RETURN)
    {
      continue;
    }

    FETCH_WRITE_INSTANCE_IF_NEEDED();

    if (instance->send_failed)
    {
      rc_arr_ret[i]= MEMCACHED_FAILURE;
      continue;
    }

    if (ptr->flags.binary_protocol)
    {
      rc= memcached_send_to_binary_server(ptr, server_key, instance, req[i], verb);
    }
    else
    {
      rc= memcached_send_to_ascii_server(ptr, instance, req[i], verb);
    }

    if (memcached_failed(rc))
    {
      failure_occurred= true;
      instance->send_failed= true;
      instance->next_failed= failed_instances;
      failed_instances= instance;

      rc_arr_ret[i]= rc;
    }
    else
    {
      if (sent_ever == false)
      {
        sent_ever= true;
        min_sent_index= i;
      }
      max_sent_index= i;
    }
  }

  while (failed_instances != NULL)
  {
    memcached_server_write_instance_st current= failed_instances;
    failed_instances= current->next_failed;
    current->next_failed= NULL;
    current->send_failed= false;
  }

  if (sent_ever == false)
  {
    return BUILD_RETURN_VALUE();
  }

#ifdef ENABLE_REPLICATION
  bool switchover_needed_ever= false;
  size_t min_switchover_needed_index= -1;
  size_t max_switchover_needed_index= -1;
#endif

  FOR_INDEX(min_sent_index, max_sent_index)
  {
    if (rc_arr_ret[i] != MEMCACHED_MAXIMUM_RETURN)
    {
      continue;
    }

    FETCH_WRITE_INSTANCE_IF_NEEDED();

    if (instance->recv_failed == true)
    {
      rc_arr_ret[i]= MEMCACHED_FAILURE;
      continue;
    }

    if (memcached_server_response_count(instance) == 0)
    {
      failure_occurred= true;
      rc_arr_ret[i]= MEMCACHED_FAILURE;
      continue;
    }

    rc= memcached_recv_from_server(ptr, instance);
    if (rc == MEMCACHED_STORED)
    {
      success_occurred= true;
      rc_arr_ret[i]= MEMCACHED_SUCCESS;
    }
#ifdef ENABLE_REPLICATION
    else if (rc == MEMCACHED_SWITCHOVER or rc == MEMCACHED_REPL_SLAVE)
    {
      instance->switchover_state= return_t_to_switchover_state(rc);

      if (switchover_needed_ever == false)
      {
        switchover_needed_ever= true;
        min_switchover_needed_index= i;
      }
      max_switchover_needed_index= i;
    }
#endif
    else if (memcached_failed(rc_arr_ret[i]= rc))
    {
      failure_occurred= true;
      instance->recv_failed= true;
      instance->next_failed= failed_instances;
      failed_instances= instance;
    }
  }

  while (failed_instances != NULL)
  {
    memcached_server_write_instance_st current= failed_instances;
    failed_instances= current->next_failed;
    current->next_failed= NULL;
    current->recv_failed= false;
  }

  ptr->flags.bulked= false;

#ifdef ENABLE_REPLICATION
  if (switchover_needed_ever == false)
  {
    return BUILD_RETURN_VALUE();
  }

  bool switchover_done_ever= false;
  size_t min_switchover_done_index= -1;
  size_t max_switchover_done_index= -1;

  bool switchover_failed_ever= false;
  size_t min_switchover_failed_index= -1;
  size_t max_switchover_failed_index= -1;

  FOR_INDEX(min_switchover_needed_index, max_switchover_needed_index)
  {
    FETCH_WRITE_INSTANCE_IF_NEEDED();

    if (instance->switchover_state == MEMCACHED_SERVER_SWITCHOVER_NEEDED ||
        instance->switchover_state == MEMCACHED_SERVER_SWITCHOVER_NEEDED_REPL_SLAVE)
    {
      rc= switchover_state_to_return_t(instance->switchover_state);

      ZOO_LOG_INFO(("Switchover: hostname=%s port=%d error=%s",
                    instance->hostname, instance->port, memcached_strerror(ptr, rc)));
      if (memcached_rgroup_switchover(ptr, instance) == true)
      {
        if (switchover_done_ever == false)
        {
          switchover_done_ever= true;
          min_switchover_done_index= i;
        }
        max_switchover_done_index= i;

        instance->switchover_state= MEMCACHED_SERVER_SWITCHOVER_DONE;
      }
      else
      {
        if (switchover_failed_ever== false)
        {
          switchover_failed_ever= true;
          min_switchover_failed_index= i;
        }
        max_switchover_failed_index= i;

        rc_arr_ret[i]= MEMCACHED_FAILURE;
        instance->switchover_state= MEMCACHED_SERVER_SWITCHOVER_FAILED;
      }
    }
    else if (instance->switchover_state == MEMCACHED_SERVER_SWITCHOVER_FAILED)
    {
      rc_arr_ret[i]= MEMCACHED_FAILURE;
    }
  }

  unlikely (switchover_failed_ever)
  {
    FOR_INDEX(min_switchover_failed_index, max_switchover_failed_index)
    {
      FETCH_WRITE_INSTANCE_IF_NEEDED();

      if (instance->switchover_state == MEMCACHED_SERVER_SWITCHOVER_FAILED)
      {
        instance->switchover_state= MEMCACHED_SERVER_SWITCHOVER_DONE;
      }
    }
  }

  if (switchover_done_ever)
  {
    min_index= min_switchover_done_index;
    max_index= max_switchover_done_index;

    min_sent_index= max_sent_index= -1;
    min_switchover_needed_index= max_switchover_needed_index= -1;
    min_switchover_done_index= max_switchover_done_index= -1;
    min_switchover_failed_index= max_switchover_failed_index= -1;

    sent_ever= switchover_needed_ever= switchover_done_ever= switchover_failed_ever= false;
    goto do_action;
  }
#endif

  return BUILD_RETURN_VALUE();
}

#define BUILD_STORAGE_REQUEST(cas) {  \
  (char *) key, key_length,           \
  (char *) value, value_length,       \
  expiration, flags, cas              \
}

memcached_return_t memcached_set(memcached_st *ptr, const char *key, size_t key_length,
                                 const char *value, size_t value_length,
                                 time_t expiration,
                                 uint32_t flags)
{
  memcached_return_t rc;
  memcached_storage_request_st req= BUILD_STORAGE_REQUEST(0);
  LIBMEMCACHED_MEMCACHED_SET_START();
  rc= memcached_storage(ptr, key, key_length, req, SET_OP);
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
memcached_storage_request_st req= BUILD_STORAGE_REQUEST(0);
  LIBMEMCACHED_MEMCACHED_ADD_START();
  rc= memcached_storage(ptr, key, key_length, req, ADD_OP);
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
  memcached_storage_request_st req= BUILD_STORAGE_REQUEST(0);
  LIBMEMCACHED_MEMCACHED_REPLACE_START();
  rc= memcached_storage(ptr, key, key_length, req, REPLACE_OP);
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
  memcached_storage_request_st req= BUILD_STORAGE_REQUEST(0);
  rc= memcached_storage(ptr, key, key_length, req, PREPEND_OP);
  return rc;
}

memcached_return_t memcached_append(memcached_st *ptr,
                                    const char *key, size_t key_length,
                                    const char *value, size_t value_length,
                                    time_t expiration,
                                    uint32_t flags)
{
  memcached_return_t rc;
  memcached_storage_request_st req= BUILD_STORAGE_REQUEST(0);
  rc= memcached_storage(ptr, key, key_length, req, APPEND_OP);
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
  memcached_storage_request_st req= BUILD_STORAGE_REQUEST(cas);
  rc= memcached_storage(ptr, key, key_length, req, CAS_OP);
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
  memcached_storage_request_st req= BUILD_STORAGE_REQUEST(0);
  LIBMEMCACHED_MEMCACHED_SET_START();
  rc= memcached_storage(ptr, group_key, group_key_length, req, SET_OP);
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
  memcached_storage_request_st req= BUILD_STORAGE_REQUEST(0);
  LIBMEMCACHED_MEMCACHED_ADD_START();
  rc= memcached_storage(ptr, group_key, group_key_length, req, ADD_OP);
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
  memcached_storage_request_st req= BUILD_STORAGE_REQUEST(0);
  LIBMEMCACHED_MEMCACHED_REPLACE_START();
  rc= memcached_storage(ptr, group_key, group_key_length, req, REPLACE_OP);
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
  memcached_storage_request_st req= BUILD_STORAGE_REQUEST(0);
  rc= memcached_storage(ptr, group_key, group_key_length, req, PREPEND_OP);
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
  memcached_storage_request_st req= BUILD_STORAGE_REQUEST(0);
  rc= memcached_storage(ptr, group_key, group_key_length, req, APPEND_OP);
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
  memcached_storage_request_st req= BUILD_STORAGE_REQUEST(cas);
  rc= memcached_storage(ptr, group_key, group_key_length, req, CAS_OP);
  return rc;
}

memcached_return_t memcached_mset(memcached_st *ptr,
                                  const memcached_storage_request_st *req,
                                  const size_t number_of_req,
                                  memcached_return_t *rc_arr_ret)
{
  return memcached_mset_by_key(ptr, NULL, 0, req, number_of_req, rc_arr_ret);
}

memcached_return_t memcached_mset_by_key(memcached_st *ptr,
                                         const char *group_key,
                                         const size_t group_key_length,
                                         const memcached_storage_request_st *req,
                                         const size_t number_of_req,
                                         memcached_return_t *rc_arr_ret)
{
  memcached_return_t rc;
  LIBMEMCACHED_MEMCACHED_MSET_START();
  rc= memcached_bulk_storage(ptr, group_key, group_key_length,
                             req, number_of_req, SET_OP, rc_arr_ret);
  LIBMEMCACHED_MEMCACHED_MSET_END();
  return rc;
}
