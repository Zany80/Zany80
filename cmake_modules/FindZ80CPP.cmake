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

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Z80CPP  DEFAULT_MSG
                                  Z80CPP_LIBRARY Z80CPP_INCLUDE_DIR)

mark_as_advanced(Z80CPP_LIBRARY Z80CPP_INCLUDE_DIR)
