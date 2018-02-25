if(NOT DEFINED ZANY80_BUILD_UNUSED)
	set(ZANY80_BUILD_UNUSED FALSE CACHE BOOL "")
endif()

if(ZANY80_BUILD_UNUSED)
	add_subdirectory(z80)
endif()

add_subdirectory(z80cpp_core)
