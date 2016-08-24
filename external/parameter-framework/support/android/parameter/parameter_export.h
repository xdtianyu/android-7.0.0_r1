
#ifndef PARAMETER_EXPORT_H
#define PARAMETER_EXPORT_H

#ifdef PARAMETER_STATIC_DEFINE
#  define PARAMETER_EXPORT
#  define PARAMETER_NO_EXPORT
#else
#  ifndef PARAMETER_EXPORT
#    ifdef parameter_EXPORTS
        /* We are building this library */
#      define PARAMETER_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define PARAMETER_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef PARAMETER_NO_EXPORT
#    define PARAMETER_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef PARAMETER_DEPRECATED
#  define PARAMETER_DEPRECATED __attribute__ ((__deprecated__))
#  define PARAMETER_DEPRECATED_EXPORT PARAMETER_EXPORT __attribute__ ((__deprecated__))
#  define PARAMETER_DEPRECATED_NO_EXPORT PARAMETER_NO_EXPORT __attribute__ ((__deprecated__))
#endif

#define DEFINE_NO_DEPRECATED 0
#if DEFINE_NO_DEPRECATED
# define PARAMETER_NO_DEPRECATED
#endif

#endif
