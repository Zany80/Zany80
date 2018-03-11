add_library(kcc MODULE ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
set_target_properties(kcc PROPERTIES PREFIX "../")
target_include_directories(kcc PRIVATE ../../include ${CMAKE_SOURCE_DIR}/include)

if (WIN32)
	install(TARGETS kcc DESTINATION plugins/compiler)
else()
	install(TARGETS kcc DESTINATION share/zany80/plugins/compiler)
endif()
