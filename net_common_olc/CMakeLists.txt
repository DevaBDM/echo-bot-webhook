cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(net_common_olc)

# create compile_commands.json file for clangd to use
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_FLAGS_IDE_CONFIGURATION
    "-std=c++17 -pedantic-errors -Wshadow -Wall -Weffc++ -Wextra -Wconversion -Wsign-conversion"
)
# set it too false to build in release build configuration you should run cmake
# after changing this
set(DEBUG true)

if(DEBUG)
  set(CMAKE_BUILD_TYPE debug)
  set(CMAKE_CXX_FLAGS "-O0 -ggdb ${CMAKE_CXX_FLAGS_IDE_CONFIGURATION}")
  set(EXECUTABLE_OUTPUT_PATH build/debug)
else()
  set(CMAKE_BUILD_TYPE release)
  set(CMAKE_CXX_FLAGS "-O2 -DNDEBUG ${CMAKE_CXX_FLAGS_IDE_CONFIGURATION}")
  set(EXECUTABLE_OUTPUT_PATH build/release)
endif()

file(GLOB_RECURSE SOURCES "./src/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE include)
add_custom_command(
  TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND
    echo "\#\!\/bin\/bash\\n.\/${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME} \"\$@\"" > Run
    # \\necho \"\\n${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME} returned \$?\""
  VERBATIM)
