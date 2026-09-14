cmake_minimum_required(VERSION 3.20)
project(axm_render_fabric VERSION 0.1.0 LANGUAGES CXX)

add_executable(axm-render src/main.cpp)
target_compile_features(axm-render PRIVATE cxx_std_20)

if (MSVC)
  target_compile_options(axm-render PRIVATE /W4 /permissive-)
else()
  target_compile_options(axm-render PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(NAME deterministic-reference-render COMMAND axm-render --self-test)
