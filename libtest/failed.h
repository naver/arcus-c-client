/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  libtest
 *
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

#ifndef __LIBTEST_FAILED_H__
#define __LIBTEST_FAILED_H__

#ifdef __cplusplus
extern "C" {
#endif

LIBTEST_INTERNAL_API
  void push_failed_test(const char *collection, const char *test);

LIBTEST_INTERNAL_API
  void print_failed_test(void);

#ifdef __cplusplus
}
#endif

#endif /* __LIBTEST_FAILED_H__ */
