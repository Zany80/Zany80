fips_begin_sharedlib(display)
fips_files(main.c ring_buffer.c)
fips_deps(zany80)
fips_include_directories(${CMAKE_CURRENT_BINARY_DIR})
fips_end_sharedlib()

set_target_properties(display PROPERTIES PREFIX "")

if (FIPS_LINUX)
	install(TARGETS display DESTINATION share/zany80/plugins)
elseif(FIPS_WINDOWS)
	install(TARGETS display DESTINATION plugins)
endif()
