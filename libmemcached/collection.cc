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

#include "common.h"
#include "libmemcached/arcus_priv.h"

typedef enum {
  LOP_INSERT_OP,
  LOP_GET_OP,
  LOP_DELETE_OP,
  SOP_INSERT_OP,
  SOP_GET_OP,
  SOP_DELETE_OP,
  SOP_EXIST_OP,
  BOP_INSERT_OP,
  BOP_GET_OP,
  BOP_DELETE_OP,
  SETATTRS_OP,
  GETATTRS_OP,
  LOP_CREATE_OP,
  SOP_CREATE_OP,
  BOP_CREATE_OP,
  BOP_UPSERT_OP,
  BOP_UPDATE_OP,
  BOP_INCR_OP,
  BOP_DECR_OP,
  BOP_COUNT_OP,
  BOP_SMGET_OP,
  BOP_MGET_OP,
  UNKNOWN_OP
} memcached_coll_action_t;

static inline const char *coll_op_string(memcached_coll_action_t verb)
{
  switch (verb)
  {
  case LOP_INSERT_OP:           return "lop insert ";
  case LOP_GET_OP:              return "lop get ";
  case LOP_DELETE_OP:           return "lop delete ";
  case SOP_INSERT_OP:           return "sop insert ";
  case SOP_GET_OP:              return "sop get ";
  case SOP_DELETE_OP:           return "sop delete ";
  case SOP_EXIST_OP:            return "sop exist ";
  case BOP_INSERT_OP:           return "bop insert ";
  case BOP_GET_OP:              return "bop get ";
  case BOP_DELETE_OP:           return "bop delete ";
  case SETATTRS_OP:             return "setattr ";
  case GETATTRS_OP:             return "getattr ";
  case LOP_CREATE_OP:           return "lop create ";
  case SOP_CREATE_OP:           return "sop create ";
  case BOP_CREATE_OP:           return "bop create ";
  case BOP_UPSERT_OP:           return "bop upsert ";
  case BOP_UPDATE_OP:           return "bop update ";
  case BOP_INCR_OP:             return "bop incr ";
  case BOP_DECR_OP:             return "bop decr ";
  case BOP_COUNT_OP:            return "bop count ";
  case BOP_SMGET_OP:            return "bop smget ";
  case BOP_MGET_OP:             return "bop mget ";
  case UNKNOWN_OP:              return "unknown";
  default:
    return "invalid action";
  }
}

static inline int coll_op_length(memcached_coll_action_t verb)
{
  switch (verb)
  {
  case LOP_INSERT_OP:           return 11;
  case LOP_GET_OP:              return 8;
  case LOP_DELETE_OP:           return 11;
  case SOP_INSERT_OP:           return 11;
  case SOP_GET_OP:              return 8;
  case SOP_DELETE_OP:           return 11;
  case SOP_EXIST_OP:            return 10;
  case BOP_INSERT_OP:           return 11;
  case BOP_GET_OP:              return 8;
  case BOP_DELETE_OP:           return 11;
  case SETATTRS_OP:             return 8;
  case GETATTRS_OP:             return 8;
  case LOP_CREATE_OP:           return 11;
  case SOP_CREATE_OP:           return 11;
  case BOP_CREATE_OP:           return 11;
  case BOP_UPSERT_OP:           return 11;
  case BOP_UPDATE_OP:           return 11;
  case BOP_INCR_OP:             return 9;
  case BOP_DECR_OP:             return 9;
  case BOP_COUNT_OP:            return 10;
  case BOP_SMGET_OP:            return 10;
  case BOP_MGET_OP:             return 9;
  case UNKNOWN_OP:              return 0;
  default:
    return 0;
  }
}

memcached_coll_type_t find_collection_type_by_opcode(const char *opcode)
{
  switch (opcode[0])
  {
  case 'l': return COLL_LIST;
  case 's': return COLL_SET;
  case 'b': return COLL_BTREE;
  default : return COLL_NONE;
  }
}

static inline const char *coll_overflowaction_string(memcached_coll_overflowaction_t overflowaction)
{
  switch (overflowaction)
  {
  case OVERFLOWACTION_ERROR:            return "error";
  case OVERFLOWACTION_HEAD_TRIM:        return "head_trim";
  case OVERFLOWACTION_TAIL_TRIM:        return "tail_trim";
  case OVERFLOWACTION_SMALLEST_TRIM:    return "smallest_trim";
  case OVERFLOWACTION_LARGEST_TRIM:     return "largest_trim";
  case OVERFLOWACTION_SMALLEST_SILENT_TRIM:    return "smallest_silent_trim";
  case OVERFLOWACTION_LARGEST_SILENT_TRIM:     return "largest_silent_trim";
  case OVERFLOWACTION_NONE:             return "uninitialized";
  default:
    return "invalid overflowaction";
  }
}

static inline memcached_coll_overflowaction_t str_to_overflowaction(const char *value, size_t value_length)
{
       if (strncmp("error",         value, value_length) == 0) return OVERFLOWACTION_ERROR;
  else if (strncmp("head_trim",     value, value_length) == 0) return OVERFLOWACTION_HEAD_TRIM;
  else if (strncmp("tail_trim",     value, value_length) == 0) return OVERFLOWACTION_TAIL_TRIM;
  else if (strncmp("smallest_trim", value, value_length) == 0) return OVERFLOWACTION_SMALLEST_TRIM;
  else if (strncmp("largest_trim",  value, value_length) == 0) return OVERFLOWACTION_LARGEST_TRIM;
  else if (strncmp("smallest_silent_trim", value, value_length) == 0) return OVERFLOWACTION_SMALLEST_SILENT_TRIM;
  else if (strncmp("largest_silent_trim",  value, value_length) == 0) return OVERFLOWACTION_LARGEST_SILENT_TRIM;
  else
  {
    return OVERFLOWACTION_NONE;
  }
}

static inline memcached_coll_type_t str_to_type(const char *value, size_t value_length)
{
       if (strncmp("kv",     value, value_length) == 0) return COLL_KV;
  else if (strncmp("list",   value, value_length) == 0) return COLL_LIST;
  else if (strncmp("set",    value, value_length) == 0) return COLL_SET;
  else if (strncmp("b+tree", value, value_length) == 0) return COLL_BTREE;
  else
  {
    return COLL_NONE;
  }
}

/* Hexadecimal */

int memcached_compare_two_hexadecimal(memcached_hexadecimal_st *lhs, memcached_hexadecimal_st *rhs)
{
  if (not lhs or not rhs)
  {
    return -2;
  }

  size_t min= (lhs->length < rhs->length)? lhs->length : rhs->length;

  /* In lexicographical order */
  for (size_t i=0; i<min; i++)
  {
    if (lhs->array[i] == rhs->array[i]) continue;
    if (lhs->array[i] <  rhs->array[i]) return -1;
    else                                return 1;
  }

  if (lhs->length == rhs->length) return  0;
  if (lhs->length <  rhs->length) return -1;
  else                            return  1;
}

static inline void convert_hex_to_str(const unsigned char *bin, char *str, const int size)
{
  if (not str)
  {
    return;
  }

  for (int i=0; i < size; i++)
  {
      str[(i*2)  ] = (bin[i] & 0xF0) >> 4;
      str[(i*2)+1] = (bin[i] & 0x0F);

      if (str[(i*2)  ] < 10) str[(i*2)  ] += ('0');
      else                   str[(i*2)  ] += ('A' - 10);
      if (str[(i*2)+1] < 10) str[(i*2)+1] += ('0');
      else                   str[(i*2)+1] += ('A' - 10);
  }

  str[size*2] = '\0';
}

static inline bool convert_str_to_hex(const char *str, unsigned char *bin, const int size) {
    int  slen = strlen(str);
    char ch1, ch2;

    if (slen <= 0 || slen > (2*size) || (slen%2) != 0)
    {
      return false;
    }

    for (int i=0; i < (slen/2); i++)
    {
      ch1 = str[2*i]; ch2 = str[2*i+1];

      if      (ch1 >= '0' && ch1 <= '9') bin[i] = (ch1 - '0');
      else if (ch1 >= 'A' && ch1 <= 'F') bin[i] = (ch1 - 'A' + 10);
      else if (ch1 >= 'a' && ch1 <= 'f') bin[i] = (ch1 - 'a' + 10);
      else return false;

      if      (ch2 >= '0' && ch2 <= '9') bin[i] = (bin[i] << 4) + (ch2 - '0');
      else if (ch2 >= 'A' && ch2 <= 'F') bin[i] = (bin[i] << 4) + (ch2 - 'A' + 10);
      else if (ch2 >= 'a' && ch2 <= 'f') bin[i] = (bin[i] << 4) + (ch2 - 'a' + 10);
      else return false;
    }

    return true;
}

memcached_return_t memcached_conv_hex_to_str(memcached_st *ptr __attribute__((unused)),
                                             memcached_hexadecimal_st *hex, char *str, size_t str_length)
{
  if (str_length <= hex->length*2)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  convert_hex_to_str(hex->array, str, hex->length);

  return MEMCACHED_SUCCESS;
}

memcached_return_t memcached_conv_str_to_hex(memcached_st *ptr,
                                             char *str, size_t str_length, memcached_hexadecimal_st *hex)
{
  if (str_length > MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH or hex == NULL)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  ALLOCATE_ARRAY_OR_RETURN(ptr, hex->array, unsigned char, str_length/2);
  hex->length = str_length/2;
  hex->options.array_is_allocated= true;

  convert_str_to_hex(str, hex->array, hex->length);

  return MEMCACHED_SUCCESS;
}

/* Filter */

static inline const char *bitwise_to_str(memcached_coll_bitwise_t bitwise)
{
  switch (bitwise)
  {
  case MEMCACHED_COLL_BITWISE_AND: return "&";
  case MEMCACHED_COLL_BITWISE_OR: return "|";
  case MEMCACHED_COLL_BITWISE_XOR: return "^";
  default: return "ERROR";
  }
}

static inline const char *comp_to_str(memcached_coll_comp_t comp)
{
  switch (comp)
  {
  case MEMCACHED_COLL_COMP_EQ: return "EQ";
  case MEMCACHED_COLL_COMP_NE: return "NE";
  case MEMCACHED_COLL_COMP_LT: return "LT";
  case MEMCACHED_COLL_COMP_LE: return "LE";
  case MEMCACHED_COLL_COMP_GT: return "GT";
  case MEMCACHED_COLL_COMP_GE: return "GE";
  default: return "ERROR";
  }
}

