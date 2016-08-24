/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/* dbus-threads.h  D-Bus threads handling
 *
 * Copyright (C) 2002, 2003, 2006 Red Hat Inc.
 *
 * Licensed under the Academic Free License version 2.1
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#include <config.h>
#include "dbus-threads.h"
#include "dbus-internals.h"
#include "dbus-threads-internal.h"
#include "dbus-list.h"

static int thread_init_generation = 0;
 
static DBusList *uninitialized_rmutex_list = NULL;
static DBusList *uninitialized_cmutex_list = NULL;
static DBusList *uninitialized_condvar_list = NULL;

/** This is used for the no-op default mutex pointer, just to be distinct from #NULL */
#define _DBUS_DUMMY_MUTEX ((DBusMutex*)0xABCDEF)
#define _DBUS_DUMMY_RMUTEX ((DBusRMutex *) _DBUS_DUMMY_MUTEX)
#define _DBUS_DUMMY_CMUTEX ((DBusCMutex *) _DBUS_DUMMY_MUTEX)

/** This is used for the no-op default mutex pointer, just to be distinct from #NULL */
#define _DBUS_DUMMY_CONDVAR ((DBusCondVar*)0xABCDEF2)

/**
 * @defgroup DBusThreadsInternals Thread functions
 * @ingroup  DBusInternals
 * @brief _dbus_rmutex_lock(), etc.
 *
 * Functions and macros related to threads and thread locks.
 *
 * @{
 */

/**
 * Creates a new mutex
 * or creates a no-op mutex if threads are not initialized.
 * May return #NULL even if threads are initialized, indicating
 * out-of-memory.
 *
 * If possible, the mutex returned by this function is recursive, to
 * avoid deadlocks. However, that cannot be relied on.
 *
 * The extra level of indirection given by allocating a pointer
 * to point to the mutex location allows the threading
 * module to swap out dummy mutexes for a real mutex so libraries
 * can initialize threads even after the D-Bus API has been used.
 *
 * @param location_p the location of the new mutex, can return #NULL on OOM
 */
void
_dbus_rmutex_new_at_location (DBusRMutex **location_p)
{
  _dbus_assert (location_p != NULL);

  if (thread_init_generation == _dbus_current_generation)
    {
      *location_p = _dbus_platform_rmutex_new ();
    }
  else
    {
      *location_p = _DBUS_DUMMY_RMUTEX;

      if (!_dbus_list_append (&uninitialized_rmutex_list, location_p))
        *location_p = NULL;
    }
}

/**
 * Creates a new mutex
 * or creates a no-op mutex if threads are not initialized.
 * May return #NULL even if threads are initialized, indicating
 * out-of-memory.
 *
 * The returned mutex is suitable for use with condition variables.
 *
 * The extra level of indirection given by allocating a pointer
 * to point to the mutex location allows the threading
 * module to swap out dummy mutexes for a real mutex so libraries
 * can initialize threads even after the D-Bus API has been used.
 *
 * @param location_p the location of the new mutex, can return #NULL on OOM
 */
void
_dbus_cmutex_new_at_location (DBusCMutex **location_p)
{
  _dbus_assert (location_p != NULL);

  if (thread_init_generation == _dbus_current_generation)
    {
      *location_p = _dbus_platform_cmutex_new ();
    }
  else
    {
      *location_p = _DBUS_DUMMY_CMUTEX;

      if (!_dbus_list_append (&uninitialized_cmutex_list, location_p))
        *location_p = NULL;
    }
}

/**
 * Frees a DBusRMutex or removes it from the uninitialized mutex list;
 * does nothing if passed a #NULL pointer.
 */
void
_dbus_rmutex_free_at_location (DBusRMutex **location_p)
{
  if (location_p == NULL)
    return;

  if (thread_init_generation == _dbus_current_generation)
    {
      if (*location_p != NULL)
        _dbus_platform_rmutex_free (*location_p);
    }
  else
    {
      _dbus_assert (*location_p == NULL || *location_p == _DBUS_DUMMY_RMUTEX);

      _dbus_list_remove (&uninitialized_rmutex_list, location_p);
    }
}

/**
 * Frees a DBusCMutex and removes it from the
 * uninitialized mutex list;
 * does nothing if passed a #NULL pointer.
 */
void
_dbus_cmutex_free_at_location (DBusCMutex **location_p)
{
  if (location_p == NULL)
    return;

  if (thread_init_generation == _dbus_current_generation)
    {
      if (*location_p != NULL)
        _dbus_platform_cmutex_free (*location_p);
    }
  else
    {
      _dbus_assert (*location_p == NULL || *location_p == _DBUS_DUMMY_CMUTEX);

      _dbus_list_remove (&uninitialized_cmutex_list, location_p);
    }
}

