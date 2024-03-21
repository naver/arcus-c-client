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
 *  Copyright (C) 2006-2009 Brian Aker All rights reserved.
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
#include <libmemcached/string.hpp>

static memcached_return_t textual_value_fetch(memcached_server_write_instance_st ptr,
                                              char *buffer, size_t buffer_length,
                                              memcached_result_st *result)
{
  char *string_ptr;
  char *end_ptr;
  char *next_ptr;
  size_t value_length;
  size_t to_read;
  ssize_t read_length= 0;

  if (ptr->root->flags.use_udp)
    return memcached_set_error(*ptr, MEMCACHED_NOT_SUPPORTED, MEMCACHED_AT);

  WATCHPOINT_ASSERT(ptr->root);
  end_ptr= buffer + buffer_length;

  memcached_result_reset(result);

  string_ptr= buffer;
  string_ptr+= 6; /* "VALUE " */


  /* We load the key */
  {
    char *key;
    size_t prefix_length;

    key= result->item_key;
    result->key_length= 0;

    for (prefix_length= memcached_array_size(ptr->root->_namespace); !(iscntrl(*string_ptr) || isspace(*string_ptr)) ; string_ptr++)
    {
      if (prefix_length == 0)
      {
        *key= *string_ptr;
        key++;
        result->key_length++;
      }
      else
        prefix_length--;
    }
    result->item_key[result->key_length]= 0;
  }

  if (end_ptr == string_ptr)
    goto read_error;

  /* Flags fetch move past space */
  string_ptr++;
  if (end_ptr == string_ptr)
    goto read_error;

  for (next_ptr= string_ptr; isdigit(*string_ptr); string_ptr++) {};
  result->item_flags= (uint32_t) strtoul(next_ptr, &string_ptr, 10);

  if (end_ptr == string_ptr)
    goto read_error;

  /* Length fetch move past space*/
  string_ptr++;
  if (end_ptr == string_ptr)
    goto read_error;

  for (next_ptr= string_ptr; isdigit(*string_ptr); string_ptr++) {};
  value_length= (size_t)strtoull(next_ptr, &string_ptr, 10);

  if (end_ptr == string_ptr)
    goto read_error;

  /* Skip spaces */
  if (*string_ptr == '\r')
  {
    /* Skip past the \r\n */
    string_ptr+= 2;
  }
  else
  {
    string_ptr++;
    for (next_ptr= string_ptr; isdigit(*string_ptr); string_ptr++) {};
    result->item_cas= strtoull(next_ptr, &string_ptr, 10);
  }

  if (end_ptr < string_ptr)
    goto read_error;

  /* We add two bytes so that we can walk the \r\n */
  if (memcached_failed(memcached_string_check(&result->value, value_length +2)))
  {
    return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT);
  }

  {
    char *value_ptr= memcached_string_value_mutable(&result->value);
    /*
      We read the \r\n into the string since not doing so is more
      cycles then the waster of memory to do so.

      We are null terminating through, which will most likely make
      some people lazy about using the return length.
    */
    to_read= (value_length) + 2;
    memcached_return_t rrc= memcached_io_read(ptr, value_ptr, to_read, &read_length);
    if (memcached_failed(rrc) and rrc == MEMCACHED_IN_PROGRESS)
    {
      memcached_quit_server(ptr, true);
      return memcached_set_error(*ptr, MEMCACHED_IN_PROGRESS, MEMCACHED_AT);
    }
    else if (memcached_failed(rrc))
    {
      return rrc;
    }
  }

  if (read_length != (ssize_t)(value_length + 2))
  {
    goto read_error;
  }

  /* This next bit blows the API, but this is internal....*/
  {
    char *char_ptr;
    char_ptr= memcached_string_value_mutable(&result->value);;
    char_ptr[value_length]= 0;
    char_ptr[value_length +1]= 0;
    memcached_string_set_length(&result->value, value_length);
  }

  return MEMCACHED_SUCCESS;

read_error:
  return MEMCACHED_PARTIAL_READ;
}

static memcached_return_t textual_version_fetch(memcached_server_write_instance_st instance, char *buffer)
{
  char *response_ptr= buffer;
  /* UNKNOWN */
  if (*response_ptr == 'U')
  {
    instance->major_version= instance->minor_version= instance->micro_version= UINT8_MAX;
    return MEMCACHED_SUCCESS;
  }

  /* parse version */
  char *end_ptr;
  errno= 0;
  long int version= strtol(response_ptr, &end_ptr, 10);
  if (errno != 0 || version == LONG_MIN || version == LONG_MAX || version > UINT8_MAX)
  {
    return memcached_set_error(*instance, MEMCACHED_UNKNOWN_READ_FAILURE, MEMCACHED_AT,
                               memcached_literal_param("strtol() failed to parse major version"));
  }
  instance->major_version= uint8_t(version);

  end_ptr++;
  errno= 0;
  version= strtol(end_ptr, &end_ptr, 10);
  if (errno != 0 || version == LONG_MIN || version == LONG_MAX || version > UINT8_MAX)
  {
    instance->major_version= UINT8_MAX;
    return memcached_set_error(*instance, MEMCACHED_UNKNOWN_READ_FAILURE, MEMCACHED_AT,
                               memcached_literal_param("strtol() failed to parse minor version"));
  }
  instance->minor_version= uint8_t(version);

  end_ptr++;
  errno= 0;
  version= strtol(end_ptr, &end_ptr, 10);
  if (errno != 0 || version == LONG_MIN || version == LONG_MAX || version > UINT8_MAX)
  {
    instance->major_version= instance->minor_version= UINT8_MAX;
    return memcached_set_error(*instance, MEMCACHED_UNKNOWN_READ_FAILURE, MEMCACHED_AT,
                               memcached_literal_param("strtol() failed to parse micro version"));
  }
  instance->micro_version= uint8_t(version);
  instance->is_enterprise= (strrchr(response_ptr, 'E') == NULL) ? false : true;

  return MEMCACHED_SUCCESS;
}

#ifdef ENABLE_REPLICATION
static void textual_switchover_peer_check(memcached_server_write_instance_st instance, char *buffer)
{
  if (memcmp(buffer, " ", 1) != 0) {
    instance->switchover_sidx= 1; /* set first slave */
    return;
  }

  int str_length;
  int buf_length;
  char *startptr= buffer+1;
  char *endptr= startptr;
  while (*endptr != '\r' && *endptr != '\n') endptr++;

  str_length = (endptr - startptr);
  buf_length = sizeof(instance->switchover_peer); /* currently 128 */

  if (str_length < buf_length) {
    /* OK */
    memcpy(instance->switchover_peer, startptr, str_length);
    instance->switchover_peer[str_length]= '\0';
    instance->switchover_sidx= -1; /* undefined */
  } else {
    /* something is wrong */
    memcpy(instance->switchover_peer, startptr, buf_length-1);
    instance->switchover_peer[buf_length-1]= '\0';
    instance->switchover_sidx= 1; /* set first slave */
  }
}
#endif

static memcached_return_t textual_read_one_response(memcached_server_write_instance_st ptr,
                                                    char *buffer, size_t buffer_length,
                                                    memcached_result_st *result)
{
  size_t total_read;
  memcached_return_t rc= memcached_io_readline(ptr, buffer, buffer_length, total_read);

  if (memcached_failed(rc))
  {
    return rc;
  }

  switch(buffer[0])
  {
  case 'V':
    if (memcmp(buffer, "VALUE", 5) == 0)
    {
      /* We add back in one because we will need to search for END */
      memcached_server_response_increment(ptr);
      return textual_value_fetch(ptr, buffer, buffer_length, result);
    }
    else if (memcmp(buffer, "VERSION", 7) == 0)
    {
      /* Find the space, and then move one past it to copy version */
      char *version_ptr= index(buffer, ' ');
      version_ptr++;
      return textual_version_fetch(ptr, version_ptr);
    }
    break;

  case 'O':
    if (memcmp(buffer, "OK", 2) == 0)
    {
      return MEMCACHED_SUCCESS;
    }
    break;

  case 'S':
    if (memcmp(buffer, "STAT", 4) == 0) /* STORED STATS */
    {
      memcached_server_response_increment(ptr);
      return MEMCACHED_STAT;
    }
    else if (memcmp(buffer, "SERVER_ERROR", 12) == 0)
    {
      if (total_read == memcached_literal_param_size("SERVER_ERROR"))
      {
        return MEMCACHED_SERVER_ERROR;
      }

      // ensure compatibility. old cache server returns this error as SERVER_ERROR
      if (total_read > memcached_literal_param_size("SERVER_ERROR object too large for cache") and
          (memcmp(buffer, memcached_literal_param("SERVER_ERROR object too large for cache")) == 0))
      {
        return MEMCACHED_E2BIG;
      }

      // Move past the basic error message and whitespace
      char *startptr= buffer + memcached_literal_param_size("SERVER_ERROR");
      if (startptr[0] == ' ')
      {
        startptr++;
      }

      char *endptr= startptr;
      while (*endptr != '\r' && *endptr != '\n') endptr++;

      return memcached_set_error(*ptr, MEMCACHED_SERVER_ERROR, MEMCACHED_AT,
                                 startptr, size_t(endptr - startptr));
    }
    else if (memcmp(buffer, "STORED", 6) == 0)
    {
      return MEMCACHED_STORED;
    }
#ifdef ENABLE_REPLICATION
    else if (memcmp(buffer, "SWITCHOVER", 10) == 0)
    {
      textual_switchover_peer_check(ptr, &buffer[10]);
      return MEMCACHED_SWITCHOVER;
    }
#endif
    break;

  case 'D':
    if (memcmp(buffer, "DELETED", 7) == 0)
    {
      return MEMCACHED_DELETED;
    }
    break;

  case 'N':
    if (memcmp(buffer, "NOT_FOUND", 9) == 0)
    {
      return MEMCACHED_NOTFOUND;
    }
    else if (memcmp(buffer, "NOT_STORED", 10) == 0)
    {
      return MEMCACHED_NOTSTORED;
    }
    else if (memcmp(buffer, "NOT_SUPPORTED", 13) == 0)
    {
      return MEMCACHED_NOT_SUPPORTED;
    }
    break;

  case 'R':
#ifdef ENABLE_REPLICATION
    if (memcmp(buffer, "REPL_SLAVE", 10) == 0)
    {
      textual_switchover_peer_check(ptr, &buffer[10]);
      return MEMCACHED_REPL_SLAVE;
    }
#endif
    break;

  case 'U':
    if (memcmp(buffer, "UNREADABLE", 10) == 0)
    {
      return MEMCACHED_UNREADABLE;
    }
    break;

  case 'E':
    if (memcmp(buffer, "END", 3) == 0)
    {
      return MEMCACHED_END;
    }
    else if (memcmp(buffer, "ERROR", 5) == 0)
    {
      // Move past the basic error message and whitespace
      char *startptr= buffer + memcached_literal_param_size("ERROR");
      if (startptr[0] == ' ')
      {
        startptr++;
      }

      char *endptr= startptr;
      while (*endptr != '\r' && *endptr != '\n') endptr++;

      return memcached_set_error(*ptr, MEMCACHED_PROTOCOL_ERROR, MEMCACHED_AT,
                                 startptr, size_t(endptr - startptr));
    }
    else if (memcmp(buffer, "EXISTS", 6) == 0)
    {
      return MEMCACHED_DATA_EXISTS;
    }
    break;

  case 'I':
    if (memcmp(buffer, "ITEM", 4) == 0) /* ITEM STATS */
    {
      /* We add back in one because we will need to search for END */
      memcached_server_response_increment(ptr);
      return MEMCACHED_ITEM;
    }
    break;

  case 'T': /* TYPE MISMATCH */
    if (memcmp(buffer, "TYPE_MISMATCH", 13) == 0)
    {
      return MEMCACHED_TYPE_MISMATCH;
    }
    break;

  case 'C': /* CLIENT ERROR */
    if (memcmp(buffer, "CLIENT_ERROR", 12) == 0)
    {
      if (total_read > memcached_literal_param_size("CLIENT_ERROR object too large for cache") and
          (memcmp(buffer, memcached_literal_param("CLIENT_ERROR object too large for cache")) == 0))
      {
        return MEMCACHED_E2BIG;
      }

      // Move past the basic error message and whitespace
      char *startptr= buffer + memcached_literal_param_size("CLIENT_ERROR");
      if (startptr[0] == ' ')
      {
        startptr++;
      }

      char *endptr= startptr;
      while (*endptr != '\r' && *endptr != '\n') endptr++;

      return memcached_set_error(*ptr, MEMCACHED_CLIENT_ERROR, MEMCACHED_AT,
                                 startptr, size_t(endptr - startptr));
    }
    break;

  default:
    {
      unsigned long long auto_return_value;
      if (sscanf(buffer, "%llu", &auto_return_value) == 1)
        return MEMCACHED_SUCCESS;
    }
    break;
  }

  WATCHPOINT_STRING(buffer);
  return MEMCACHED_UNKNOWN_READ_FAILURE;
}

