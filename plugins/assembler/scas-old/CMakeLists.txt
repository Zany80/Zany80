add_library(scas MODULE ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
set_target_properties(scas PROPERTIES PREFIX "../")
target_include_directories(scas PRIVATE ../../include ${CMAKE_SOURCE_DIR}/include)

if (WIN32)
	install(TARGETS scas DESTINATION plugins/assembler)
else()
	install(TARGETS scas DESTINATION share/zany80/plugins/assembler)
endif()
