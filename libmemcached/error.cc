/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  LibMemcached
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  All rights reserved.
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
#include <cstdarg>

#define MAX_ERROR_LENGTH 2048
#ifdef REFACTORING_ERROR_PRINT
#define ERROR_HDR_LENGTH 128
#endif
struct memcached_error_t
{
  memcached_st *root;
  uint64_t query_id;
  struct memcached_error_t *next;
  memcached_return_t rc;
  int local_errno;
  size_t size;
  char message[MAX_ERROR_LENGTH];
};

static void _set(memcached_server_st& server, memcached_st& memc)
{
  if (server.error_messages && server.error_messages->query_id != server.root->query_id)
  {
    memcached_error_free(server);
  }

  if (memc.error_messages == NULL)
    return;

  memcached_error_t *error= (struct memcached_error_t *)libmemcached_malloc(&memc, sizeof(struct memcached_error_t));
  if (not error) // Bad business if this happens
    return;

  memcpy(error, memc.error_messages, sizeof(memcached_error_t));
  error->next= server.error_messages;
  server.error_messages= error;
}

static void _set(memcached_st& memc, memcached_string_t *str, memcached_return_t &rc, const char *at, int local_errno= 0)
{
  (void)at;
  if (memc.error_messages && memc.error_messages->query_id != memc.query_id)
  {
    memcached_error_free(memc);
  }

  // For memory allocation we use our error since it is a bit more specific
  if (local_errno == ENOMEM and rc == MEMCACHED_ERRNO)
  {
    rc= MEMCACHED_MEMORY_ALLOCATION_FAILURE;
  }

  if (rc == MEMCACHED_MEMORY_ALLOCATION_FAILURE)
  {
    local_errno= ENOMEM;
  }

  if (rc == MEMCACHED_ERRNO and not local_errno)
  {
    local_errno= errno;
    rc= MEMCACHED_ERRNO;
  }

  if (rc == MEMCACHED_ERRNO and local_errno == ENOTCONN)
  {
    rc= MEMCACHED_CONNECTION_FAILURE;
  }

  if (local_errno == EINVAL)
  {
    rc= MEMCACHED_INVALID_ARGUMENTS;
  }

  if (local_errno == ECONNREFUSED)
  {
    rc= MEMCACHED_CONNECTION_FAILURE;
  }

  memcached_error_t *error= (struct memcached_error_t *)libmemcached_malloc(&memc, sizeof(struct memcached_error_t));
  if (not error) // Bad business if this happens
    return;

  error->root= &memc;
  error->query_id= memc.query_id;
  error->rc= rc;
  error->local_errno= local_errno;

  const char *errmsg_ptr;
  char errmsg[MAX_ERROR_LENGTH];
  errmsg[0]= 0;
  errmsg_ptr= errmsg;

  if (local_errno)
  {
#ifdef STRERROR_R_CHAR_P
    errmsg_ptr= strerror_r(local_errno, errmsg, sizeof(errmsg));
#else
    strerror_r(local_errno, errmsg, sizeof(errmsg));
    errmsg_ptr= errmsg;
#endif
  }


#ifdef REFACTORING_ERROR_PRINT
  struct timeval time;
  struct tm *tm_info;
  gettimeofday(&time, NULL);
  tm_info= localtime(&time.tv_sec);
  char errmsg_header[ERROR_HDR_LENGTH];
  snprintf(errmsg_header, ERROR_HDR_LENGTH,
           "[%04d-%02d-%02d %02d:%02d:%02d.%06ld] mc_id:%" PRIu64 " mc_qid:%" PRIu64 " er_qid:%" PRIu64 "",
           tm_info->tm_year+1900, tm_info->tm_mon+1, tm_info->tm_mday,
           tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, (long)time.tv_usec,
           memc.mc_id, memc.query_id, error->query_id);
  if (str and str->size and local_errno)
  {
    error->size= (int)snprintf(error->message, MAX_ERROR_LENGTH, "%s %s(%s), %.*s -> %s",
                               errmsg_header,
                               memcached_strerror(&memc, rc),
                               errmsg_ptr,
                               memcached_string_printf(*str), at);
  }
  else if (local_errno)
  {
    error->size= (int)snprintf(error->message, MAX_ERROR_LENGTH, "%s %s(%s) -> %s",
                               errmsg_header,
                               memcached_strerror(&memc, rc),
                               errmsg_ptr,
                               at);
  }
  else if (rc == MEMCACHED_PARSE_ERROR and str and str->size)
  {
    error->size= (int)snprintf(error->message, MAX_ERROR_LENGTH, "%s %.*s -> %s",
                               errmsg_header, int(str->size), str->c_str, at);
  }
  else if (str and str->size)
  {
    error->size= (int)snprintf(error->message, MAX_ERROR_LENGTH, "%s %s, %.*s -> %s",
                               errmsg_header,
                               memcached_strerror(&memc, rc),
                               int(str->size), str->c_str, at);
  }
  else
  {
    error->size= (int)snprintf(error->message, MAX_ERROR_LENGTH, "%s %s -> %s",
                               errmsg_header, memcached_strerror(&memc, rc), at);
  }
#else
  if (str and str->size and local_errno)
  {
    error->size= (int)snprintf(error->message, MAX_ERROR_LENGTH, "%s(%s), %.*s -> %s",
                               memcached_strerror(&memc, rc),
                               errmsg_ptr,
                               memcached_string_printf(*str), at);
  }
  else if (local_errno)
  {
    error->size= (int)snprintf(error->message, MAX_ERROR_LENGTH, "%s(%s) -> %s",
                               memcached_strerror(&memc, rc),
                               errmsg_ptr,
                               at);
  }
  else if (rc == MEMCACHED_PARSE_ERROR and str and str->size)
  {
    error->size= (int)snprintf(error->message, MAX_ERROR_LENGTH, "%.*s -> %s",
                               int(str->size), str->c_str, at);
  }
  else if (str and str->size)
  {
    error->size= (int)snprintf(error->message, MAX_ERROR_LENGTH, "%s, %.*s -> %s",
                               memcached_strerror(&memc, rc),
                               int(str->size), str->c_str, at);
  }
  else
  {
    error->size= (int)snprintf(error->message, MAX_ERROR_LENGTH, "%s -> %s",
                               memcached_strerror(&memc, rc), at);
  }
#endif

  error->next= memc.error_messages;
  memc.error_messages= error;
}

