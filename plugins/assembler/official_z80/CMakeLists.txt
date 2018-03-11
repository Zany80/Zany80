set(official_z80_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/assembler.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/MachineObject.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/instructions.cpp
)

add_library(official_z80 MODULE ${official_z80_SOURCES})
set_target_properties(official_z80 PROPERTIES PREFIX "../")
target_include_directories(official_z80 PRIVATE ../../include ${CMAKE_SOURCE_DIR}/include ./include)

if (WIN32)
	install(TARGETS official_z80 DESTINATION plugins/assembler)
else()
	install(TARGETS official_z80 DESTINATION share/zany80/plugins/assembler)
endif()
