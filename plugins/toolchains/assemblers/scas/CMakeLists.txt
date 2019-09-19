set(VERSION 2.0.0.0)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/scas_version.h)

fips_begin_sharedlib(scas)
fips_src(.)
fips_include_directories(${CMAKE_CURRENT_BINARY_DIR})
fips_end_sharedlib()

set_target_properties(scas PROPERTIES PREFIX "")
set_target_properties(scas PROPERTIES OUTPUT_NAME "assembler")

if (FIPS_LINUX)
	install(TARGETS scas DESTINATION share/zany80/plugins)
elseif(FIPS_WINDOWS)
	install(TARGETS scas DESTINATION plugins)
endif()
