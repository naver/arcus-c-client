/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  libtest
 *
 *  Copyright 2010-2014 NAVER Corp.
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <libtest/common.h>

static char global_socket[1024] = { 0 };

namespace libtest {

const char *default_socket()
{
  if (global_socket[0] == 0)
  {
    return NULL;
  }
  return global_socket;
}

void set_default_socket(const char *socket)
{
  if (socket && strlen(socket) < sizeof(global_socket))
  {
    strncpy(global_socket, socket, sizeof(global_socket));
  }
}

}