/* memcached_set_error() comment :
 * MEMCACHED_ERRNO must not be handled in memcached_set_errno().
 * But because the version of this libmemcached is old, there is a problem and the errors comes in.
 * Check the overall error handling and set "assert" instead of "if-return".
 */
memcached_return_t memcached_set_error(memcached_st& memc, memcached_return_t rc, const char *at, const char *str, size_t length)
{
  if (rc == MEMCACHED_ERRNO)
  {
    return rc;
  }
  //assert_msg(rc != MEMCACHED_ERRNO, "Programmer error, MEMCACHED_ERRNO was set to be returned to client");
  memcached_string_t tmp= { str, length };
  return memcached_set_error(memc, rc, at, tmp);
}

memcached_return_t memcached_set_error(memcached_server_st& self, memcached_return_t rc, const char *at, const char *str, size_t length)
{
  if (rc == MEMCACHED_ERRNO)
  {
    return rc;
  }
  //assert_msg(rc != MEMCACHED_ERRNO, "Programmer error, MEMCACHED_ERRNO was set to be returned to client");
  assert_msg(rc != MEMCACHED_SOME_ERRORS, "Programmer error, MEMCACHED_SOME_ERRORS was about to be set on a memcached_server_st");
  memcached_string_t tmp= { str, length };
  return memcached_set_error(self, rc, at, tmp);
}

