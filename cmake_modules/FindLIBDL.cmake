#
#  LIBDL_FOUND - system has libdl
#  LIBDL_INCLUDE_DIR - the libdl include directory
#  LIBDL_LIBRARY - Link these to use libdl
#  LIBDL_NEEDS_UNDERSCORE - If extern "C" symbols are prefixed (BSD/Apple)
#

find_path (LIBDL_INCLUDE_DIR NAMES dlfcn.h)
find_library (LIBDL_LIBRARY NAMES dl)
include (FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(libDL DEFAULT_MSG
  LIBDL_LIBRARY
LIBDL_INCLUDE_DIR)

include(CheckCSourceRuns)
CHECK_C_SOURCE_RUNS("#include <dlfcn.h>
#include <stdlib.h>
void testfunc() {}
int main() {
  testfunc();
  if (dlsym(0, \"_testfunc\") != 0) {
    return 0;
  }
    return 1;
}" LIBDL_NEEDS_UNDERSCORE)

mark_as_advanced(LIBDL_INCLUDE_DIRS LIBDL_LIBRARIES LIBDL_NEEDS_UNDERSCORE)
