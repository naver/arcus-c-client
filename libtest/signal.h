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



#ifndef __LIBTEST_SIGNAL_H__
#define __LIBTEST_SIGNAL_H__

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

enum shutdown_t {
  SHUTDOWN_RUNNING,
  SHUTDOWN_GRACEFUL,
  SHUTDOWN_FORCED
};

namespace libtest {

class SignalThread {
  sigset_t set;
  sem_t lock;
  uint64_t magic_memory;
  volatile shutdown_t __shutdown;
  pthread_mutex_t shutdown_mutex;
  pthread_t thread;

public:

  SignalThread();

  void test();
  void post();
  bool setup();

  int wait(int& sig)
  {
    return sigwait(&set, &sig);
  }

  ~SignalThread();

  void set_shutdown(shutdown_t arg);
  bool is_shutdown();
  shutdown_t get_shutdown();
};

} // namespace libtest

#endif /* __LIBTEST_SIGNAL_H__ */