/**
 * Locks a mutex. Does nothing if passed a #NULL pointer.
 * Locks may be recursive if threading implementation initialized
 * recursive locks.
 */
void
_dbus_rmutex_lock (DBusRMutex *mutex)
{
  if (mutex && thread_init_generation == _dbus_current_generation)
    _dbus_platform_rmutex_lock (mutex);
}

/**
 * Locks a mutex. Does nothing if passed a #NULL pointer.
 * Locks may be recursive if threading implementation initialized
 * recursive locks.
 */
void
_dbus_cmutex_lock (DBusCMutex *mutex)
{
  if (mutex && thread_init_generation == _dbus_current_generation)
    _dbus_platform_cmutex_lock (mutex);
}

/**
 * Unlocks a mutex. Does nothing if passed a #NULL pointer.
 *
 * @returns #TRUE on success
 */
void
_dbus_rmutex_unlock (DBusRMutex *mutex)
{
  if (mutex && thread_init_generation == _dbus_current_generation)
    _dbus_platform_rmutex_unlock (mutex);
}

/**
 * Unlocks a mutex. Does nothing if passed a #NULL pointer.
 *
 * @returns #TRUE on success
 */
void
_dbus_cmutex_unlock (DBusCMutex *mutex)
{
  if (mutex && thread_init_generation == _dbus_current_generation)
    _dbus_platform_cmutex_unlock (mutex);
}

/**
 * Creates a new condition variable using the function supplied
 * to dbus_threads_init(), or creates a no-op condition variable
 * if threads are not initialized. May return #NULL even if
 * threads are initialized, indicating out-of-memory.
 *
 * @returns new mutex or #NULL
 */
DBusCondVar *
_dbus_condvar_new (void)
{
  if (thread_init_generation == _dbus_current_generation)
    return _dbus_platform_condvar_new ();
  else
    return _DBUS_DUMMY_CONDVAR;
}


/**
 * This does the same thing as _dbus_condvar_new.  It however
 * gives another level of indirection by allocating a pointer
 * to point to the condvar location.  This allows the threading
 * module to swap out dummy condvars for a real condvar so libraries
 * can initialize threads even after the D-Bus API has been used.
 *
 * @returns the location of a new condvar or #NULL on OOM
 */

void 
_dbus_condvar_new_at_location (DBusCondVar **location_p)
{
  _dbus_assert (location_p != NULL);

  if (thread_init_generation == _dbus_current_generation)
    {
      *location_p = _dbus_condvar_new();
    }
  else
    {
      *location_p = _DBUS_DUMMY_CONDVAR;

      if (!_dbus_list_append (&uninitialized_condvar_list, location_p))
        *location_p = NULL;
    }
}


/**
 * Frees a conditional variable created with dbus_condvar_new(); does
 * nothing if passed a #NULL pointer.
 */
void
_dbus_condvar_free (DBusCondVar *cond)
{
  if (cond && thread_init_generation == _dbus_current_generation)
    _dbus_platform_condvar_free (cond);
}

/**
 * Frees a conditional variable and removes it from the 
 * uninitialized_condvar_list; 
 * does nothing if passed a #NULL pointer.
 */
void
_dbus_condvar_free_at_location (DBusCondVar **location_p)
{
  if (location_p == NULL)
    return;

  if (thread_init_generation == _dbus_current_generation)
    {
      if (*location_p != NULL)
        _dbus_platform_condvar_free (*location_p);
    }
  else
    {
      _dbus_assert (*location_p == NULL || *location_p == _DBUS_DUMMY_CONDVAR);

      _dbus_list_remove (&uninitialized_condvar_list, location_p);
    }
}

/**
 * Atomically unlocks the mutex and waits for the conditions
 * variable to be signalled. Locks the mutex again before
 * returning.
 * Does nothing if passed a #NULL pointer.
 */
void
_dbus_condvar_wait (DBusCondVar *cond,
                    DBusCMutex  *mutex)
{
  if (cond && mutex && thread_init_generation == _dbus_current_generation)
    _dbus_platform_condvar_wait (cond, mutex);
}

/**
 * Atomically unlocks the mutex and waits for the conditions variable
 * to be signalled, or for a timeout. Locks the mutex again before
 * returning.  Does nothing if passed a #NULL pointer.  Return value
 * is #FALSE if we timed out, #TRUE otherwise.
 *
 * @param cond the condition variable
 * @param mutex the mutex
 * @param timeout_milliseconds the maximum time to wait
 * @returns #FALSE if the timeout occurred, #TRUE if not
 */