static inline size_t memcached_coll_eflag_filter_to_str(memcached_coll_eflag_filter_st *filter, char *buffer, size_t buffer_length)
{
  if (not filter)
  {
    buffer[0] = 0;
    return 0;
  }

  size_t length;

#ifdef ENABLE_EFLAGS_FILTER
  if (filter->options.is_bitwised)
  {
    char foperand_str[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
    memcached_conv_hex_to_str(NULL, &filter->bitwise.foperand,
                              foperand_str, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);

    length = (size_t)snprintf(buffer, buffer_length, " %u %s 0x%s %s ",
                      (int)filter->fwhere,
                      bitwise_to_str(filter->bitwise.op), foperand_str,
                      comp_to_str(filter->comp.op));
  }
  else
  {
    length = (size_t)snprintf(buffer, buffer_length, " %u %s ",
                      (int)filter->fwhere,
                      comp_to_str(filter->comp.op));
  }

  char fvalue_str[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
  for (int i = 0; i < (int)filter->comp.count; i++)
  {
    memcached_conv_hex_to_str(NULL, &filter->comp.fvalue[i],
                              fvalue_str, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);

    length += (size_t)snprintf(buffer+length, buffer_length, "%s0x%s",
                      ((i == 0)?"":","), fvalue_str);
  }
#else
  char fvalue_str[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
  memcached_conv_hex_to_str(NULL, &filter->comp.fvalue,
                            fvalue_str, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);

  if (filter->options.is_bitwised)
  {
    char foperand_str[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
    memcached_conv_hex_to_str(NULL, &filter->bitwise.foperand,
                              foperand_str, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);

    length = snprintf(buffer, buffer_length, " %u %s 0x%s %s 0x%s",
                      (int)filter->fwhere,
                      bitwise_to_str(filter->bitwise.op), foperand_str,
                      comp_to_str(filter->comp.op), fvalue_str);
  }
  else
  {
    length = snprintf(buffer, buffer_length, " %u %s 0x%s",
                      (int)filter->fwhere,
                      comp_to_str(filter->comp.op), fvalue_str);
  }
#endif

  return length;
}

static inline size_t memcached_coll_update_filter_to_str(memcached_coll_update_filter_st *filter, char *buffer, size_t buffer_length)
{
  if (not filter)
  {
    buffer[0] = 0;
    return 0;
  }

  size_t length;

  char fvalue_str[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
  memcached_conv_hex_to_str(NULL, &filter->comp.fvalue,
                            fvalue_str, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);

  if (filter->options.is_bitwised)
  {
    length = snprintf(buffer, buffer_length, " %u %s 0x%s",
                      (int)filter->fwhere,
                      bitwise_to_str(filter->bitwise.op), fvalue_str);
  }
  else
  {
    length = snprintf(buffer, buffer_length, " 0x%s",
                      fvalue_str);
  }

  return length;
}

/* Attributes getters/setters */

memcached_return_t memcached_coll_attrs_init(memcached_coll_attrs_st *ptr)
{
  if (not ptr)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  ptr->options.is_initialized = true;

  ptr->flags = 0;
  ptr->expiretime = 0;
  ptr->maxcount = 0;
  ptr->overflowaction = OVERFLOWACTION_NONE;
  ptr->readable = false;
  ptr->trimmed = 0;

  ptr->options.set_flags = false;
  ptr->options.set_expiretime = false;
  ptr->options.set_maxcount = false;
  ptr->options.set_overflowaction = false;
  ptr->options.set_readable = false;
  ptr->options.set_maxbkeyrange = false;

  return MEMCACHED_SUCCESS;
}

memcached_return_t memcached_coll_attrs_set_flags(memcached_coll_attrs_st *attrs, uint32_t flags)
{
  attrs->flags = flags;
  attrs->options.set_flags = true;

  return MEMCACHED_SUCCESS;
}

uint32_t memcached_coll_attrs_get_flags(memcached_coll_attrs_st *attrs)
{
  return attrs->flags;
}

memcached_return_t memcached_coll_attrs_set_expiretime(memcached_coll_attrs_st *attrs, int32_t expiretime)
{
  attrs->expiretime = expiretime;
  attrs->options.set_expiretime = true;

  return MEMCACHED_SUCCESS;
}

int32_t memcached_coll_attrs_get_expiretime(memcached_coll_attrs_st *attrs)
{
  return attrs->expiretime;
}

memcached_return_t memcached_coll_attrs_set_maxcount(memcached_coll_attrs_st *attrs, uint32_t maxcount)
{
  attrs->maxcount = maxcount;
  attrs->options.set_maxcount = true;

  return MEMCACHED_SUCCESS;
}

uint32_t memcached_coll_attrs_get_maxcount(memcached_coll_attrs_st *attrs)
{
  return attrs->maxcount;
}

memcached_return_t memcached_coll_attrs_set_readable_on(memcached_coll_attrs_st *attrs)
{
  attrs->readable = true;
  attrs->options.set_readable = true;

  return MEMCACHED_SUCCESS;
}

bool memcached_coll_attrs_is_readable(memcached_coll_attrs_st *attrs)
{
  return attrs->readable;
}

uint32_t memcached_coll_attrs_get_minbkey(memcached_coll_attrs_st *attrs)
{
  if (attrs->options.subkey_type != MEMCACHED_COLL_QUERY_BOP) {
    return 0;
  }
  return attrs->minbkey.bkey;
}
uint32_t memcached_coll_attrs_get_maxbkey(memcached_coll_attrs_st *attrs)
{
  if (attrs->options.subkey_type != MEMCACHED_COLL_QUERY_BOP) {
    return 0;
  }
  return attrs->maxbkey.bkey;
}
memcached_return_t memcached_coll_attrs_get_minbkey_by_byte(memcached_coll_attrs_st *attrs, unsigned char **bkey, size_t *size)
{
  if (attrs->options.subkey_type != MEMCACHED_COLL_QUERY_BOP_EXT) {
    return MEMCACHED_INVALID_ARGUMENTS;
  }
  *bkey= attrs->minbkey.bkey_ext.array;
  *size= attrs->minbkey.bkey_ext.length;
  return MEMCACHED_SUCCESS;
}
memcached_return_t memcached_coll_attrs_get_maxbkey_by_byte(memcached_coll_attrs_st *attrs, unsigned char **bkey, size_t *size)
{
  if (attrs->options.subkey_type != MEMCACHED_COLL_QUERY_BOP_EXT) {
    return MEMCACHED_INVALID_ARGUMENTS;
  }
  *bkey= attrs->maxbkey.bkey_ext.array;
  *size= attrs->maxbkey.bkey_ext.length;
  return MEMCACHED_SUCCESS;
}

memcached_return_t memcached_coll_attrs_set_maxbkeyrange(memcached_coll_attrs_st *attrs, uint32_t maxbkeyrange)
{
  attrs->maxbkeyrange.bkey = maxbkeyrange;
  attrs->options.subkey_type = MEMCACHED_COLL_QUERY_BOP;
  attrs->options.set_maxbkeyrange = true;

  return MEMCACHED_SUCCESS;
}

uint32_t memcached_coll_attrs_get_maxbkeyrange(memcached_coll_attrs_st *attrs)
{
  if (attrs->options.subkey_type != MEMCACHED_COLL_QUERY_BOP) {
    return 0;
  }
  return attrs->maxbkeyrange.bkey;
}

memcached_return_t memcached_coll_attrs_set_maxbkeyrange_by_byte(memcached_coll_attrs_st *attrs, unsigned char *maxbkeyrange, size_t maxbkeyrange_size)
{
  if (maxbkeyrange == NULL
      || maxbkeyrange_size == 0
      || maxbkeyrange_size > MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH) {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  attrs->maxbkeyrange.bkey_ext.array = maxbkeyrange;
  attrs->maxbkeyrange.bkey_ext.length = maxbkeyrange_size;
  attrs->maxbkeyrange.bkey_ext.options.array_is_allocated = false;
  attrs->options.subkey_type = MEMCACHED_COLL_QUERY_BOP_EXT;
  attrs->options.set_maxbkeyrange = true;
  return MEMCACHED_SUCCESS;
}

memcached_return_t memcached_coll_attrs_get_maxbkeyrange_by_byte(memcached_coll_attrs_st *attrs, unsigned char **maxbkeyrange, size_t *maxbkeyrange_size)
{
  if (attrs->options.subkey_type != MEMCACHED_COLL_QUERY_BOP_EXT) {
    return MEMCACHED_INVALID_ARGUMENTS;
  }
  *maxbkeyrange = attrs->maxbkeyrange.bkey_ext.array;
  *maxbkeyrange_size = attrs->maxbkeyrange.bkey_ext.length;
  return MEMCACHED_SUCCESS;
}

memcached_return_t memcached_coll_attrs_set_overflowaction(memcached_coll_attrs_st *attrs, memcached_coll_overflowaction_t overflowaction)
{
  attrs->overflowaction= overflowaction;
  attrs->options.set_overflowaction= true;

  return MEMCACHED_SUCCESS;
}

memcached_coll_overflowaction_t memcached_coll_attrs_get_overflowaction(memcached_coll_attrs_st *attrs)
{
  return attrs->overflowaction;
}

uint32_t memcached_coll_attrs_get_trimmed(memcached_coll_attrs_st *attrs)
{
  return attrs->trimmed;
}

memcached_return_t memcached_set_attrs(memcached_st *ptr,
                                       const char *key, size_t key_length,
                                       memcached_coll_attrs_st *attrs)
{
  arcus_server_check_for_update(ptr);

  memcached_return_t rc = before_query(ptr, &key, &key_length, 1);
  if (rc != MEMCACHED_SUCCESS)
    return rc;

  /* Find a memcached */
  uint32_t server_key= memcached_generate_hash_with_redistribution(ptr, key, key_length);
  memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, server_key);

  /* Command */
  const char *command= coll_op_string(SETATTRS_OP);
  uint8_t command_length= coll_op_length(SETATTRS_OP);

  /* Attributes */
  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
  size_t write_length= 0;

  if (attrs->options.set_maxcount)
    write_length+= (size_t) snprintf(buffer + write_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                     " maxcount=%u", attrs->maxcount);

  if (attrs->options.set_expiretime)
    write_length+= (size_t) snprintf(buffer + write_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                     " expiretime=%d", attrs->expiretime);

  if (attrs->options.set_overflowaction)
    write_length+= (size_t) snprintf(buffer + write_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                     " overflowaction=%s", coll_overflowaction_string(attrs->overflowaction));

  if (attrs->options.set_readable)
    write_length+= (size_t) snprintf(buffer + write_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                     " readable=on");

  if (attrs->options.set_maxbkeyrange)
  {
    if (attrs->options.subkey_type == MEMCACHED_COLL_QUERY_BOP)
    {
      write_length+= (size_t) snprintf(buffer + write_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                       " maxbkeyrange=%" PRIu64, attrs->maxbkeyrange.bkey);
    }
    else if (attrs->options.subkey_type == MEMCACHED_COLL_QUERY_BOP_EXT)
    {
      char hex_string[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
      memcached_conv_hex_to_str(ptr, &attrs->maxbkeyrange.bkey_ext,
                                hex_string, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);
      write_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                      " maxbkeyrange=0x%s", hex_string);
    }
  }

  if (write_length >= MEMCACHED_DEFAULT_COMMAND_SIZE)
  {
    rc= memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                            memcached_literal_param("snprintf(MEMCACHED_DEFAULT_COMMAND_SIZE)"));
  }
  else
  {
    bool to_write = false;
    struct libmemcached_io_vector_st vector[]=
    {
      { command_length, command },
      { key_length, key },
      { write_length, buffer },
      { 2, "\r\n" }
    };

    if (ptr->flags.buffer_requests)
      to_write= false;
    else
      to_write= true;

    /* Send command header */
    rc=  memcached_vdo(instance, vector, 4, to_write);

    WATCHPOINT_IFERROR(rc);

    if (rc == MEMCACHED_SUCCESS)
    {
      if (to_write == false)
      {
        rc= MEMCACHED_BUFFERED;
      }
      else if (ptr->flags.no_reply)
      {
        rc= MEMCACHED_SUCCESS;
      }
      else
      {
        // expecting OK (MEMCACHED_SUCCESS)
        rc= memcached_coll_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);
      }
    }
  }

  memcached_set_last_response_code(ptr, rc);

  if (rc == MEMCACHED_WRITE_FAILURE)
    memcached_io_reset(instance);

  return rc;
}

memcached_return_t memcached_get_attrs(memcached_st *ptr,
                                       const char *key, size_t key_length,
                                       memcached_coll_attrs_st *attrs)
{
  arcus_server_check_for_update(ptr);

  memcached_return_t rc = before_query(ptr, &key, &key_length, 1);
  if (rc != MEMCACHED_SUCCESS)
    return rc;

  /* Find a memcached */
  uint32_t server_key= memcached_generate_hash_with_redistribution(ptr, key, key_length);
  memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, server_key);

  /* Command */
  const char *command= coll_op_string(GETATTRS_OP);
  uint8_t command_length= coll_op_length(GETATTRS_OP);

  struct libmemcached_io_vector_st vector[]=
  {
    { command_length, command },
    { key_length, key },
    { 2, "\r\n" }
  };

  /* Request */
  rc= memcached_vdo(instance, vector, 3, true);

  if (rc != MEMCACHED_SUCCESS)
    return rc;

  /* Response */
  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE]; // Uninitialized... valgrind would warn about this, but that would be okay.
  while ((rc= memcached_coll_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL)) == MEMCACHED_ATTR)
  {
    int c = 0;
    char seps[] = "=";

    char attr_name[32];

    char *word_buffer = NULL;
    char *word = NULL;

    // <NAME>=<VALUE>\r\n
    for (word = strtok_r(buffer+5, seps, &word_buffer); word;
         word = strtok_r(NULL, seps, &word_buffer), c++) {
      if (c == 0) {
        /* NAME */
        snprintf(attr_name, sizeof(attr_name), "%s", word);
      } else if (c == 1) {
        /* VALUE */
        char *e = strchr(word, '\n');
        size_t word_length = (size_t)(e - word) + 1;
        if (strncmp("flags", attr_name, sizeof(attr_name)) == 0)
        {
          attrs->flags = (uint32_t)atoi(word);
        }
        else if (strncmp("expiretime", attr_name, sizeof(attr_name)) == 0)
        {
          attrs->expiretime = (int32_t)atoi(word);
        }
        else if (strncmp("type", attr_name, sizeof(attr_name)) == 0)
        {
          word[word_length - 2] = '\0'; // \r
          word[word_length - 1] = '\0'; // \n
          attrs->type = str_to_type(word, strlen(word));
        }
        else if (strncmp("count", attr_name, sizeof(attr_name)) == 0)
        {
          attrs->count = (uint32_t)atoi(word);
        }
        else if (strncmp("maxcount", attr_name, sizeof(attr_name)) == 0)
        {
          attrs->maxcount = (uint32_t)atoi(word);
        }
        else if (strncmp("overflowaction", attr_name, sizeof(attr_name)) == 0)
        {
          word[word_length - 2] = '\0'; // \r
          word[word_length - 1] = '\0'; // \n
          attrs->overflowaction = str_to_overflowaction(word, strlen(word));
        }
        else if (strncmp("readable", attr_name, sizeof(attr_name)) == 0)
        {
          attrs->readable = word[0] - '0';
        }
        else if (strncmp("minbkey", attr_name, sizeof(attr_name)) == 0)
        {
          if (word_length > 6 && word[0] == '0' && word[1] == 'x') // 0x00\r\n
          {
            // byte array bkey
            rc = memcached_conv_str_to_hex(ptr, word+2, word_length-4, &attrs->minbkey.bkey_ext);
            if (rc == MEMCACHED_SUCCESS)
            {
              attrs->options.subkey_type = MEMCACHED_COLL_QUERY_BOP_EXT;
            }
            else
            {
              attrs->minbkey.bkey = 0;
              attrs->options.subkey_type = MEMCACHED_COLL_QUERY_BOP;
            }
          }
          else
          {
            // long type bkey
            attrs->minbkey.bkey = (uint64_t)strtoull(word, NULL, 10);
            attrs->options.subkey_type = MEMCACHED_COLL_QUERY_BOP;
          }
        }
        else if (strncmp("maxbkey", attr_name, sizeof(attr_name)) == 0)
        {
          if (word_length > 6 && word[0] == '0' && word[1] == 'x') // 0x00\r\n
          {
            // byte array bkey
            rc = memcached_conv_str_to_hex(ptr, word+2, word_length-4, &attrs->maxbkey.bkey_ext);
            if (rc == MEMCACHED_SUCCESS)
            {
              attrs->options.subkey_type = MEMCACHED_COLL_QUERY_BOP_EXT;
            }
            else
            {
              attrs->maxbkey.bkey = 0;
              attrs->options.subkey_type = MEMCACHED_COLL_QUERY_BOP;
            }
          }
          else
          {
            // long type bkey
            attrs->maxbkey.bkey = (uint64_t)strtoull(word, NULL, 10);
            attrs->options.subkey_type = MEMCACHED_COLL_QUERY_BOP;
          }
        }
        else if (strncmp("maxbkeyrange", attr_name, sizeof(attr_name)) == 0)
        {
          if (word_length > 6 && word[0] == '0' && word[1] == 'x') // 0x00\r\n
          {
            // byte array bkey : attrs->maxbkeyrange.bkey_ext must be freed.
            rc = memcached_conv_str_to_hex(ptr, word+2, word_length-4, &attrs->maxbkeyrange.bkey_ext);
            if (rc == MEMCACHED_SUCCESS)
            {
              attrs->options.subkey_type = MEMCACHED_COLL_QUERY_BOP_EXT;
            }
            else
            {
              attrs->maxbkeyrange.bkey = 0;
              attrs->options.subkey_type = MEMCACHED_COLL_QUERY_BOP;
            }
          }
          else
          {
            // long type bkey
            attrs->maxbkeyrange.bkey = (uint64_t)strtoull(word, NULL, 10);
            attrs->options.subkey_type = MEMCACHED_COLL_QUERY_BOP;
          }
        }
        else if (strncmp("trimmed", attr_name, sizeof(attr_name)) == 0)
        {
          attrs->trimmed = (uint32_t)atoi(word);
        }
      } else {
        break;
      }
    }
  }

  memcached_set_last_response_code(ptr, rc);

  if (rc == MEMCACHED_END) {
    rc = MEMCACHED_SUCCESS;
  }

  return rc;
}

/* Internal collection operations */

static memcached_return_t do_coll_create(memcached_st *ptr,
                                         const char *key, size_t key_length,
                                         memcached_coll_create_attrs_st *attributes,
                                         memcached_coll_action_t verb)
{
  arcus_server_check_for_update(ptr);

  memcached_return_t rc = before_query(ptr, &key, &key_length, 1);
  if (rc != MEMCACHED_SUCCESS)
    return rc;

  /* Check function arguments */
  if (not attributes)
  {
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                               memcached_literal_param("Invalid attributes were provided"));
  }

  uint32_t server_key= memcached_generate_hash_with_redistribution(ptr, key, key_length);
  memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, server_key);

  /* Command */
  const char *command= coll_op_string(verb);
  uint8_t command_length= coll_op_length(verb);
  ptr->last_op_code= command;

  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
  size_t write_length= 0;

  /* Query header */
  bool set_overflowaction= verb != SOP_CREATE_OP and
                           attributes->overflowaction and
                           attributes->overflowaction != OVERFLOWACTION_NONE;

  write_length+= (size_t) snprintf(buffer+write_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                   " %u %d %u%s%s%s%s",
                                   attributes->flags, attributes->expiretime, attributes->maxcount,
                                   (set_overflowaction)? " " : "",
                                   (set_overflowaction)? coll_overflowaction_string(attributes->overflowaction) : "",
                                   (attributes->is_unreadable)? " unreadable" : "",
                                   (ptr->flags.no_reply)? " noreply" : (ptr->flags.piped)? " pipe" : "");

  if (write_length >= MEMCACHED_DEFAULT_COMMAND_SIZE)
  {
    return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                               memcached_literal_param("snprintf(MEMCACHED_DEFAULT_COMMAND_SIZE)"));
  }

  /* Request */
  bool to_write= not ptr->flags.buffer_requests;

  struct libmemcached_io_vector_st vector[]=
  {
    { command_length, command },
    { key_length, key },
    { write_length, buffer },
    { 2, "\r\n" }
  };

  rc= memcached_vdo(instance, vector, 4, to_write);

  if (rc == MEMCACHED_SUCCESS)
  {
    if (to_write == false)
    {
      rc= MEMCACHED_BUFFERED;
    }
    else if (ptr->flags.no_reply || ptr->flags.piped)
    {
      rc= MEMCACHED_SUCCESS;
    }
    else
    {
      rc= memcached_coll_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);
      memcached_set_last_response_code(ptr, rc);

      if (rc == MEMCACHED_CREATED)
      {
        rc= MEMCACHED_SUCCESS;
      }
    }
  }

  if (rc == MEMCACHED_WRITE_FAILURE)
  {
    memcached_io_reset(instance);
  }

  return rc;
}

static memcached_return_t internal_coll_insert(memcached_st *ptr,
                                               memcached_server_write_instance_st instance,
                                               const char *key, size_t key_length,
                                               memcached_coll_query_st *query,
                                               memcached_hexadecimal_st *eflag,
                                               memcached_coll_create_attrs_st *attributes,
                                               memcached_coll_action_t verb)
{
  /* Already called before_operation(). */

  memcached_return_t rc= MEMCACHED_SUCCESS;

  if (not instance)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  /* Command */
  const char *command= coll_op_string(verb);
  uint8_t command_length= coll_op_length(verb);
  ptr->last_op_code= command;

  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
  size_t write_length= 0;

  /* Query header */

  /* 1. sub key */
  if (verb == LOP_INSERT_OP)
  {
    write_length= (size_t) snprintf(buffer, 30, " %d", query->sub_key.index);
  }
  else if (verb == SOP_INSERT_OP)
  {
    /* no sub key */
  }
  else if (verb == BOP_INSERT_OP || verb == BOP_UPSERT_OP)
  {
    if (MEMCACHED_COLL_QUERY_BOP == query->type)
    {
      write_length= (size_t) snprintf(buffer, 30, " %llu", (unsigned long long) query->sub_key.bkey);
    }
    else if (MEMCACHED_COLL_QUERY_BOP_EXT == query->type)
    {
      char bkey_str[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
      memcached_conv_hex_to_str(ptr, &query->sub_key.bkey_ext,
                                bkey_str, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);
      write_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, " 0x%s", bkey_str);
    }

    if (eflag && eflag->array)
    {
      char eflag_str[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
      memcached_conv_hex_to_str(ptr, eflag,
                                eflag_str, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);
      write_length+= (size_t) snprintf(buffer+write_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                       " 0x%s", eflag_str);
    }
  }
  else
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  /* 2. value length */
  write_length+= (size_t) snprintf(buffer+write_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                   " %u", (int)query->value_length);

  /* 3. creation attributes */
  if (attributes)
  {
    bool set_overflowaction= verb != SOP_INSERT_OP
                                  && attributes->overflowaction
                                  && attributes->overflowaction != OVERFLOWACTION_NONE;

    write_length+= (size_t) snprintf(buffer+write_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                     " create %u %d %u%s%s%s%s",
                                     attributes->flags, attributes->expiretime, attributes->maxcount,
                                     (set_overflowaction)? " " : "",
                                     (set_overflowaction)? coll_overflowaction_string(attributes->overflowaction) : "",
                                     (attributes->is_unreadable)? " unreadable" : "",
                                     (ptr->flags.no_reply)? " noreply" : (ptr->flags.piped)? " pipe" : "");
  }

  if (write_length >= MEMCACHED_DEFAULT_COMMAND_SIZE)
  {
    return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                               memcached_literal_param("snprintf(MEMCACHED_DEFAULT_COMMAND_SIZE)"));
  }

  bool to_write= not ptr->flags.buffer_requests;

  struct libmemcached_io_vector_st vector[]=
  {
    { command_length, command },
    { key_length, key },
    { write_length, buffer },
    { 2, "\r\n" },
    { query->value_length, query->value },
    { 2, "\r\n" }
  };

  /* Send command header */
  rc= memcached_vdo(instance, vector, 6, to_write);

  if (rc == MEMCACHED_SUCCESS)
  {
    if (to_write == false)
    {
      rc= MEMCACHED_BUFFERED;
    }
    else if (ptr->flags.no_reply or ptr->flags.piped)
    {
      rc= MEMCACHED_SUCCESS;
    }
    else
    {
      rc= memcached_coll_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);
      memcached_set_last_response_code(ptr, rc);

      if (rc == MEMCACHED_STORED || rc == MEMCACHED_CREATED_STORED)
      {
        rc= MEMCACHED_SUCCESS;
      }
      else if (rc == MEMCACHED_REPLACED && verb == BOP_UPSERT_OP)
      {
        /* bop upsert returns REPLACED if the same bkey element is replaced. */
        rc= MEMCACHED_SUCCESS;
      }
    }
  }

  if (rc == MEMCACHED_WRITE_FAILURE)
    memcached_io_reset(instance);

  return rc;
}

