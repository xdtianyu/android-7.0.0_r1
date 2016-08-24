#ifdef ANDROID
  #include "memcheck/memcheck.h"
#else
  // On Chrome OS, these files will be added in a patch applied in the ebuild.
  #include <base/third_party/valgrind/memcheck.h>
#endif
