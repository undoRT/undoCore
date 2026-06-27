cmake_minimum_required(VERSION 3.20)

project(test
    VERSION 0.1.0
    DESCRIPTION "test"
    LANGUAGES CXX
)

add_executable(test_processImage test_processImage.cpp)

set(UNDOCORE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../../include)

# target_link_libraries(test_processImage PRIVATE undoCore)

target_compile_features(test_processImage PRIVATE cxx_std_20)

target_include_directories(test_processImage PRIVATE
    ${UNDOCORE_INCLUDE_DIR}
)
