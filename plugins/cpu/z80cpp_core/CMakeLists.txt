fips_begin_sharedlib(z80cpp_core)
fips_src(src/)
fips_deps(z80cpp)
fips_end_sharedlib()

set_target_properties(z80cpp_core PROPERTIES PREFIX "")

if (FIPS_LINUX)
	install(TARGETS z80cpp_core DESTINATION share/zany80/plugins)
elseif(FIPS_WINDOWS)
	install(TARGETS z80cpp_core DESTINATION plugins)
endif()
