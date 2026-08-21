cmake_minimum_required(VERSION 3.14)
project(royal-ac-wifi-controller)

# GoogleTest requires at least C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
  ac_tests
  test/ac_state_test.cc
  src/utils.cpp
  test/utils_test.cc
  src/ac_state.cpp
)

target_include_directories(
    ac_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src

)

target_link_libraries(
  ac_tests
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(ac_tests)