dbus_bool_t
_dbus_condvar_wait_timeout (DBusCondVar               *cond,
                            DBusCMutex                *mutex,
                            int                        timeout_milliseconds)
{
  if (cond && mutex && thread_init_generation == _dbus_current_generation)
    return _dbus_platform_condvar_wait_timeout (cond, mutex,
                                                timeout_milliseconds);
  else
    return TRUE;
}

/**
 * If there are threads waiting on the condition variable, wake
 * up exactly one. 
 * Does nothing if passed a #NULL pointer.
 */
void
_dbus_condvar_wake_one (DBusCondVar *cond)
{
  if (cond && thread_init_generation == _dbus_current_generation)
    _dbus_platform_condvar_wake_one (cond);
}

static void
shutdown_global_locks (void *data)
{
  DBusRMutex ***locks = data;
  int i;

  i = 0;
  while (i < _DBUS_N_GLOBAL_LOCKS)
    {
      if (*(locks[i]) != NULL)
        _dbus_platform_rmutex_free (*(locks[i]));

      *(locks[i]) = NULL;
      ++i;
    }
  
  dbus_free (locks);
}

static void
shutdown_uninitialized_locks (void *data)
{
  _dbus_list_clear (&uninitialized_rmutex_list);
  _dbus_list_clear (&uninitialized_cmutex_list);
  _dbus_list_clear (&uninitialized_condvar_list);
}

static dbus_bool_t
init_uninitialized_locks (void)
{
  DBusList *link;

  _dbus_assert (thread_init_generation != _dbus_current_generation);

  link = uninitialized_rmutex_list;
  while (link != NULL)
    {
      DBusRMutex **mp;

      mp = link->data;
      _dbus_assert (*mp == _DBUS_DUMMY_RMUTEX);

      *mp = _dbus_platform_rmutex_new ();
      if (*mp == NULL)
        goto fail_mutex;

      link = _dbus_list_get_next_link (&uninitialized_rmutex_list, link);
    }

  link = uninitialized_cmutex_list;
  while (link != NULL)
    {
      DBusCMutex **mp;

      mp = link->data;
      _dbus_assert (*mp == _DBUS_DUMMY_CMUTEX);

      *mp = _dbus_platform_cmutex_new ();
      if (*mp == NULL)
        goto fail_mutex;

      link = _dbus_list_get_next_link (&uninitialized_cmutex_list, link);
    }

  link = uninitialized_condvar_list;
  while (link != NULL)
    {
      DBusCondVar **cp;

      cp = (DBusCondVar **)link->data;
      _dbus_assert (*cp == _DBUS_DUMMY_CONDVAR);

      *cp = _dbus_platform_condvar_new ();
      if (*cp == NULL)
        goto fail_condvar;

      link = _dbus_list_get_next_link (&uninitialized_condvar_list, link);
    }

  _dbus_list_clear (&uninitialized_rmutex_list);
  _dbus_list_clear (&uninitialized_cmutex_list);
  _dbus_list_clear (&uninitialized_condvar_list);

  if (!_dbus_register_shutdown_func (shutdown_uninitialized_locks,
                                     NULL))
    goto fail_condvar;

  return TRUE;

 fail_condvar:
  link = uninitialized_condvar_list;
  while (link != NULL)
    {
      DBusCondVar **cp;

      cp = link->data;

      if (*cp != _DBUS_DUMMY_CONDVAR && *cp != NULL)
        _dbus_platform_condvar_free (*cp);

      *cp = _DBUS_DUMMY_CONDVAR;

      link = _dbus_list_get_next_link (&uninitialized_condvar_list, link);
    }

 fail_mutex:
  link = uninitialized_rmutex_list;
  while (link != NULL)
    {
      DBusRMutex **mp;

      mp = link->data;

      if (*mp != _DBUS_DUMMY_RMUTEX && *mp != NULL)
        _dbus_platform_rmutex_free (*mp);

      *mp = _DBUS_DUMMY_RMUTEX;

      link = _dbus_list_get_next_link (&uninitialized_rmutex_list, link);
    }

  link = uninitialized_cmutex_list;
  while (link != NULL)
    {
      DBusCMutex **mp;

      mp = link->data;

      if (*mp != _DBUS_DUMMY_CMUTEX && *mp != NULL)
        _dbus_platform_cmutex_free (*mp);

      *mp = _DBUS_DUMMY_CMUTEX;

      link = _dbus_list_get_next_link (&uninitialized_cmutex_list, link);
    }

  return FALSE;
}

