if(NOT DEFINED ZANY80_BUILD_UNUSED)
	set(ZANY80_BUILD_UNUSED FALSE CACHE BOOL "")
endif()

if(ZANY80_BUILD_UNUSED)
	add_subdirectory(experimental_dualcore)
endif()
