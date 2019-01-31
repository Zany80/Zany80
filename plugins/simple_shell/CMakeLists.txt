set(VERSION 1.0.0.0)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/shell_config.h)

fips_begin_sharedlib(simple_shell)
fips_files(simple_shell.cc interface.cc)
fips_include_directories(${CMAKE_CURRENT_BINARY_DIR})
fips_end_sharedlib()

set_target_properties(simple_shell PROPERTIES PREFIX "")

if (FIPS_LINUX)
	install(TARGETS simple_shell DESTINATION share/zany80/plugins)
elseif(FIPS_WINDOWS)
	install(TARGETS simple_shell DESTINATION plugins)
endif()