static memcached_return_t internal_coll_piped_insert(memcached_st *ptr,
                                                     memcached_server_write_instance_st instance,
                                                     const char *key, size_t key_length,
                                                     memcached_coll_query_st *query,
                                                     memcached_hexadecimal_st *eflag,
                                                     memcached_coll_create_attrs_st *attributes,
                                                     memcached_return_t *responses, size_t responses_cursor,
                                                     memcached_return_t *piped_rc,
                                                     memcached_coll_action_t verb)
{
  /* Already called before_operation(). */

  memcached_return_t rc= MEMCACHED_SUCCESS;

  if (not instance)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  /* Command */
  const char *command= coll_op_string(verb);
  uint8_t command_length= coll_op_length(verb);
  ptr->last_op_code= command;

  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
  size_t write_length= 0;

  /* Query header */

  /* 1. sub key */
  if (verb == LOP_INSERT_OP)
  {
    write_length= (size_t) snprintf(buffer, 30, " %d", query->sub_key.index);
  }
  else if (verb == SOP_INSERT_OP)
  {
    /* no sub key */
  }
  else if (verb == BOP_INSERT_OP)
  {
    if (MEMCACHED_COLL_QUERY_BOP == query->type)
    {
      write_length= (size_t) snprintf(buffer, 30, " %llu", (unsigned long long) query->sub_key.bkey);
    }
    else if (MEMCACHED_COLL_QUERY_BOP_EXT == query->type)
    {
      char bkey_str[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
      memcached_conv_hex_to_str(ptr, &query->sub_key.bkey_ext,
                                bkey_str, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);

      write_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                      " 0x%s", bkey_str);
    }

    if (eflag && eflag->array)
    {
      char eflag_str[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
      memcached_conv_hex_to_str(ptr, eflag,
                                eflag_str, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);

      write_length+= (size_t) snprintf(buffer+write_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                       " 0x%s", eflag_str);
    }
  }
  else
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  /* 2. value length */
  write_length+= (size_t) snprintf(buffer+write_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                   " %u", (int)query->value_length);

  /* 3. creation attributes */
  if (attributes)
  {
    bool set_overflowaction= verb != SOP_INSERT_OP
                                  && attributes->overflowaction
                                  && attributes->overflowaction != OVERFLOWACTION_NONE;

    write_length+= (size_t) snprintf(buffer+write_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                     " create %u %d %u%s%s%s",
                                     attributes->flags, attributes->expiretime, attributes->maxcount,
                                     (set_overflowaction)? " " : "",
                                     (set_overflowaction)? coll_overflowaction_string(attributes->overflowaction) : "",
                                     (attributes->is_unreadable)? " unreadable" : "");
  }

  write_length+= (size_t) snprintf(buffer+write_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                   "%s", (ptr->flags.no_reply)? " noreply" : (ptr->flags.piped)? " pipe" : "");

  if (write_length >= MEMCACHED_DEFAULT_COMMAND_SIZE)
  {
    return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                               memcached_literal_param("snprintf(MEMCACHED_DEFAULT_COMMAND_SIZE)"));
  }

  bool to_write = true; /* do not buffer requests internally. */
  int request_length= write_length + command_length + key_length + query->value_length + 4;

  /* Buffer piped request */
  if (ptr->flags.piped)
  {
    if (ptr->pipe_buffer_pos + request_length >= MEMCACHED_COLL_MAX_PIPED_BUFFER_SIZE)
    {
      /* flushing */
      struct libmemcached_io_vector_st buffered_vector[]=
      {
        { ptr->pipe_buffer_pos, ptr->pipe_buffer }
      };
      rc= memcached_vdo(instance, buffered_vector, 1, to_write);
      ptr->pipe_buffer_pos = 0;
    }

    /* buffering */
    ptr->pipe_buffer_pos += snprintf(ptr->pipe_buffer+ptr->pipe_buffer_pos,
                                     MEMCACHED_COLL_MAX_PIPED_BUFFER_SIZE - ptr->pipe_buffer_pos,
                                     "%s%s%s\r\n", command, key, buffer);
    memcpy(ptr->pipe_buffer+ptr->pipe_buffer_pos, query->value, query->value_length);
    ptr->pipe_buffer_pos += query->value_length;
    ptr->pipe_buffer[ptr->pipe_buffer_pos++] = '\r';
    ptr->pipe_buffer[ptr->pipe_buffer_pos++] = '\n';

    if (rc == MEMCACHED_WRITE_FAILURE)
    {
      memcached_io_reset(instance);
    }

    return rc;
  }

  /* At the end of pipe command */
  struct libmemcached_io_vector_st vector[]=
  {
    { ptr->pipe_buffer_pos, ptr->pipe_buffer },
    { command_length, command },
    { key_length, key },
    { write_length, buffer },
    { 2, "\r\n" },
    { query->value_length, query->value },
    { 2, "\r\n" }
  };

  rc= memcached_vdo(instance, vector, 7, to_write);
  ptr->pipe_buffer_pos = 0;

  if (rc != MEMCACHED_SUCCESS)
  {
    if (rc == MEMCACHED_WRITE_FAILURE)
    {
      memcached_io_reset(instance);
    }
    return rc;
  }

  /* Fetch responses */
  ptr->pipe_responses= responses;
  ptr->pipe_responses_length= responses_cursor;
  rc= memcached_coll_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);

  *piped_rc= ptr->pipe_return_code;

  if (rc == MEMCACHED_END)
  {
    rc= MEMCACHED_SUCCESS;
  }

  return rc;
}

static memcached_return_t internal_coll_piped_exist(memcached_st *ptr,
                                                    memcached_server_write_instance_st instance,
                                                    const char *key, size_t key_length,
                                                    const char *value, size_t value_length,
                                                    memcached_return_t *responses, size_t responses_cursor,
                                                    memcached_return_t *piped_rc,
                                                    memcached_coll_action_t verb)
{
  /* Already called before_operation(). */

  memcached_return_t rc= MEMCACHED_SUCCESS;

  if (not instance)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  /* Command */
  const char *command= coll_op_string(verb);
  uint8_t command_length= coll_op_length(verb);
  ptr->last_op_code= command;

  /* Query header */
  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
  size_t write_length= 0;
  write_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                  " %u%s\r\n",
                                  (int)value_length,
                                  (ptr->flags.no_reply)? " noreply" : (ptr->flags.piped)? " pipe" : "");

  if (write_length >= MEMCACHED_DEFAULT_COMMAND_SIZE)
  {
    return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                               memcached_literal_param("snprintf(MEMCACHED_DEFAULT_COMMAND_SIZE)"));
  }

  bool to_write = true; /* do not buffer requests internally. */
  int request_length= write_length + command_length + key_length + value_length + 2;

  /* Buffer piped request */
  if (ptr->flags.piped)
  {
    if (ptr->pipe_buffer_pos + request_length >= MEMCACHED_COLL_MAX_PIPED_BUFFER_SIZE)
    {
      /* flushing */
      struct libmemcached_io_vector_st buffered_vector[]=
      {
        { ptr->pipe_buffer_pos, ptr->pipe_buffer }
      };
      rc= memcached_vdo(instance, buffered_vector, 1, to_write);
      ptr->pipe_buffer_pos = 0;
    }

    /* buffering */
    ptr->pipe_buffer_pos += snprintf(ptr->pipe_buffer+ptr->pipe_buffer_pos,
                                     MEMCACHED_COLL_MAX_PIPED_BUFFER_SIZE - ptr->pipe_buffer_pos,
                                     "%s%s%s", command, key, buffer);
    memcpy(ptr->pipe_buffer+ptr->pipe_buffer_pos, value, value_length);
    ptr->pipe_buffer_pos += value_length;
    ptr->pipe_buffer[ptr->pipe_buffer_pos++] = '\r';
    ptr->pipe_buffer[ptr->pipe_buffer_pos++] = '\n';

    if (rc == MEMCACHED_WRITE_FAILURE)
    {
      memcached_io_reset(instance);
    }

    return rc;
  }

  /* At the end of pipe command */
  struct libmemcached_io_vector_st vector[]=
  {
    { ptr->pipe_buffer_pos, ptr->pipe_buffer },
    { command_length, command },
    { key_length, key },
    { write_length, buffer },
    { value_length, value },
    { 2, "\r\n" }
  };

  rc= memcached_vdo(instance, vector, 6, to_write);
  ptr->pipe_buffer_pos = 0;

  if (rc != MEMCACHED_SUCCESS)
  {
    if (rc == MEMCACHED_WRITE_FAILURE)
    {
      memcached_io_reset(instance);
    }
    return rc;
  }

  /* Fetch responses */
  ptr->pipe_responses= responses;
  ptr->pipe_responses_length= responses_cursor;
  rc= memcached_coll_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);

  *piped_rc= ptr->pipe_return_code;

  if (rc == MEMCACHED_END || rc == MEMCACHED_EXIST)
  {
    rc= MEMCACHED_SUCCESS;
  }

  return rc;
}

static memcached_return_t do_coll_insert(memcached_st *ptr,
                                         const char *key, size_t key_length,
                                         memcached_coll_query_st *query,
                                         memcached_hexadecimal_st *eflag,
                                         memcached_coll_create_attrs_st *attributes,
                                         memcached_coll_action_t verb)
{
  arcus_server_check_for_update(ptr);

  memcached_return_t rc = before_query(ptr, &key, &key_length, 1);
  if (rc != MEMCACHED_SUCCESS)
    return rc;

  uint32_t server_key= memcached_generate_hash_with_redistribution(ptr, key, key_length);
  memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, server_key);

  return internal_coll_insert(ptr, instance, key, key_length, query, eflag, attributes, verb);
}

static memcached_return_t do_coll_delete(memcached_st *ptr,
                                         const char *key, size_t key_length,
                                         memcached_coll_query_st *query,
                                         size_t count,
                                         bool drop_if_empty,
                                         memcached_coll_action_t verb)
{
  arcus_server_check_for_update(ptr);

  memcached_return_t rc = before_query(ptr, &key, &key_length, 1);
  if (rc != MEMCACHED_SUCCESS)
    return rc;

  uint32_t server_key= memcached_generate_hash_with_redistribution(ptr, key, key_length);
  memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, server_key);

  /* Command */
  const char *command= coll_op_string(verb);
  uint8_t command_length= coll_op_length(verb);
  ptr->last_op_code= command;

  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
  size_t write_length= 0;

  /* Query header */
  if (verb == LOP_DELETE_OP)
  {
    if (MEMCACHED_COLL_QUERY_LOP == query->type)
    {
      write_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                      " %d%s%s",
                                      query->sub_key.index,
                                      drop_if_empty ? " drop" :"",
                                      ptr->flags.no_reply ? " noreply" :"");
    }
    else if (MEMCACHED_COLL_QUERY_LOP_RANGE == query->type)
    {
      write_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                      " %d..%d%s%s",
                                      query->sub_key.index_range[0],
                                      query->sub_key.index_range[1],
                                      drop_if_empty ? " drop" :"",
                                      ptr->flags.no_reply ? " noreply" :"");
    }
  }
  else if (verb == SOP_DELETE_OP)
  {
    write_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                    " %u%s%s\r\n",
                                    (int)query->value_length,
                                    drop_if_empty ? " drop" :"",
                                    ptr->flags.no_reply ? " noreply" :"");
  }
  else if (verb == BOP_DELETE_OP)
  {
    char filter_str[MEMCACHED_COLL_MAX_FILTER_STR_LENGTH];
    memcached_coll_eflag_filter_to_str(query->eflag_filter, filter_str, MEMCACHED_COLL_MAX_FILTER_STR_LENGTH);

    if (MEMCACHED_COLL_QUERY_BOP == query->type)
    {
      write_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                      " %llu%s",
                                      (unsigned long long)query->sub_key.bkey,
                                      filter_str);
    }
    else if (MEMCACHED_COLL_QUERY_BOP_RANGE == query->type)
    {
      write_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                      " %llu..%llu%s %u",
                                      (unsigned long long)query->sub_key.bkey_range[0],
                                      (unsigned long long)query->sub_key.bkey_range[1],
                                      filter_str,
                                      (int)count);
    }
    else if (MEMCACHED_COLL_QUERY_BOP_EXT == query->type)
    {
      char bkey_str[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
      memcached_conv_hex_to_str(ptr, &query->sub_key.bkey_ext,
                                bkey_str, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);

      write_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                      " 0x%s%s", bkey_str, filter_str);
    }
    else if (MEMCACHED_COLL_QUERY_BOP_EXT_RANGE == query->type)
    {
      char bkey_str_from[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
      char bkey_str_to[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];

      memcached_conv_hex_to_str(ptr, &query->sub_key.bkey_ext_range[0],
                                bkey_str_from, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);
      memcached_conv_hex_to_str(ptr, &query->sub_key.bkey_ext_range[1],
                                bkey_str_to, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);

      write_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                      " 0x%s..0x%s%s %u", bkey_str_from, bkey_str_to,
                                      filter_str, (int)count);
    }

    // drop & noreply
    write_length+= (size_t) snprintf(buffer+write_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                     "%s%s",
                                     drop_if_empty ? " drop" :"",
                                     ptr->flags.no_reply ? " noreply" :"");
  }
  else
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  if (write_length >= MEMCACHED_DEFAULT_COMMAND_SIZE)
  {
    return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                               memcached_literal_param("snprintf(MEMCACHED_DEFAULT_COMMAND_SIZE)"));
  }

  /* Request */
  bool to_write= not ptr->flags.buffer_requests;

  if (SOP_DELETE_OP == verb)
  {
    /* delete by value */
    struct libmemcached_io_vector_st vector[]=
    {
      { command_length, command },
      { key_length, key },
      { write_length, buffer },
      { query->value_length, query->value },
      { 2, "\r\n" }
    };
    rc=  memcached_vdo(instance, vector, 5, to_write);
  }
  else
  {
    /* delete by sub key(index or bkey) */
    struct libmemcached_io_vector_st vector[]=
    {
      { command_length, command },
      { key_length, key },
      { write_length, buffer },
      { 2, "\r\n" }
    };
    rc=  memcached_vdo(instance, vector, 4, to_write);
  }

  if (rc == MEMCACHED_SUCCESS)
  {
    if (to_write == false)
    {
      rc= MEMCACHED_BUFFERED;
    }
    else if (ptr->flags.no_reply)
    {
      rc= MEMCACHED_SUCCESS;
    }
    else
    {
      rc= memcached_coll_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);
      memcached_set_last_response_code(ptr, rc);

      if (rc == MEMCACHED_DELETED or
          rc == MEMCACHED_DELETED_DROPPED)
      {
        rc= MEMCACHED_SUCCESS;
      }
    }
  }

  if (rc == MEMCACHED_WRITE_FAILURE)
  {
    memcached_io_reset(instance);
  }

  return rc;
}

