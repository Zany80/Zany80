if(NOT DEFINED ZANY80_BUILD_UNUSED)
	set(ZANY80_BUILD_UNUSED FALSE CACHE BOOL "")
endif()

if(ZANY80_BUILD_UNUSED)
	fips_add_subdirectory(z80)
endif()

fips_add_subdirectory(limn)
