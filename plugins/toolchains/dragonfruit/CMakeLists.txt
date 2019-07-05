set(VERSION 1.0.0.0)

fips_begin_sharedlib(dragonfruit)
fips_deps(zany80)
fips_files(interface.c compiler.c lexer.c limn.c)
fips_end_sharedlib()

set_target_properties(dragonfruit PROPERTIES PREFIX "")

if (FIPS_LINUX)
	install(TARGETS dragonfruit DESTINATION share/zany80/plugins)
elseif(FIPS_WINDOWS)
	install(TARGETS dragonfruit DESTINATION plugins)
endif()
