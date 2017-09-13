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

static memcached_return_t textual_read_one_response(memcached_server_write_instance_st ptr,
                                                    char *buffer, size_t buffer_length,
                                                    memcached_result_st *result);
static memcached_return_t binary_read_one_response(memcached_server_write_instance_st ptr,
                                                   char *buffer, size_t buffer_length,
                                                   memcached_result_st *result);

static memcached_return_t textual_read_one_coll_response(memcached_server_write_instance_st ptr,
                                                         char *buffer, size_t buffer_length,
                                                         memcached_coll_result_st *result);

static memcached_return_t textual_read_one_coll_smget_response(memcached_server_write_instance_st ptr,
                                                               char *buffer, size_t buffer_length,
                                                               memcached_coll_smget_result_st *result);

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

static memcached_return_t fetch_value_header(memcached_server_write_instance_st ptr,
                                             char *string, ssize_t *string_length,
                                             size_t max_read_length)
{
  memcached_return_t rc;
  ssize_t read_length= 0;

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
      rc= memcached_io_read(ptr, string+i, 1, &read_length); // search for '\n'
      if (memcached_failed(rc))
      {
        if (rc == MEMCACHED_IN_PROGRESS) {
          memcached_quit_server(ptr, true);
          rc = memcached_set_error(*ptr, MEMCACHED_IN_PROGRESS, MEMCACHED_AT);
        }
        return rc;
      }
      if (string[i] != '\n')
      {
        return MEMCACHED_PROTOCOL_ERROR;
      }
      string[i] = '\0';
      *string_length= i+1;
      return MEMCACHED_END; /* the end of line */
    }
  }

  return MEMCACHED_PROTOCOL_ERROR;
}

static memcached_return_t fetch_value_header_with(char *buffer,
                                                  char *string, ssize_t *string_length,
                                                  size_t max_read_length)
{
  /* Read until meeting a space */
  for (size_t i=0; i<max_read_length; i++)
  {
    string[i]= buffer[i];

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
      string[i]= buffer[i+1]; // search for '\n'
      if (string[i] != '\n')
      {
        return MEMCACHED_PROTOCOL_ERROR;
      }
      string[i] = '\0';
      *string_length= i+1;
      return MEMCACHED_END; /* the end of line */
    }
  }

  return MEMCACHED_PROTOCOL_ERROR;
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

  unlikely(rc == MEMCACHED_UNKNOWN_READ_FAILURE or
           rc == MEMCACHED_PROTOCOL_ERROR or
           rc == MEMCACHED_CLIENT_ERROR or
           rc == MEMCACHED_MEMORY_ALLOCATION_FAILURE)
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

static memcached_return_t textual_value_fetch(memcached_server_write_instance_st ptr,
                                              char *buffer,
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
  end_ptr= buffer + MEMCACHED_DEFAULT_COMMAND_SIZE;

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
  memcached_io_reset(ptr);

  return MEMCACHED_PARTIAL_READ;
}

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
  case 'V': /* VALUE || VERSION */
    if (buffer[1] == 'A') /* VALUE */
    {
      /* We add back in one because we will need to search for END */
      memcached_server_response_increment(ptr);
      return textual_value_fetch(ptr, buffer, result);
    }
    else if (buffer[1] == 'E') /* VERSION */
    {
      return MEMCACHED_SUCCESS;
    }
    else
    {
      WATCHPOINT_STRING(buffer);
      return MEMCACHED_UNKNOWN_READ_FAILURE;
    }
  case 'O': /* OK */
    return MEMCACHED_SUCCESS;
  case 'S': /* STORED STATS SERVER_ERROR */
    {
      if (buffer[2] == 'A') /* STORED STATS */
      {
        memcached_server_response_increment(ptr);
        return MEMCACHED_STAT;
      }
      else if (buffer[1] == 'E') /* SERVER_ERROR */
      {
        if (total_read == memcached_literal_param_size("SERVER_ERROR"))
        {
          return MEMCACHED_SERVER_ERROR;
        }

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
      else if (buffer[1] == 'T')
      {
        return MEMCACHED_STORED;
      }
#ifdef ENABLE_REPLICATION
      else if (buffer[1] == 'W')
      {
        return MEMCACHED_SWITCHOVER;
      }
#endif
      else
      {
        WATCHPOINT_STRING(buffer);
        return MEMCACHED_UNKNOWN_READ_FAILURE;
      }
    }
  case 'D': /* DELETED */
    return MEMCACHED_DELETED;

  case 'N': /* NOT_FOUND */
    {
      if (buffer[4] == 'F')
        return MEMCACHED_NOTFOUND;
      else if (buffer[4] == 'S')
      {
        if (buffer[5] == 'T')
          return MEMCACHED_NOTSTORED;
        if (buffer[5] == 'U')
          return MEMCACHED_NOT_SUPPORTED;
      }
      else
      {
        WATCHPOINT_STRING(buffer);
        return MEMCACHED_UNKNOWN_READ_FAILURE;
      }
    }
#ifdef ENABLE_REPLICATION
  case 'R': /* REPL_SLAVE */
    return MEMCACHED_REPL_SLAVE;
#endif
  case 'U': /* UNREADABLE */
    return MEMCACHED_UNREADABLE;
  case 'E': /* PROTOCOL ERROR or END */
    {
      if (buffer[1] == 'N')
        return MEMCACHED_END;
      else if (buffer[1] == 'R')
        return MEMCACHED_PROTOCOL_ERROR;
      else if (buffer[1] == 'X')
        return MEMCACHED_DATA_EXISTS;
      else
      {
        WATCHPOINT_STRING(buffer);
        return MEMCACHED_UNKNOWN_READ_FAILURE;
      }

    }
  case 'I': /* CLIENT ERROR */
    /* We add back in one because we will need to search for END */
    memcached_server_response_increment(ptr);
    return MEMCACHED_ITEM;
  case 'C': /* CLIENT ERROR */
    return MEMCACHED_CLIENT_ERROR;
  default:
    {
      unsigned long long auto_return_value;

      if (sscanf(buffer, "%llu", &auto_return_value) == 1)
        return MEMCACHED_SUCCESS;

      WATCHPOINT_STRING(buffer);
      return MEMCACHED_UNKNOWN_READ_FAILURE;
    }
  }

  /* NOTREACHED */
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
        memset(buffer, 0, buffer_length);
        if (bodylen >= buffer_length)
        {
          /* not enough space in buffer.. should not happen... */
          return MEMCACHED_UNKNOWN_READ_FAILURE;
        }
        else if ((rc= memcached_safe_read(ptr, buffer, bodylen)) != MEMCACHED_SUCCESS)
        {
          WATCHPOINT_ERROR(rc);
          return MEMCACHED_UNKNOWN_READ_FAILURE;
        }
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