static memcached_return_t binary_read_one_response(memcached_server_write_instance_st ptr,
                                                   char *buffer, size_t buffer_length,
                                                   memcached_result_st *result)
{
  memcached_return_t rc;
  protocol_binary_response_header header;

  if ((rc= memcached_safe_read(ptr, &header.bytes, sizeof(header.bytes))) != MEMCACHED_SUCCESS)
  {
    WATCHPOINT_ERROR(rc);
    return rc;
  }

  if (header.response.magic != PROTOCOL_BINARY_RES)
  {
    return MEMCACHED_PROTOCOL_ERROR;
  }

  /*
   ** Convert the header to host local endian!
 */
  header.response.keylen= ntohs(header.response.keylen);
  header.response.status= ntohs(header.response.status);
  header.response.bodylen= ntohl(header.response.bodylen);
  header.response.cas= memcached_ntohll(header.response.cas);
  uint32_t bodylen= header.response.bodylen;

  if (header.response.status == PROTOCOL_BINARY_RESPONSE_SUCCESS or
      header.response.status == PROTOCOL_BINARY_RESPONSE_AUTH_CONTINUE)
  {
    switch (header.response.opcode)
    {
    case PROTOCOL_BINARY_CMD_GETKQ:
      /*
       * We didn't increment the response counter for the GETKQ packet
       * (only the final NOOP), so we need to increment the counter again.
       */
      memcached_server_response_increment(ptr);
      /* FALLTHROUGH */
    case PROTOCOL_BINARY_CMD_GETK:
      {
        uint16_t keylen= header.response.keylen;
        memcached_result_reset(result);
        result->item_cas= header.response.cas;

        if ((rc= memcached_safe_read(ptr, &result->item_flags, sizeof (result->item_flags))) != MEMCACHED_SUCCESS)
        {
          WATCHPOINT_ERROR(rc);
          return MEMCACHED_UNKNOWN_READ_FAILURE;
        }

        result->item_flags= ntohl(result->item_flags);
        bodylen -= header.response.extlen;

        result->key_length= keylen;
        if (memcached_failed(rc= memcached_safe_read(ptr, result->item_key, keylen)))
        {
          WATCHPOINT_ERROR(rc);
          return MEMCACHED_UNKNOWN_READ_FAILURE;
        }

        // Only bother with doing this if key_length > 0
        if (result->key_length)
        {
          if (memcached_array_size(ptr->root->_namespace) and memcached_array_size(ptr->root->_namespace) >= result->key_length)
          {
            return memcached_set_error(*ptr, MEMCACHED_UNKNOWN_READ_FAILURE, MEMCACHED_AT);
          }

          if (memcached_array_size(ptr->root->_namespace))
          {
            result->key_length-= memcached_array_size(ptr->root->_namespace);
            memmove(result->item_key, result->item_key +memcached_array_size(ptr->root->_namespace), result->key_length);
          }
        }

        bodylen -= keylen;
        if (memcached_failed(memcached_string_check(&result->value, bodylen)))
        {
          return MEMCACHED_MEMORY_ALLOCATION_FAILURE;
        }

        char *vptr= memcached_string_value_mutable(&result->value);
        if (memcached_failed(rc= memcached_safe_read(ptr, vptr, bodylen)))
        {
          WATCHPOINT_ERROR(rc);
          return MEMCACHED_UNKNOWN_READ_FAILURE;
        }

        memcached_string_set_length(&result->value, bodylen);
      }
      break;

    case PROTOCOL_BINARY_CMD_INCREMENT:
    case PROTOCOL_BINARY_CMD_DECREMENT:
      {
        if (bodylen != sizeof(uint64_t) || buffer_length != sizeof(uint64_t))
        {
          return MEMCACHED_PROTOCOL_ERROR;
        }

        WATCHPOINT_ASSERT(bodylen == buffer_length);
        uint64_t val;
        if ((rc= memcached_safe_read(ptr, &val, sizeof(val))) != MEMCACHED_SUCCESS)
        {
          WATCHPOINT_ERROR(rc);
          return MEMCACHED_UNKNOWN_READ_FAILURE;
        }

        val= memcached_ntohll(val);
        memcpy(buffer, &val, sizeof(val));
      }
      break;

    case PROTOCOL_BINARY_CMD_SASL_LIST_MECHS:
    case PROTOCOL_BINARY_CMD_VERSION:
      {
        char version_buffer[32];
        memset(version_buffer, 0, sizeof(version_buffer));
        if (memcached_safe_read(ptr, version_buffer, bodylen) != MEMCACHED_SUCCESS)
        {
          return MEMCACHED_UNKNOWN_READ_FAILURE;
        }
        rc= textual_version_fetch(ptr, version_buffer);
      }
      break;
    case PROTOCOL_BINARY_CMD_FLUSH:
    case PROTOCOL_BINARY_CMD_QUIT:
    case PROTOCOL_BINARY_CMD_SET:
    case PROTOCOL_BINARY_CMD_ADD:
    case PROTOCOL_BINARY_CMD_REPLACE:
    case PROTOCOL_BINARY_CMD_APPEND:
    case PROTOCOL_BINARY_CMD_PREPEND:
    case PROTOCOL_BINARY_CMD_DELETE:
      {
        WATCHPOINT_ASSERT(bodylen == 0);
        return MEMCACHED_SUCCESS;
      }
    case PROTOCOL_BINARY_CMD_NOOP:
      {
        WATCHPOINT_ASSERT(bodylen == 0);
        return MEMCACHED_END;
      }
    case PROTOCOL_BINARY_CMD_STAT:
      {
        if (bodylen == 0)
        {
          return MEMCACHED_END;
        }
        else if (bodylen + 1 > buffer_length)
        {
          /* not enough space in buffer.. should not happen... */
          return MEMCACHED_UNKNOWN_READ_FAILURE;
        }
        else
        {
          size_t keylen= header.response.keylen;
          memset(buffer, 0, buffer_length);
          if ((rc= memcached_safe_read(ptr, buffer, keylen)) != MEMCACHED_SUCCESS ||
              (rc= memcached_safe_read(ptr, buffer + keylen + 1, bodylen - keylen)) != MEMCACHED_SUCCESS)
          {
            WATCHPOINT_ERROR(rc);
            return MEMCACHED_UNKNOWN_READ_FAILURE;
          }
        }
      }
      break;

    case PROTOCOL_BINARY_CMD_SASL_AUTH:
    case PROTOCOL_BINARY_CMD_SASL_STEP:
      {
        memcached_result_reset(result);
        result->item_cas= header.response.cas;

        if (memcached_string_check(&result->value,
                                   bodylen) != MEMCACHED_SUCCESS)
          return MEMCACHED_MEMORY_ALLOCATION_FAILURE;

        char *vptr= memcached_string_value_mutable(&result->value);
        if ((rc= memcached_safe_read(ptr, vptr, bodylen)) != MEMCACHED_SUCCESS)
        {
          WATCHPOINT_ERROR(rc);
          return MEMCACHED_UNKNOWN_READ_FAILURE;
        }

        memcached_string_set_length(&result->value, bodylen);
      }
      break;
    default:
      {
        /* Command not implemented yet! */
        WATCHPOINT_ASSERT(0);
        return MEMCACHED_PROTOCOL_ERROR;
      }
    }
  }
  else if (header.response.bodylen)
  {
    /* What should I do with the error message??? just discard it for now */
    char hole[SMALL_STRING_LEN];
    while (bodylen > 0)
    {
      size_t nr= (bodylen > SMALL_STRING_LEN) ? SMALL_STRING_LEN : bodylen;
      if ((rc= memcached_safe_read(ptr, hole, nr)) != MEMCACHED_SUCCESS)
      {
        WATCHPOINT_ERROR(rc);
        return memcached_set_error(*ptr, MEMCACHED_UNKNOWN_READ_FAILURE, MEMCACHED_AT);
      }
      bodylen-= (uint32_t) nr;
    }

    /* This might be an error from one of the quiet commands.. if
     * so, just throw it away and get the next one. What about creating
     * a callback to the user with the error information?
   */
    switch (header.response.opcode)
    {
    case PROTOCOL_BINARY_CMD_SETQ:
    case PROTOCOL_BINARY_CMD_ADDQ:
    case PROTOCOL_BINARY_CMD_REPLACEQ:
    case PROTOCOL_BINARY_CMD_APPENDQ:
    case PROTOCOL_BINARY_CMD_PREPENDQ:
      return binary_read_one_response(ptr, buffer, buffer_length, result);

    default:
      break;
    }
  }

  rc= MEMCACHED_SUCCESS;
  if (header.response.status != 0)
  {
    switch (header.response.status)
    {
    case PROTOCOL_BINARY_RESPONSE_KEY_ENOENT:
      rc= MEMCACHED_NOTFOUND;
      break;

    case PROTOCOL_BINARY_RESPONSE_KEY_EEXISTS:
      rc= MEMCACHED_DATA_EXISTS;
      break;

    case PROTOCOL_BINARY_RESPONSE_NOT_STORED:
      rc= MEMCACHED_NOTSTORED;
      break;

    case PROTOCOL_BINARY_RESPONSE_NOT_SUPPORTED:
      rc= MEMCACHED_NOT_SUPPORTED;
      break;

    case PROTOCOL_BINARY_RESPONSE_E2BIG:
      rc= MEMCACHED_E2BIG;
      break;

    case PROTOCOL_BINARY_RESPONSE_ENOMEM:
      rc= MEMCACHED_MEMORY_ALLOCATION_FAILURE;
      break;

    case PROTOCOL_BINARY_RESPONSE_AUTH_CONTINUE:
      rc= MEMCACHED_AUTH_CONTINUE;
      break;

    case PROTOCOL_BINARY_RESPONSE_AUTH_ERROR:
      rc= MEMCACHED_AUTH_FAILURE;
      break;

    case PROTOCOL_BINARY_RESPONSE_EINVAL:
    case PROTOCOL_BINARY_RESPONSE_UNKNOWN_COMMAND:
    default:
      /* @todo fix the error mappings */
      rc= MEMCACHED_PROTOCOL_ERROR;
      break;
    }
  }
  return rc;
}

