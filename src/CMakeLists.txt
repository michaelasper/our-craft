SET(pwd ${CMAKE_CURRENT_LIST_DIR})

SET(src "")
AUX_SOURCE_DIRECTORY(${pwd} src)
add_executable(minecraft ${src})
message(STATUS "minecraft added")

target_link_libraries(minecraft ${stdgl_libraries})
