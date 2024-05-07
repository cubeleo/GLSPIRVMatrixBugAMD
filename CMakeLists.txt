cmake_minimum_required(VERSION 3.26)
project(SPIRVMatrixBug C CXX)
set(CMAKE_CXX_STANDARD 20)

include_directories(
    Import/glfw/include
)
function(AddGLFW)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(Import/glfw)
endfunction(AddGLFW)

AddGLFW()

find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

set(GLSL_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Shader.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/Shader.frag
)

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${GLSL_VALIDATOR} -G ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    SPIRVShaders 
    DEPENDS ${SPIRV_BINARY_FILES}
)

add_executable(SPIRVMatrixBug SPIRVMatrixBug.cpp)
target_link_libraries(SPIRVMatrixBug glfw)
add_dependencies(SPIRVMatrixBug SPIRVShaders)
