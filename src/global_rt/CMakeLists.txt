configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/global_initialization.lua"
	"${CMAKE_BINARY_DIR}/global_rt/global_initialization.lua"
	COPYONLY
)

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/main.lua"
	"${CMAKE_BINARY_DIR}/global_rt/main.lua"
	COPYONLY
)

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/basic_functions.lua"
	"${CMAKE_BINARY_DIR}/global_rt/basic_functions.lua"
	COPYONLY
)

if(WIN32)
	install(
		FILES
			"${CMAKE_CURRENT_SOURCE_DIR}/global_initialization.lua"
			"${CMAKE_CURRENT_SOURCE_DIR}/main.lua"
			"${CMAKE_CURRENT_SOURCE_DIR}/basic_functions.lua"
		DESTINATION global_rt
	)
else()
	install(
		FILES
			"${CMAKE_CURRENT_SOURCE_DIR}/global_initialization.lua"
			"${CMAKE_CURRENT_SOURCE_DIR}/main.lua"
			"${CMAKE_CURRENT_SOURCE_DIR}/basic_functions.lua"
		DESTINATION share/zany80/global_rt
	)
endif()
