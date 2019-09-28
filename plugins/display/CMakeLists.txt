fips_begin_sharedlib(serial_display)
fips_files(serial.c)
fips_include_directories(${CMAKE_CURRENT_BINARY_DIR})
fips_end_sharedlib()

set_target_properties(serial_display PROPERTIES PREFIX "")

if (FIPS_LINUX)
	install(TARGETS serial_display DESTINATION share/zany80/plugins)
elseif(FIPS_WINDOWS)
	install(TARGETS serial_display DESTINATION plugins)
endif()

fips_begin_sharedlib(graphical_display)
fips_files(graphical.c)
fips_include_directories(${CMAKE_CURRENT_BINARY_DIR})
fips_end_sharedlib()

set_target_properties(graphical_display PROPERTIES PREFIX "")

if (FIPS_LINUX)
	install(TARGETS graphical_display DESTINATION share/zany80/plugins)
elseif(FIPS_WINDOWS)
	install(TARGETS graphical_display DESTINATION plugins)
endif()