static memcached_return_t do_coll_get(memcached_st *ptr,
                                      const char *key, const size_t key_length,
                                      memcached_coll_query_st *query,
                                      bool with_delete, bool drop_if_empty,
                                      memcached_coll_result_st *result,
                                      memcached_coll_action_t verb)
{
  arcus_server_check_for_update(ptr);

  memcached_return_t rc = before_query(ptr, &key, &key_length, 1);
  if (rc != MEMCACHED_SUCCESS)
    return rc;

  /* Check function arguments */
  if (not with_delete and drop_if_empty)
  {
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                               memcached_literal_param("with_delete=false, drop_if_empty=true"));
  }

  uint32_t server_key= memcached_generate_hash_with_redistribution(ptr, key, key_length);
  memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, server_key);

  /* Command */
  const char *command= coll_op_string(verb);
  uint8_t command_length= coll_op_length(verb);
  ptr->last_op_code= command;

  memcached_coll_type_t type = COLL_NONE;

  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
  size_t buffer_length= 0;

  /* Query header */

  /* 1. sub key */
  if (verb == LOP_GET_OP)
  {
    type= COLL_LIST;
    if (MEMCACHED_COLL_QUERY_LOP == query->type)
    {
      buffer_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                       " %d", query->sub_key.index);
    }
    else if (MEMCACHED_COLL_QUERY_LOP_RANGE == query->type)
    {
      buffer_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                       " %d..%d",
                                       query->sub_key.index_range[0],
                                       query->sub_key.index_range[1]);
    }
    else
    {
      return MEMCACHED_INVALID_ARGUMENTS;
    }
  }
  else if (verb == SOP_GET_OP)
  {
    type= COLL_SET;
    buffer_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                     " %u", (int)query->count);
  }
  else if (verb == BOP_GET_OP)
  {
    type= COLL_BTREE;
    if (MEMCACHED_COLL_QUERY_BOP == query->type)
    {
      buffer_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                       " %llu",
                                       (unsigned long long)query->sub_key.bkey);
    }
    else if (MEMCACHED_COLL_QUERY_BOP_RANGE == query->type)
    {
      buffer_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                       " %llu..%llu",
                                       (unsigned long long)query->sub_key.bkey_range[0],
                                       (unsigned long long)query->sub_key.bkey_range[1]);
    }
    else if (MEMCACHED_COLL_QUERY_BOP_EXT == query->type)
    {
      char bkey_str[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
      memcached_conv_hex_to_str(NULL, &query->sub_key.bkey_ext,
                                bkey_str, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);

      buffer_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                      " 0x%s", bkey_str);
    }
    else if (MEMCACHED_COLL_QUERY_BOP_EXT_RANGE == query->type)
    {
      char bkey_str_from[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
      char bkey_str_to[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];

      memcached_conv_hex_to_str(ptr, &query->sub_key.bkey_ext_range[0],
                                bkey_str_from, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);
      memcached_conv_hex_to_str(ptr, &query->sub_key.bkey_ext_range[1],
                                bkey_str_to, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);

      buffer_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                      " 0x%s..0x%s",
                                      bkey_str_from, bkey_str_to);
    }
    else
    {
      return MEMCACHED_INVALID_ARGUMENTS;
    }

    /* Filter */
    if (query->eflag_filter)
    {
      char filter_str[MEMCACHED_COLL_MAX_FILTER_STR_LENGTH];
      memcached_coll_eflag_filter_to_str(query->eflag_filter, filter_str, MEMCACHED_COLL_MAX_FILTER_STR_LENGTH);

      buffer_length+= (size_t) snprintf(buffer+buffer_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                        "%s", filter_str);
    }

    /* Options */
    buffer_length+= (size_t) snprintf(buffer+buffer_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                      " %u %u", (int)query->offset, (int)query->count);
  }
  else
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  /* 2. delete or drop */
  if (with_delete or drop_if_empty)
  {
    buffer_length+= (size_t) snprintf(buffer+buffer_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                      "%s",
                                      (with_delete && drop_if_empty)?" drop":" delete");
  }

  if (buffer_length >= MEMCACHED_DEFAULT_COMMAND_SIZE)
  {
    return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                               memcached_literal_param("snprintf(MEMCACHED_DEFAULT_COMMAND_SIZE)"));
  }

  /* Request */
  bool to_write= not ptr->flags.buffer_requests;

  struct libmemcached_io_vector_st vector[]=
  {
    { command_length, command },
    { key_length, key },
    { buffer_length, buffer },
    { 2, "\r\n" }
  };

  rc= memcached_vdo(instance, vector, 4, to_write);

  if (rc == MEMCACHED_SUCCESS)
  {
    if (to_write == false)
    {
      rc= MEMCACHED_BUFFERED;
    }
    else if (ptr->flags.no_reply || ptr->flags.piped)
    {
      rc= MEMCACHED_SUCCESS;
    }
    else
    {
      /* Fetch results */
      result = memcached_coll_fetch_result(ptr, result, &rc);

      /* Search for END or something */
      if (result)
      {
        memcached_coll_result_reset(&ptr->collection_result);
        memcached_coll_fetch_result(ptr, &ptr->collection_result, &rc);
      }

      if (rc == MEMCACHED_END             or
          rc == MEMCACHED_TRIMMED         or
          rc == MEMCACHED_DELETED         or
          rc == MEMCACHED_DELETED_DROPPED )
      {
        rc= MEMCACHED_SUCCESS;
      }
    }
  }

  if (rc == MEMCACHED_WRITE_FAILURE)
  {
    memcached_io_reset(instance);
  }

  return rc;
}

static memcached_return_t do_coll_mget(memcached_st *ptr,
                                       const char * const *keys,
                                       const size_t *key_length,
                                       size_t number_of_keys,
                                       memcached_coll_query_st *query,
                                       memcached_coll_action_t verb)
{
  const size_t MAX_SERVERS_FOR_COLLECTION_MGET= 200;

  arcus_server_check_for_update(ptr);

  memcached_return_t rc = before_query(ptr, keys, key_length, number_of_keys);
  if (rc != MEMCACHED_SUCCESS)
    return rc;

  /* Check function arguments */
  if (not keys or not key_length)
  {
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                               memcached_literal_param("key (length) list is null"));
  }
  if (not query)
  {
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                               memcached_literal_param("query is null"));
  }
  if (number_of_keys > MEMCACHED_COLL_MAX_BOP_MGET_KEY_COUNT)
  {
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                               memcached_literal_param("size of key list should be <= 200"));
  }
  if (query->count > MEMCACHED_COLL_MAX_BOP_MGET_ELEM_COUNT)
  {
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                               memcached_literal_param("count should be <= 50"));
  }
  if (memcached_server_count(ptr) > MAX_SERVERS_FOR_COLLECTION_MGET)
  {
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                               memcached_literal_param("memcached instances should be <= 200"));
  }

  /* Command */
  const char *command= coll_op_string(BOP_MGET_OP);
  uint8_t command_length= coll_op_length(BOP_MGET_OP);
  ptr->last_op_code= command;

  /* Key-Server mapping */
  uint32_t *key_to_serverkey= NULL;
  ALLOCATE_ARRAY_OR_RETURN(ptr, key_to_serverkey, uint32_t, number_of_keys);

  for (uint32_t x= 0; x<number_of_keys; x++)
  {
    key_to_serverkey[x]= memcached_generate_hash_with_redistribution(ptr, keys[x], key_length[x]);
  }

  /* Prepare <lenkeys> and <numkeys> */
  int32_t lenkeys[MAX_SERVERS_FOR_COLLECTION_MGET]= { 0 };
  int32_t numkeys[MAX_SERVERS_FOR_COLLECTION_MGET]= { 0 };
  char lenkeys_numkeys_str[MAX_SERVERS_FOR_COLLECTION_MGET][64];
  size_t lenkeys_numkeys_length[MAX_SERVERS_FOR_COLLECTION_MGET];

  for (size_t i=0; i<number_of_keys; i++)
  {
    lenkeys[key_to_serverkey[i]]+= (key_length[i] + 1); // +1 for the comma(,)
    numkeys[key_to_serverkey[i]]+= 1;
  }

  for (size_t i=0; i<memcached_server_count(ptr); i++)
  {
    if (numkeys[i] > MEMCACHED_COLL_MAX_BOP_MGET_KEY_COUNT)
    {
      DEALLOCATE_ARRAY(ptr, key_to_serverkey);
      return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                                 memcached_literal_param("key size for a server should be <= 200"));
    }
    else
    {
      lenkeys_numkeys_length[i]= snprintf(lenkeys_numkeys_str[i], 64, "%d %d", lenkeys[i]-1, numkeys[i]); // -1 for the comma-less first key
    }
  }

  /* Prepare the request header */
  size_t buffer_length= 0;
  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];

  if (verb == BOP_MGET_OP)
  {
    /* 1. <bkey or "bkey range"> */
    if (MEMCACHED_COLL_QUERY_BOP == query->type)
    {
      buffer_length+= (size_t) snprintf(buffer+buffer_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                        " %llu",
                                        (unsigned long long)query->sub_key.bkey);
    }
    else if (MEMCACHED_COLL_QUERY_BOP_RANGE == query->type)
    {
      buffer_length+= (size_t) snprintf(buffer+buffer_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                        " %llu..%llu",
                                        (unsigned long long)query->sub_key.bkey_range[0],
                                        (unsigned long long)query->sub_key.bkey_range[1]);
    }
    else if (MEMCACHED_COLL_QUERY_BOP_EXT == query->type)
    {
      char bkey_str[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
      memcached_conv_hex_to_str(NULL, &query->sub_key.bkey_ext,
                                bkey_str, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);

      buffer_length+= (size_t) snprintf(buffer+buffer_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                        " 0x%s", bkey_str);
    }
    else if (MEMCACHED_COLL_QUERY_BOP_EXT_RANGE == query->type)
    {
      char bkey_str_from[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
      char bkey_str_to[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
      memcached_conv_hex_to_str(NULL, &query->sub_key.bkey_ext_range[0],
                                bkey_str_from, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);
      memcached_conv_hex_to_str(NULL, &query->sub_key.bkey_ext_range[1],
                                bkey_str_to, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);

      buffer_length+= (size_t) snprintf(buffer+buffer_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                        " 0x%s..0x%s",
                                        bkey_str_from, bkey_str_to);
    }
    else
    {
      return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                                 memcached_literal_param("unknown b+tree query type"));
    }

    /* 2. [<eflag_filter>] */
    if (query->eflag_filter)
    {
      char filter_str[MEMCACHED_COLL_MAX_FILTER_STR_LENGTH];
      memcached_coll_eflag_filter_to_str(query->eflag_filter, filter_str, MEMCACHED_COLL_MAX_FILTER_STR_LENGTH);

      buffer_length+= (size_t) snprintf(buffer+buffer_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                        "%s", filter_str);
    }

    /* 3. [<offset>] <count> */
    buffer_length+= (size_t) snprintf(buffer+buffer_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                      " %lu %lu", (unsigned long)query->offset, (unsigned long)query->count);
  }
  else
  {
    DEALLOCATE_ARRAY(ptr, key_to_serverkey);
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                               memcached_literal_param("invalid opcode"));
  }

  /* Send the request (buffered) */
  bool failures_occured_in_sending= false;
  size_t hosts_connected= 0;

  for (size_t i=0; i<number_of_keys; i++)
  {
    memcached_server_write_instance_st instance=
        memcached_server_instance_fetch(ptr, key_to_serverkey[i]);

    if (not instance)
    {
      fprintf(stderr, "[debug] instance is null : serverkey=%d\n", key_to_serverkey[i]);
    }

    struct libmemcached_io_vector_st vector[]=
    {
      // request header
      { command_length, command },
      { lenkeys_numkeys_length[key_to_serverkey[i]], lenkeys_numkeys_str[key_to_serverkey[i]] },
      { buffer_length, buffer },
      { 2, "\r\n" },
      { key_length[i], keys[i] },
      // comma-seperated request keys
      { 1, "," },
      { key_length[i], keys[i] },
    };

    if (memcached_server_response_count(instance) == 0)
    {
      /* Sending the request header */
      rc= memcached_connect(instance);
      if (memcached_failed(rc))
      {
        memcached_set_error(*instance, rc, MEMCACHED_AT);
        continue;
      }
      hosts_connected++;

      if ((memcached_io_writev(instance, vector, 5, false)) == -1)
      {
        failures_occured_in_sending= true;
        continue;
      }
      memcached_server_response_increment(instance);
    }
    else
    {
      /* Sending each key */
      if ((memcached_io_writev(instance, (vector + 5), 2, false)) == -1)
      {
        memcached_server_response_reset(instance);
        failures_occured_in_sending= true;
        continue;
      }
    }
  }

  DEALLOCATE_ARRAY(ptr, key_to_serverkey);

  if (hosts_connected == 0)
  {
    if (memcached_failed(rc))
    {
      return rc;
    }
    return memcached_set_error(*ptr, MEMCACHED_NO_SERVERS, MEMCACHED_AT);
  }

  /* End of request */
  bool success_happened= false;
  for (uint32_t x= 0; x < memcached_server_count(ptr); x++)
  {
    memcached_server_write_instance_st instance=
      memcached_server_instance_fetch(ptr, x);

    if (memcached_server_response_count(instance))
    {
      /* We need to do something about non-connnected hosts in the future */
      if ((memcached_io_write(instance, "\r\n", 2, true)) == -1)
      {
        failures_occured_in_sending= true;
      }
      else
      {
        success_happened= true;
      }
    }
  }

  if (failures_occured_in_sending && success_happened)
  {
    return MEMCACHED_SOME_ERRORS;
  }

  if (success_happened)
    return MEMCACHED_SUCCESS;

  return MEMCACHED_FAILURE; // Complete failure occurred
}

