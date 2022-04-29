cmake_minimum_required(VERSION 3.22)
project(WorkspaceForArchiso C)
link_directories(lib)

set(CMAKE_C_STANDARD 99)

add_executable(WorkspaceForArchiso main.c)
target_link_libraries(WorkspaceForArchiso FoxString)
target_link_libraries(WorkspaceForArchiso toml)

set(EXECUTABLE_OUTPUT_PATH "../")