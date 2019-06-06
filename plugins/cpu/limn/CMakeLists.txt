set(VERSION 1.0.0.0)

fips_begin_sharedlib(limn)
fips_deps(zany80)
fips_files(main.c limn.c rom.c)
fips_end_sharedlib()

set_target_properties(limn PROPERTIES PREFIX "")

if (FIPS_LINUX)
	install(TARGETS limn DESTINATION share/zany80/plugins)
elseif(FIPS_WINDOWS)
	install(TARGETS limn DESTINATION plugins)
endif()
