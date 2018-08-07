set(Z80CPP_PATHS
    ${Z80CPP_ROOT}
    $ENV{Z80CPP_ROOT}
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /sw
    /opt/local
    /opt/csw
    /opt
)

find_path(Z80CPP_INCLUDE_DIR z80cpp/z80operations.h
	PATH_SUFFIXES include
	PATHS ${Z80CPP_PATHS}
)
find_library(Z80CPP_LIBRARY NAMES z80cpp libz80cpp
	PATH_SUFFIXES lib
	PATHS ${Z80CPP_PATHS}
)

if (WIN32)
	find_file(Z80CPP_DLL NAMES libz80cpp.dll
		PATH_SUFFIXES lib
		PATHS ${Z80CPP_PATHS} /usr/i686-w64-mingw32
	)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Z80CPP  DEFAULT_MSG
                                  Z80CPP_LIBRARY Z80CPP_INCLUDE_DIR)

mark_as_advanced(Z80CPP_LIBRARY Z80CPP_INCLUDE_DIR)
