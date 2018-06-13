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

install(
	FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/global_initialization.lua"
		"${CMAKE_CURRENT_SOURCE_DIR}/main.lua"
	DESTINATION global_rt
)
