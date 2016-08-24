
#ifndef REMOTE_PROCESSOR_EXPORT_H
#define REMOTE_PROCESSOR_EXPORT_H

#ifdef REMOTE_PROCESSOR_STATIC_DEFINE
#  define REMOTE_PROCESSOR_EXPORT
#  define REMOTE_PROCESSOR_NO_EXPORT
#else
#  ifndef REMOTE_PROCESSOR_EXPORT
#    ifdef remote_processor_EXPORTS
        /* We are building this library */
#      define REMOTE_PROCESSOR_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define REMOTE_PROCESSOR_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef REMOTE_PROCESSOR_NO_EXPORT
#    define REMOTE_PROCESSOR_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef REMOTE_PROCESSOR_DEPRECATED
#  define REMOTE_PROCESSOR_DEPRECATED __attribute__ ((__deprecated__))
#  define REMOTE_PROCESSOR_DEPRECATED_EXPORT REMOTE_PROCESSOR_EXPORT __attribute__ ((__deprecated__))
#  define REMOTE_PROCESSOR_DEPRECATED_NO_EXPORT REMOTE_PROCESSOR_NO_EXPORT __attribute__ ((__deprecated__))
#endif

#define DEFINE_NO_DEPRECATED 0
#if DEFINE_NO_DEPRECATED
# define REMOTE_PROCESSOR_NO_DEPRECATED
#endif

#endif
