cmake_minimum_required(VERSION 3.16)
project(mysql_connector)

set(CMAKE_CXX_STANDARD 20)

include_directories(C:/dev/vcpkg/installed/x64-windows/include)
link_directories(C:/dev/vcpkg/installed/x64-windows/lib)

# Déclarez votre exécutable
add_executable(mysql_connector main.cpp)

target_link_libraries(mysql_connector PRIVATE sqlite3)