if(NOT DEFINED ZANY80_BUILD_UNUSED)
	set(ZANY80_BUILD_UNUSED FALSE CACHE BOOL "")
endif()

fips_add_subdirectory(limn)
#fips_add_subdirectory(z80cpp_core)