static dbus_bool_t
init_locks (void)
{
  int i;
  DBusRMutex ***dynamic_global_locks;
  DBusRMutex **global_locks[] = {
#define LOCK_ADDR(name) (& _dbus_lock_##name)
    LOCK_ADDR (win_fds),
    LOCK_ADDR (sid_atom_cache),
    LOCK_ADDR (list),
    LOCK_ADDR (connection_slots),
    LOCK_ADDR (pending_call_slots),
    LOCK_ADDR (server_slots),
    LOCK_ADDR (message_slots),
#if !DBUS_USE_SYNC
    LOCK_ADDR (atomic),
#endif
    LOCK_ADDR (bus),
    LOCK_ADDR (bus_datas),
    LOCK_ADDR (shutdown_funcs),
    LOCK_ADDR (system_users),
    LOCK_ADDR (message_cache),
    LOCK_ADDR (shared_connections),
    LOCK_ADDR (machine_uuid)
#undef LOCK_ADDR
  };

  _dbus_assert (_DBUS_N_ELEMENTS (global_locks) ==
                _DBUS_N_GLOBAL_LOCKS);

  i = 0;
  
  dynamic_global_locks = dbus_new (DBusRMutex**, _DBUS_N_GLOBAL_LOCKS);
  if (dynamic_global_locks == NULL)
    goto failed;
  
  while (i < _DBUS_N_ELEMENTS (global_locks))
    {
      *global_locks[i] = _dbus_platform_rmutex_new ();

      if (*global_locks[i] == NULL)
        goto failed;

      dynamic_global_locks[i] = global_locks[i];

      ++i;
    }
  
  if (!_dbus_register_shutdown_func (shutdown_global_locks,
                                     dynamic_global_locks))
    goto failed;

  if (!init_uninitialized_locks ())
    goto failed;
  
  return TRUE;

 failed:
  dbus_free (dynamic_global_locks);
                                     
  for (i = i - 1; i >= 0; i--)
    {
      _dbus_platform_rmutex_free (*global_locks[i]);
      *global_locks[i] = NULL;
    }
  return FALSE;
}

/** @} */ /* end of internals */

/**
 * @defgroup DBusThreads Thread functions
 * @ingroup  DBus
 * @brief dbus_threads_init() and dbus_threads_init_default()
 *
 * Functions and macros related to threads and thread locks.
 *
 * If threads are initialized, the D-Bus library has locks on all
 * global data structures.  In addition, each #DBusConnection has a
 * lock, so only one thread at a time can touch the connection.  (See
 * @ref DBusConnection for more on connection locking.)
 *
 * Most other objects, however, do not have locks - they can only be
 * used from a single thread at a time, unless you lock them yourself.
 * For example, a #DBusMessage can't be modified from two threads
 * at once.
 * 
 * @{
 */

/**
 * Initializes threads, like dbus_threads_init_default().
 * This version previously allowed user-specified threading
 * primitives, but since D-Bus 1.6 it ignores them and behaves
 * exactly like dbus_threads_init_default().
 *
 * @param functions ignored, formerly functions for using threads
 * @returns #TRUE on success, #FALSE if no memory
 */
dbus_bool_t
dbus_threads_init (const DBusThreadFunctions *functions)
{
  if (thread_init_generation == _dbus_current_generation)
    return TRUE;

  if (!init_locks ())
    return FALSE;

  thread_init_generation = _dbus_current_generation;
  
  return TRUE;
}



/* Default thread implemenation */

/**
 * Initializes threads. If this function is not called, the D-Bus
 * library will not lock any data structures.  If it is called, D-Bus
 * will do locking, at some cost in efficiency. Note that this
 * function must be called BEFORE the second thread is started.
 *
 * It's safe to call dbus_threads_init_default() as many times as you
 * want, but only the first time will have an effect.
 *
 * dbus_shutdown() reverses the effects of this function when it
 * resets all global state in libdbus.
 * 
 * @returns #TRUE on success, #FALSE if not enough memory
 */
dbus_bool_t
dbus_threads_init_default (void)
{
  return _dbus_threads_init_platform_specific ();
}


/** @} */

#ifdef DBUS_BUILD_TESTS

dbus_bool_t
_dbus_threads_init_debug (void)
{
  return _dbus_threads_init_platform_specific();
}

#endif /* DBUS_BUILD_TESTS */
