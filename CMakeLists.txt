cmake_minimum_required(VERSION 3.7)

project(plytest)

# set the use of C++17 globally as all examples require it
set(CMAKE_CXX_STANDARD 17)

# find_package(vsg REQUIRED)
# find_package(vsgXchange REQUIRED)

add_executable(plytest2 ply2.cpp)
add_executable(reflect reflect.cpp)

# target_link_libraries(plytest)
