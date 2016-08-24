#include <sys/cdefs.h>  /* Defines __BIONIC__ */

#if defined(__BIONIC__)
#  include <event2/event-config-bionic.h>
#else
#  if defined(__linux__)
#    include <event2/event-config-linux.h>
#  elif defined(__APPLE__)
#    include <event2/event-config-darwin.h>
#  else
#    error No event-config.h suitable for this distribution.
#  endif
#endif  /* ifdef __BIONIC__ */
