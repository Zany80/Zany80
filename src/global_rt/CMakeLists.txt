configure_file("${CMAKE_CURRENT_SOURCE_DIR}/global_initialization.lua" "${CMAKE_BINARY_DIR}/global_rt/global_initialization.lua" COPYONLY)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/global_initialization.lua" DESTINATION global_rt)
