cmake_minimum_required(VERSION 3.20)
project(axm_render_fabric VERSION 0.1.0 LANGUAGES CXX)

add_executable(axm-render src/main.cpp)
target_compile_features(axm-render PRIVATE cxx_std_20)

add_executable(state_residency_bench research/state-native-rendering/state_residency.cpp)
target_compile_features(state_residency_bench PRIVATE cxx_std_20)

foreach(target axm-render state_residency_bench)
  if (MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive-)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endforeach()

enable_testing()
add_test(NAME deterministic-reference-render COMMAND axm-render --self-test)
add_test(NAME state-native-residency-equivalence COMMAND state_residency_bench --self-test)
