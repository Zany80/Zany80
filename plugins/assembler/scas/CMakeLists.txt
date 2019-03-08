set(VERSION 1.1.0.0)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/scas_version.h)

fips_begin_sharedlib(scas_plugin)
fips_deps(scas)
fips_src(.)
fips_include_directories(${CMAKE_CURRENT_BINARY_DIR})
fips_end_sharedlib()

set_target_properties(scas_plugin PROPERTIES PREFIX "")
set_target_properties(scas_plugin PROPERTIES OUTPUT_NAME "assembler")

if (FIPS_LINUX)
	install(TARGETS scas_plugin DESTINATION share/zany80/plugins)
elseif(FIPS_WINDOWS)
	install(TARGETS scas_plugin DESTINATION plugins)
endif()