memcached_return_t memcached_set_error(memcached_st& memc, memcached_return_t rc, const char *at, memcached_string_t& str)
{
  if (rc == MEMCACHED_ERRNO)
  {
    return rc;
  }
  //assert_msg(rc != MEMCACHED_ERRNO, "Programmer error, MEMCACHED_ERRNO was set to be returned to client");
  if (memcached_success(rc))
    return MEMCACHED_SUCCESS;

  _set(memc, &str, rc, at);

  return rc;
}

memcached_return_t memcached_set_parser_error(memcached_st& memc,
                                              const char *at,
                                              const char *format, ...)
{
  va_list args;

  char buffer[BUFSIZ];
  va_start(args, format);
  int length= vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  return memcached_set_error(memc, MEMCACHED_PARSE_ERROR, at, buffer, length);
}

memcached_return_t memcached_set_error(memcached_server_st& self, memcached_return_t rc, const char *at, memcached_string_t& str)
{
  if (rc == MEMCACHED_ERRNO)
  {
    return rc;
  }
  //assert_msg(rc != MEMCACHED_ERRNO, "Programmer error, MEMCACHED_ERRNO was set to be returned to client");
  assert_msg(rc != MEMCACHED_SOME_ERRORS, "Programmer error, MEMCACHED_SOME_ERRORS was about to be set on a memcached_server_st");
  if (memcached_success(rc))
    return MEMCACHED_SUCCESS;

  char hostname_port_message[MAX_ERROR_LENGTH];
  size_t size;
  if (str.size)
  {
    size= snprintf(hostname_port_message, sizeof(hostname_port_message), "%.*s, host: %s:%d",
                   memcached_string_printf(str),
                   self.hostname, int(self.port));
  }
  else
  {
    size= snprintf(hostname_port_message, sizeof(hostname_port_message), "host: %s:%d",
                   self.hostname, int(self.port));
  }

  memcached_string_t error_host= { hostname_port_message, size };

  if (not self.root)
    return rc;

  _set(*self.root, &error_host, rc, at);
  _set(self, (*self.root));

  return rc;
}

memcached_return_t memcached_set_error(memcached_server_st& self, memcached_return_t rc, const char *at)
{
  if (rc == MEMCACHED_ERRNO)
  {
    return rc;
  }
  //assert_msg(rc != MEMCACHED_ERRNO, "Programmer error, MEMCACHED_ERRNO was set to be returned to client");
  assert_msg(rc != MEMCACHED_SOME_ERRORS, "Programmer error, MEMCACHED_SOME_ERRORS was about to be set on a memcached_server_st");
  if (memcached_success(rc))
    return MEMCACHED_SUCCESS;

  char hostname_port[NI_MAXHOST +NI_MAXSERV + sizeof("host : ")];
  size_t size= snprintf(hostname_port, sizeof(hostname_port), "host: %s:%d", self.hostname, int(self.port));

  memcached_string_t error_host= { hostname_port, size};

  if (not self.root)
    return rc;

  _set(*self.root, &error_host, rc, at);
  _set(self, *self.root);

  return rc;
}

memcached_return_t memcached_set_error(memcached_st& self, memcached_return_t rc, const char *at)
{
  if (rc == MEMCACHED_ERRNO)
  {
    return rc;
  }
  //assert_msg(rc != MEMCACHED_ERRNO, "Programmer error, MEMCACHED_ERRNO was set to be returned to client");
  if (memcached_success(rc))
    return MEMCACHED_SUCCESS;

  _set(self, NULL, rc, at);

  return rc;
}

memcached_return_t memcached_set_errno(memcached_st& self, int local_errno, const char *at, const char *str, size_t length)
{
  memcached_string_t tmp= { str, length };
  return memcached_set_errno(self, local_errno, at, tmp);
}

memcached_return_t memcached_set_errno(memcached_server_st& self, int local_errno, const char *at, const char *str, size_t length)
{
  memcached_string_t tmp= { str, length };
  return memcached_set_errno(self, local_errno, at, tmp);
}

