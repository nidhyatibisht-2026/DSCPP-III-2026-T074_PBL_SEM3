cmake_minimum_required(VERSION 3.16)

project(FoodFlow LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(FOODFLOW_CORE_SOURCES
    src/Database.cpp
    src/Models.cpp
    src/HashTable.cpp
    src/FoodInventoryList.cpp
    src/MinHeapExpiryQueue.cpp
    src/FIFOQueue.cpp
    src/User.cpp
    src/Utilities.cpp
    src/FoodRescueSystem.cpp
)

add_library(foodflow_core STATIC ${FOODFLOW_CORE_SOURCES})
target_include_directories(foodflow_core PUBLIC include)
target_link_libraries(foodflow_core PUBLIC sqlite3)
set_target_properties(foodflow_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_executable(foodflow src/main.cpp)
target_include_directories(foodflow PRIVATE include)
target_link_libraries(foodflow PRIVATE foodflow_core)

add_library(foodflow_bridge SHARED integration/cpp_bridge.cpp)
target_include_directories(foodflow_bridge PRIVATE include)
target_link_libraries(foodflow_bridge PRIVATE foodflow_core)
set_target_properties(foodflow_bridge PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

if(MSVC)
    target_compile_options(foodflow_core PRIVATE /W4)
    target_compile_options(foodflow PRIVATE /W4)
    target_compile_options(foodflow_bridge PRIVATE /W4)
else()
    target_compile_options(foodflow_core PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(foodflow PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(foodflow_bridge PRIVATE -Wall -Wextra -pedantic)
endif()