memcached_return_t memcached_read_one_coll_response(memcached_server_write_instance_st ptr,
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

  unlikely(rc == MEMCACHED_UNKNOWN_READ_FAILURE ||
           rc == MEMCACHED_PROTOCOL_ERROR ||
           rc == MEMCACHED_CLIENT_ERROR ||
           rc == MEMCACHED_MEMORY_ALLOCATION_FAILURE)
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

  /* If the requests were piped, just return one response.
   * The API should control the remaining responses properly.
   */
  if (ptr->root->flags.piped)
  {
    return memcached_read_one_coll_response(ptr, buffer, buffer_length, result);
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
              rc != MEMCACHED_DELETED_TRIMMED  and
              rc != MEMCACHED_NOTFOUND_ELEMENT and
              rc != MEMCACHED_ELEMENT_EXISTS   and
              rc != MEMCACHED_UNREADABLE       and
              rc != MEMCACHED_CREATED          and
              rc != MEMCACHED_CREATED_STORED )
      return rc;
  }

  return memcached_read_one_coll_response(ptr, buffer, buffer_length, result);
}

static void aggregate_pipe_return_code(memcached_st *ptr, memcached_return_t response,
                                       memcached_return_t *pipe_return_code)
{
  switch (ptr->last_op_code[4])
  {
  case 'e': /* exist */
    if (response == MEMCACHED_EXIST)
    {
      if (*pipe_return_code == MEMCACHED_MAXIMUM_RETURN)
        *pipe_return_code= MEMCACHED_ALL_EXIST;
      else if (*pipe_return_code == MEMCACHED_ALL_NOT_EXIST)
        *pipe_return_code= MEMCACHED_SOME_EXIST;
    }
    else if (response == MEMCACHED_NOT_EXIST)
    {
      if (*pipe_return_code == MEMCACHED_MAXIMUM_RETURN)
        *pipe_return_code= MEMCACHED_ALL_NOT_EXIST;
      else if (*pipe_return_code == MEMCACHED_ALL_EXIST)
        *pipe_return_code= MEMCACHED_SOME_EXIST;
    }
    else /* failure */
    {
      *pipe_return_code= MEMCACHED_ALL_FAILURE; /* FIXME */
    }
    break;

  case 'i': /* insert */
    if (response == MEMCACHED_STORED or response == MEMCACHED_CREATED_STORED)
    {
      if (*pipe_return_code == MEMCACHED_MAXIMUM_RETURN)
        *pipe_return_code= MEMCACHED_ALL_SUCCESS;
      else if (*pipe_return_code == MEMCACHED_ALL_FAILURE)
        *pipe_return_code= MEMCACHED_SOME_SUCCESS;
    }
    else /* failure */
    {
      if (*pipe_return_code == MEMCACHED_MAXIMUM_RETURN)
        *pipe_return_code= MEMCACHED_ALL_FAILURE;
      else if (*pipe_return_code == MEMCACHED_ALL_SUCCESS)
        *pipe_return_code= MEMCACHED_SOME_SUCCESS;
    }
    break;
  }
}

/*
 * Fetching the piped responses : RESPONSE <count>\r\n
 * (works only for same kind of operations)
 */
