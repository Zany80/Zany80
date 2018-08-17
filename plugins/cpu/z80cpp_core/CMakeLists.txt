set(z80cpp_core_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/z80cpp_core.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/core.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/BIOSPlugin.cpp
)

find_package(Z80CPP REQUIRED)

add_library(z80cpp_core MODULE ${z80cpp_core_SOURCES})
target_link_libraries(z80cpp_core ${Z80CPP_LIBRARY})
set_target_properties(z80cpp_core PROPERTIES PREFIX "../")
target_include_directories(z80cpp_core PRIVATE ../../include ${CMAKE_SOURCE_DIR}/include ${Z80CPP_INCLUDE_DIR})
if (WIN32)
	install(TARGETS z80cpp_core DESTINATION plugins/cpu)
	install(FILES ${Z80CPP_DLL} DESTINATION plugins/cpu)
else()
	install(TARGETS z80cpp_core DESTINATION share/zany80/plugins/cpu)
endif()
