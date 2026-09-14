cmake_minimum_required(VERSION 3.20)
project(axm_render_fabric VERSION 0.1.0 LANGUAGES CXX)

add_library(axm_render_native STATIC
  src/reference_renderer.cpp
  src/scene_contract.cpp
)
add_library(axm::render_native ALIAS axm_render_native)
target_include_directories(axm_render_native PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(axm_render_native PUBLIC cxx_std_20)

add_executable(axm-render src/main.cpp)
target_link_libraries(axm-render PRIVATE axm_render_native)
target_compile_features(axm-render PRIVATE cxx_std_20)

add_executable(native_library_smoke tests/native_library_smoke.cpp)
target_link_libraries(native_library_smoke PRIVATE axm_render_native)
target_compile_features(native_library_smoke PRIVATE cxx_std_20)

add_executable(scene_file_smoke tests/scene_file_smoke.cpp)
target_link_libraries(scene_file_smoke PRIVATE axm_render_native)
target_compile_features(scene_file_smoke PRIVATE cxx_std_20)

add_executable(state_residency_bench research/state-native-rendering/state_residency.cpp)
target_compile_features(state_residency_bench PRIVATE cxx_std_20)

foreach(target axm_render_native axm-render native_library_smoke scene_file_smoke state_residency_bench)
  if (MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive-)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endforeach()

enable_testing()
add_test(NAME deterministic-reference-render COMMAND axm-render --self-test)
add_test(NAME native-render-library-smoke COMMAND native_library_smoke)
add_test(
  NAME scene-file-reference-render
  COMMAND scene_file_smoke ${CMAKE_CURRENT_SOURCE_DIR}/examples/reference.axmscene
)
add_test(
  NAME scene-file-rejects-unsupported-version
  COMMAND axm-render --scene ${CMAKE_CURRENT_SOURCE_DIR}/examples/unsupported-v2.axmscene --self-test
)
set_tests_properties(scene-file-rejects-unsupported-version PROPERTIES WILL_FAIL TRUE)
add_test(NAME state-native-residency-equivalence COMMAND state_residency_bench --self-test)
