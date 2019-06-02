set(VERSION 2.0.0.0)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/editor_config.h)

fips_begin_sharedlib(editor)
fips_src(.)
fips_include_directories(${CMAKE_CURRENT_BINARY_DIR})
fips_deps(zany80)
fips_end_sharedlib()

set_target_properties(editor PROPERTIES PREFIX "")

if (FIPS_LINUX)
	install(TARGETS editor DESTINATION share/zany80/plugins)
elseif(FIPS_WINDOWS)
	install(TARGETS editor DESTINATION plugins)
endif()
 