memcached_return_t memcached_set_errno(memcached_st& self, int local_errno, const char *at)
{
  if (not local_errno)
    return MEMCACHED_SUCCESS;

  memcached_return_t rc= MEMCACHED_ERRNO;
  _set(self, NULL, rc, at, local_errno);

  return rc;
}

memcached_return_t memcached_set_errno(memcached_st& memc, int local_errno, const char *at, memcached_string_t& str)
{
  if (not local_errno)
    return MEMCACHED_SUCCESS;

  memcached_return_t rc= MEMCACHED_ERRNO;
  _set(memc, &str, rc, at, local_errno);

  return rc;
}

memcached_return_t memcached_set_errno(memcached_server_st& self, int local_errno, const char *at, memcached_string_t& str)
{
  if (not local_errno)
    return MEMCACHED_SUCCESS;

  char hostname_port_message[MAX_ERROR_LENGTH];
  size_t size;
  if (str.size)
  {
    size= snprintf(hostname_port_message, sizeof(hostname_port_message), "%.*s, host: %s:%d",
                   memcached_string_printf(str),
                   self.hostname, int(self.port));
  }
  else
  {
    size= snprintf(hostname_port_message, sizeof(hostname_port_message), "host: %s:%d",
                   self.hostname, int(self.port));
  }

  memcached_string_t error_host= { hostname_port_message, size };

  memcached_return_t rc= MEMCACHED_ERRNO;
  if (not self.root)
    return rc;

  _set(*self.root, &error_host, rc, at, local_errno);
  _set(self, (*self.root));

  return rc;
}

memcached_return_t memcached_set_errno(memcached_server_st& self, int local_errno, const char *at)
{
  if (not local_errno)
    return MEMCACHED_SUCCESS;

  char hostname_port_message[MAX_ERROR_LENGTH];
  size_t size = snprintf(hostname_port_message, sizeof(hostname_port_message), "host: %s:%d",
                         self.hostname, int(self.port));

  memcached_string_t error_host= { hostname_port_message, size };

  memcached_return_t rc= MEMCACHED_ERRNO;
  if (not self.root)
    return rc;

  _set(*self.root, &error_host, rc, at, local_errno);
  _set(self, (*self.root));

  return rc;
}

#ifdef REFACTORING_ERROR_PRINT
static bool _error_msg_buffer_alloc(memcached_st *self)
{
  size_t buffer_size= 0;
  size_t size= ERROR_HDR_LENGTH;
  char *buffer= NULL;

  memcached_error_t *error= self->error_messages;
  while (error != NULL)
  {
    size += (error->size + 1);
    error= error->next;
  }

  buffer_size= MAX_ERROR_LENGTH + (MAX_ERROR_LENGTH * (int)(size/MAX_ERROR_LENGTH));
  if (self->error_msg_buffer)
  {
    if (self->error_msg_buffer_size > size)
    {
      return true;
    }
    buffer= static_cast<char *>(libmemcached_realloc(self, self->error_msg_buffer, buffer_size * sizeof(char)));
  }
  else
  {
    buffer= static_cast<char *>(libmemcached_malloc(self, buffer_size * sizeof(char)));
  }
  if (not buffer)
  {
    return false;
  }

  self->error_msg_buffer= buffer;
  self->error_msg_buffer_size= buffer_size;
  return true;
}

static void _error_msg_buffer_free(memcached_st& self)
{
  if (self.error_msg_buffer != NULL)
  {
    free(self.error_msg_buffer);
    self.error_msg_buffer= NULL;
    self.error_msg_buffer_size= 0;
  }
}

const char *memcached_detail_error_message(memcached_st *self, memcached_return_t rc)
{
  if (not self)
  {
    return memcached_strerror(NULL, MEMCACHED_INVALID_ARGUMENTS);
  }

  if (not self->error_messages)
  {
    return memcached_strerror(NULL, rc);
  }

  if (_error_msg_buffer_alloc(self) == false)
  {
    return memcached_strerror(NULL, MEMCACHED_MEMORY_ALLOCATION_FAILURE);
  }

  int write_length= 0;
  char *buffer= self->error_msg_buffer;
  size_t buffer_size= self->error_msg_buffer_size;
  memcached_error_t *error= self->error_messages;

  write_length= (int)snprintf(buffer, ERROR_HDR_LENGTH, "rc(%s)\n", memcached_strerror(NULL, rc));
  while (error != NULL)
  {
    write_length += (int)snprintf(buffer+write_length, buffer_size-write_length, "%s\n", error->message);
    error= error->next;
  }
  return self->error_msg_buffer;
}
#endif