memcached_return_t memcached_read_one_response(memcached_server_write_instance_st ptr,
                                               char *buffer, size_t buffer_length,
                                               memcached_result_st *result)
{
  memcached_server_response_decrement(ptr);

  if (result == NULL)
  {
    memcached_st *root= (memcached_st *)ptr->root;
    result = &root->result;
  }

  memcached_return_t rc;
  if (ptr->root->flags.binary_protocol)
  {
    rc= binary_read_one_response(ptr, buffer, buffer_length, result);
  }
  else
  {
    rc= textual_read_one_response(ptr, buffer, buffer_length, result);
  }
#ifdef IMMEDIATELY_RECONNECT
  if (rc == MEMCACHED_UNKNOWN_READ_FAILURE or
      rc == MEMCACHED_PROTOCOL_ERROR or
      rc == MEMCACHED_CLIENT_ERROR or
      rc == MEMCACHED_SERVER_ERROR or
      rc == MEMCACHED_PARTIAL_READ)
  {
    memcached_server_set_immediate_reconnect(ptr);
  }
#endif

#ifdef IMMEDIATELY_RECONNECT
  unlikely(rc == MEMCACHED_UNKNOWN_READ_FAILURE or
           rc == MEMCACHED_PROTOCOL_ERROR or
           rc == MEMCACHED_CLIENT_ERROR or
           rc == MEMCACHED_SERVER_ERROR or
           rc == MEMCACHED_PARTIAL_READ or
           rc == MEMCACHED_MEMORY_ALLOCATION_FAILURE)
#else
  unlikely(rc == MEMCACHED_UNKNOWN_READ_FAILURE or
           rc == MEMCACHED_PROTOCOL_ERROR or
           rc == MEMCACHED_CLIENT_ERROR or
           rc == MEMCACHED_PARTIAL_READ or
           rc == MEMCACHED_MEMORY_ALLOCATION_FAILURE)
#endif
    memcached_io_reset(ptr);

  return rc;
}

memcached_return_t memcached_response(memcached_server_write_instance_st ptr,
                                      char *buffer, size_t buffer_length,
                                      memcached_result_st *result)
{
  /* We may have old commands in the buffer not set, first purge */
  if ((ptr->root->flags.no_block) && (memcached_is_processing_input(ptr->root) == false))
  {
    (void)memcached_io_write(ptr, NULL, 0, true);
  }

  /*
   * The previous implementation purged all pending requests and just
   * returned the last one. Purge all pending messages to ensure backwards
   * compatibility.
 */
  if (ptr->root->flags.binary_protocol == false)
  {
    while (memcached_server_response_count(ptr) > 1)
    {
      memcached_return_t rc= memcached_read_one_response(ptr, buffer, buffer_length, result);

      unlikely (rc != MEMCACHED_END              and
                rc != MEMCACHED_STORED           and
                rc != MEMCACHED_SUCCESS          and
                rc != MEMCACHED_STAT             and
                rc != MEMCACHED_DELETED          and
                rc != MEMCACHED_NOTFOUND         and
                rc != MEMCACHED_TYPE_MISMATCH    and
                rc != MEMCACHED_NOTFOUND_ELEMENT and
                rc != MEMCACHED_UNREADABLE       and
                rc != MEMCACHED_NOTSTORED        and
                rc != MEMCACHED_NOT_SUPPORTED    and
                rc != MEMCACHED_DATA_EXISTS )
        return rc;
    }
  }

  return memcached_read_one_response(ptr, buffer, buffer_length, result);
}

/*
 * Support Collections
 */
#define MAX_UINT32_STRING_LENGTH (10+1) /* We add one to have it null terminated */

static bool parse_response_header(char *buffer,
                                  const char *header __attribute__((unused)),
                                  size_t header_length,
                                  uint32_t *values,
                                  size_t value_length)
{
  char *string_ptr;
  char *end_ptr;
  char *next_ptr;
  size_t i= 0;

  end_ptr= buffer + MEMCACHED_DEFAULT_COMMAND_SIZE;

  string_ptr= buffer;
  string_ptr+= header_length;

  for (i=0; i<value_length; i++)
  {
    /* fetch move past space */
    string_ptr++;
    if (end_ptr == string_ptr)
      return false;

    for (next_ptr= string_ptr; isdigit(*string_ptr); string_ptr++) {};
    values[i]= (uint32_t) strtoul(next_ptr, &string_ptr, 10);

    if (end_ptr == string_ptr)
      return false;
  }

  /* Skip spaces */
  if (*string_ptr == '\r')
  {
    /* Skip past the \r\n */
    string_ptr+= 2;
  }

  if (end_ptr < string_ptr)
    return false;

  return true;
}

static bool parse_response_uint32_value(char *buffer, int length,
                                        int value_count, uint32_t *value_array)
{
  char *str_ptr= buffer;
  char *end_ptr= buffer + length;
  char *val_ptr;

  for (int i=0; i<value_count; i++)
  {
    /* skip a space */
    if (*str_ptr != ' ')
      return false;
    str_ptr++;

   /* fetch uint32 value */
    val_ptr= str_ptr;
    for ( ; str_ptr < end_ptr && isdigit(*str_ptr); str_ptr++) {};
    if ((str_ptr == val_ptr) || (str_ptr >= end_ptr) ||
        (str_ptr - val_ptr) >= MAX_UINT32_STRING_LENGTH)
      return false;
    value_array[i]= (uint32_t) strtoul(val_ptr, &str_ptr, 10);
  }

  /* Skip spaces */
  if (*str_ptr == '\r' && (str_ptr+1) < end_ptr && *(str_ptr+1) == '\n')
  {
    str_ptr+= 2; /* Skip past the \r\n */
  }
  return true;
}

static bool parse_response_string_value(char *buffer, int length,
                                        char **string_ptr, int *string_len)
{
  if (buffer[0] != ' ')
    return false;

  for (int i=1; i<length; i++)
  {
    if (buffer[i] == ' ' || buffer[i] == '\r')
    {
      if (i == 1) break;

      *string_ptr = &buffer[1];
      *string_len = i-1;
      return true;
    }
  }
  return false;
}

static memcached_return_t get_status_of_bop_mget_response(char *string_ptr, int string_len)
{
  switch (string_ptr[0])
  {
  case 'O':
    if (string_len == 2 && memcmp(string_ptr, "OK", string_len) == 0)
      return MEMCACHED_SUCCESS;
    if (string_len == 12 && memcmp(string_ptr, "OUT_OF_RANGE", string_len) == 0)
      return MEMCACHED_OUT_OF_RANGE;
    break;
  case 'N':
    if (string_len == 9 && memcmp(string_ptr, "NOT_FOUND", string_len) == 0)
      return MEMCACHED_NOTFOUND;
    if (string_len == 17 && memcmp(string_ptr, "NOT_FOUND_ELEMENT", string_len) == 0)
      return MEMCACHED_NOTFOUND_ELEMENT;
    break;
  case 'T':
    if (string_len == 7 && memcmp(string_ptr, "TRIMMED", string_len) == 0)
      return MEMCACHED_TRIMMED;
    if (string_len == 13 && memcmp(string_ptr, "TYPE_MISMATCH", string_len) == 0)
      return MEMCACHED_TYPE_MISMATCH;
    break;
  case 'B':
    if (string_len == 13 && memcmp(string_ptr, "BKEY_MISMATCH", string_len) == 0)
      return MEMCACHED_BKEY_MISMATCH;
    break;
  case 'U':
    if (string_len == 10 && memcmp(string_ptr, "UNREADABLE", string_len) == 0)
      return MEMCACHED_UNREADABLE;
    break;
  default:
    break;
  }
  return MEMCACHED_UNKNOWN_READ_FAILURE;
}

static memcached_return_t get_status_of_coll_pipe_response(memcached_server_write_instance_st ptr,
                                                           char *string_ptr, int string_len)
{
  switch (string_ptr[0])
  {
    case 'P':
      if (memcmp(string_ptr, "PIPE_ERROR", 10) == 0)
      {
        if (string_len == 27 && memcmp(string_ptr + 10, " command overflow", string_len - 10) == 0)
          return MEMCACHED_PIPE_ERROR_COMMAND_OVERFLOW;
        else if (string_len == 26 && memcmp(string_ptr + 10, " memory overflow", string_len - 10) == 0)
          return MEMCACHED_PIPE_ERROR_MEMORY_OVERFLOW;
        else if (string_len == 20 && memcmp(string_ptr + 10, " bad error", string_len - 10) == 0)
          return MEMCACHED_PIPE_ERROR_BAD_ERROR;
      }
      break;

    case 'O':
      if (string_len == 12 && memcmp(string_ptr, "OUT_OF_RANGE", string_len) == 0)
        return MEMCACHED_OUT_OF_RANGE;
      else if (string_len == 10 && memcmp(string_ptr, "OVERFLOWED", string_len) == 0)
        return MEMCACHED_OVERFLOWED;
      break;

    case 'S':
      if (string_len == 6 && memcmp(string_ptr, "STORED", string_len) == 0)
        return MEMCACHED_STORED;
#ifdef ENABLE_REPLICATION
      else if (string_len >= 10 && memcmp(string_ptr, "SWITCHOVER", string_len) == 0)
      {
        textual_switchover_peer_check(ptr, &string_ptr[10]);
        return MEMCACHED_SWITCHOVER;
      }
#endif
      /* error message will follow after SERVER_ERROR */
      else if (string_len > 12 && memcmp(string_ptr, "SERVER_ERROR", 12) == 0)
      {
        // Move past the basic error message and whitespace
        char *startptr= string_ptr + memcached_literal_param_size("SERVER_ERROR");
        if (startptr[0] == ' ')
        {
          startptr++;
        }

        char *endptr= startptr;
        while (*endptr != '\r' && *endptr != '\n') endptr++;

        return memcached_set_error(*ptr, MEMCACHED_SERVER_ERROR, MEMCACHED_AT,
                                   startptr, size_t(endptr - startptr));
      }
      break;

    case 'D':
      if (string_len == 7 && memcmp(string_ptr, "DELETED", string_len) == 0)
        return MEMCACHED_DELETED;
      else if (string_len == 15 && memcmp(string_ptr, "DELETED_DROPPED", string_len) == 0)
        return MEMCACHED_DELETED_DROPPED;
      break;

    case 'N':
      if (string_len == 17 && memcmp(string_ptr, "NOT_FOUND_ELEMENT", string_len) == 0)
        return MEMCACHED_NOTFOUND_ELEMENT;
      else if (string_len == 9 && memcmp(string_ptr, "NOT_FOUND", string_len) == 0)
        return MEMCACHED_NOTFOUND;
      else if (string_len == 13 && memcmp(string_ptr, "NOT_SUPPORTED", string_len) == 0)
        return MEMCACHED_NOT_SUPPORTED;
      else if (string_len == 9 && memcmp(string_ptr, "NOT_EXIST", string_len) == 0)
        return MEMCACHED_NOT_EXIST;
      break;

    case 'R':
      if (string_len == 8 && memcmp(string_ptr, "REPLACED", string_len) == 0)
        /* REPLACED in response to bop upsert. */
        return MEMCACHED_REPLACED;
#ifdef ENABLE_REPLICATION
      else if (string_len >= 10 && memcmp(string_ptr, "REPL_SLAVE", string_len) == 0)
      {
        textual_switchover_peer_check(ptr, &string_ptr[10]);
        return MEMCACHED_REPL_SLAVE;
      }
#endif
      break;

    case 'U':
      if (string_len == 7 && memcmp(string_ptr, "UPDATED", string_len) == 0)
        return MEMCACHED_UPDATED;
      else if (string_len == 10 && memcmp(string_ptr, "UNREADABLE", string_len) == 0)
        return MEMCACHED_UNREADABLE;
      break;

    case 'E':
      if (string_len == 3 && memcmp(string_ptr, "END", string_len) == 0)
        return MEMCACHED_END;
      else if (string_len == 14 && memcmp(string_ptr, "EFLAG_MISMATCH", string_len) == 0)
        return MEMCACHED_EFLAG_MISMATCH;
      else if (string_len == 5 && memcmp(string_ptr, "EXIST", string_len) == 0)
        return MEMCACHED_EXIST; /* COLLECTION SET MEMBERSHIP CHECK */
      else if (string_len == 14 && memcmp(string_ptr, "ELEMENT_EXISTS", string_len) == 0)
        return MEMCACHED_ELEMENT_EXISTS;
      /* error message will follow after ERROR */
      else if (string_len > 5 && memcmp(string_ptr, "ERROR", 5) == 0)
      {
        // Move past the basic error message and whitespace
        char *startptr= string_ptr + memcached_literal_param_size("ERROR");
        if (startptr[0] == ' ')
        {
          startptr++;
        }

        char *endptr= startptr;
        while (*endptr != '\r' && *endptr != '\n') endptr++;

        return memcached_set_error(*ptr, MEMCACHED_PROTOCOL_ERROR, MEMCACHED_AT,
                                   startptr, size_t(endptr - startptr));
      }
      break;

    case 'T':
      if (string_len == 13 && memcmp(string_ptr, "TYPE_MISMATCH", string_len) == 0)
        return MEMCACHED_TYPE_MISMATCH;
      break;

    case 'C':
      /* error message will follow after CLIENT_ERROR */
      if (string_len > 12 && memcmp(string_ptr, "CLIENT_ERROR", 12) == 0)
      {
        // Move past the basic error message and whitespace
        char *startptr= string_ptr + memcached_literal_param_size("CLIENT_ERROR");
        if (startptr[0] == ' ')
        {
          startptr++;
        }

        char *endptr= startptr;
        while (*endptr != '\r' && *endptr != '\n') endptr++;

        return memcached_set_error(*ptr, MEMCACHED_CLIENT_ERROR, MEMCACHED_AT,
                                   startptr, size_t(endptr - startptr));
      }
      else if (string_len == 14 && memcmp(string_ptr, "CREATED_STORED", string_len) == 0)
        return MEMCACHED_CREATED_STORED;
      break;

    case 'B':
      if (string_len == 13 && memcmp(string_ptr, "BKEY_MISMATCH", string_len) == 0)
        return MEMCACHED_BKEY_MISMATCH;
      break;

    default:
      break;
  }

  return MEMCACHED_UNKNOWN_READ_FAILURE;
}