static memcached_return_t textual_coll_piped_response_fetch(memcached_server_write_instance_st ptr, char *buffer)
{
#ifdef ENABLE_REPLICATION
  memcached_return_t switchover_rc= MEMCACHED_SUCCESS;
#endif
  memcached_return_t rc= MEMCACHED_SUCCESS;
  memcached_return_t *responses= ptr->root->pipe_responses;
  size_t i, offset= ptr->root->pipe_responses_length;
  uint32_t count[1];

  if (not parse_response_header(buffer, "RESPONSE", 8, count, 1))
  {
    memcached_io_reset(ptr);
    return MEMCACHED_PARTIAL_READ;
  }

  ptr->root->flags.piped= true;

  for (i= 0; i< count[0]; i++)
  {
    memcached_server_response_increment(ptr);
    responses[offset+i]= memcached_coll_response(ptr, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);
#ifdef ENABLE_REPLICATION
    if (responses[offset+i] == MEMCACHED_SWITCHOVER or responses[offset+i] == MEMCACHED_REPL_SLAVE)
    {
      switchover_rc= responses[offset+i];
      for (size_t j= i+1; j< count[0]; j++) {
        memcached_server_response_increment(ptr);
        (void)memcached_coll_response(ptr, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);
      }
      break;
    }
#endif
    aggregate_pipe_return_code(ptr->root, responses[offset+i], &ptr->root->pipe_return_code);
  }
  ptr->root->pipe_responses_length+= i; /* i can be smaller than count[0] */

  ptr->root->flags.piped= false;

  /* We add back in one because we will need to search for END */
  memcached_server_response_increment(ptr);

  rc= memcached_coll_response(ptr, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);
#ifdef ENABLE_REPLICATION
  if (switchover_rc != MEMCACHED_SUCCESS && rc == MEMCACHED_END) {
    return switchover_rc; 
  }
#endif
  return rc;
}