static void _error_print(const memcached_error_t *error)
{
  if (error == NULL)
  {
    return;
  }

  if (error->size == 0)
  {
    fprintf(stderr, "%s\n", memcached_strerror(NULL, error->rc) );
  }
  else
  {
    fprintf(stderr, "%s %s\n", memcached_strerror(NULL, error->rc), error->message);
  }

  _error_print(error->next);
}

void memcached_error_print(const memcached_st *self)
{
  if (not self)
    return;

  _error_print(self->error_messages);
}

static void _error_free(memcached_error_t *error)
{
  if (not error)
    return;

  _error_free(error->next);

  if (error && error->root)
  {
    libmemcached_free(error->root, error);
  }
  else if (error)
  {
    free(error);
  }
}

void memcached_error_free(memcached_st& self)
{
  _error_free(self.error_messages);
  self.error_messages= NULL;
#ifdef REFACTORING_ERROR_PRINT
  if (self.query_id == 0) { /* by memcached_free */
    _error_msg_buffer_free(self);
  }
#endif
}

void memcached_error_free(memcached_server_st& self)
{
  _error_free(self.error_messages);
  self.error_messages= NULL;
}

const char *memcached_last_error_message(memcached_st *memc)
{
  if (memc == NULL)
  {
    return memcached_strerror(memc, MEMCACHED_INVALID_ARGUMENTS);
  }

  if (not memc->error_messages)
    return memcached_strerror(memc, MEMCACHED_SUCCESS);

  if (not memc->error_messages->size)
    return memcached_strerror(memc, memc->error_messages->rc);

  return memc->error_messages->message;
}

bool memcached_has_current_error(memcached_st &memc)
{
  if (memc.error_messages
      and memc.error_messages->query_id == memc.query_id
      and memcached_failed(memc.error_messages->rc))
  {
    return true;
  }

  return false;
}

bool memcached_has_current_error(memcached_server_st& server)
{
  return memcached_has_current_error(*(server.root));
}

memcached_return_t memcached_last_error(memcached_st *memc)
{
  if (memc == NULL)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  if (not memc->error_messages)
    return MEMCACHED_SUCCESS;

  return memc->error_messages->rc;
}

int memcached_last_error_errno(memcached_st *memc)
{
  if (memc == NULL)
  {
    return 0;
  }

  if (not memc->error_messages)
  {
    return 0;
  }

  return memc->error_messages->local_errno;
}

const char *memcached_server_error(memcached_server_instance_st server)
{
  if (server == NULL)
  {
    return NULL;
  }

  if (not server->error_messages)
    return memcached_strerror(server->root, MEMCACHED_SUCCESS);

  if (not server->error_messages->size)
    return memcached_strerror(server->root, server->error_messages->rc);

  return server->error_messages->message;
}


memcached_error_t *memcached_error_copy(const memcached_server_st& server)
{
  if (server.error_messages == NULL)
  {
    return NULL;
  }

  memcached_error_t *error= (memcached_error_t *)libmemcached_malloc(server.root, sizeof(memcached_error_t));
  memcpy(error, server.error_messages, sizeof(memcached_error_t));
  error->next= NULL;

  return error;
}

memcached_return_t memcached_server_error_return(memcached_server_instance_st ptr)
{
  if (ptr == NULL)
  {
    return MEMCACHED_INVALID_ARGUMENTS;
  }

  if (ptr and ptr->error_messages)
  {
    return ptr->error_messages->rc;
  }

  return MEMCACHED_SUCCESS;
}
