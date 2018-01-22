set(z80_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/z80.cpp
)

add_library(z80 SHARED ${z80_SOURCES})
set_target_properties(z80 PROPERTIES PREFIX "../")
target_include_directories(z80 PRIVATE ../include ${CMAKE_SOURCE_DIR}/include)
if (WIN32)
	install(TARGETS z80 DESTINATION plugins/cpu)
else()
	install(TARGETS z80 DESTINATION share/zany80/plugins/cpu)
endif()