static memcached_return_t do_bop_smget(memcached_st *ptr,
                                const char * const *keys,
                                const size_t *key_length, size_t number_of_keys,
                                memcached_bop_query_st *query,
                                memcached_coll_smget_result_st *result)
{
  const size_t MAX_SERVERS_FOR_BOP_SMGET= 200;

  arcus_server_check_for_update(ptr);

  memcached_return_t rc = before_query(ptr, keys, key_length, number_of_keys);
  if (rc != MEMCACHED_SUCCESS)
    return rc;

  /* Check function arguments */
  if (not keys or not key_length)
  {
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                               memcached_literal_param("key (length) list is null"));
  }
  if (not query)
  {
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                               memcached_literal_param("query is null"));
  }
  if (not result)
  {
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                               memcached_literal_param("result is null"));
  }
  if (query->offset + query->count > MEMCACHED_COLL_MAX_BOP_SMGET_ELEM_COUNT)
  {
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                               memcached_literal_param("'offset + count' should be <= 1000"));
  }

  if (memcached_server_count(ptr) > MAX_SERVERS_FOR_BOP_SMGET)
  {
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                               memcached_literal_param("memcached instances should be <= 200"));
  }

  /* Command */
  const char *command= coll_op_string(BOP_SMGET_OP);
  uint8_t command_length= coll_op_length(BOP_SMGET_OP);
  ptr->last_op_code= command;

  memcached_coll_type_t type= COLL_BTREE;

  /* Key-Server mapping */
  uint32_t *key_to_serverkey= NULL;

  ALLOCATE_ARRAY_OR_RETURN(ptr, key_to_serverkey, uint32_t, number_of_keys);

  for (uint32_t x= 0; x<number_of_keys; x++)
  {
    key_to_serverkey[x]= memcached_generate_hash_with_redistribution(ptr, keys[x], key_length[x]);
  }

  /* Prepare <lenkeys> and <numkeys> */
  int32_t lenkeys[MAX_SERVERS_FOR_BOP_SMGET]= { 0 };
  int32_t numkeys[MAX_SERVERS_FOR_BOP_SMGET]= { 0 };
  char lenkeys_numkeys_str[MAX_SERVERS_FOR_BOP_SMGET][64];
  size_t lenkeys_numkeys_length[MAX_SERVERS_FOR_BOP_SMGET];

  for (size_t i=0; i<number_of_keys; i++)
  {
    lenkeys[key_to_serverkey[i]]+= (key_length[i] + 1); // +1 for the comma(,)
    numkeys[key_to_serverkey[i]]+= 1;
  }

  for (size_t i=0; i<memcached_server_count(ptr); i++)
  {
    if (numkeys[i] > MEMCACHED_COLL_MAX_BOP_SMGET_KEY_COUNT)
    {
      DEALLOCATE_ARRAY(ptr, key_to_serverkey);
      return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                                 memcached_literal_param("key size for a server should be <= 2000"));
    }
    else
    {
      lenkeys_numkeys_length[i]= snprintf(lenkeys_numkeys_str[i], 64, "%d %d", lenkeys[i]-1, numkeys[i]); // -1 for the comma-less first key
    }
  }

  /* Key Group & Sub key */
  size_t buffer_length= 0;
  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];

  result->sub_key_type= query->type;

  if (MEMCACHED_COLL_QUERY_BOP == query->type)
  {
    buffer_length+= (size_t) snprintf(buffer+buffer_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                      " %llu",
                                      (unsigned long long)query->sub_key.bkey);
  }
  else if (MEMCACHED_COLL_QUERY_BOP_RANGE == query->type)
  {
    buffer_length+= (size_t) snprintf(buffer+buffer_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                      " %llu..%llu",
                                      (unsigned long long)query->sub_key.bkey_range[0],
                                      (unsigned long long)query->sub_key.bkey_range[1]);
    if (query->sub_key.bkey_range[0] > query->sub_key.bkey_range[1])
    {
      result->options.is_descending= true;
    }
  }
  else if (MEMCACHED_COLL_QUERY_BOP_EXT == query->type)
  {
    char bkey_str[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
    memcached_conv_hex_to_str(NULL, &query->sub_key.bkey_ext,
                              bkey_str, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);

    buffer_length+= (size_t) snprintf(buffer+buffer_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                      " 0x%s", bkey_str);
  }
  else if (MEMCACHED_COLL_QUERY_BOP_EXT_RANGE == query->type)
  {
    char bkey_str_from[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
    char bkey_str_to[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
    memcached_conv_hex_to_str(NULL, &query->sub_key.bkey_ext_range[0],
                              bkey_str_from, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);
    memcached_conv_hex_to_str(NULL, &query->sub_key.bkey_ext_range[1],
                              bkey_str_to, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);

    buffer_length+= (size_t) snprintf(buffer+buffer_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                      " 0x%s..0x%s",
                                      bkey_str_from, bkey_str_to);

    if (memcached_compare_two_hexadecimal(&query->sub_key.bkey_ext_range[0], &query->sub_key.bkey_ext_range[1]) == 1)
    {
      result->options.is_descending= true;
    }
  }
  else
  {
    DEALLOCATE_ARRAY(ptr, key_to_serverkey);
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                               memcached_literal_param("unknown b+tree query type"));
  }

  /* Filter */
  if (query->eflag_filter)
  {
    char filter_str[MEMCACHED_COLL_MAX_FILTER_STR_LENGTH];
    memcached_coll_eflag_filter_to_str(query->eflag_filter, filter_str, MEMCACHED_COLL_MAX_FILTER_STR_LENGTH);

    buffer_length+= (size_t) snprintf(buffer+buffer_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                      "%s", filter_str);
  }

  /* Options */
  buffer_length+= (size_t) snprintf(buffer+buffer_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                    " %u %u", (int)0, (int)(query->offset + query->count));
  if (result != NULL)
  {
    result->offset= query->offset;
    result->count= query->count;
  }

  /* Send the request (buffered) */
  bool failures_occured_in_sending= false;
  size_t hosts_connected= 0;

  for (size_t i=0; i<number_of_keys; i++)
  {
    memcached_server_write_instance_st instance=
        memcached_server_instance_fetch(ptr, key_to_serverkey[i]);

    if (not instance)
    {
      fprintf(stderr, "[debug] instance is null : serverkey=%d\n", key_to_serverkey[i]);
    }

    struct libmemcached_io_vector_st vector[]=
    {
      // request header
      { command_length, command },
      { lenkeys_numkeys_length[key_to_serverkey[i]], lenkeys_numkeys_str[key_to_serverkey[i]] },
      { buffer_length, buffer },
      { 2, "\r\n" },
      { key_length[i], keys[i] },
      // comma-seperated request keys
      { 1, "," },
      { key_length[i], keys[i] },
    };

    if (memcached_server_response_count(instance) == 0)
    {
      /* Sending the request header */
      rc= memcached_connect(instance);
      if (memcached_failed(rc))
      {
        memcached_set_error(*instance, rc, MEMCACHED_AT);
        continue;
      }
      hosts_connected++;

      if ((memcached_io_writev(instance, vector, 5, false)) == -1)
      {
        failures_occured_in_sending= true;
        continue;
      }
      memcached_server_response_increment(instance);
    }
    else
    {
      /* Sending each key */
      if ((memcached_io_writev(instance, (vector + 5), 2, false)) == -1)
      {
        memcached_server_response_reset(instance);
        failures_occured_in_sending= true;
        continue;
      }
    }
  }

  DEALLOCATE_ARRAY(ptr, key_to_serverkey);

  if (hosts_connected == 0)
  {
    if (memcached_failed(rc))
    {
      return rc;
    }
    return memcached_set_error(*ptr, MEMCACHED_NO_SERVERS, MEMCACHED_AT);
  }

  /* End of request */
  bool success_happened= false;
  for (uint32_t x= 0; x < memcached_server_count(ptr); x++)
  {
    memcached_server_write_instance_st instance=
      memcached_server_instance_fetch(ptr, x);

    if (memcached_server_response_count(instance))
    {
      /* We need to do something about non-connnected hosts in the future */
      if ((memcached_io_write(instance, "\r\n", 2, true)) == -1)
      {
        failures_occured_in_sending= true;
      }
      else
      {
        success_happened= true;
      }
    }
  }

  if (failures_occured_in_sending && success_happened)
  {
    return MEMCACHED_SOME_ERRORS;
  }

  if (success_happened)
  {
    result = memcached_coll_smget_fetch_result(ptr, result, &rc, type);
    return rc;
  }
  else
  {
    memcached_coll_smget_result_free(result);
    result= NULL;
    return MEMCACHED_FAILURE; // Complete failure occurred
  }
}

static memcached_return_t do_coll_exist(memcached_st *ptr,
                                        const char *key, size_t key_length,
                                        const char *value, size_t value_length,
                                        memcached_coll_action_t verb)
{
  arcus_server_check_for_update(ptr);

  memcached_return_t rc = before_query(ptr, &key, &key_length, 1);
  if (rc != MEMCACHED_SUCCESS)
    return rc;

  uint32_t server_key= memcached_generate_hash_with_redistribution(ptr, key, key_length);
  memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, server_key);

  /* Command */
  const char *command= coll_op_string(verb);
  uint8_t command_length= coll_op_length(verb);
  ptr->last_op_code= command;

  /* Query header */
  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
  size_t write_length= 0;
  write_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                  " %u%s\r\n",
                                  (int)value_length,
                                  (ptr->flags.no_reply)? " noreply" : (ptr->flags.piped)? " pipe" : "");

  if (write_length >= MEMCACHED_DEFAULT_COMMAND_SIZE)
  {
    return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                               memcached_literal_param("snprintf(MEMCACHED_DEFAULT_COMMAND_SIZE)"));
  }

  /* Request */
  bool to_write= not ptr->flags.buffer_requests;

  struct libmemcached_io_vector_st vector[]=
  {
    { command_length, command },
    { key_length, key },
    { write_length, buffer },
    { value_length, value },
    { 2, "\r\n" }
  };
  rc= memcached_vdo(instance, vector, 5, to_write);

  if (rc == MEMCACHED_SUCCESS)
  {
    if (to_write == false)
    {
      rc= MEMCACHED_BUFFERED;
    }
    else if (ptr->flags.no_reply)
    {
      rc= MEMCACHED_SUCCESS;
    }
    else
    {
      rc= memcached_coll_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);
      memcached_set_last_response_code(ptr, rc);

      if (rc == MEMCACHED_EXIST)
      {
        rc= MEMCACHED_SUCCESS;
      }
    }
  }

  if (rc == MEMCACHED_WRITE_FAILURE)
  {
    memcached_io_reset(instance);
  }

  return rc;
}

static memcached_return_t do_coll_piped_exist(memcached_st *ptr, const char *key, size_t key_length,
                                              const size_t number_of_piped_items,
                                              const char * const *values, const size_t *values_length,
                                              memcached_return_t *responses, memcached_return_t *piped_rc)
{
  arcus_server_check_for_update(ptr);

  memcached_return_t rc = before_query(ptr, &key, &key_length, 1);
  if (rc != MEMCACHED_SUCCESS)
    return rc;

  uint32_t server_key= memcached_generate_hash_with_redistribution(ptr, key, key_length);
  memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, server_key);

  memcached_return_t piped_return_code, last_piped_return_code;
  piped_return_code= last_piped_return_code= MEMCACHED_MAXIMUM_RETURN;

  size_t requested_items= 0;
  size_t offset= 0;
  ptr->pipe_buffer_pos= 0;

  for (size_t i= 0; i< number_of_piped_items; i++)
  {
    requested_items++;

    /* Get piped responses */
    unlikely ((requested_items == MEMCACHED_COLL_MAX_PIPED_CMD_SIZE) or
              (requested_items == number_of_piped_items))
    {
      ptr->flags.piped= false;
      offset= i - requested_items + 1;

      last_piped_return_code= piped_return_code;

      rc= internal_coll_piped_exist(ptr, instance, key, key_length,
                                    values[i], values_length[i],
                                    responses, offset, &piped_return_code,
                                    SOP_EXIST_OP);

      if (rc != MEMCACHED_SUCCESS)
      {
        break;
      }

      piped_return_code= (last_piped_return_code == MEMCACHED_MAXIMUM_RETURN)? piped_return_code :
                         (piped_return_code == last_piped_return_code)? piped_return_code :
                          MEMCACHED_SOME_EXIST;

      requested_items= 0;
    }
    /* Throw a piped operation */
    else
    {
      ptr->flags.piped= true;

      rc= internal_coll_piped_exist(ptr, instance, key, key_length,
                                    values[i], values_length[i], NULL, 0, NULL, SOP_EXIST_OP);

      if (rc != MEMCACHED_SUCCESS)
      {
        break;
      }
    }
  }

  if (number_of_piped_items == 1)
  {
    if (rc == MEMCACHED_SUCCESS || rc == MEMCACHED_NOT_EXIST)
    {
      responses[0]= (rc == MEMCACHED_SUCCESS)? MEMCACHED_EXIST : MEMCACHED_NOT_EXIST;
      piped_return_code= (rc == MEMCACHED_SUCCESS)? MEMCACHED_ALL_EXIST : MEMCACHED_ALL_NOT_EXIST;
      rc= MEMCACHED_SUCCESS;
    }
    if (rc == MEMCACHED_SUCCESS)
    {
      responses[0]= MEMCACHED_EXIST;
      piped_return_code= MEMCACHED_ALL_EXIST;
    }
    else if (rc == MEMCACHED_NOT_EXIST)
    {
      responses[0]= MEMCACHED_NOT_EXIST;
      piped_return_code= MEMCACHED_ALL_NOT_EXIST;
      rc= MEMCACHED_SUCCESS;
    }
  }

  *piped_rc= piped_return_code;
  ptr->flags.piped= false;

  return rc;
}

static memcached_return_t do_coll_piped_insert(memcached_st *ptr, const char *key, const size_t key_length,
                                               const size_t number_of_piped_items,
                                               memcached_coll_query_st *queries,
                                               const unsigned char * const *eflags, const size_t *eflags_length,
                                               const char * const *values, const size_t *values_length,
                                               memcached_coll_create_attrs_st *attributes,
                                               memcached_return_t *responses, memcached_return_t *piped_rc,
                                               memcached_coll_action_t verb)
{
  arcus_server_check_for_update(ptr);

  memcached_return_t rc = before_query(ptr, &key, &key_length, 1);
  if (rc != MEMCACHED_SUCCESS)
    return rc;

  uint32_t server_key= memcached_generate_hash_with_redistribution(ptr, key, key_length);
  memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, server_key);

  memcached_return_t piped_return_code, last_piped_return_code;
  piped_return_code= last_piped_return_code= MEMCACHED_MAXIMUM_RETURN;

  int requested_items= 0;
  size_t offset= 0;
  ptr->pipe_buffer_pos= 0;

  for (size_t i=0; i<number_of_piped_items; i++)
  {
    requested_items++;

    /* Prepare an eflag */
    memcached_hexadecimal_st eflag_hex = { NULL, 0, { 0 } };

    if (verb == BOP_INSERT_OP and eflags and eflags_length )
    {
      eflag_hex.array = (unsigned char *)eflags[i];
      eflag_hex.length = eflags_length[i];
    }

    /* Prepare a query */
    queries[i].value = values[i];
    queries[i].value_length = values_length[i];

    /* Get piped responses */
    if ((requested_items == MEMCACHED_COLL_MAX_PIPED_CMD_SIZE) or
        (i+1 == number_of_piped_items))
    {
      ptr->flags.piped= false;
      offset= i - requested_items + 1;

      last_piped_return_code= piped_return_code;

      rc= internal_coll_piped_insert(ptr, instance, key, key_length,
                                     &queries[i], &eflag_hex, attributes, responses, offset,
                                     &piped_return_code, verb);

      if (rc != MEMCACHED_SUCCESS)
      {
        break;
      }

      piped_return_code= (last_piped_return_code == MEMCACHED_MAXIMUM_RETURN)? piped_return_code :
                         (piped_return_code == last_piped_return_code)? piped_return_code :
                          MEMCACHED_SOME_SUCCESS;

      requested_items= 0;
    }
    /* Throw a piped operation */
    else
    {
      ptr->flags.piped = true;
      rc= internal_coll_piped_insert(ptr, instance, key, key_length,
                                     &queries[i], &eflag_hex, attributes, NULL, 0, NULL, verb);

      if (rc != MEMCACHED_SUCCESS)
      {
        break;
      }
    }
  }

  if (number_of_piped_items == 1)
  {
    if (rc == MEMCACHED_STORED         or
        rc == MEMCACHED_CREATED_STORED )
    {
      responses[0]= rc;
      piped_return_code= MEMCACHED_ALL_SUCCESS;
      rc= MEMCACHED_SUCCESS;
    }
    else
    {
      responses[0]= rc;
      piped_return_code= MEMCACHED_ALL_FAILURE;
    }
  }

  *piped_rc= piped_return_code;
  ptr->flags.piped= false;

  return rc;
}

typedef struct memcached_index_array_st
{
  struct memcached_st *root;
  size_t size;
  size_t cursor;
  size_t index_list[1];
} memcached_index_array_st;

static memcached_index_array_st *memcached_index_array_create(struct memcached_st *ptr, size_t size)
{
  memcached_index_array_st *array;
  array= (memcached_index_array_st *)libmemcached_malloc(ptr, sizeof(struct memcached_index_array_st) + size*sizeof(size_t));

  if (not array)
  {
    return NULL;
  }

  array->root= ptr;
  array->size= size;
  array->cursor= 0;

  return array;
}

static void memcached_index_array_free(memcached_index_array_st *array)
{
  if (array)
  {
    libmemcached_free(array->root, array);
    array= NULL;
  }
}

static memcached_return_t memcached_index_array_get(memcached_index_array_st *array, size_t i, size_t *v)
{
  assert_msg_with_return(i < array->size, "programmer error, index overflows", MEMCACHED_INVALID_ARGUMENTS);
  *v= array->index_list[i];
  return MEMCACHED_SUCCESS;
}

static memcached_return_t memcached_index_array_push(memcached_index_array_st *array, size_t v)
{
  if (array->cursor == array->size)
  {
    return MEMCACHED_FAILURE;
  }

  array->index_list[array->cursor++]= v;
  return MEMCACHED_SUCCESS;
}

static memcached_return_t do_coll_piped_insert_bulk(memcached_st *ptr,
                                                    const char * const *keys,
                                                    const size_t *key_length, const size_t number_of_keys,
                                                    memcached_coll_query_st *query,
                                                    const unsigned char *eflag, const size_t eflag_length,
                                                    memcached_coll_create_attrs_st *attributes,
                                                    memcached_return_t *results,
                                                    memcached_return_t *piped_rc,
                                                    memcached_coll_action_t verb)
{
  const size_t MAX_SERVERS_FOR_COLL_INSERT_BULK= 200;

  arcus_server_check_for_update(ptr);

  memcached_return_t rc = before_query(ptr, keys, key_length, number_of_keys);
  if (rc != MEMCACHED_SUCCESS)
    return rc;

  /* Check function arguments */
  if (not keys or not key_length)
  {
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                               memcached_literal_param("key (length) list is null"));
  }
  if (not query)
  {
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                               memcached_literal_param("query is null"));
  }
  if (not results)
  {
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                               memcached_literal_param("result is null"));
  }
  if (memcached_server_count(ptr) > MAX_SERVERS_FOR_COLL_INSERT_BULK)
  {
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                               memcached_literal_param("memcached instances should be <= 200"));
  }

  /* Key-Server mapping */
  uint32_t *key_to_serverkey= NULL;
  int32_t numkeys[MAX_SERVERS_FOR_COLL_INSERT_BULK]= { 0 };

  ALLOCATE_ARRAY_OR_RETURN(ptr, key_to_serverkey, uint32_t, number_of_keys);

  for (uint32_t x= 0; x<number_of_keys; x++)
  {
    key_to_serverkey[x]= memcached_generate_hash_with_redistribution(ptr, keys[x], key_length[x]);
    numkeys[key_to_serverkey[x]]+= 1;
  }

  /* Server-Keys mapping */
  memcached_index_array_st **server_to_keys= NULL;
  ALLOCATE_ARRAY_OR_RETURN(ptr, server_to_keys, memcached_index_array_st *, memcached_server_count(ptr));

  for (size_t i=0; i<memcached_server_count(ptr); i++)
  {
    if (numkeys[i] == 0)
      server_to_keys[i]= NULL;
    else
    {
      server_to_keys[i]= memcached_index_array_create(ptr, numkeys[i]);
      assert_msg_with_return(server_to_keys[i], "failed to allocate memory", MEMCACHED_MEMORY_ALLOCATION_FAILURE);
    }
  }

  /* Distribute keys */
  for (size_t i=0; i<number_of_keys; i++)
  {
    memcached_return_t error= memcached_index_array_push(server_to_keys[key_to_serverkey[i]], i);
    assert_msg_with_return(error == MEMCACHED_SUCCESS, "failed to push key index", MEMCACHED_FAILURE);
  }

  /* Request */
  memcached_return_t piped_return_code, last_piped_return_code;
  piped_return_code= last_piped_return_code= MEMCACHED_MAXIMUM_RETURN;

  for (size_t i=0; i<memcached_server_count(ptr); i++)
  {
    if (numkeys[i] == 0) continue;

    size_t number_of_piped_items= server_to_keys[i]->cursor;

    memcached_return_t *res= NULL;
    ALLOCATE_ARRAY_OR_RETURN(ptr, res, memcached_return_t, number_of_piped_items);

    size_t key_index= 0;
    int requested_items= 0;
    size_t offset= 0;
    ptr->pipe_buffer_pos= 0;

    for (size_t j=0; j<number_of_piped_items; j++)
    {
      memcached_index_array_get(server_to_keys[i], j, &key_index);

      memcached_server_write_instance_st instance=
          memcached_server_instance_fetch(ptr, key_to_serverkey[key_index]);

      requested_items++;

      /* Prepare an eflag */
      memcached_hexadecimal_st eflag_hex = { NULL, 0, { 0 } };

      if (verb == BOP_INSERT_OP and eflag)
      {
        eflag_hex.array = (unsigned char *)eflag;
        eflag_hex.length = eflag_length;
      }

      /* Get piped responses */
      if ((requested_items == MEMCACHED_COLL_MAX_PIPED_CMD_SIZE) or
          (j+1 == number_of_piped_items))
      {
        ptr->flags.piped= false;
        offset= j - requested_items + 1;

        last_piped_return_code= piped_return_code;

        rc= internal_coll_piped_insert(ptr, instance, keys[key_index], key_length[key_index],
                                       query, &eflag_hex, attributes, res, offset,
                                       &piped_return_code, verb);

        if (rc != MEMCACHED_SUCCESS)
        {
          break;
        }

        piped_return_code= (last_piped_return_code == MEMCACHED_MAXIMUM_RETURN)? piped_return_code :
                           (piped_return_code == last_piped_return_code)? piped_return_code :
                            MEMCACHED_SOME_SUCCESS;

        requested_items= 0;
      }
      /* Throw a piped operation */
      else
      {
        ptr->flags.piped = true;
        rc= internal_coll_piped_insert(ptr, instance, keys[key_index], key_length[key_index],
                                       query, &eflag_hex, attributes, NULL, 0, NULL, verb);

        if (rc != MEMCACHED_SUCCESS)
        {
          break;
        }
      }
    }

    if (number_of_piped_items == 1)
    {
      if (rc == MEMCACHED_STORED         or
          rc == MEMCACHED_CREATED_STORED )
      {
        res[0]= rc;
        piped_return_code= MEMCACHED_ALL_SUCCESS;
        rc= MEMCACHED_SUCCESS;
      }
      else
      {
        res[0]= rc;
        piped_return_code= MEMCACHED_ALL_FAILURE;
      }
    }

    *piped_rc= piped_return_code;

    for (size_t j=0; j<number_of_piped_items; j++)
    {
      memcached_index_array_get(server_to_keys[i], j, &key_index);
      results[key_index]= res[j];
    }

    DEALLOCATE_ARRAY(ptr, res);
  }

  for (size_t i=0; i<memcached_server_count(ptr); i++)
  {
    if (numkeys[i] != 0)
    {
      memcached_index_array_free(server_to_keys[i]);
    }
  }

  DEALLOCATE_ARRAY(ptr, key_to_serverkey);
  DEALLOCATE_ARRAY(ptr, server_to_keys);

  ptr->flags.piped= false;

  return rc;
}