static memcached_return_t fetch_value_header(memcached_server_write_instance_st ptr,
                                             char *string, ssize_t *string_length,
                                             size_t max_read_length)
{
  memcached_return_t rc;
  ssize_t read_length= 0;
  bool met_CR_char = false; /* met the `\r` */

  /* Read until meeting a space */
  for (size_t i=0; i<max_read_length; i++)
  {
    rc= memcached_io_read(ptr, string+i, 1, &read_length);
    if (memcached_failed(rc))
    {
      if (rc == MEMCACHED_IN_PROGRESS) {
        memcached_quit_server(ptr, true);
        rc = memcached_set_error(*ptr, MEMCACHED_IN_PROGRESS, MEMCACHED_AT);
      }
      return rc;
    }

    /* met a space */
    if (string[i] == ' ')
    {
      string[i] = '\0';
      *string_length= i+1;
      return MEMCACHED_SUCCESS;
    }

    /* met the "\r\n" */
    if (string[i] == '\r')
    {
      if (met_CR_char)
        break;

      met_CR_char = true;
      i--;
    }
    else if (met_CR_char)
    {
      if (string[i] != '\n')
        break;

      string[i] = '\0';
      *string_length= i+1;
      return MEMCACHED_END; /* the end of line */
    }
  }

  return MEMCACHED_PROTOCOL_ERROR;
}

static memcached_return_t textual_coll_element_fetch(memcached_server_write_instance_st ptr,
                                                     const char *header __attribute__((unused)), ssize_t header_size,
                                                     size_t count, memcached_coll_result_st *result)
{
  memcached_return_t rc;
  const size_t MAX_ELEMENT_BUFFER_SIZE= MEMCACHED_COLL_MAX_MOP_MKEY_LENG+1; /* add one to have it null terminated */

  char *string_ptr;
  size_t i= 0, to_read= 0, value_length= 0;
  ssize_t read_length= 0;

  char to_read_string[MAX_ELEMENT_BUFFER_SIZE];
  bool has_eflag= false;

  for (i=0; i<count; i++)
  {
    has_eflag= false;

    /* Moves past the element header if it exists */
    if (header_size > 0)
    {
      rc= fetch_value_header(ptr, to_read_string, &read_length, header_size+1);
      if (rc != MEMCACHED_SUCCESS) {
        if (rc == MEMCACHED_END) /* "\r\n" is found */
          rc= MEMCACHED_PROTOCOL_ERROR;
        return rc;
      }

      if (read_length != header_size + 1) // +1 for a space
        goto read_error;
    }

    /* B+Tree value starts with <bkey> and <eflag> */
    if (result->type == COLL_BTREE)
    {
      /* <bkey> */
      {
        rc= fetch_value_header(ptr, to_read_string, &read_length, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH+2); // with '0x'
        if (rc != MEMCACHED_SUCCESS) {
          if (rc == MEMCACHED_END) /* "\r\n" is found */
            rc= MEMCACHED_PROTOCOL_ERROR;
          return rc;
        }

        /* byte array bkey : starts with 0x */
        if (to_read_string[0] == '0' && to_read_string[1] == 'x')
        {
          memcached_conv_str_to_hex(ptr->root, to_read_string+2, read_length-2-1, &result->sub_keys[i].bkey_ext); // except '0x' and '\0'
          result->sub_key_type = MEMCACHED_COLL_QUERY_BOP_EXT;
        }
        /* normal bkey */
        else
        {
          result->sub_keys[i].bkey = strtoull(to_read_string, &string_ptr, 10);
          result->sub_key_type = MEMCACHED_COLL_QUERY_BOP;
        }
      }

      /* <eflag> - optional */
      {
        rc= fetch_value_header(ptr, to_read_string, &read_length, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH+2); // with '0x'
        if (rc != MEMCACHED_SUCCESS) {
          if (rc == MEMCACHED_END) /* "\r\n" is found */
            rc= MEMCACHED_PROTOCOL_ERROR;
          return rc;
        }

        /* if the number starts with 0x, it is EFLAG */
        if (to_read_string[0] == '0' && to_read_string[1] == 'x')
        {
          memcached_conv_str_to_hex(ptr->root, to_read_string+2, read_length-2-1, &result->eflags[i]); // except '0x' and '\0'
          has_eflag= true;
        }
      }
    }

    /* Map value starts with <mkey> */
    if (result->type == COLL_MAP)
    {
      /* <mkey> */
      rc= fetch_value_header(ptr, to_read_string, &read_length, MAX_ELEMENT_BUFFER_SIZE);
      if (rc != MEMCACHED_SUCCESS) {
        if (rc == MEMCACHED_END) /* "\r\n" is found */
          rc= MEMCACHED_PROTOCOL_ERROR;
        return rc;
      }

      result->sub_keys[i].mkey.string= (char*)libmemcached_malloc(ptr->root, sizeof(char) * read_length);
      /* to_read_string is already null-terminated string. */
      memcpy((char*)result->sub_keys[i].mkey.string, to_read_string, read_length);
      result->sub_keys[i].mkey.length= strlen(to_read_string);
      result->sub_key_type= MEMCACHED_COLL_QUERY_MOP;
    }

    /* <bytes> */
    {
      if (result->type != COLL_BTREE || has_eflag)
      {
        rc= fetch_value_header(ptr, to_read_string, &read_length, MAX_UINT32_STRING_LENGTH);
        if (rc != MEMCACHED_SUCCESS) {
          if (rc == MEMCACHED_END) /* "\r\n" is found */
            rc= MEMCACHED_PROTOCOL_ERROR;
          return rc;
        }
      }

      value_length= static_cast<size_t>(strtoull(to_read_string, &string_ptr, 10));
    }

    /* prepare memory for a value (+2 bytes to walk the \r\n) */
    if (not memcached_string_create(ptr->root, &result->values[i], value_length+2))
    {
      return MEMCACHED_MEMORY_ALLOCATION_FAILURE;
    }

    char *value_ptr= memcached_string_value_mutable(&result->values[i]);

    /*
      We read the \r\n into the string since not doing so is more
      cycles then the waster of memory to do so.

      We are null terminating through, which will most likely make
      some people lazy about using the return length.
    */
    to_read= (value_length) + 2;
    rc= memcached_io_read(ptr, value_ptr, to_read, &read_length);
    if (memcached_failed(rc) and rc == MEMCACHED_IN_PROGRESS)
    {
      memcached_quit_server(ptr, true);
      return memcached_set_error(*ptr, MEMCACHED_IN_PROGRESS, MEMCACHED_AT);
    }
    else if (memcached_failed(rc))
    {
      return rc;
    }

    if (read_length != (ssize_t)(value_length + 2))
      goto read_error;

    /* This next bit blows the API, but this is internal....*/
    {
      char *char_ptr;
      char_ptr= memcached_string_value_mutable(&result->values[i]);;
      char_ptr[value_length]= 0;
      char_ptr[value_length + 1]= 0;
      memcached_string_set_length(&result->values[i], value_length);
    }

    result->collection_count++;
  }

  return MEMCACHED_SUCCESS;

read_error:
  return MEMCACHED_PARTIAL_READ;
}

