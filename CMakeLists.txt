cmake_minimum_required(VERSION 3.29)
project(Project)

set(CMAKE_CXX_STANDARD 20)

# Find and link sqlite3 package
find_package(unofficial-sqlite3 CONFIG REQUIRED)

# Add your executable and link sqlite3
add_executable(Project main.cpp)
target_link_libraries(Project PRIVATE unofficial::sqlite3::sqlite3)
