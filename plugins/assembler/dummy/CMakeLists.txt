set(VERSION 1.0.0.0)

fips_begin_sharedlib(dummy_assembler)
fips_deps(zany80)
fips_files(dummy.c)
fips_end_sharedlib()

set_target_properties(dummy_assembler PROPERTIES PREFIX "")

if (FIPS_LINUX)
	install(TARGETS dummy_assembler DESTINATION share/zany80/plugins)
elseif(FIPS_WINDOWS)
	install(TARGETS dummy_assembler DESTINATION plugins)
endif()
