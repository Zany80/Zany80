add_subdirectory(tools)

set(SOURCES
	${SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/Zany80.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Drawing.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/GenericException.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/icon.cpp
	PARENT_SCOPE
)