static memcached_return_t do_coll_update(memcached_st *ptr,
                                         const char *key, size_t key_length,
                                         memcached_coll_query_st *query,
                                         memcached_coll_update_filter_st *update_filter,
                                         const char *value, int value_length,
                                         memcached_coll_action_t verb)
{
  arcus_server_check_for_update(ptr);

  memcached_return_t rc = before_query(ptr, &key, &key_length, 1);
  if (rc != MEMCACHED_SUCCESS)
    return rc;

  /* Check function arguments */
  if (not update_filter && not value)
  {
    return memcached_set_error(*ptr, MEMCACHED_NOTHING_TO_UPDATE, MEMCACHED_AT,
                               memcached_literal_param("Nothing to update"));
  }

  uint32_t server_key= memcached_generate_hash_with_redistribution(ptr, key, key_length);
  memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, server_key);

  /* Command */
  const char *command= coll_op_string(verb);
  uint8_t command_length= coll_op_length(verb);
  ptr->last_op_code= command;

  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
  size_t buffer_length= 0;

  /* Query header */

  if (verb != BOP_UPDATE_OP)
  {
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                               memcached_literal_param("Not a b+tree operation"));
  }

  /* 1. sub key */
  if (MEMCACHED_COLL_QUERY_BOP == query->type)
  {
    buffer_length= (size_t) snprintf(buffer, 30, " %llu", (unsigned long long) query->sub_key.bkey);
  }
  else if (MEMCACHED_COLL_QUERY_BOP_EXT == query->type)
  {
    char bkey_str[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
    memcached_conv_hex_to_str(ptr, &query->sub_key.bkey_ext,
                              bkey_str, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);

    buffer_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                     " 0x%s", bkey_str);
  }

  /* 2. update filter */
  if (update_filter)
  {
    char filter_str[MEMCACHED_COLL_MAX_FILTER_STR_LENGTH];
    memcached_coll_update_filter_to_str(update_filter, filter_str, MEMCACHED_COLL_MAX_FILTER_STR_LENGTH);

    buffer_length+= (size_t) snprintf(buffer+buffer_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                      "%s", filter_str);
  }

  /* 3. value length & options */
  buffer_length+= (size_t) snprintf(buffer+buffer_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                    " %d%s", value_length, (ptr->flags.no_reply)? " noreply":"");

  if (buffer_length >= MEMCACHED_DEFAULT_COMMAND_SIZE)
  {
    return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                               memcached_literal_param("snprintf(MEMCACHED_DEFAULT_COMMAND_SIZE)"));
  }

  /* Request */
  bool to_write= not ptr->flags.buffer_requests;

  if (value_length < 0)
  {
    /* update without value */
    struct libmemcached_io_vector_st vector[]=
    {
      { command_length, command },
      { key_length, key },
      { buffer_length, buffer },
      { 2, "\r\n" }
    };

    rc= memcached_vdo(instance, vector, 4, to_write);
  }
  else
  {
    /* update with value */
    struct libmemcached_io_vector_st vector[]=
    {
      { command_length, command },
      { key_length, key },
      { buffer_length, buffer },
      { 2, "\r\n" },
      { value_length, value },
      { 2, "\r\n" }
    };

    rc= memcached_vdo(instance, vector, 6, to_write);
  }

  if (rc == MEMCACHED_SUCCESS)
  {
    if (to_write == false)
    {
      rc= MEMCACHED_BUFFERED;
    }
    else if (ptr->flags.no_reply || ptr->flags.piped)
    {
      rc= MEMCACHED_SUCCESS;
    }
    else
    {
      rc= memcached_coll_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);
      memcached_set_last_response_code(ptr, rc);

      if (rc == MEMCACHED_UPDATED)
      {
        rc= MEMCACHED_SUCCESS;
      }
    }
  }

  if (rc == MEMCACHED_WRITE_FAILURE)
  {
    memcached_io_reset(instance);
  }

  return rc;
}

static memcached_return_t do_coll_arithmetic(memcached_st *ptr,
                                         const char *key, size_t key_length,
                                         memcached_coll_query_st *query,
                                         const uint64_t delta,
                                         uint64_t *value,
                                         memcached_coll_action_t verb)
{
  arcus_server_check_for_update(ptr);

  memcached_return_t rc = before_query(ptr, &key, &key_length, 1);
  if (rc != MEMCACHED_SUCCESS)
    return rc;

  uint32_t server_key= memcached_generate_hash_with_redistribution(ptr, key, key_length);
  memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, server_key);

  /* Command */
  const char *command= coll_op_string(verb);
  uint8_t command_length= coll_op_length(verb);
  ptr->last_op_code= command;

  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
  size_t buffer_length= 0;

  /* Query header */
  if (verb != BOP_INCR_OP && verb != BOP_DECR_OP)
  {
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                               memcached_literal_param("Not a b+tree operation"));
  }

  /* 1. sub key */
  if (MEMCACHED_COLL_QUERY_BOP == query->type)
  {
    buffer_length= (size_t) snprintf(buffer, 30, " %llu", (unsigned long long) query->sub_key.bkey);
  }
  else if (MEMCACHED_COLL_QUERY_BOP_EXT == query->type)
  {
    char bkey_str[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
    memcached_conv_hex_to_str(ptr, &query->sub_key.bkey_ext,
                              bkey_str, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);

    buffer_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                     " 0x%s", bkey_str);
  }

  /* 2. delta */
  buffer_length+= (size_t) snprintf(buffer+buffer_length, 30, " %llu", (unsigned long long) delta);

  /* 3. options */
  buffer_length+= (size_t) snprintf(buffer+buffer_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                    "%s", (ptr->flags.no_reply)? " noreply":"");

  if (buffer_length >= MEMCACHED_DEFAULT_COMMAND_SIZE)
  {
    return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                               memcached_literal_param("snprintf(MEMCACHED_DEFAULT_COMMAND_SIZE)"));
  }

  /* Request */
  bool to_write= not ptr->flags.buffer_requests;

  /* update without value */
  struct libmemcached_io_vector_st vector[]=
  {
    { command_length, command },
    { key_length, key },
    { buffer_length, buffer },
    { 2, "\r\n" }
  };

  rc= memcached_vdo(instance, vector, 4, to_write);

  if (rc == MEMCACHED_SUCCESS)
  {
    if (to_write == false)
    {
      rc= MEMCACHED_BUFFERED;
    }
    else if (ptr->flags.no_reply || ptr->flags.piped)
    {
      rc= MEMCACHED_SUCCESS;
    }
    else
    {
      rc= memcached_coll_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);
      memcached_set_last_response_code(ptr, rc);

      if (rc == MEMCACHED_NOTFOUND
          || rc == MEMCACHED_NOTFOUND_ELEMENT
          || rc == MEMCACHED_CLIENT_ERROR
          || rc == MEMCACHED_TYPE_MISMATCH
          || rc == MEMCACHED_BKEY_MISMATCH
          || rc == MEMCACHED_SERVER_ERROR)
      {
        *value= 0;
        return rc;
      }
      else
      {
        *value= strtoull(buffer, (char **)NULL, 10);
      }
    }
  }

  return rc;
}

static memcached_return_t do_coll_count(memcached_st *ptr,
                                        const char *key, size_t key_length,
                                        memcached_bop_query_st *query,
                                        size_t *count,
                                        memcached_coll_action_t verb)
{
  arcus_server_check_for_update(ptr);

  memcached_return_t rc = before_query(ptr, &key, &key_length, 1);
  if (rc != MEMCACHED_SUCCESS)
    return rc;

  uint32_t server_key= memcached_generate_hash_with_redistribution(ptr, key, key_length);
  memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, server_key);

  /* Command */
  const char *command= coll_op_string(BOP_COUNT_OP);
  uint8_t command_length= coll_op_length(BOP_COUNT_OP);
  ptr->last_op_code= command;

  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
  size_t buffer_length= 0;

  /* Query header */
  if (verb != BOP_COUNT_OP)
  {
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                               memcached_literal_param("Not a b+tree operation"));
  }

  /* 1. sub key */
  if (MEMCACHED_COLL_QUERY_BOP == query->type)
  {
    buffer_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                     " %llu",
                                     (unsigned long long)query->sub_key.bkey);
  }
  else if (MEMCACHED_COLL_QUERY_BOP_RANGE == query->type)
  {
    buffer_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                     " %llu..%llu",
                                     (unsigned long long)query->sub_key.bkey_range[0],
                                     (unsigned long long)query->sub_key.bkey_range[1]);
  }
  else if (MEMCACHED_COLL_QUERY_BOP_EXT == query->type)
  {
    char bkey_str[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
    memcached_conv_hex_to_str(NULL, &query->sub_key.bkey_ext,
                              bkey_str, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);

    buffer_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                     " 0x%s", bkey_str);
  }
  else if (MEMCACHED_COLL_QUERY_BOP_EXT_RANGE == query->type)
  {
    char bkey_str_from[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
    char bkey_str_to[MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH];
    memcached_conv_hex_to_str(NULL, &query->sub_key.bkey_ext_range[0],
                              bkey_str_from, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);
    memcached_conv_hex_to_str(NULL, &query->sub_key.bkey_ext_range[1],
                              bkey_str_to, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH);

    buffer_length= (size_t) snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                    " 0x%s..0x%s",
                                    bkey_str_from, bkey_str_to);
  }
  else
  {
      return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                                 memcached_literal_param("An invalid query was provided"));
  }

  /* 2. filter */
  if (query->eflag_filter)
  {
#ifdef ENABLE_EFLAGS_FILTER
    char filter_str[MEMCACHED_COLL_MAX_FILTER_STR_WITH_FLAGS_LENGTH];
    memcached_coll_eflag_filter_to_str(query->eflag_filter, filter_str, MEMCACHED_COLL_MAX_FILTER_STR_WITH_FLAGS_LENGTH);
#else
    char filter_str[MEMCACHED_COLL_MAX_FILTER_STR_LENGTH];
    memcached_coll_eflag_filter_to_str(query->eflag_filter, filter_str, MEMCACHED_COLL_MAX_FILTER_STR_LENGTH);
#endif

    buffer_length+= (size_t) snprintf(buffer+buffer_length, MEMCACHED_DEFAULT_COMMAND_SIZE,
                                      "%s", filter_str);
  }

  if (buffer_length >= MEMCACHED_DEFAULT_COMMAND_SIZE)
  {
    return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT,
                               memcached_literal_param("snprintf(MEMCACHED_DEFAULT_COMMAND_SIZE)"));
  }

  /* Request */
  bool to_write= not ptr->flags.buffer_requests;

  struct libmemcached_io_vector_st vector[]=
  {
    { command_length, command },
    { key_length, key },
    { buffer_length, buffer },
    { 2, "\r\n" }
  };

  rc= memcached_vdo(instance, vector, 4, to_write);

  if (rc == MEMCACHED_SUCCESS)
  {
    if (to_write == false)
    {
      rc= MEMCACHED_BUFFERED;
    }
    else if (ptr->flags.no_reply || ptr->flags.piped)
    {
      rc= MEMCACHED_SUCCESS;
    }
    else
    {
      rc= memcached_coll_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);
      memcached_set_last_response_code(ptr, rc);

      if (rc == MEMCACHED_COUNT)
      {
        rc= MEMCACHED_SUCCESS;
      }
      else
      {
        return rc;
      }

      /* <count> */
      char *string_ptr= buffer;
      char *end_ptr= buffer + MEMCACHED_DEFAULT_COMMAND_SIZE;
      char *next_ptr;

      string_ptr+= 6; // "COUNT="

      if (end_ptr == string_ptr)
        goto read_error;

      for (next_ptr= string_ptr; isdigit(*string_ptr); string_ptr++) {};
      *count = (size_t) strtoull(next_ptr, &string_ptr, 10);

      if (*string_ptr != '\r')
        goto read_error;
    }
  }

  if (rc == MEMCACHED_WRITE_FAILURE)
  {
    memcached_io_reset(instance);
  }

  return rc;

read_error:
  memcached_io_reset(instance);
  return MEMCACHED_PARTIAL_READ;
}

/* APIs : insert, upsert, update */

memcached_return_t memcached_lop_insert(memcached_st *ptr, const char *key, size_t key_length,
                                        const int32_t index,
                                        const char *value, size_t value_length,
                                        memcached_coll_create_attrs_st *attributes)
{
  memcached_coll_query_st query;
  memcached_lop_query_init(&query, index);
  query.value = value;
  query.value_length = value_length;

  return do_coll_insert(ptr, key, key_length,
                        &query, NULL, attributes, LOP_INSERT_OP);
}

memcached_return_t memcached_sop_insert(memcached_st *ptr, const char *key, size_t key_length,
                                        const char *value, size_t value_length,
                                        memcached_coll_create_attrs_st *attributes)
{
  memcached_coll_query_st query;
  memcached_sop_value_query_init(&query, value, value_length);

  return do_coll_insert(ptr, key, key_length,
                        &query, NULL, attributes, SOP_INSERT_OP);
}

memcached_return_t memcached_bop_ext_insert(memcached_st *ptr, const char *key, size_t key_length,
                                            const unsigned char *bkey, size_t bkey_length,
                                            const unsigned char *eflag, size_t eflag_length,
                                            const char *value, size_t value_length,
                                            memcached_coll_create_attrs_st *attributes)
{
  memcached_coll_query_st query;
  memcached_bop_ext_query_init(&query, bkey, bkey_length, NULL);
  query.value = value;
  query.value_length = value_length;

  memcached_hexadecimal_st eflag_hex = { NULL, 0, { 0 } };
  eflag_hex.array = (unsigned char *)eflag;
  eflag_hex.length = eflag_length;

  return do_coll_insert(ptr, key, key_length,
                        &query, &eflag_hex, attributes, BOP_INSERT_OP);
}

memcached_return_t memcached_bop_insert(memcached_st *ptr, const char *key, size_t key_length,
                                        const uint64_t bkey,
                                        const unsigned char *eflag, size_t eflag_length,
                                        const char *value, size_t value_length,
                                        memcached_coll_create_attrs_st *attributes)
{
  memcached_coll_query_st query;
  memcached_bop_query_init(&query, bkey, NULL);
  query.value = value;
  query.value_length = value_length;

  memcached_hexadecimal_st eflag_hex = { NULL, 0, { 0 } };
  eflag_hex.array = (unsigned char *)eflag;
  eflag_hex.length = eflag_length;

  return do_coll_insert(ptr, key, key_length,
                        &query, &eflag_hex, attributes, BOP_INSERT_OP);
}

memcached_return_t memcached_bop_ext_upsert(memcached_st *ptr, const char *key, size_t key_length,
                                            const unsigned char *bkey, size_t bkey_length,
                                            const unsigned char *eflag, size_t eflag_length,
                                            const char *value, size_t value_length,
                                            memcached_coll_create_attrs_st *attributes)
{
  memcached_coll_query_st query;
  memcached_bop_ext_query_init(&query, bkey, bkey_length, NULL);
  query.value = value;
  query.value_length = value_length;

  memcached_hexadecimal_st eflag_hex = { NULL, 0, { 0 } };
  eflag_hex.array = (unsigned char *)eflag;
  eflag_hex.length = eflag_length;

  return do_coll_insert(ptr, key, key_length,
                        &query, &eflag_hex, attributes, BOP_UPSERT_OP);
}