static memcached_return_t textual_coll_fetch_elements(memcached_server_write_instance_st ptr,
                                                      const char *header __attribute__((unused)), ssize_t header_size,
                                                      size_t count, memcached_coll_result_st *result)
{
  memcached_return_t rc;
  const size_t MAX_ELEMENT_BUFFER_SIZE= 100;

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
      rc= fetch_value_header(ptr, to_read_string, &read_length, MAX_ELEMENT_BUFFER_SIZE);
      if (rc != MEMCACHED_SUCCESS && rc != MEMCACHED_END)
        goto read_error;

      if (read_length != header_size + 1) // +1 for a space
        goto read_error;
    }

    /* B+Tree value starts with <bkey> and <eflag> */
    if (result->type == COLL_BTREE)
    {
      /* <bkey> */
      {
        rc= fetch_value_header(ptr, to_read_string, &read_length, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH); // with '0x'
        if (rc != MEMCACHED_SUCCESS && rc != MEMCACHED_END)
          goto read_error;

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
        if (rc != MEMCACHED_SUCCESS && rc != MEMCACHED_END)
          goto read_error;

        /* if the number starts with 0x, it is EFLAG */
        if (to_read_string[0] == '0' && to_read_string[1] == 'x')
        {
          memcached_conv_str_to_hex(ptr->root, to_read_string+2, read_length-2-1, &result->eflags[i]); // except '0x' and '\0'
          has_eflag= true;
        }
      }
    }

    /* <bytes> */
    {
      if (result->type != COLL_BTREE || has_eflag)
      {
        rc= fetch_value_header(ptr, to_read_string, &read_length, BYTES_MAX_LENGTH);
        if (rc != MEMCACHED_SUCCESS && rc != MEMCACHED_END)
          goto read_error;
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
  memcached_io_reset(ptr);

  return MEMCACHED_PARTIAL_READ;
}

/*
 * Fetching collection values.
 *
 * VALUE <flags> <count>\r\n
 * <bytes> <data>\r\n                  (List)
 * <bytes> <data>\r\n                  (Set)
 * <bkey> [<eflag>] <bytes> <data>\r\n (B+Tree)
 */
static memcached_return_t textual_coll_value_fetch(memcached_server_write_instance_st ptr,
                                                   char *buffer, memcached_coll_result_st *result)
{
  uint32_t header_params[4];
  uint32_t num_params= 2;
  int PARAM_FLAGS= 0;
  int PARAM_COUNT= 1;
  int PARAM_POSITION= -1;
  int PARAM_RSTINDEX= -1;

  if (ptr->root->last_op_code[4] == 'p' && ptr->root->last_op_code[5] == 'w') /* bop pwg */
  {
    num_params= 4;
    PARAM_POSITION= 0;
    PARAM_FLAGS= 1;
    PARAM_COUNT= 2;
    PARAM_RSTINDEX= 3;
  }

  if (not parse_response_header(buffer, "VALUE", 5, header_params, num_params) or
      not header_params[PARAM_COUNT] )
  {
    memcached_io_reset(ptr);
    return MEMCACHED_PARTIAL_READ;
  }

  result->collection_flags= header_params[PARAM_FLAGS];
  if (PARAM_POSITION != -1) /* bop pwg */
  {
    result->btree_position= header_params[PARAM_POSITION];
    result->result_position= header_params[PARAM_RSTINDEX];
  }

  size_t count= header_params[PARAM_COUNT];
  if (count < 1)
  {
    return MEMCACHED_SUCCESS;
  }

  /* Prepare memory for the returning values */
  ALLOCATE_ARRAY_OR_RETURN(result->root, result->values, memcached_string_st, count);

  if (result->type == COLL_BTREE)
  {
    ALLOCATE_ARRAY_OR_RETURN(result->root, result->sub_keys, memcached_coll_sub_key_st, count);
    ALLOCATE_ARRAY_OR_RETURN(result->root, result->eflags,   memcached_hexadecimal_st,  count);
  }

  /* Fetch all values */
  return textual_coll_fetch_elements(ptr, "", 0, count, result);
}

/*
 * Fetching collection values for mget.
 *
 * VALUE <key> <status> [<flags> <ecount>]\r\n
 * ELEMENT <bkey> [<eflag>] <bytes> <data>\r\n
 * ...
 */
static memcached_return_t textual_coll_multiple_value_fetch(memcached_server_write_instance_st ptr,
                                                            char *buffer, memcached_coll_result_st *result)
{
  const size_t MEMCACHED_MAX_MGET_BUFFER_SIZE= MEMCACHED_MAX_KEY;

  char *buffer_ptr, *string_ptr;
  ssize_t read_length= 0;

  memcached_return_t rc;
  char to_read_string[MEMCACHED_MAX_MGET_BUFFER_SIZE];

  /* "VALUE " */
  buffer_ptr= buffer;
  buffer_ptr+= 6;

  /* <key> */
  rc= fetch_value_header_with(buffer_ptr, to_read_string, &read_length, MEMCACHED_MAX_MGET_BUFFER_SIZE);
  buffer_ptr+= read_length;
  if (rc == MEMCACHED_SUCCESS or rc == MEMCACHED_END)
  {
    strncpy(result->item_key, to_read_string, read_length);
    result->key_length= read_length;
    result->item_key[result->key_length]= '\0';
  }
  else
  {
    return rc;
  }

  /* <status> */
  memcached_return_t status= MEMCACHED_UNKNOWN_READ_FAILURE;

  rc= fetch_value_header_with(buffer_ptr, to_read_string, &read_length, MEMCACHED_MAX_MGET_BUFFER_SIZE);
  buffer_ptr+= read_length;
  if (rc == MEMCACHED_SUCCESS or rc == MEMCACHED_END)
  {
    switch (to_read_string[0])
    {
    case 'O': /* OK */
        if (to_read_string[1] == 'K') status= MEMCACHED_SUCCESS;
      else                            status= MEMCACHED_OUT_OF_RANGE;
      break;
    case 'N': /* NOT_FOUND or NOT_FOUND_ELEMENT */
        if (to_read_string[9] == '_') status= MEMCACHED_NOTFOUND_ELEMENT;
      else                            status= MEMCACHED_NOTFOUND;
      break;
    case 'D': /* DELETED or DELETED_DROPPED */
        if (to_read_string[7] == '_') status= MEMCACHED_DELETED_DROPPED;
      else                            status= MEMCACHED_DELETED;
    case 'T': /* TYPE_MISMATCH or TRIMMED */
        if (to_read_string[1] == 'R') status= MEMCACHED_TRIMMED;
      else                            status= MEMCACHED_TYPE_MISMATCH;
      break;
    case 'B': /* BKEY_MISMATCH */
      status= MEMCACHED_BKEY_MISMATCH;
      break;
    case 'U': /* UNREADABLE */
      status= MEMCACHED_UNREADABLE;
      break;
    case 'C': /* CLIENT_ERROR */
      status= MEMCACHED_CLIENT_ERROR;
      break;
    case 'S': /* SERVER_ERROR */
      status= MEMCACHED_SERVER_ERROR;
      break;
    }
  }
  else
  {
    return rc;
  }

  if (status != MEMCACHED_SUCCESS and status != MEMCACHED_TRIMMED)
  {
    return status;
  }

  /* <flags> */
  size_t flags= 0;

  rc= fetch_value_header_with(buffer_ptr, to_read_string, &read_length, MEMCACHED_MAX_MGET_BUFFER_SIZE);
  buffer_ptr+= read_length;
  if (rc == MEMCACHED_SUCCESS or rc == MEMCACHED_END)
  {
    flags= static_cast<size_t>(strtoull(to_read_string, &string_ptr, 10));
  }
  else
  {
    return rc;
  }

  /* <ecount> */
  size_t ecount= 0;

  rc= fetch_value_header_with(buffer_ptr, to_read_string, &read_length, MEMCACHED_MAX_MGET_BUFFER_SIZE);
  buffer_ptr+= read_length;
  if (rc == MEMCACHED_SUCCESS or rc == MEMCACHED_END)
  {
    ecount= static_cast<size_t>(strtoull(to_read_string, &string_ptr, 10));
  }
  else
  {
    return rc;
  }

  fprintf(stderr, "[debug] b+tree: status=%s, flags=%lu, ecount=%lu\n", memcached_strerror(NULL, status), flags, ecount);
  result->collection_flags= flags;

  /* Prepare memory for the returning values */
  ALLOCATE_ARRAY_OR_RETURN(result->root, result->values, memcached_string_st, ecount);

  if (result->type == COLL_BTREE)
  {
    ALLOCATE_ARRAY_OR_RETURN(result->root, result->sub_keys, memcached_coll_sub_key_st, ecount);
    ALLOCATE_ARRAY_OR_RETURN(result->root, result->eflags,   memcached_hexadecimal_st,  ecount);
  }

  /* Fetch all values */
  return textual_coll_fetch_elements(ptr, "ELEMENT", 7, ecount, result);
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
  case 'A': /* ATTR */
    if (buffer[4] == ' ') /* ATTR */
    {
      memcached_server_response_increment(ptr);
      return MEMCACHED_ATTR;
    }
    else if (buffer[5] == 'M') /* ATTR_MISMATCH */
      return MEMCACHED_ATTR_MISMATCH;
    else if (buffer[11] == 'n') /* ATTR_ERROR not found */
      return MEMCACHED_ATTR_ERROR_NOT_FOUND;
    else if (buffer[11] == 'b') /* ATTR_ERROR bad value */
      return MEMCACHED_ATTR_ERROR_BAD_VALUE;
    /* This should not be happened */
    return MEMCACHED_NOTFOUND;
  case 'V': /* VALUE */
    if (buffer[1] == 'A') /* VALUE */
    {
      if (ptr->root->last_op_code[4] == 'm')
      {
        /* multiple value fetch */
        rc= textual_coll_multiple_value_fetch(ptr, buffer, result);
      }
      else
      {
        /* single value fetch */
        rc= textual_coll_value_fetch(ptr, buffer, result);
      }

      /* We add back in one because we will need to search for END or next VALUE */
      if (rc == MEMCACHED_SUCCESS)
      {
        memcached_server_response_increment(ptr);
      }

      return rc;
    }
    else
    {
      WATCHPOINT_STRING(buffer);
      return MEMCACHED_UNKNOWN_READ_FAILURE;
    }
  case 'P': /* PIPE_ERROR */
    if (buffer[1] == 'O') /* POSITION */
    {
      return MEMCACHED_POSITION;
    }
    else
    {
      if (buffer[11] == 'c')
        return MEMCACHED_PIPE_ERROR_COMMAND_OVERFLOW;
      else if (buffer[11] == 'm')
        return MEMCACHED_PIPE_ERROR_MEMORY_OVERFLOW;
      else if (buffer[11] == 'b')
        return MEMCACHED_PIPE_ERROR_BAD_ERROR;
      else
      {
        WATCHPOINT_STRING(buffer);
        return MEMCACHED_UNKNOWN_READ_FAILURE;
      }
    }
  case 'O': /* OK */
    if (buffer[1] == 'U') /* OUT_OF_RANGE */
      return MEMCACHED_OUT_OF_RANGE;
    else if (buffer[1] == 'V') /* OVERFLOWED */
      return MEMCACHED_OVERFLOWED;
    return MEMCACHED_SUCCESS;
  case 'S': /* STORED STATS SERVER_ERROR */
    {
      if (buffer[2] == 'A') /* STORED STATS */
      {
        memcached_server_response_increment(ptr);
        return MEMCACHED_STAT;
      }
      else if (buffer[1] == 'E') /* SERVER_ERROR */
      {
        if (total_read == memcached_literal_param_size("SERVER_ERROR"))
        {
          return MEMCACHED_SERVER_ERROR;
        }

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
      else if (buffer[1] == 'T')
      {
        return MEMCACHED_STORED;
      }
#ifdef ENABLE_REPLICATION
      else if (buffer[1] == 'W')
      {
        return MEMCACHED_SWITCHOVER;
      }
#endif
      else
      {
        WATCHPOINT_STRING(buffer);
        return MEMCACHED_UNKNOWN_READ_FAILURE;
      }
    }
  case 'D': /* DELETED or DELETED_DROPPED or DELETED_TRIMMED */
    {
      if (buffer[8] == 'D')
        return MEMCACHED_DELETED_DROPPED;
      else if (buffer[8] == 'T')
        return MEMCACHED_DELETED_TRIMMED;
      else if (buffer[7] == '\r')
        return MEMCACHED_DELETED;
      else
      {
        WATCHPOINT_STRING(buffer);
        return MEMCACHED_UNKNOWN_READ_FAILURE;
      }
    }
  case 'L': /* LENGTH MISMATCH */
    return MEMCACHED_LENGTH_MISMATCH;
  case 'N': /* NOT_FOUND */
    {
      if (buffer[4] == 'F')
      {
        if (buffer[9] == '_')
          return MEMCACHED_NOTFOUND_ELEMENT;
        return MEMCACHED_NOTFOUND;
      }
      else if (buffer[4] == 'S')
      {
        if (buffer[5] == 'T')
          return MEMCACHED_NOTSTORED;
        if (buffer[5] == 'U')
          return MEMCACHED_NOT_SUPPORTED;
      }
      else if (buffer[4] == 'E')
        return MEMCACHED_NOT_EXIST;
      else
      {
        WATCHPOINT_STRING(buffer);
        return MEMCACHED_UNKNOWN_READ_FAILURE;
      }
    }
  case 'R': /* RESPONSE - piped operation */
    {
#ifdef ENABLE_REPLICATION
      if (buffer[1] == 'E' && buffer[4] == '_') {
        return MEMCACHED_REPL_SLAVE;
      }
#endif
      if (buffer[1] == 'E' && buffer[2] == 'P') {
        /* REPLACED in response to bop upsert. */
        return MEMCACHED_REPLACED;
      }
      else {
        /* Assume RESPONSE for piped operations */
        return textual_coll_piped_response_fetch(ptr, buffer);
      }
    }

  case 'U': /* UNREADABLE or UPDATED */
    {
      if (buffer[1] == 'P')
        return MEMCACHED_UPDATED;
      else if (buffer[1] == 'N')
        return MEMCACHED_UNREADABLE;
      else
      {
        WATCHPOINT_STRING(buffer);
        return MEMCACHED_UNKNOWN_READ_FAILURE;
      }
    }
  case 'E': /* PROTOCOL ERROR or END */
    {
      if (buffer[1] == 'N')
        return MEMCACHED_END;
      else if (buffer[1] == 'F')
        return MEMCACHED_EFLAG_MISMATCH;
      else if (buffer[1] == 'R')
        return MEMCACHED_PROTOCOL_ERROR;
      else if (buffer[4] == 'T' && buffer[5] == '\r')
        return MEMCACHED_EXIST; /* COLLECTION SET MEMBERSHIP CHECK */
      else if (buffer[4] == 'T' && buffer[5] == 'S')
        return MEMCACHED_EXISTS; /* EXISTS */
      else if (buffer[1] == 'X')
        return MEMCACHED_DATA_EXISTS;
      else if (buffer[1] == 'L')
        return MEMCACHED_ELEMENT_EXISTS;
      else
      {
        WATCHPOINT_STRING(buffer);
        return MEMCACHED_UNKNOWN_READ_FAILURE;
      }
    }
  case 'T': /* TYPE MISMATCH or TRIMMED */
    {
      if (buffer[1] == 'R')
        return MEMCACHED_TRIMMED;
      else if (buffer[1] == 'Y')
        return MEMCACHED_TYPE_MISMATCH;
      else
      {
        WATCHPOINT_STRING(buffer);
        return MEMCACHED_UNKNOWN_READ_FAILURE;
      }
    }
  case 'I': /* CLIENT ERROR */
    /* We add back in one because we will need to search for END */
    memcached_server_response_increment(ptr);
    return MEMCACHED_ITEM;
  case 'C': /* CLIENT ERROR or CREATED or CREATED_STORED */
    {
      if (buffer[7] == '_')
        return MEMCACHED_CREATED_STORED;
      else if (buffer[6] == 'D')
        return MEMCACHED_CREATED;
      else if (buffer[1] == 'O')
        return MEMCACHED_COUNT;
      return MEMCACHED_CLIENT_ERROR;
    }
  case 'B': /* BKEY_MISMATCH */
    return MEMCACHED_BKEY_MISMATCH;

  default:
    {
      unsigned long long auto_return_value;

      if (sscanf(buffer, "%llu", &auto_return_value) == 1)
        return MEMCACHED_SUCCESS;

      WATCHPOINT_STRING(buffer);
      return MEMCACHED_UNKNOWN_READ_FAILURE;
    }
  }

  /* NOTREACHED */
}

void memcached_add_coll_pipe_return_code(memcached_server_write_instance_st ptr,
                                         memcached_return_t return_code)
{
  memcached_return_t *responses= ptr->root->pipe_responses;

  responses[ptr->root->pipe_responses_length]= return_code;
  ptr->root->pipe_responses_length+= 1;
  aggregate_pipe_return_code(ptr->root, return_code, &ptr->root->pipe_return_code);
}

/* Sort-merge-get */

memcached_return_t memcached_read_one_coll_smget_response(memcached_server_write_instance_st ptr,
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

  unlikely(rc == MEMCACHED_UNKNOWN_READ_FAILURE or
           rc == MEMCACHED_PROTOCOL_ERROR or
           rc == MEMCACHED_CLIENT_ERROR or
           rc == MEMCACHED_KEY_TOO_BIG or
           rc == MEMCACHED_MEMORY_ALLOCATION_FAILURE )
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

/*
 * Fetching B+Tree's sort-merge-get values.
 *
 * VALUE <value_count>\r\n
 * <key> <flags> <bkey> <bytes> <value>\r\n
 * ...
 */
static memcached_return_t textual_coll_smget_value_fetch(memcached_server_write_instance_st ptr,
                                                         char *buffer, memcached_coll_smget_result_st *result)
{
  char *string_ptr;
  size_t to_read;
  ssize_t read_length= 0;

  uint32_t header_params[1];
  const size_t PARAM_COUNT= 0;

  if (not parse_response_header(buffer, "VALUE", 5, header_params, 1))
  {
    memcached_io_reset(ptr);
    return MEMCACHED_PARTIAL_READ;
  }

  size_t count= header_params[PARAM_COUNT];
  if (count < 1)
  {
    return MEMCACHED_SUCCESS;
  }

  /* Prepare memory for the returning values */
  ALLOCATE_ARRAY_OR_RETURN(result->root, result->keys,     memcached_string_st,       count);
  ALLOCATE_ARRAY_OR_RETURN(result->root, result->values,   memcached_string_st,       count);
  ALLOCATE_ARRAY_OR_RETURN(result->root, result->flags,    uint32_t,                  count);
  ALLOCATE_ARRAY_OR_RETURN(result->root, result->sub_keys, memcached_coll_sub_key_st, count);
  ALLOCATE_ARRAY_OR_RETURN(result->root, result->eflags,   memcached_hexadecimal_st,  count);
  ALLOCATE_ARRAY_OR_RETURN(result->root, result->bytes,    size_t,                    count);

  /* Fetch all values */
  size_t i;
  size_t value_length;
  memcached_return_t rrc;

  char *value_ptr;
  char to_read_string[MEMCACHED_MAX_KEY+1];

  for (i=0; i<count; i++)
  {
    bool has_eflag= false;

    /* <key> */
    {
      rrc= fetch_value_header(ptr, to_read_string, &read_length, KEY_MAX_LENGTH);
      if (rrc != MEMCACHED_SUCCESS && rrc != MEMCACHED_END)
      {
        fprintf(stderr, "[debug] key_fetch_error=%s\n", to_read_string);
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
      rrc= fetch_value_header(ptr, to_read_string, &read_length, FLAGS_MAX_LENGTH);
      if (rrc != MEMCACHED_SUCCESS && rrc != MEMCACHED_END)
      {
        return rrc;
      }

      result->flags[i]= static_cast<size_t>(strtoull(to_read_string, &string_ptr, 10));
    }

    /* <bkey> */
    {
      rrc= fetch_value_header(ptr, to_read_string, &read_length, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH+2); // with '0x'
      if (rrc != MEMCACHED_SUCCESS && rrc != MEMCACHED_END)
      {
        return rrc;
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
      rrc= fetch_value_header(ptr, to_read_string, &read_length, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH+2); // with '0x'
      if (rrc != MEMCACHED_SUCCESS && rrc != MEMCACHED_END)
      {
        fprintf(stderr, "[debug] eflag_fetch_error=%s\n", to_read_string);
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
        rrc= fetch_value_header(ptr, to_read_string, &read_length, MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH);
        if (rrc != MEMCACHED_SUCCESS && rrc != MEMCACHED_END)
        {
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
  memcached_io_reset(ptr);

  return MEMCACHED_PARTIAL_READ;
}

/*
 * Fetching the smget missed_keys.
 *
 * MISSED_KEYS <missed_key_count>\r\n
 * <missed_key>\r\n
 * ...
 */
static memcached_return_t textual_coll_smget_missed_key_fetch(memcached_server_write_instance_st ptr,
                                                              char *buffer, memcached_coll_smget_result_st *result)
{
  char *value_ptr;

  uint32_t header_params[1];
  const size_t PARAM_COUNT= 0;

  if (not parse_response_header(buffer, "MISSED_KEYS", 11, header_params, 1))
  {
    memcached_io_reset(ptr);
    return MEMCACHED_PARTIAL_READ;
  }

  size_t count= header_params[PARAM_COUNT];
  if (count < 1)
  {
    return MEMCACHED_SUCCESS;
  }

  /* Prepare memory for the returning values */
  ALLOCATE_ARRAY_OR_RETURN(result->root, result->missed_keys, memcached_string_st, count);

  /* Fetch all values */
  size_t value_length;
  memcached_return_t rrc;

  for (size_t i=0; i<count; i++)
  {
    char to_read_string[MEMCACHED_MAX_KEY+2]; // +2: "\r\n"

    size_t total_read= 0;
    rrc= memcached_io_readline(ptr, to_read_string, MEMCACHED_MAX_KEY+2, total_read);

    if (rrc != MEMCACHED_SUCCESS)
    {
      return rrc;
    }

    /* actual string size */
    value_length= total_read - 2;

    if (value_length > MEMCACHED_MAX_KEY)
    {
      goto read_error;
    }

    /* prepare memory for a value (+2 bytes to walk the \r\n) */
    if (not memcached_string_create(ptr->root, &result->missed_keys[i], value_length+2))
    {
      return MEMCACHED_MEMORY_ALLOCATION_FAILURE;
    }

    value_ptr= memcached_string_value_mutable(&result->missed_keys[i]);
    strncpy(value_ptr, to_read_string, value_length);
    {
      value_ptr[value_length]= 0;
      value_ptr[value_length+1]= 0;
      memcached_string_set_length(&result->missed_keys[i], value_length);
    }

    result->missed_key_count++;
  }

  return MEMCACHED_SUCCESS;

read_error:
  memcached_io_reset(ptr);

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
    case 'A': /* ATTR_MISMATCH */
      {
        if (buffer[1] == 'T')
          return MEMCACHED_ATTR_MISMATCH;
        else
        {
          WATCHPOINT_STRING(buffer);
          return MEMCACHED_UNKNOWN_READ_FAILURE;
        }
      }
    case 'B': /* BKEY_MISMATCH */
      {
        if (buffer[1] == 'K')
          return MEMCACHED_BKEY_MISMATCH;
        else
        {
          WATCHPOINT_STRING(buffer);
          return MEMCACHED_UNKNOWN_READ_FAILURE;
        }
      }
  case 'V': /* VALUE */
    /* We add back in one because we will need to search for MISSED_KEYS */
    memcached_server_response_increment(ptr);
    return textual_coll_smget_value_fetch(ptr, buffer, result);
    /* rc == MEMCACHED_SUCCESS or
     * rc == MEMCACHED_PARTIAL_READ or
     * rc == MEMCACHED_MEMORY_ALLOCATION_FAILURE or
     * rc == one of return values of memcached_io_readline()
     */
  case 'M': /* MISSED_KEYS */
    /* We add back in one because we will need to search for END */
    memcached_server_response_increment(ptr);
    return textual_coll_smget_missed_key_fetch(ptr, buffer, result);
    /* rc == MEMCACHED_SUCCESS or
     * rc == MEMCACHED_PARTIAL_READ or
     * rc == MEMCACHED_MEMORY_ALLOCATION_FAILURE or
     * rc == one of return values of memcached_io_readline()
     */
  case 'E': /* PROTOCOL ERROR or END */
    {
      if (buffer[1] == 'N')
        return MEMCACHED_END;
      else if (buffer[1] == 'R')
        return MEMCACHED_PROTOCOL_ERROR;
      else
      {
        WATCHPOINT_STRING(buffer);
        return MEMCACHED_UNKNOWN_READ_FAILURE;
      }
    }
  case 'O': /* OUT_OF_RANGE */
    if (buffer[1] == 'U') /* OUT_OF_RANGE */
      return MEMCACHED_OUT_OF_RANGE;
    else
    {
      WATCHPOINT_STRING(buffer);
      return MEMCACHED_UNKNOWN_READ_FAILURE;
    }
  case 'D': /* DUPLICATED or DUPLICATED_TRIMMED */
    {
      if (buffer[10] == '_')
        return MEMCACHED_DUPLICATED_TRIMMED;
      else if (buffer[10] == '\r')
        return MEMCACHED_DUPLICATED;
      else
      {
        WATCHPOINT_STRING(buffer);
        return MEMCACHED_UNKNOWN_READ_FAILURE;
      }
    }
  case 'T': /* TYPE MISMATCH or TRIMMED */
    {
      if (buffer[1] == 'R')
        return MEMCACHED_TRIMMED;
      else if (buffer[1] == 'Y')
        return MEMCACHED_TYPE_MISMATCH;
      else
      {
        WATCHPOINT_STRING(buffer);
        return MEMCACHED_UNKNOWN_READ_FAILURE;
      }
    }
  case 'N': /* NOT_FOUND */
    {
      if (buffer[4] == 'F')
      {
        if (buffer[10] == '_')
          return MEMCACHED_NOTFOUND_ELEMENT;
        return MEMCACHED_NOTFOUND;
      }
      else if (buffer[4] == 'S')
      {
        if (buffer[5] == 'T')
          return MEMCACHED_NOTSTORED;
        if (buffer[5] == 'U')
          return MEMCACHED_NOT_SUPPORTED;
      }
      else if (buffer[4] == 'E')
        return MEMCACHED_NOT_EXIST;
      else
      {
        WATCHPOINT_STRING(buffer);
        return MEMCACHED_UNKNOWN_READ_FAILURE;
      }
    }
  case 'U': /* UNREADABLE */
    return MEMCACHED_UNREADABLE;
  case 'I': /* CLIENT ERROR */
    /* We add back in one because we will need to search for END */
    memcached_server_response_increment(ptr);
    return MEMCACHED_ITEM;
  case 'C': /* CLIENT ERROR */
    return MEMCACHED_CLIENT_ERROR;
  case 'S': /* SERVER ERROR */
    return MEMCACHED_SERVER_ERROR;
  default:
    {
      unsigned long long auto_return_value;

      if (sscanf(buffer, "%llu", &auto_return_value) == 1)
        return MEMCACHED_SUCCESS;

      WATCHPOINT_STRING(buffer);
      return MEMCACHED_UNKNOWN_READ_FAILURE;
    }
  }

  /* NOTREACHED */
}