static memcached_return_t textual_coll_value_fetch(memcached_server_write_instance_st ptr,
                                                   char *buffer, memcached_coll_result_st *result)
{
  uint32_t digit_params[4];
  uint32_t count_params;
  uint32_t ecount;
  int PARAM_FLAGS;
  int PARAM_COUNT;
  int PARAM_POSITION;
  int PARAM_RSTINDEX;
  char *ehdr_string= NULL;
  int   ehdr_length= 0;
  int   buff_length= MEMCACHED_DEFAULT_COMMAND_SIZE;
  int   read_length= 5; /* skip "VALUE" */

  if (memcmp(&ptr->root->last_op_code[1], "op g", 4) == 0) /* coll get, bop gbp */
  {
    /* VALUE <flags> <ecount>\r\n
     * <bytes> <data>\r\n                  (List/Set)
     * <mkey> <bytes> <data>\r\n           (Map)
     * <bkey> [<eflag>] <bytes> <data>\r\n (B+Tree)
     */
    count_params= 2;
    PARAM_FLAGS= 0;
    PARAM_COUNT= 1;
  }
  else if (memcmp(&ptr->root->last_op_code[0], "bop pwg", 7) == 0)
  {
    /* VALUE <position> <flags> <ecount> <resultidx>r\n
     * <bkey> [<eflag>] <bytes> <data>\r\n
     * ...
     */
    count_params= 4;
    PARAM_POSITION= 0;
    PARAM_FLAGS= 1;
    PARAM_COUNT= 2;
    PARAM_RSTINDEX= 3;
  }
  else if (memcmp(&ptr->root->last_op_code[0], "bop mget", 8) == 0)
  {
    char *string_ptr;
    int   string_len;
    memcached_return_t status;

    /* VALUE <key> <status> [<flags> <ecount>]\r\n
     * [ELEMENT <bkey> [<eflag>] <bytes> <data>\r\n
     * ...]
     */
    /* <key> */
    if (! parse_response_string_value(&buffer[read_length], buff_length-read_length,
                                      &string_ptr, &string_len))
    {
      return MEMCACHED_PARTIAL_READ;
    }
    read_length+= (1+string_len);
    memcpy(result->item_key, string_ptr, string_len);
    result->item_key[string_len]= '\0';
    result->key_length= (string_len+1); /* add one to include '\0' */

    /* <status> */
    if (! parse_response_string_value(&buffer[read_length], buff_length-read_length,
                                      &string_ptr, &string_len))
    {
      return MEMCACHED_PARTIAL_READ;
    }
    read_length+= (1+string_len);
    status= get_status_of_bop_mget_response(string_ptr, string_len);
    if (status != MEMCACHED_SUCCESS && status != MEMCACHED_TRIMMED)
    {
      if (status != MEMCACHED_UNKNOWN_READ_FAILURE)
      {
        if (buffer[read_length] != '\r') /* NOT "\r\n" */
          status= MEMCACHED_PARTIAL_READ;
      }
      return status;
    }

    /* <flags> <ecount> */
    count_params= 2;
    PARAM_FLAGS= 0;
    PARAM_COUNT= 1;
    ehdr_string= (char*)"ELEMENT";
    ehdr_length= 7;
  }
  else
  {
    return MEMCACHED_UNKNOWN_READ_FAILURE;
  }

  if (! parse_response_uint32_value(&buffer[read_length], buff_length-read_length,
                                    count_params, digit_params))
  {
    return MEMCACHED_PARTIAL_READ;
  }

  ecount= digit_params[PARAM_COUNT];
  result->collection_flags= digit_params[PARAM_FLAGS];
  if (PARAM_FLAGS == 1) /* bop pwg */
  {
    result->btree_position= digit_params[PARAM_POSITION];
    result->result_position= digit_params[PARAM_RSTINDEX];
  }
  if (ecount == 0)
  {
      return MEMCACHED_PROTOCOL_ERROR;
  }

  /* Prepare memory for the returning values */
  ALLOCATE_ARRAY_OR_RETURN(result->root, result->values, memcached_string_st, ecount);

  if (result->type == COLL_BTREE)
  {
    ALLOCATE_ARRAY_OR_RETURN(result->root, result->sub_keys, memcached_coll_sub_key_st, ecount);
    ALLOCATE_ARRAY_OR_RETURN(result->root, result->eflags,   memcached_hexadecimal_st,  ecount);
  }
  else if (result->type == COLL_MAP)
  {
    ALLOCATE_ARRAY_OR_RETURN(result->root, result->sub_keys, memcached_coll_sub_key_st, ecount);
  }

  /* Fetch all elements */
  return textual_coll_element_fetch(ptr, ehdr_string, ehdr_length, ecount, result);
}

/*
 * Fetching the piped responses : RESPONSE <count>\r\n
 * (works only for same kind of operations)
 */
static memcached_return_t textual_coll_piped_response_fetch(memcached_server_write_instance_st ptr, char *buffer,
                                                            size_t buffer_length)
{
#ifdef ENABLE_REPLICATION
  memcached_return_t switchover_rc= MEMCACHED_SUCCESS;
#endif
  memcached_return_t rc= MEMCACHED_SUCCESS;
  memcached_return_t *responses= ptr->root->pipe_responses;
  size_t i, offset= ptr->root->pipe_responses_length;
  uint32_t count[1];
  size_t total_read= 0;

  if (not parse_response_header(buffer, "RESPONSE", 8, count, 1))
  {
    return MEMCACHED_PARTIAL_READ;
  }

  for (i= 0; i< count[0]; i++)
  {
    rc= memcached_io_readline(ptr, buffer, buffer_length, total_read);
    if (rc != MEMCACHED_SUCCESS)
    {
      break;
    }
    rc= get_status_of_coll_pipe_response(ptr, buffer, total_read-2);
    if (rc == MEMCACHED_UNKNOWN_READ_FAILURE)
    {
      break;
    }
    responses[offset+i]= rc;
#ifdef ENABLE_REPLICATION
    if (rc == MEMCACHED_SWITCHOVER or rc == MEMCACHED_REPL_SLAVE)
    {
      switchover_rc= rc;
    }
#endif
  }

  ptr->root->pipe_responses_length+= i; /* i can be smaller than count[0] */
  if (i < count[0] && rc != MEMCACHED_SUCCESS)
  {
    return rc;
  }

  rc= memcached_io_readline(ptr, buffer, buffer_length, total_read);
  if (rc != MEMCACHED_SUCCESS)
  {
    return rc;
  }
  rc= get_status_of_coll_pipe_response(ptr, buffer, total_read-2);
#ifdef ENABLE_REPLICATION
  /* Pipe operation is stopped if switchover is done. */
  if (switchover_rc != MEMCACHED_SUCCESS && rc == MEMCACHED_PIPE_ERROR_BAD_ERROR) {
    return switchover_rc;
  }
#endif
  return rc;
}

static memcached_return_t textual_read_one_coll_response(memcached_server_write_instance_st ptr,
                                                         char *buffer, size_t buffer_length,
                                                         memcached_coll_result_st *result)
{
  size_t total_read;
  memcached_return_t rc= memcached_io_readline(ptr, buffer, buffer_length, total_read);

  if (rc != MEMCACHED_SUCCESS)
    return rc;

  switch(buffer[0])
  {
  case 'A':
    if (memcmp(buffer, "ATTR", 4) == 0)
    {
      if (memcmp(buffer + 4, " ", 1) == 0)
      {
        memcached_server_response_increment(ptr);
        return MEMCACHED_ATTR;
      }
      else if (memcmp(buffer + 4, "_MISMATCH", 9) == 0)
        return MEMCACHED_ATTR_MISMATCH;
      else if (memcmp(buffer + 4, "_ERROR not found", 16) == 0)
        return MEMCACHED_ATTR_ERROR_NOT_FOUND;
      else if (memcmp(buffer + 4, "_ERROR bad value", 16) == 0)
        return MEMCACHED_ATTR_ERROR_BAD_VALUE;
    }
    break;

  case 'V':
    if (memcmp(buffer, "VALUE", 5) == 0)
    {
      rc= textual_coll_value_fetch(ptr, buffer, result);
      if (rc == MEMCACHED_SUCCESS)
      {
        /* We add back in one to search for END or next VALUE */
        memcached_server_response_increment(ptr);
      }
      return rc;
    }
    break;

  case 'P':
    if (memcmp(buffer, "POSITION=", 9) == 0)
    {
      uint32_t position= 0;
      /* parse_response_uint32_value() assumes that the first character of the buffer is blank. */
      buffer[8]= ' ';
      if (! parse_response_uint32_value(&buffer[8], buffer_length-8, 1, &position))
      {
        return MEMCACHED_PARTIAL_READ;
      }
      result->btree_position= (size_t)position;
      return MEMCACHED_POSITION;
    }
    else if (memcmp(buffer, "PIPE_ERROR", 10) == 0)
    {
      if (memcmp(buffer + 10, " command overflow", 17) == 0)
        return MEMCACHED_PIPE_ERROR_COMMAND_OVERFLOW;
      else if (memcmp(buffer + 10, " memory overflow", 16) == 0)
        return MEMCACHED_PIPE_ERROR_MEMORY_OVERFLOW;
      else if (memcmp(buffer + 10, " bad error", 10) == 0)
        return MEMCACHED_PIPE_ERROR_BAD_ERROR;
    }
    break;

 case 'O':
    if (memcmp(buffer, "OK", 2) == 0)
      return MEMCACHED_SUCCESS;
    else if (memcmp(buffer, "OUT_OF_RANGE", 12) == 0)
      return MEMCACHED_OUT_OF_RANGE;
    else if (memcmp(buffer, "OVERFLOWED", 10) == 0)
      return MEMCACHED_OVERFLOWED;
    break;

  case 'S':
    if (memcmp(buffer, "STORED", 6) == 0)
    {
      return MEMCACHED_STORED;
    }
    else if (memcmp(buffer, "STAT", 4) == 0)
    {
      memcached_server_response_increment(ptr);
      return MEMCACHED_STAT;
    }
    else if (memcmp(buffer, "SERVER_ERROR", 12) == 0)
    {
      if (total_read == memcached_literal_param_size("SERVER_ERROR"))
      {
        return MEMCACHED_SERVER_ERROR;
      }

      // ensure compatibility. old cache server returns this error as SERVER_ERROR
      if (total_read > memcached_literal_param_size("SERVER_ERROR object too large for cache") and
          (memcmp(buffer, memcached_literal_param("SERVER_ERROR object too large for cache")) == 0))
      {
        return MEMCACHED_E2BIG;
      }

      // Move past the basic error message and whitespace
      char *startptr= buffer + memcached_literal_param_size("SERVER_ERROR");
      if (startptr[0] == ' ')
      {
        startptr++;
      }

      char *endptr= startptr;
      while (*endptr != '\r' && *endptr != '\n') endptr++;

      return memcached_set_error(*ptr, MEMCACHED_SERVER_ERROR, MEMCACHED_AT, startptr, size_t(endptr - startptr));
    }
#ifdef ENABLE_REPLICATION
    else if (memcmp(buffer, "SWITCHOVER", 10) == 0)
    {
      textual_switchover_peer_check(ptr, &buffer[10]);
      return MEMCACHED_SWITCHOVER;
    }
#endif
    break;

  case 'D':
    if (memcmp(buffer, "DELETED", 7) == 0)
    {
      if (memcmp(buffer + 7, "\r", 1) == 0)
        return MEMCACHED_DELETED;
      else if (memcmp(buffer + 7, "_DROPPED", 8) == 0)
        return MEMCACHED_DELETED_DROPPED;
    }
    break;

  case 'N':
    if (memcmp(buffer, "NOT_FOUND_ELEMENT", 17) == 0)
      return MEMCACHED_NOTFOUND_ELEMENT;
    else if (memcmp(buffer, "NOT_FOUND", 9) == 0)
      return MEMCACHED_NOTFOUND;
    else if (memcmp(buffer, "NOT_STORED", 10) == 0)
      return MEMCACHED_NOTSTORED;
    else if (memcmp(buffer, "NOT_SUPPORTED", 13) == 0)
      return MEMCACHED_NOT_SUPPORTED;
    else if (memcmp(buffer, "NOT_EXIST", 9) == 0)
      return MEMCACHED_NOT_EXIST;
    break;

  case 'R':
#ifdef ENABLE_REPLICATION
    if (memcmp(buffer, "REPL_SLAVE", 10) == 0)
    {
      textual_switchover_peer_check(ptr, &buffer[10]);
      return MEMCACHED_REPL_SLAVE;
    }
#endif
    if (memcmp(buffer, "REPLACED", 8) == 0)
    {
      /* REPLACED in response to bop upsert. */
      return MEMCACHED_REPLACED;
    }
    else if(memcmp(buffer, "RESPONSE", 8) == 0)
    {
      /* Assume RESPONSE for piped operations */
      return textual_coll_piped_response_fetch(ptr, buffer, buffer_length);
    }
    break;

  case 'U':
    if (memcmp(buffer, "UPDATED", 7) == 0)
      return MEMCACHED_UPDATED;
    else if (memcmp(buffer, "UNREADABLE", 10) == 0)
      return MEMCACHED_UNREADABLE;
    break;

  case 'E':
    if (memcmp(buffer, "END", 3) == 0)
      return MEMCACHED_END;
    else if (memcmp(buffer, "EFLAG_MISMATCH", 14) == 0)
      return MEMCACHED_EFLAG_MISMATCH;
    else if (memcmp(buffer, "ERROR", 5) == 0)
    {
      // Move past the basic error message and whitespace
      char *startptr= buffer + memcached_literal_param_size("ERROR");
      if (startptr[0] == ' ')
      {
        startptr++;
      }

      char *endptr= startptr;
      while (*endptr != '\r' && *endptr != '\n') endptr++;

      return memcached_set_error(*ptr, MEMCACHED_PROTOCOL_ERROR, MEMCACHED_AT,
                                 startptr, size_t(endptr - startptr));
    }
    else if (memcmp(buffer, "EXISTS", 6) == 0)
      return MEMCACHED_EXISTS;
    else if (memcmp(buffer, "EXIST\r", 6) == 0)
      return MEMCACHED_EXIST; /* COLLECTION SET MEMBERSHIP CHECK */
    else if (memcmp(buffer, "EXIST", 5) == 0)
      return MEMCACHED_DATA_EXISTS;
    else if (memcmp(buffer, "ELEMENT_EXISTS", 14) == 0)
      return MEMCACHED_ELEMENT_EXISTS;
    break;

  case 'T':
    if (memcmp(buffer, "TRIMMED", 7) == 0)
      return MEMCACHED_TRIMMED;
    else if (memcmp(buffer, "TYPE_MISMATCH", 13) == 0)
      return MEMCACHED_TYPE_MISMATCH;
    break;

  case 'I':
    if (memcmp(buffer, "ITEM", 4) == 0) /* ITEM STATS */
    {
      /* We add back in one because we will need to search for END */
      memcached_server_response_increment(ptr);
      return MEMCACHED_ITEM;
    }
    break;

  case 'C':
    if (memcmp(buffer, "CREATED", 7) == 0)
    {
      if (memcmp(buffer + 7, "\r", 1) == 0)
        return MEMCACHED_CREATED;
      else if (memcmp(buffer + 7, "_STORED", 7) == 0)
        return MEMCACHED_CREATED_STORED;
    }
    else if (memcmp(buffer, "COUNT=", 6) == 0)
    {
      uint32_t count= 0;
      /* parse_response_uint32_value() assumes that the first character of the buffer is blank. */
      buffer[5]= ' ';
      if (! parse_response_uint32_value(&buffer[5], buffer_length-5, 1, &count))
      {
        return MEMCACHED_PARTIAL_READ;
      }
      result->collection_count= (size_t)count;
      return MEMCACHED_COUNT;
    }
    else if (memcmp(buffer, "CLIENT_ERROR", 12) == 0)
    {
      if (total_read > memcached_literal_param_size("CLIENT_ERROR object too large for cache") and
          (memcmp(buffer, memcached_literal_param("CLIENT_ERROR object too large for cache")) == 0))
      {
        return MEMCACHED_E2BIG;
      }

      // Move past the basic error message and whitespace
      char *startptr= buffer + memcached_literal_param_size("CLIENT_ERROR");
      if (startptr[0] == ' ')
      {
        startptr++;
      }

      char *endptr= startptr;
      while (*endptr != '\r' && *endptr != '\n') endptr++;

      return memcached_set_error(*ptr, MEMCACHED_CLIENT_ERROR, MEMCACHED_AT,
                                 startptr, size_t(endptr - startptr));
    }
    break;

  case 'B':
    if (memcmp(buffer, "BKEY_MISMATCH", 13) == 0)
      return MEMCACHED_BKEY_MISMATCH;
    break;

  default:
    {
      unsigned long long auto_return_value;

      if (sscanf(buffer, "%llu", &auto_return_value) == 1)
        return MEMCACHED_SUCCESS;
      break;
    }
  }

  WATCHPOINT_STRING(buffer);
  return MEMCACHED_UNKNOWN_READ_FAILURE;
  /* NOTREACHED */
}

