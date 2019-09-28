fips_begin_sharedlib(example)
fips_files(main.c)
fips_include_directories(${CMAKE_CURRENT_BINARY_DIR})
fips_end_sharedlib()

set_target_properties(example PROPERTIES PREFIX "")

if (FIPS_LINUX)
	install(TARGETS example DESTINATION share/zany80/plugins)
elseif(FIPS_WINDOWS)
	install(TARGETS example DESTINATION plugins)
endif()