memcached_return_t memcached_bop_upsert(memcached_st *ptr, const char *key, size_t key_length,
                                        const uint64_t bkey,
                                        const unsigned char *eflag, size_t eflag_length,
                                        const char *value, size_t value_length,
                                        memcached_coll_create_attrs_st *attributes)
{
  memcached_coll_query_st query;
  memcached_bop_query_init(&query, bkey, NULL);
  query.value = value;
  query.value_length = value_length;

  memcached_hexadecimal_st eflag_hex = { NULL, 0, { 0 } };
  eflag_hex.array = (unsigned char *)eflag;
  eflag_hex.length = eflag_length;

  return do_coll_insert(ptr, key, key_length,
                        &query, &eflag_hex, attributes, BOP_UPSERT_OP);
}

memcached_return_t memcached_bop_update(memcached_st *ptr, const char *key, size_t key_length,
                                        const uint64_t bkey,
                                        memcached_coll_update_filter_st *update_filter,
                                        const char *value, size_t value_length)
{
  memcached_coll_query_st query;
  memcached_bop_query_init(&query, bkey, NULL);

  return do_coll_update(ptr, key, key_length,
                        &query, update_filter, value, value_length,
                        BOP_UPDATE_OP);
}

memcached_return_t memcached_bop_ext_update(memcached_st *ptr, const char *key, size_t key_length,
                                            const unsigned char *bkey, size_t bkey_length,
                                            memcached_coll_update_filter_st *update_filter,
                                            const char *value, size_t value_length)
{
  memcached_coll_query_st query;
  memcached_bop_ext_query_init(&query, bkey, bkey_length, NULL);

  return do_coll_update(ptr, key, key_length,
                        &query, update_filter, value, value_length,
                        BOP_UPDATE_OP);
}

/* APIs : delete */

memcached_return_t memcached_lop_delete(memcached_st *ptr, const char *key, size_t key_length,
                                        const int32_t index, bool drop_if_empty)
{
  memcached_coll_query_st query;
  memcached_lop_query_init(&query, index);

  return do_coll_delete(ptr, key, key_length,
                        &query, 0, drop_if_empty,
                        LOP_DELETE_OP);
}

memcached_return_t memcached_lop_delete_by_range(memcached_st *ptr, const char *key, size_t key_length,
                                                 const int32_t from, const int32_t to, bool drop_if_empty)
{
  memcached_coll_query_st query;
  memcached_lop_range_query_init(&query, from, to);

  return do_coll_delete(ptr, key, key_length,
                        &query, 0, drop_if_empty,
                        LOP_DELETE_OP);
}

memcached_return_t memcached_sop_delete(memcached_st *ptr, const char *key, size_t key_length,
                                        const char *value, size_t value_length, bool drop_if_empty)
{
  memcached_coll_query_st query;
  memcached_sop_value_query_init(&query, value, value_length);

  return do_coll_delete(ptr, key, key_length,
                        &query, 0, drop_if_empty,
                        SOP_DELETE_OP);
}

memcached_return_t memcached_bop_delete(memcached_st *ptr, const char *key, size_t key_length,
                                        const uint64_t bkey, memcached_coll_eflag_filter_st *eflag_filter,
                                        bool drop_if_empty)
{
  memcached_coll_query_st query;
  memcached_bop_query_init(&query, bkey, eflag_filter);

  return do_coll_delete(ptr, key, key_length,
                        &query, 0, drop_if_empty,
                        BOP_DELETE_OP);
}

memcached_return_t memcached_bop_delete_by_range(memcached_st *ptr, const char *key, size_t key_length,
                                                 const uint64_t from, const uint64_t to,
                                                 memcached_coll_eflag_filter_st *eflag_filter, const size_t count,
                                                 bool drop_if_empty)
{
  memcached_coll_query_st query;
  memcached_bop_range_query_init(&query, from, to, eflag_filter, 0, count);

  return do_coll_delete(ptr, key, key_length,
                        &query, count, drop_if_empty,
                        BOP_DELETE_OP);
}

memcached_return_t memcached_bop_ext_delete(memcached_st *ptr, const char *key, size_t key_length,
                                            const unsigned char *bkey, size_t bkey_length,
                                            memcached_coll_eflag_filter_st *eflag_filter,
                                            bool drop_if_empty)
{
  memcached_coll_query_st query;
  memcached_bop_ext_query_init(&query, bkey, bkey_length, eflag_filter);

  return do_coll_delete(ptr, key, key_length,
                        &query, 0, drop_if_empty,
                        BOP_DELETE_OP);
}

memcached_return_t memcached_bop_ext_delete_by_range(memcached_st *ptr, const char *key, size_t key_length,
                                                     const unsigned char *from, size_t from_length,
                                                     const unsigned char *to, size_t to_length,
                                                     memcached_coll_eflag_filter_st *eflag_filter,
                                                     size_t count, bool drop_if_empty)
{
  memcached_coll_query_st query;
  memcached_bop_ext_range_query_init(&query, from, from_length, to, to_length, eflag_filter, 0, count);

  return do_coll_delete(ptr, key, key_length,
                        &query, count, drop_if_empty,
                        BOP_DELETE_OP);
}

/* APIs : incr & decr */

memcached_return_t memcached_bop_incr(memcached_st *ptr, const char *key, size_t key_length,
                                      const uint64_t bkey, const uint64_t delta, uint64_t *value)
{
  memcached_coll_query_st query;
  memcached_bop_query_init(&query, bkey, NULL);

  return do_coll_arithmetic(ptr, key, key_length,
                            &query, delta, value, BOP_INCR_OP);
}

memcached_return_t memcached_bop_ext_incr(memcached_st *ptr, const char *key, size_t key_length,
                                          const unsigned char *bkey, size_t bkey_length,
                                          const uint64_t delta, uint64_t *value)
{
  memcached_coll_query_st query;
  memcached_bop_ext_query_init(&query, bkey, bkey_length, NULL);

  return do_coll_arithmetic(ptr, key, key_length,
                            &query, delta, value, BOP_INCR_OP);
}

memcached_return_t memcached_bop_decr(memcached_st *ptr, const char *key, size_t key_length,
                                      const uint64_t bkey, const uint64_t delta, uint64_t *value)
{
  memcached_coll_query_st query;
  memcached_bop_query_init(&query, bkey, NULL);

  return do_coll_arithmetic(ptr, key, key_length,
                            &query, delta, value, BOP_DECR_OP);
}

memcached_return_t memcached_bop_ext_decr(memcached_st *ptr, const char *key, size_t key_length,
                                          const unsigned char *bkey, size_t bkey_length,
                                          const uint64_t delta, uint64_t *value)
{
  memcached_coll_query_st query;
  memcached_bop_ext_query_init(&query, bkey, bkey_length, NULL);

  return do_coll_arithmetic(ptr, key, key_length,
                            &query, delta, value, BOP_DECR_OP);
}

/* APIs : exist */

memcached_return_t memcached_sop_exist(memcached_st *ptr, const char *key, size_t key_length,
                                       const char *value, size_t value_length)
{
  return do_coll_exist(ptr, key, key_length,
                       value, value_length,
                       SOP_EXIST_OP);
}

memcached_return_t memcached_sop_piped_exist(memcached_st *ptr, const char *key, size_t key_length,
                                             const size_t number_of_piped_items,
                                             const char * const *values, const size_t *values_length,
                                             memcached_return_t *responses, memcached_return_t *piped_rc)
{
  return do_coll_piped_exist(ptr, key, key_length,
                             number_of_piped_items, values, values_length,
                             responses, piped_rc);
}

/* APIs : get */

memcached_return_t memcached_bop_get_by_query(memcached_st *ptr, const char *key, size_t key_length,
                                              memcached_bop_query_st *query,
                                              bool with_delete, bool drop_if_empty,
                                              memcached_coll_result_st *result)
{
  if (not query)
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                               memcached_literal_param("Query is null.\n"));

  memcached_coll_action_t op;

  switch (query->type)
  {
  case MEMCACHED_COLL_QUERY_LOP:
  case MEMCACHED_COLL_QUERY_LOP_RANGE:
  case MEMCACHED_COLL_QUERY_SOP:
       return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                                  memcached_literal_param("Invalid query type.\n"));
  case MEMCACHED_COLL_QUERY_BOP:
  case MEMCACHED_COLL_QUERY_BOP_RANGE:
  case MEMCACHED_COLL_QUERY_BOP_EXT:
  case MEMCACHED_COLL_QUERY_BOP_EXT_RANGE:
       op= BOP_GET_OP;
       break;
  case MEMCACHED_COLL_QUERY_UNKNOWN:
  default:
       return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT,
                                  memcached_literal_param("Unknown query type.\n"));
  }

  return do_coll_get(ptr, key, key_length,
                     query, with_delete, drop_if_empty, result,
                     op);
}

memcached_return_t memcached_lop_get(memcached_st *ptr, const char *key, size_t key_length,
                                     const int32_t index, bool with_delete, bool drop_if_empty,
                                     memcached_coll_result_st *result)
{
  memcached_coll_query_st query;
  memcached_lop_query_init(&query, index);

  return do_coll_get(ptr, key, key_length,
                     &query, with_delete, drop_if_empty, result,
                     LOP_GET_OP);
}

memcached_return_t memcached_lop_get_by_range(memcached_st *ptr, const char *key, size_t key_length,
                                              const int32_t from, const int32_t to,
                                              bool with_delete, bool drop_if_empty,
                                              memcached_coll_result_st *result)
{
  memcached_coll_query_st query;
  memcached_lop_range_query_init(&query, from, to);

  return do_coll_get(ptr, key, key_length,
                     &query, with_delete, drop_if_empty, result,
                     LOP_GET_OP);
}

memcached_return_t memcached_sop_get(memcached_st *ptr, const char *key, size_t key_length,
                                     size_t count, bool with_delete, bool drop_if_empty,
                                     memcached_coll_result_st *result)
{
  memcached_coll_query_st query;
  memcached_sop_query_init(&query, count);

  return do_coll_get(ptr, key, key_length,
                     &query, with_delete, drop_if_empty, result,
                     SOP_GET_OP);
}

memcached_return_t memcached_bop_get(memcached_st *ptr, const char *key, size_t key_length,
                                     const uint64_t bkey,
                                     memcached_coll_eflag_filter_st *eflag_filter,
                                     bool with_delete, bool drop_if_empty,
                                     memcached_coll_result_st *result)
{
  memcached_coll_query_st query;
  memcached_bop_query_init(&query, bkey, eflag_filter);

  return do_coll_get(ptr, key, key_length,
                     &query, with_delete, drop_if_empty, result,
                     BOP_GET_OP);
}

memcached_return_t memcached_bop_get_by_range(memcached_st *ptr, const char *key, size_t key_length,
                                              const uint64_t from, const uint64_t to,
                                              memcached_coll_eflag_filter_st *eflag_filter,
                                              const size_t offset, const size_t count,
                                              bool with_delete, bool drop_if_empty,
                                              memcached_coll_result_st *result)
{
  memcached_coll_query_st query;
  memcached_bop_range_query_init(&query, from, to, eflag_filter, offset, count);

  return do_coll_get(ptr, key, key_length,
                     &query, with_delete, drop_if_empty, result,
                     BOP_GET_OP);
}

memcached_return_t memcached_bop_ext_get(memcached_st *ptr, const char *key, size_t key_length,
                                         const unsigned char *bkey, size_t bkey_length,
                                         memcached_coll_eflag_filter_st *eflag_filter,
                                         bool with_delete, bool drop_if_empty,
                                         memcached_coll_result_st *result)
{
  memcached_coll_query_st query;
  memcached_bop_ext_query_init(&query, bkey, bkey_length, eflag_filter);

  return do_coll_get(ptr, key, key_length,
                     &query, with_delete, drop_if_empty, result,
                     BOP_GET_OP);
}

memcached_return_t memcached_bop_ext_get_by_range(memcached_st *ptr, const char *key, size_t key_length,
                                                  const unsigned char *from, size_t from_length,
                                                  const unsigned char *to, size_t to_length,
                                                  memcached_coll_eflag_filter_st *eflag_filter,
                                                  const size_t offset, const size_t count,
                                                  bool with_delete, bool drop_if_empty,
                                                  memcached_coll_result_st *result)
{
  memcached_coll_query_st query;
  memcached_bop_ext_range_query_init(&query, from, from_length, to, to_length, eflag_filter, offset, count);

  return do_coll_get(ptr, key, key_length,
                     &query, with_delete, drop_if_empty, result,
                     BOP_GET_OP);
}

memcached_return_t memcached_bop_mget(memcached_st *ptr,
                                      const char * const *keys, const size_t *key_length,
                                      size_t number_of_keys,
                                      memcached_coll_query_st *query)
{
  return do_coll_mget(ptr, keys, key_length, number_of_keys, query, BOP_MGET_OP);
}

memcached_return_t memcached_bop_smget(memcached_st *ptr,
                                       const char * const *keys,
                                       const size_t *key_length, size_t number_of_keys,
                                       memcached_bop_query_st *query,
                                       memcached_coll_smget_result_st *result)
{
  return do_bop_smget(ptr, keys, key_length, number_of_keys, query, result);
}

/* APIs : create */

memcached_return_t memcached_lop_create(memcached_st *ptr, const char *key, size_t key_length,
                                        memcached_coll_create_attrs_st *attributes)
{
  return do_coll_create(ptr, key, key_length, attributes, LOP_CREATE_OP);
}

memcached_return_t memcached_sop_create(memcached_st *ptr, const char *key, size_t key_length,
                                        memcached_coll_create_attrs_st *attributes)
{
  return do_coll_create(ptr, key, key_length, attributes, SOP_CREATE_OP);
}

memcached_return_t memcached_bop_create(memcached_st *ptr, const char *key, size_t key_length,
                                        memcached_coll_create_attrs_st *attributes)
{
  return do_coll_create(ptr, key, key_length, attributes, BOP_CREATE_OP);
}

/* APIs : count */

memcached_return_t memcached_bop_count(memcached_st *ptr,
                                       const char *key, size_t key_length,
                                       const uint64_t bkey,
                                       memcached_coll_eflag_filter_st *eflag_filter,
                                       size_t *count)
{
  memcached_coll_query_st query;
  memcached_bop_query_init(&query, bkey, eflag_filter);

  return do_coll_count(ptr, key, key_length, &query, count, BOP_COUNT_OP);
}

memcached_return_t memcached_bop_count_by_range(memcached_st *ptr, const char *key, size_t key_length,
                                                const uint64_t from, const uint64_t to,
                                                memcached_coll_eflag_filter_st *eflag_filter,
                                                size_t *count)
{
  memcached_coll_query_st query;
  memcached_bop_range_query_init(&query, from, to, eflag_filter, 0, 0);

  return do_coll_count(ptr, key, key_length, &query, count, BOP_COUNT_OP);
}

memcached_return_t memcached_bop_ext_count(memcached_st *ptr, const char *key, size_t key_length,
                                           const unsigned char *bkey, size_t bkey_length,
                                           memcached_coll_eflag_filter_st *eflag_filter,
                                           size_t *count)
{
  memcached_coll_query_st query;
  memcached_bop_ext_query_init(&query, bkey, bkey_length, eflag_filter);

  return do_coll_count(ptr, key, key_length, &query, count, BOP_COUNT_OP);
}

memcached_return_t memcached_bop_ext_count_by_range(memcached_st *ptr, const char *key, size_t key_length,
                                                    const unsigned char *from, size_t from_length,
                                                    const unsigned char *to, size_t to_length,
                                                    memcached_coll_eflag_filter_st *eflag_filter,
                                                    size_t *count)
{
  memcached_coll_query_st query;
  memcached_bop_ext_range_query_init(&query, from, from_length, to, to_length, eflag_filter, 0, 0);

  return do_coll_count(ptr, key, key_length, &query, count, BOP_COUNT_OP);
}

/* APIs : memcached_coll_create_attrs_st */

memcached_return_t memcached_coll_create_attrs_init(memcached_coll_create_attrs_st *ptr,
                                                    uint32_t flags, int32_t expiretime, uint32_t maxcount)
{
  if (not ptr)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  ptr->options.is_initialized = true;

  ptr->flags = flags;
  ptr->options.set_flags = true;

  ptr->expiretime = expiretime;
  ptr->options.set_expiretime = true;

  ptr->maxcount = maxcount;
  ptr->options.set_maxcount = true;

  ptr->overflowaction = OVERFLOWACTION_NONE;
  ptr->options.set_overflowaction = false;

  ptr->is_unreadable = false;
  ptr->options.set_readable = false;

  return MEMCACHED_SUCCESS;
}

memcached_return_t memcached_coll_create_attrs_set_flags(memcached_coll_create_attrs_st *ptr, uint32_t flags)
{
  ptr->flags = flags;
  ptr->options.set_flags = true;

  return MEMCACHED_SUCCESS;
}

memcached_return_t memcached_coll_create_attrs_set_expiretime(memcached_coll_create_attrs_st *ptr, int32_t expiretime)
{
  ptr->expiretime = expiretime;
  ptr->options.set_expiretime = true;

  return MEMCACHED_SUCCESS;
}

memcached_return_t memcached_coll_create_attrs_set_maxcount(memcached_coll_create_attrs_st *ptr, uint32_t maxcount)
{
  ptr->maxcount = maxcount;
  ptr->options.set_maxcount = true;

  return MEMCACHED_SUCCESS;
}