static memcached_return_t memcached_read_one_coll_response(memcached_server_write_instance_st ptr,
                                                           char *buffer, size_t buffer_length,
                                                           memcached_coll_result_st *result)
{
  memcached_server_response_decrement(ptr);

  if (result == NULL)
  {
    memcached_st *root= (memcached_st *)ptr->root;
    result = &root->collection_result;
  }
  else
  {
    result->root= (memcached_st *)ptr->root;
  }

  memcached_return_t rc;
  if (ptr->root->flags.binary_protocol)
  {
    fprintf(stderr, "Binary protocols for the collection are not supported.\n");
    return MEMCACHED_INVALID_ARGUMENTS;
  }
  else
  {
    rc= textual_read_one_coll_response(ptr, buffer, buffer_length, result);
  }
#ifdef IMMEDIATELY_RECONNECT
  if (rc == MEMCACHED_UNKNOWN_READ_FAILURE ||
      rc == MEMCACHED_PROTOCOL_ERROR ||
      rc == MEMCACHED_CLIENT_ERROR ||
      rc == MEMCACHED_SERVER_ERROR ||
      rc == MEMCACHED_PARTIAL_READ)
  {
    memcached_server_set_immediate_reconnect(ptr);
  }
#endif

#ifdef IMMEDIATELY_RECONNECT
  unlikely(rc == MEMCACHED_UNKNOWN_READ_FAILURE ||
           rc == MEMCACHED_PROTOCOL_ERROR ||
           rc == MEMCACHED_CLIENT_ERROR ||
           rc == MEMCACHED_SERVER_ERROR ||
           rc == MEMCACHED_PARTIAL_READ ||
           rc == MEMCACHED_MEMORY_ALLOCATION_FAILURE)
#else
  unlikely(rc == MEMCACHED_UNKNOWN_READ_FAILURE ||
           rc == MEMCACHED_PROTOCOL_ERROR ||
           rc == MEMCACHED_CLIENT_ERROR ||
           rc == MEMCACHED_PARTIAL_READ ||
           rc == MEMCACHED_MEMORY_ALLOCATION_FAILURE)
#endif
    memcached_io_reset(ptr);

  return rc;
}

