set(rom_runner_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/rom_runner.cpp
)

add_library(rom_runner MODULE ${rom_runner_SOURCES})
set_target_properties(rom_runner PROPERTIES PREFIX "../")
target_include_directories(rom_runner PRIVATE ../include ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(rom_runner ${LIBLIB_LIBRARY} ${SFML_LIBRARIES} ${SFML_DEPENDENCIES})
if (WIN32)
	install(TARGETS rom_runner DESTINATION plugins/)
else()
	install(TARGETS rom_runner DESTINATION share/zany80/plugins/)
endif()
