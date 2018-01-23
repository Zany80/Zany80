set(rom_runner_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/rom_runner.cpp
)

add_library(rom_runner SHARED ${rom_runner_SOURCES})
set_target_properties(rom_runner PROPERTIES PREFIX "../")
target_include_directories(rom_runner PRIVATE ../include ${CMAKE_SOURCE_DIR}/include)
if (WIN32)
	install(TARGETS rom_runner DESTINATION plugins/cpu)
else()
	install(TARGETS rom_runner DESTINATION share/zany80/plugins/)
endif()