memcached_return_t memcached_coll_response(memcached_server_write_instance_st ptr,
                                           char *buffer, size_t buffer_length,
                                           memcached_coll_result_st *result)
{
  /* We may have old commands in the buffer not set, first purge */
  if ((ptr->root->flags.no_block) && (memcached_is_processing_input(ptr->root) == false))
  {
    (void)memcached_io_write(ptr, NULL, 0, true);
  }

  if (ptr->root->flags.binary_protocol == true) {
    fprintf(stderr, "Binary protocols for the collection are not supported.\n");
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  /*
   * The previous implementation purged all pending requests and just
   * returned the last one. Purge all pending messages to ensure backwards
   * compatibility.
   */
  while (memcached_server_response_count(ptr) > 1)
  {
    memcached_return_t rc= memcached_read_one_coll_response(ptr, buffer, buffer_length, result);

    unlikely (rc != MEMCACHED_END              and
              rc != MEMCACHED_STORED           and
              rc != MEMCACHED_SUCCESS          and
              rc != MEMCACHED_STAT             and
              rc != MEMCACHED_DELETED          and
              rc != MEMCACHED_DELETED_DROPPED  and
              rc != MEMCACHED_NOTFOUND         and
              rc != MEMCACHED_NOTSTORED        and
              rc != MEMCACHED_NOT_SUPPORTED    and
              rc != MEMCACHED_DATA_EXISTS      and
              rc != MEMCACHED_TYPE_MISMATCH    and
              rc != MEMCACHED_EXIST            and
              rc != MEMCACHED_NOT_EXIST        and
              rc != MEMCACHED_TRIMMED          and
              rc != MEMCACHED_NOTFOUND_ELEMENT and
              rc != MEMCACHED_ELEMENT_EXISTS   and
              rc != MEMCACHED_UNREADABLE       and
              rc != MEMCACHED_CREATED          and
              rc != MEMCACHED_CREATED_STORED )
      return rc;
  }

  return memcached_read_one_coll_response(ptr, buffer, buffer_length, result);
}

/* Sort-merge-get */

/*
 * Fetching B+Tree's sort-merge-get values.
 *
 * VALUE <value_count>\r\n
 * <key> <flags> <bkey> <bytes> <value>\r\n
 * ...
 */
static memcached_return_t textual_coll_smget_value_fetch(memcached_server_write_instance_st ptr,
                                                         char *buffer, const char* header, size_t header_length,
                                                         memcached_coll_smget_result_st *result)
{
  char *string_ptr;
  size_t to_read;
  ssize_t read_length= 0;

  uint32_t header_params[1];
  uint32_t ecount;
  const int PARAM_COUNT= 0;

  if (not parse_response_header(buffer, header, header_length, header_params, 1))
  {
    return MEMCACHED_PARTIAL_READ;
  }

  ecount= header_params[PARAM_COUNT];
  if (ecount < 1)
  {
    return MEMCACHED_SUCCESS;
  }

  /* Prepare memory for the returning values */
  ALLOCATE_ARRAY_OR_RETURN(result->root, result->keys,     memcached_string_st,       ecount);
  ALLOCATE_ARRAY_OR_RETURN(result->root, result->values,   memcached_string_st,       ecount);
  ALLOCATE_ARRAY_OR_RETURN(result->root, result->flags,    uint32_t,                  ecount);
  ALLOCATE_ARRAY_OR_RETURN(result->root, result->sub_keys, memcached_coll_sub_key_st, ecount);
  ALLOCATE_ARRAY_OR_RETURN(result->root, result->eflags,   memcached_hexadecimal_st,  ecount);
  ALLOCATE_ARRAY_OR_RETURN(result->root, result->bytes,    size_t,                    ecount);

  /* Fetch all values */
  size_t i, value_length;
  memcached_return_t rrc;

  char *value_ptr;
  char to_read_string[MEMCACHED_MAX_KEY+1];

  for (i=0; i<ecount; i++)
  {
    bool has_eflag= false;

    /* <key> */
    {
      rrc= fetch_value_header(ptr, to_read_string, &read_length, MEMCACHED_MAX_KEY);
      if (rrc != MEMCACHED_SUCCESS)
      {
        fprintf(stderr, "[debug] key_fetch_error=%s\n", to_read_string);
        if (rrc == MEMCACHED_END) /* "\r\n" is found */
          rrc= MEMCACHED_PROTOCOL_ERROR;
        return rrc;
      }

      if (not memcached_string_create(ptr->root, &result->keys[i], read_length))
        return MEMCACHED_MEMORY_ALLOCATION_FAILURE;

      value_ptr= memcached_string_value_mutable(&result->keys[i]);
      strncpy(value_ptr, to_read_string, read_length);
      value_ptr[read_length]= 0;
      memcached_string_set_length(&result->keys[i], read_length);
    }

    /* <flags> */
    {
      rrc= fetch_value_header(ptr, to_read_string, &read_length, MAX_UINT32_STRING_LENGTH);
      if (rrc != MEMCACHED_SUCCESS)
      {
        if (rrc == MEMCACHED_END) /* "\r\n" is found */
          rrc= MEMCACHED_PROTOCOL_ERROR;
        return rrc;
      }

      result->flags[i]= static_cast<size_t>(strtoull(to_read_string, &string_ptr, 10));
    }

    /* <bkey> */
    {
      rrc= fetch_value_header(ptr, to_read_string, &read_length, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH+2); // with '0x'
      if (rrc != MEMCACHED_SUCCESS)
      {
        if (rrc == MEMCACHED_END) /* "\r\n" is found */
          rrc= MEMCACHED_PROTOCOL_ERROR;
        return rrc;
      }

      /* byte array bkey : starts with 0x */
      if (to_read_string[0] == '0' && to_read_string[1] == 'x')
      {
        if (result->sub_key_type != MEMCACHED_COLL_QUERY_BOP_EXT &&
            result->sub_key_type != MEMCACHED_COLL_QUERY_BOP_EXT_RANGE)
            return MEMCACHED_PROTOCOL_ERROR;
        memcached_conv_str_to_hex(ptr->root, to_read_string+2, read_length-2-1, &result->sub_keys[i].bkey_ext); // except '0x' and '\0'
      }
      /* normal bkey */
      else
      {
        if (result->sub_key_type != MEMCACHED_COLL_QUERY_BOP &&
            result->sub_key_type != MEMCACHED_COLL_QUERY_BOP_RANGE)
            return MEMCACHED_PROTOCOL_ERROR;
        result->sub_keys[i].bkey = strtoull(to_read_string, &string_ptr, 10);
        result->sub_key_type = MEMCACHED_COLL_QUERY_BOP;
      }
    }

    /* <eflag> - optional */
    {
      rrc= fetch_value_header(ptr, to_read_string, &read_length, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH+2); // with '0x'
      if (rrc != MEMCACHED_SUCCESS)
      {
        fprintf(stderr, "[debug] eflag_fetch_error=%s\n", to_read_string);
        if (rrc == MEMCACHED_END) /* "\r\n" is found */
          rrc= MEMCACHED_PROTOCOL_ERROR;
        return rrc;
      }

      /* eflag : starts with 0x */
      if (to_read_string[0] == '0' && to_read_string[1] == 'x')
      {
        memcached_conv_str_to_hex(ptr->root, to_read_string+2, read_length-2-1, &result->eflags[i]); // except '0x' and '\0'
        has_eflag= true;
      }
    }

    /* <bytes> */
    {
      if (has_eflag)
      {
        rrc= fetch_value_header(ptr, to_read_string, &read_length, MAX_UINT32_STRING_LENGTH);
        if (rrc != MEMCACHED_SUCCESS)
        {
          if (rrc == MEMCACHED_END) /* "\r\n" is found */
            rrc= MEMCACHED_PROTOCOL_ERROR;
          return rrc;
        }
      }

      result->bytes[i]= static_cast<size_t>(strtoull(to_read_string, &string_ptr, 10));
    }

    /* prepare memory for a value (+2 bytes to walk the \r\n) */
    if (not memcached_string_create(ptr->root, &result->values[i], result->bytes[i]+2))
      return MEMCACHED_MEMORY_ALLOCATION_FAILURE;

    value_ptr= memcached_string_value_mutable(&result->values[i]);
    value_length= result->bytes[i];

    /*
      We read the \r\n into the string since not doing so is more
      cycles then the waster of memory to do so.

      We are null terminating through, which will most likely make
      some people lazy about using the return length.
    */
    to_read= (value_length) + 2;
    rrc= memcached_io_read(ptr, value_ptr, to_read, &read_length);
    if (memcached_failed(rrc) and rrc == MEMCACHED_IN_PROGRESS)
    {
      memcached_quit_server(ptr, true); // ?
      return memcached_set_error(*ptr, MEMCACHED_IN_PROGRESS, MEMCACHED_AT);
    }
    else if (memcached_failed(rrc))
    {
      return rrc;
    }

    if (read_length != (ssize_t)(value_length + 2))
      goto read_error;

    /* This next bit blows the API, but this is internal....*/
    {
      char *char_ptr;
      char_ptr= memcached_string_value_mutable(&result->values[i]);;
      char_ptr[value_length]= 0;
      char_ptr[value_length + 1]= 0;
      memcached_string_set_length(&result->values[i], value_length);
    }

    result->value_count++;
  }

  return MEMCACHED_SUCCESS;

read_error:
  return MEMCACHED_PARTIAL_READ;
}

/*
 * Fetching the smget missed_keys.
 *
 * New smget
 *   MISSED_KEYS <missed_key_count>\r\n
 *   <missed_key> [<missed cause>]\r\n
 *   ...
 *
 * Old smget
 *   MISSED_KEYS <missed_key_count>\r\n
 *   <missed_key>\r\n
 *   ...
 */
static memcached_return_t textual_coll_smget_missed_key_fetch(memcached_server_write_instance_st ptr,
                                                              char *buffer, memcached_coll_smget_result_st *result)
{
  char *value_ptr;

  uint32_t header_params[1];
  uint32_t kcount;
  const int PARAM_COUNT= 0;

  if (not parse_response_header(buffer, "MISSED_KEYS", 11, header_params, 1))
  {
    return MEMCACHED_PARTIAL_READ;
  }

  kcount= header_params[PARAM_COUNT];
  if (kcount < 1)
  {
    return MEMCACHED_SUCCESS;
  }

  /* Prepare memory for the returning values */
  ALLOCATE_ARRAY_OR_RETURN(result->root, result->missed_keys, memcached_string_st, kcount);
  ALLOCATE_ARRAY_OR_RETURN(result->root, result->missed_causes, memcached_return_t, kcount);

  /* Fetch all values */
  memcached_return_t rrc;

  for (size_t i=0; i<kcount; i++)
  {
    char to_read_string[MEMCACHED_MAX_KEY+2]; // +2: "\r\n"
    ssize_t read_length= 0;

    /* <missed key> */
    rrc= fetch_value_header(ptr, to_read_string, &read_length, MEMCACHED_MAX_KEY+1);
    if (rrc != MEMCACHED_SUCCESS && rrc != MEMCACHED_END)
    {
      return rrc;
    }
    if (read_length > MEMCACHED_MAX_KEY)
    {
      goto read_error;
    }

    /* prepare memory for key string (+2 bytes to walk the \r\n) */
    if (not memcached_string_create(ptr->root, &result->missed_keys[i], read_length))
    {
      return MEMCACHED_MEMORY_ALLOCATION_FAILURE;
    }

    value_ptr= memcached_string_value_mutable(&result->missed_keys[i]);
    strncpy(value_ptr, to_read_string, read_length);
    value_ptr[read_length]= 0;
    memcached_string_set_length(&result->missed_keys[i], read_length);

    if (rrc == MEMCACHED_SUCCESS) /* more data to read */
    {
      /* <missed cause> */
      /* MEMCACHED_MAX_KEY is enough length for reading cause string */
      rrc= fetch_value_header(ptr, to_read_string, &read_length, MEMCACHED_MAX_KEY+1);
      if (rrc != MEMCACHED_END)
      {
        memcached_string_free(&result->missed_keys[i]);
        if (rrc == MEMCACHED_SUCCESS) /* No "\r\n" found */
          rrc= MEMCACHED_PROTOCOL_ERROR;
        return rrc;
      }

      if (to_read_string[0] == 'N') /* NOT_FOUND */
      {
        result->missed_causes[i] = MEMCACHED_NOTFOUND;
      }
      else if (to_read_string[0] == 'O') /* OUT_OF_RANGE */
      {
        result->missed_causes[i] = MEMCACHED_OUT_OF_RANGE;
      }
      else if (to_read_string[0] == 'U' && to_read_string[2] == 'R') /* UNREADABLE */
      {
        result->missed_causes[i] = MEMCACHED_UNREADABLE;
      }
      else /* UNKNOWN FAILURE */
      {
        result->missed_causes[i] = MEMCACHED_FAILURE;
      }
    }

    result->missed_key_count++;
  }

  return MEMCACHED_SUCCESS;

read_error:
  return MEMCACHED_PARTIAL_READ;
}

/*
 * Fetching the smget trimmed_keys.
 *
 * TRIMMED_KEYS <trimmed_key_count>\r\n
 * <trimmed_key> <last_bkey>\r\n
 * ...
 */
static memcached_return_t textual_coll_smget_trimmed_key_fetch(memcached_server_write_instance_st ptr,
                                                               char *buffer, memcached_coll_smget_result_st *result)
{
  char *value_ptr;
  char *string_ptr;
  uint32_t header_params[1];
  uint32_t kcount;
  const int PARAM_COUNT= 0;

  if (not parse_response_header(buffer, "TRIMMED_KEYS", 11, header_params, 1))
  {
    return MEMCACHED_PARTIAL_READ;
  }

  kcount= header_params[PARAM_COUNT];
  if (kcount < 1)
  {
    return MEMCACHED_SUCCESS;
  }

  /* Prepare memory for the returning values */
  ALLOCATE_ARRAY_OR_RETURN(result->root, result->trimmed_keys, memcached_string_st, kcount);
  ALLOCATE_ARRAY_OR_RETURN(result->root, result->trimmed_sub_keys, memcached_coll_sub_key_st, kcount);

  /* Fetch all values */
  memcached_return_t rrc;

  for (size_t i=0; i<kcount; i++)
  {
    char to_read_string[MEMCACHED_MAX_KEY+2]; // +2: "\r\n"
    ssize_t read_length= 0;

    /* <trimmed key> */
    rrc= fetch_value_header(ptr, to_read_string, &read_length, MEMCACHED_MAX_KEY+1);
    if (rrc != MEMCACHED_SUCCESS)
    {
      if (rrc == MEMCACHED_END) /* "\r\n" is found */
        rrc= MEMCACHED_PROTOCOL_ERROR;
      return rrc;
    }
    if (read_length > MEMCACHED_MAX_KEY)
    {
      goto read_error;
    }

    /* prepare memory for key string (+2 bytes to walk the \r\n) */
    if (not memcached_string_create(ptr->root, &result->trimmed_keys[i], read_length))
    {
      return MEMCACHED_MEMORY_ALLOCATION_FAILURE;
    }

    value_ptr= memcached_string_value_mutable(&result->trimmed_keys[i]);
    strncpy(value_ptr, to_read_string, read_length);
    value_ptr[read_length]= 0;
    memcached_string_set_length(&result->trimmed_keys[i], read_length);

    /* <trimmed bkey> */
    rrc= fetch_value_header(ptr, to_read_string, &read_length, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH+2); // with '0x'
    if (rrc != MEMCACHED_END)
    {
      memcached_string_free(&result->trimmed_keys[i]);
      if (rrc == MEMCACHED_SUCCESS) /* No "\r\n" found */
        rrc= MEMCACHED_PROTOCOL_ERROR;
      return rrc;
    }

    /* byte array bkey : starts with 0x */
    if (to_read_string[0] == '0' && to_read_string[1] == 'x')
    {
      memcached_conv_str_to_hex(ptr->root, to_read_string+2, read_length-2-1,
                                &result->trimmed_sub_keys[i].bkey_ext); // except '0x' and '\0'
      if (result->sub_key_type != MEMCACHED_COLL_QUERY_BOP_EXT &&
          result->sub_key_type != MEMCACHED_COLL_QUERY_BOP_EXT_RANGE)
      {
        memcached_string_free(&result->trimmed_keys[i]);
        return MEMCACHED_PROTOCOL_ERROR;
      }
    }
    /* normal bkey */
    else
    {
      result->trimmed_sub_keys[i].bkey = strtoull(to_read_string, &string_ptr, 10);
      if (result->sub_key_type != MEMCACHED_COLL_QUERY_BOP &&
          result->sub_key_type != MEMCACHED_COLL_QUERY_BOP_RANGE)
      {
        memcached_string_free(&result->trimmed_keys[i]);
        return MEMCACHED_PROTOCOL_ERROR;
      }
    }

    result->trimmed_key_count++;
  }

  return MEMCACHED_SUCCESS;

read_error:
  return MEMCACHED_PARTIAL_READ;
}

static memcached_return_t textual_read_one_coll_smget_response(memcached_server_write_instance_st ptr,
                                                               char *buffer, size_t buffer_length,
                                                               memcached_coll_smget_result_st *result)
{
  size_t total_read;
  memcached_return_t rc= memcached_io_readline(ptr, buffer, buffer_length, total_read);
  if (rc != MEMCACHED_SUCCESS) {
    /* rc == MEMCACHED_CONNECTION_FAILURE or
     * rc == MEMCACHED_UNKNOWN_READ_FAILURE or
     * rc == MEMCACHED_IN_PROGRESS or
     * rc == MEMCACHED_ERRNO : get_socket_errno() or
     * rc == MEMCACHED_PROTOCOL_ERROR
     */
    return rc;
  }

  switch(buffer[0])
  {
    case 'A':
      if (memcmp(buffer, "ATTR_MISMATCH", 13) == 0)
        return MEMCACHED_ATTR_MISMATCH;
      break;

    case 'B':
      if (memcmp(buffer, "BKEY_MISMATCH", 13) == 0)
        return MEMCACHED_BKEY_MISMATCH;
      break;

    case 'V':
      if (memcmp(buffer, "VALUE", 5) == 0)
      {
        /* We add back in one because we will need to search for MISSED_KEYS */
        memcached_server_response_increment(ptr);
        return textual_coll_smget_value_fetch(ptr, buffer, "VALUE", 5, result);
        /* rc == MEMCACHED_SUCCESS or
         * rc == MEMCACHED_PARTIAL_READ or
         * rc == MEMCACHED_MEMORY_ALLOCATION_FAILURE or
         * rc == one of return values of memcached_io_readline()
         */
      }
      break;

  case 'M':
    if (memcmp(buffer, "MISSED_KEYS", 11) == 0)
    {
      /* We add back in one because we will need to search for END */
      memcached_server_response_increment(ptr);
      return textual_coll_smget_missed_key_fetch(ptr, buffer, result);
      /* rc == MEMCACHED_SUCCESS or
       * rc == MEMCACHED_PARTIAL_READ or
       * rc == MEMCACHED_MEMORY_ALLOCATION_FAILURE or
       * rc == one of return values of memcached_io_readline()
       */
    }
    break;

  case 'E':
    if (memcmp(buffer, "END", 3) == 0)
      return MEMCACHED_END;
    else if (memcmp(buffer, "ELEMENTS", 8) == 0)
    {
      /* We add back in one because we will need to search for MISSED_KEYS */
      memcached_server_response_increment(ptr);
      return textual_coll_smget_value_fetch(ptr, buffer, "ELEMENTS", 8, result);
      /* rc == MEMCACHED_SUCCESS or
       * rc == MEMCACHED_PARTIAL_READ or
       * rc == MEMCACHED_MEMORY_ALLOCATION_FAILURE or
       * rc == one of return values of memcached_io_readline()
       */
    }
    else if (memcmp(buffer, "ERROR", 5) == 0)
    {
      // Move past the basic error message and whitespace
      char *startptr= buffer + memcached_literal_param_size("ERROR");
      if (startptr[0] == ' ')
      {
        startptr++;
      }

      char *endptr= startptr;
      while (*endptr != '\r' && *endptr != '\n') endptr++;

      return memcached_set_error(*ptr, MEMCACHED_PROTOCOL_ERROR, MEMCACHED_AT,
                                 startptr, size_t(endptr - startptr));
    }
    break;

  case 'O':
    if (memcmp(buffer, "OUT_OF_RANGE", 12) == 0)
      return MEMCACHED_OUT_OF_RANGE;
    break;

  case 'D':
    if (memcmp(buffer, "DUPLICATED", 10) == 0)
    {
      if (memcmp(buffer + 10, "\r", 1) == 0)
        return MEMCACHED_DUPLICATED;
      else if (memcmp(buffer + 10, "_TRIMMED", 8) == 0)
        return MEMCACHED_DUPLICATED_TRIMMED;
    }
    break;

  case 'T':
    if (memcmp(buffer, "TRIMMED", 7) == 0)
    {
      if (memcmp(buffer + 7, "_KEYS", 5) == 0) { /* TRIMMED_KEYS */
        /* We add back in one because we will need to search for END */
        memcached_server_response_increment(ptr);
        return textual_coll_smget_trimmed_key_fetch(ptr, buffer, result);
      }
      return MEMCACHED_TRIMMED;
    }
    else if (memcmp(buffer, "TYPE_MISMATCH", 13) == 0)
    {
      return MEMCACHED_TYPE_MISMATCH;
    }
    break;

  case 'N':
    if (memcmp(buffer, "NOT_FOUND_ELEMENT", 17) == 0)
      return MEMCACHED_NOTFOUND_ELEMENT;
    else if (memcmp(buffer, "NOT_FOUND", 9) == 0)
      return MEMCACHED_NOTFOUND;
    else if (memcmp(buffer, "NOT_STORED", 10) == 0)
      return MEMCACHED_NOTSTORED;
    else if (memcmp(buffer, "NOT_SUPPORTED", 13) == 0)
      return MEMCACHED_NOT_SUPPORTED;
    else if (memcmp(buffer, "NOT_EXIST", 9) == 0)
      return MEMCACHED_NOT_EXIST;
    break;

  case 'U':
    if (memcmp(buffer, "UNREADABLE", 10) == 0)
      return MEMCACHED_UNREADABLE;
    break;

  case 'I':
    if (memcmp(buffer, "ITEM", 4) == 0) /* ITEM STATS */
    {
      /* We add back in one because we will need to search for END */
      memcached_server_response_increment(ptr);
      return MEMCACHED_ITEM;
    }
    break;

  case 'C':
    if (memcmp(buffer, "CLIENT_ERROR", 12) == 0)
    {
      // Move past the basic error message and whitespace
      char *startptr= buffer + memcached_literal_param_size("CLIENT_ERROR");
      if (startptr[0] == ' ')
      {
        startptr++;
      }

      char *endptr= startptr;
      while (*endptr != '\r' && *endptr != '\n') endptr++;

      return memcached_set_error(*ptr, MEMCACHED_CLIENT_ERROR, MEMCACHED_AT,
                                 startptr, size_t(endptr - startptr));
    }
    break;

  case 'S':
    if (memcmp(buffer, "SERVER_ERROR", 12) == 0)
    {
      // Move past the basic error message and whitespace
      char *startptr= buffer + memcached_literal_param_size("SERVER_ERROR");
      if (startptr[0] == ' ')
      {
        startptr++;
      }

      char *endptr= startptr;
      while (*endptr != '\r' && *endptr != '\n') endptr++;

      return memcached_set_error(*ptr, MEMCACHED_SERVER_ERROR, MEMCACHED_AT,
                                 startptr, size_t(endptr - startptr));
    }
    break;

  default:
    {
      unsigned long long auto_return_value;

      if (sscanf(buffer, "%llu", &auto_return_value) == 1)
        return MEMCACHED_SUCCESS;
      break;
    }
  }

  WATCHPOINT_STRING(buffer);
  return MEMCACHED_UNKNOWN_READ_FAILURE;
  /* NOTREACHED */
}

static memcached_return_t memcached_read_one_coll_smget_response(memcached_server_write_instance_st ptr,
                                                                 char *buffer, size_t buffer_length,
                                                                 memcached_coll_smget_result_st *result)
{
  memcached_server_response_decrement(ptr);

  if (result == NULL)
  {
    memcached_st *root= (memcached_st *)ptr->root;
    result = &root->smget_result;
  }
  else
  {
    result->root= (memcached_st *)ptr->root;
  }

  memcached_return_t rc;
  if (ptr->root->flags.binary_protocol)
  {
    fprintf(stderr, "Binary protocols for the collection are not supported.\n");
    return MEMCACHED_INVALID_ARGUMENTS;
  }
  else
  {
    rc= textual_read_one_coll_smget_response(ptr, buffer, buffer_length, result);
  }
#ifdef IMMEDIATELY_RECONNECT
  if (rc == MEMCACHED_UNKNOWN_READ_FAILURE or
      rc == MEMCACHED_PROTOCOL_ERROR or
      rc == MEMCACHED_CLIENT_ERROR or
      rc == MEMCACHED_SERVER_ERROR or
      rc == MEMCACHED_PARTIAL_READ)
  {
    memcached_server_set_immediate_reconnect(ptr);
  }
#endif

#ifdef IMMEDIATELY_RECONNECT
  unlikely(rc == MEMCACHED_UNKNOWN_READ_FAILURE or
           rc == MEMCACHED_PROTOCOL_ERROR or
           rc == MEMCACHED_CLIENT_ERROR or
           rc == MEMCACHED_SERVER_ERROR or
           rc == MEMCACHED_PARTIAL_READ or
           rc == MEMCACHED_MEMORY_ALLOCATION_FAILURE )
#else
  unlikely(rc == MEMCACHED_UNKNOWN_READ_FAILURE or
           rc == MEMCACHED_PROTOCOL_ERROR or
           rc == MEMCACHED_CLIENT_ERROR or
           rc == MEMCACHED_PARTIAL_READ or
           rc == MEMCACHED_MEMORY_ALLOCATION_FAILURE )
#endif
    memcached_io_reset(ptr);

  return rc;
}

memcached_return_t memcached_coll_smget_response(memcached_server_write_instance_st ptr,
                                                 char *buffer, size_t buffer_length,
                                                 memcached_coll_smget_result_st *result)
{
  /* We may have old commands in the buffer not set, first purge */
  if ((ptr->root->flags.no_block) && (memcached_is_processing_input(ptr->root) == false))
  {
    (void)memcached_io_write(ptr, NULL, 0, true);
  }

  if (ptr->root->flags.binary_protocol == true) {
	  fprintf(stderr, "Binary protocols for the collection are not supported.\n");
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  /*
   * The previous implementation purged all pending requests and just
   * returned the last one. Purge all pending messages to ensure backwards
   * compatibility.
   */
  while (memcached_server_response_count(ptr) > 1)
  {
    memcached_return_t rc= memcached_read_one_coll_smget_response(ptr, buffer, buffer_length, result);

    unlikely (rc != MEMCACHED_END                and
              rc != MEMCACHED_SUCCESS            and
              rc != MEMCACHED_TYPE_MISMATCH      and
              rc != MEMCACHED_DUPLICATED         and
              rc != MEMCACHED_DUPLICATED_TRIMMED and
              rc != MEMCACHED_TRIMMED            and
              rc != MEMCACHED_ATTR_MISMATCH      and
              rc != MEMCACHED_BKEY_MISMATCH      and
              rc != MEMCACHED_OUT_OF_RANGE       and
              rc != MEMCACHED_NOTFOUND )
      return rc;
  }

  return memcached_read_one_coll_smget_response(ptr, buffer, buffer_length, result);
}
