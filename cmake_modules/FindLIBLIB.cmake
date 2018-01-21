find_path(LIBLIB_INCLUDE_DIR liblib/liblib.hpp)
find_library(LIBLIB_LIBRARY NAMES lib liblib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBLIB  DEFAULT_MSG
                                  LIBLIB_LIBRARY LIBLIB_INCLUDE_DIR)

mark_as_advanced(LIBLIB_LIBRARY LIBLIB_INCLUDE_DIR)
