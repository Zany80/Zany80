fips_begin_sharedlib(debug_port)
fips_files(main.c)
fips_include_directories(${CMAKE_CURRENT_BINARY_DIR})
fips_end_sharedlib()

set_target_properties(debug_port PROPERTIES PREFIX "")

if (FIPS_LINUX)
	install(TARGETS debug_port DESTINATION share/zany80/plugins)
elseif(FIPS_WINDOWS)
	install(TARGETS debug_port DESTINATION plugins)
endif()
