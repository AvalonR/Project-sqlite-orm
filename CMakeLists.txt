cmake_minimum_required(VERSION 3.29)
project(Project-sqlite-orm)

set(CMAKE_CXX_STANDARD 20)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/sqlite3 ${CMAKE_CURRENT_SOURCE_DIR}/sqlite_orm/include)

add_subdirectory(sqlite3)
# Add the executable
add_executable(Project-sqlite-orm main.cpp)
# Link the sqlite3 library to the executable
target_link_libraries(Project-sqlite-orm PRIVATE sqlite3)