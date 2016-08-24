#ifdef ANDROID
  #include "include/valgrind.h"
#else
  // These files will be added in a patch applied in the ebuild.
  #include <base/third_party/valgrind/valgrind.h>
#endif