memcached_return_t memcached_coll_create_attrs_set_overflowaction(memcached_coll_create_attrs_st *ptr,
                                              memcached_coll_overflowaction_t overflowaction)
{
  ptr->overflowaction = overflowaction;
  ptr->options.set_overflowaction = true;

  return MEMCACHED_SUCCESS;
}

memcached_return_t memcached_coll_create_attrs_set_unreadable(memcached_coll_create_attrs_st *ptr, bool is_unreadable)
{
  ptr->is_unreadable = is_unreadable;
  ptr->options.set_readable = true;

  return MEMCACHED_SUCCESS;
}

/* APIs : memcached_coll_query_st */

static void memcached_coll_query_init(memcached_coll_query_st *ptr)
{
  if (not ptr)
  {
    return;
  }

  ptr->options.is_initialized = true;
  ptr->value = 0;
  ptr->value_length = 0;
  ptr->offset = 0;
  ptr->count = 0;
  ptr->eflag_filter = NULL;

  return;
}

memcached_return_t memcached_lop_query_init(memcached_coll_query_st *ptr, const int32_t index)
{
  memcached_coll_query_init(ptr);

  ptr->type = MEMCACHED_COLL_QUERY_LOP;
  ptr->sub_key.index = index;

  return MEMCACHED_SUCCESS;
}

memcached_return_t memcached_lop_range_query_init(memcached_coll_query_st *ptr,
                                                  const int32_t index_from, const int32_t index_to)
{
  memcached_coll_query_init(ptr);

  ptr->type = MEMCACHED_COLL_QUERY_LOP_RANGE;

  ptr->sub_key.index_range[0] = index_from;
  ptr->sub_key.index_range[1] = index_to;

  return MEMCACHED_SUCCESS;
}

memcached_return_t memcached_sop_query_init(memcached_coll_query_st *ptr,
                                            size_t count)
{
  memcached_coll_query_init(ptr);

  ptr->type = MEMCACHED_COLL_QUERY_SOP;
  ptr->count = count;

  return MEMCACHED_SUCCESS;
}

memcached_return_t memcached_sop_value_query_init(memcached_coll_query_st *ptr,
                                                  const char *value, size_t value_length)
{
  memcached_coll_query_init(ptr);

  ptr->type = MEMCACHED_COLL_QUERY_SOP;
  ptr->value = value;
  ptr->value_length = value_length;

  return MEMCACHED_SUCCESS;
}

memcached_return_t memcached_bop_query_init(memcached_bop_query_st *ptr,
                                            const uint64_t bkey,
                                            memcached_coll_eflag_filter_st *eflag_filter)
{
  memcached_coll_query_init(ptr);

  ptr->type = MEMCACHED_COLL_QUERY_BOP;
  ptr->sub_key.bkey = bkey;
  ptr->eflag_filter = eflag_filter;

  return MEMCACHED_SUCCESS;
}

memcached_return_t memcached_bop_range_query_init(memcached_bop_query_st *ptr,
                                                  const uint64_t bkey_from, const uint64_t bkey_to,
                                                  memcached_coll_eflag_filter_st *eflag_filter,
                                                  size_t offset, size_t count)
{
  memcached_coll_query_init(ptr);

  ptr->type = MEMCACHED_COLL_QUERY_BOP_RANGE;
  ptr->sub_key.bkey_range[0] = bkey_from;
  ptr->sub_key.bkey_range[1] = bkey_to;
  ptr->eflag_filter = eflag_filter;
  ptr->offset = offset;
  ptr->count = count;

  return MEMCACHED_SUCCESS;
}

memcached_return_t memcached_bop_ext_query_init(memcached_bop_query_st *ptr,
                                                const unsigned char *bkey, size_t bkey_length,
                                                memcached_coll_eflag_filter_st *eflag_filter)
{
  memcached_coll_query_init(ptr);

  ptr->type = MEMCACHED_COLL_QUERY_BOP_EXT;
  ptr->sub_key.bkey_ext.array = (unsigned char *)bkey;
  ptr->sub_key.bkey_ext.length = bkey_length;
  ptr->eflag_filter = eflag_filter;

  return MEMCACHED_SUCCESS;
}

memcached_return_t memcached_bop_ext_range_query_init(memcached_bop_query_st *ptr,
                                                      const unsigned char *bkey_from, size_t bkey_from_length,
                                                      const unsigned char *bkey_to, size_t bkey_to_length,
                                                      memcached_coll_eflag_filter_st *eflag_filter,
                                                      size_t offset, size_t count)
{
  memcached_coll_query_init(ptr);

  ptr->type = MEMCACHED_COLL_QUERY_BOP_EXT_RANGE;

  ptr->sub_key.bkey_ext_range[0].array = (unsigned char *)bkey_from;
  ptr->sub_key.bkey_ext_range[0].length = bkey_from_length;
  ptr->sub_key.bkey_ext_range[1].array = (unsigned char *)bkey_to;
  ptr->sub_key.bkey_ext_range[1].length = bkey_to_length;
  ptr->eflag_filter = eflag_filter;
  ptr->offset = offset;
  ptr->count = count;

  return MEMCACHED_SUCCESS;
}

/* APIs : memcached_coll_eflag_filter_st */

memcached_return_t memcached_coll_eflag_filter_init(memcached_coll_eflag_filter_st *ptr,
                                                    const size_t fwhere,
                                                    const unsigned char *fvalue, const size_t fvalue_length,
                                                    memcached_coll_comp_t comp_op)
{
  if (not ptr)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  if (fvalue_length > MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  ptr->options.is_initialized = true;
  ptr->options.is_bitwised = false;

  ptr->fwhere = fwhere;
  ptr->flength = fvalue_length;

  ptr->comp.op = comp_op;
#ifdef ENABLE_EFLAGS_FILTER
  if (fvalue_length > 0) {
    ptr->comp.count = 1;
  }
  else
  {
    ptr->comp.count = 0;
  }

  ptr->comp.fvalue[0].array = (unsigned char *)fvalue;
  ptr->comp.fvalue[0].length = fvalue_length;
  ptr->comp.fvalue[0].options.array_is_allocated = false;
#else
  ptr->comp.fvalue.array = (unsigned char *)fvalue;
  ptr->comp.fvalue.length = fvalue_length;
  ptr->comp.fvalue.options.array_is_allocated = false;
#endif

  return MEMCACHED_SUCCESS;
}

#ifdef ENABLE_EFLAGS_FILTER
memcached_return_t memcached_coll_eflags_filter_init(memcached_coll_eflag_filter_st *ptr,
                                                    const size_t fwhere,
                                                    const unsigned char *fvalues,
                                                    const size_t fvalue_length,
                                                    const size_t fvalue_count,
                                                    memcached_coll_comp_t comp_op)
{
  if (not ptr)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  if (fvalue_length > MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  if (comp_op == MEMCACHED_COLL_COMP_EQ || comp_op == MEMCACHED_COLL_COMP_NE)
  {
    if (fvalue_count > MEMCACHED_COLL_MAX_EFLAGS_COUNT)
    {
      return MEMCACHED_INVALID_ARGUMENTS;
    }
  }
  else
  {
    if (fvalue_count > 1)
    {
      return MEMCACHED_INVALID_ARGUMENTS;
    }
  }

  ptr->options.is_initialized = true;
  ptr->options.is_bitwised = false;

  ptr->fwhere = fwhere;
  ptr->flength = fvalue_length;

  ptr->comp.op = comp_op;
  ptr->comp.count = fvalue_count;

  for (size_t i=0; i<fvalue_count; i++) {
    ptr->comp.fvalue[i].array = (unsigned char *)fvalues+(i*fvalue_length);
    ptr->comp.fvalue[i].length = fvalue_length;
    ptr->comp.fvalue[i].options.array_is_allocated = false;
  }

  return MEMCACHED_SUCCESS;
}
#endif

memcached_return_t memcached_coll_eflag_filter_set_bitwise(memcached_coll_eflag_filter_st *ptr,
                                                           const unsigned char *foperand, const size_t foperand_length,
                                                           memcached_coll_bitwise_t bitwise_op)
{
  if (not ptr)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  if (not ptr->options.is_initialized)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  if (ptr->flength != foperand_length)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  ptr->options.is_bitwised = true;

  ptr->bitwise.op = bitwise_op;
  ptr->bitwise.foperand.array = (unsigned char *)foperand;
  ptr->bitwise.foperand.length = foperand_length;
  ptr->bitwise.foperand.options.array_is_allocated = false;

  return MEMCACHED_SUCCESS;
}

/* memcached_coll_update_filter_st */

memcached_return_t memcached_coll_update_filter_init(memcached_coll_update_filter_st *ptr,
                                                     const unsigned char *fvalue, const size_t fvalue_length)
{
  if (not ptr)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  if (fvalue_length > MEMCACHED_COLL_MAX_BYTE_ARRAY_LENGTH)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  ptr->options.is_initialized = true;
  ptr->options.is_bitwised = false;

  ptr->comp.fvalue.array = (unsigned char *)fvalue;
  ptr->comp.fvalue.length = fvalue_length;
  ptr->flength = fvalue_length;

  return MEMCACHED_SUCCESS;
}

memcached_return_t memcached_coll_update_filter_set_bitwise(memcached_coll_update_filter_st *ptr,
                                                            const size_t fwhere, memcached_coll_bitwise_t bitwise_op)
{
  if (not ptr)
  {
  return MEMCACHED_INVALID_ARGUMENTS;
  }

  if (not ptr->options.is_initialized)
  {
  return MEMCACHED_INVALID_ARGUMENTS;
  }

  ptr->options.is_bitwised = true;

  ptr->fwhere= fwhere;
  ptr->bitwise.op = bitwise_op;

  return MEMCACHED_SUCCESS;
}

size_t memcached_hexadecimal_to_str(memcached_hexadecimal_st *ptr, char *buffer, size_t buffer_length)
{
  if (buffer_length <= ptr->length*2)
    return 0;

  buffer[0] = '0';
  buffer[1] = 'x';

  memcached_conv_hex_to_str(NULL, ptr, buffer+2, MEMCACHED_COLL_MAX_BYTE_STRING_LENGTH+2);

  return ptr->length*2 + 2;
}

/* API : piped_insert && piped_insert_bulk */

memcached_return_t memcached_lop_piped_insert(memcached_st *ptr, const char *key, const size_t key_length,
                                              const size_t number_of_piped_items,
                                              const int32_t *indexes,
                                              const char * const *values, const size_t *values_length,
                                              memcached_coll_create_attrs_st *attributes,
                                              memcached_return_t *results, memcached_return_t *piped_rc)
{
  memcached_return_t rc;

  memcached_coll_query_st *queries= static_cast<memcached_coll_query_st *>(libmemcached_malloc(ptr, sizeof(memcached_coll_query_st)*number_of_piped_items));
  for (size_t i=0; i<number_of_piped_items; i++)
  {
    memcached_lop_query_init(&queries[i], indexes[i]);
  }

  rc= do_coll_piped_insert(ptr, key, key_length, number_of_piped_items, queries,
                           NULL, 0, values, values_length,
                           attributes, results, piped_rc, LOP_INSERT_OP);

  libmemcached_free(ptr, queries);

  return rc;
}

memcached_return_t memcached_sop_piped_insert(memcached_st *ptr, const char *key, const size_t key_length,
                                              const size_t number_of_piped_items,
                                              const char * const *values, const size_t *values_length,
                                              memcached_coll_create_attrs_st *attributes,
                                              memcached_return_t *results, memcached_return_t *piped_rc)
{
  memcached_return_t rc;

  memcached_coll_query_st *queries= static_cast<memcached_coll_query_st *>(libmemcached_malloc(ptr, sizeof(memcached_coll_query_st)*number_of_piped_items));
  for (size_t i=0; i<number_of_piped_items; i++)
  {
    memcached_sop_value_query_init(&queries[i], values[i], values_length[i]);
  }

  rc= do_coll_piped_insert(ptr, key, key_length, number_of_piped_items, queries,
                           NULL, 0, values, values_length,
                           attributes, results, piped_rc, SOP_INSERT_OP);

  libmemcached_free(ptr, queries);

  return rc;
}

memcached_return_t memcached_bop_piped_insert(memcached_st *ptr, const char *key, const size_t key_length,
                                              const size_t number_of_piped_items,
                                              const uint64_t *bkeys,
                                              const unsigned char * const *eflags, const size_t *eflags_length,
                                              const char * const *values, const size_t *values_length,
                                              memcached_coll_create_attrs_st *attributes,
                                              memcached_return_t *results, memcached_return_t *piped_rc)
{
  memcached_return_t rc;

  memcached_coll_query_st *queries= static_cast<memcached_coll_query_st *>(libmemcached_malloc(ptr, sizeof(memcached_coll_query_st)*number_of_piped_items));
  for (size_t i=0; i<number_of_piped_items; i++)
  {
    memcached_bop_query_init(&queries[i], bkeys[i], NULL);
  }

  rc= do_coll_piped_insert(ptr, key, key_length, number_of_piped_items, queries,
                           eflags, eflags_length, values, values_length,
                           attributes, results, piped_rc, BOP_INSERT_OP);

  libmemcached_free(ptr, queries);

  return rc;
}

memcached_return_t memcached_bop_ext_piped_insert(memcached_st *ptr, const char *key, const size_t key_length,
                                                  const size_t number_of_piped_items,
                                                  const unsigned char * const *bkeys, const size_t *bkeys_length,
                                                  const unsigned char * const *eflags, const size_t *eflags_length,
                                                  const char * const *values, const size_t *values_length,
                                                  memcached_coll_create_attrs_st *attributes,
                                                  memcached_return_t *results, memcached_return_t *piped_rc)
{
  memcached_return_t rc;

  memcached_coll_query_st *queries= static_cast<memcached_coll_query_st *>(libmemcached_malloc(ptr, sizeof(memcached_coll_query_st)*number_of_piped_items));
  for (size_t i=0; i<number_of_piped_items; i++)
  {
    memcached_bop_ext_query_init(&queries[i], bkeys[i], bkeys_length[i], NULL);
  }

  rc= do_coll_piped_insert(ptr, key, key_length, number_of_piped_items, queries,
                           eflags, eflags_length, values, values_length,
                           attributes, results, piped_rc, BOP_INSERT_OP);

  libmemcached_free(ptr, queries);

  return rc;
}

memcached_return_t memcached_lop_piped_insert_bulk(memcached_st *ptr,
                                                   const char * const *keys,
                                                   const size_t *key_length, size_t number_of_keys,
                                                   const int32_t index,
                                                   const char *value, size_t value_length,
                                                   memcached_coll_create_attrs_st *attributes,
                                                   memcached_return_t *results,
                                                   memcached_return_t *piped_rc)
{
  memcached_coll_query_st query_obj;
  memcached_lop_query_init(&query_obj, index);
  query_obj.value= value;
  query_obj.value_length= value_length;

  return do_coll_piped_insert_bulk(ptr, keys, key_length, number_of_keys,
                                   &query_obj, NULL, 0, attributes, results, piped_rc,
                                   LOP_INSERT_OP);
}

memcached_return_t memcached_sop_piped_insert_bulk(memcached_st *ptr,
                                                   const char * const *keys,
                                                   const size_t *key_length, size_t number_of_keys,
                                                   const char *value, size_t value_length,
                                                   memcached_coll_create_attrs_st *attributes,
                                                   memcached_return_t *results,
                                                   memcached_return_t *piped_rc)
{
  memcached_coll_query_st query_obj;
  memcached_sop_value_query_init(&query_obj, value, value_length);

  return do_coll_piped_insert_bulk(ptr, keys, key_length, number_of_keys,
                                   &query_obj, NULL, 0, attributes, results, piped_rc,
                                   SOP_INSERT_OP);
}

memcached_return_t memcached_bop_piped_insert_bulk(memcached_st *ptr,
                                                   const char * const *keys,
                                                   const size_t *key_length, size_t number_of_keys,
                                                   const uint64_t bkey,
                                                   const unsigned char *eflag, size_t eflag_length,
                                                   const char *value, size_t value_length,
                                                   memcached_coll_create_attrs_st *attributes,
                                                   memcached_return_t *results,
                                                   memcached_return_t *piped_rc)
{
  memcached_coll_query_st query_obj;
  memcached_bop_query_init(&query_obj, bkey, NULL);
  query_obj.value= value;
  query_obj.value_length= value_length;

  return do_coll_piped_insert_bulk(ptr, keys, key_length, number_of_keys,
                                   &query_obj, eflag, eflag_length, attributes, results, piped_rc,
                                   BOP_INSERT_OP);
}

memcached_return_t memcached_bop_ext_piped_insert_bulk(memcached_st *ptr,
                                                       const char * const *keys,
                                                       const size_t *key_length, size_t number_of_keys,
                                                       const unsigned char *bkey, size_t bkey_length,
                                                       const unsigned char *eflag, size_t eflag_length,
                                                       const char *value, size_t value_length,
                                                       memcached_coll_create_attrs_st *attributes,
                                                       memcached_return_t *results,
                                                       memcached_return_t *piped_rc)
{
  memcached_coll_query_st query_obj;
  memcached_bop_ext_query_init(&query_obj, bkey, bkey_length, NULL);
  query_obj.value= value;
  query_obj.value_length= value_length;

  return do_coll_piped_insert_bulk(ptr, keys, key_length, number_of_keys,
                                   &query_obj, eflag, eflag_length, attributes, results, piped_rc,
                                   BOP_INSERT_OP);
}
