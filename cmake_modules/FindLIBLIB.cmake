set(LIBLIB_PATHS
    ${LIBLIB_ROOT}
    $ENV{LIBLIB_ROOT}
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /sw
    /opt/local
    /opt/csw
    /opt
)

find_path(LIBLIB_INCLUDE_DIR liblib/liblib.hpp
	PATH_SUFFIXES include
	PATHS ${LIBLIB_PATHS}
)
find_library(LIBLIB_LIBRARY NAMES lib liblib
	PATH_SUFFIXES liblib
	PATHS ${LIBLIB_PATHS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBLIB  DEFAULT_MSG
                                  LIBLIB_LIBRARY LIBLIB_INCLUDE_DIR)

if (WIN32)
	find_package(LIBDL REQUIRED)
	if(NOT LIBDL_FOUND)
		message(FATAL_ERROR "libdl not found!")
	endif()
	set(LIBLIB_LIBRARY ${LIBLIB_LIBRARY} ${LIBDL_LIBRARY})
endif()

mark_as_advanced(LIBLIB_LIBRARY LIBLIB_INCLUDE_DIR)
