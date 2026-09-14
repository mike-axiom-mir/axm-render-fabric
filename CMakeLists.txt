cmake_minimum_required(VERSION 3.20)
project(axm_render_fabric VERSION 0.1.0 LANGUAGES CXX)

add_library(axm_render_contracts STATIC
  src/scene_contract.cpp
  src/render_contract.cpp
  src/render_receipt.cpp
)
add_library(axm::render_contracts ALIAS axm_render_contracts)
target_include_directories(axm_render_contracts PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(axm_render_contracts PUBLIC cxx_std_20)

add_library(axm_render_native STATIC
  src/reference_renderer.cpp
)
add_library(axm::render_native ALIAS axm_render_native)
target_include_directories(axm_render_native PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(axm_render_native PUBLIC cxx_std_20)
target_link_libraries(axm_render_native PUBLIC axm_render_contracts)

add_executable(axm-render src/main.cpp)
target_link_libraries(axm-render PRIVATE axm_render_native)
target_compile_features(axm-render PRIVATE cxx_std_20)

add_executable(native_library_smoke tests/native_library_smoke.cpp)
target_link_libraries(native_library_smoke PRIVATE axm_render_native)
target_compile_features(native_library_smoke PRIVATE cxx_std_20)

add_executable(scene_file_smoke tests/scene_file_smoke.cpp)
target_link_libraries(scene_file_smoke PRIVATE axm_render_native)
target_compile_features(scene_file_smoke PRIVATE cxx_std_20)

add_executable(render_request_smoke tests/render_request_smoke.cpp)
target_link_libraries(render_request_smoke PRIVATE axm_render_native)
target_compile_features(render_request_smoke PRIVATE cxx_std_20)

add_executable(render_receipt_smoke tests/render_receipt_smoke.cpp)
target_link_libraries(render_receipt_smoke PRIVATE axm_render_native)
target_compile_features(render_receipt_smoke PRIVATE cxx_std_20)

add_executable(contract_adapter_probe tests/contract_adapter_probe.cpp)
target_link_libraries(contract_adapter_probe PRIVATE axm_render_contracts)
target_compile_features(contract_adapter_probe PRIVATE cxx_std_20)

add_executable(contract_flat_renderer tests/contract_flat_renderer.cpp)
target_link_libraries(contract_flat_renderer PRIVATE axm_render_contracts)
target_compile_features(contract_flat_renderer PRIVATE cxx_std_20)

add_executable(state_residency_bench research/state-native-rendering/state_residency.cpp)
target_compile_features(state_residency_bench PRIVATE cxx_std_20)

foreach(target axm_render_contracts axm_render_native axm-render native_library_smoke scene_file_smoke render_request_smoke render_receipt_smoke contract_adapter_probe contract_flat_renderer state_residency_bench)
  if (MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive-)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endforeach()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/examples/reference.axmscene
  ${CMAKE_CURRENT_BINARY_DIR}/reference.axmscene
  COPYONLY
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/examples/reference.axmrender
  ${CMAKE_CURRENT_BINARY_DIR}/reference.axmrender
  COPYONLY
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/examples/contract-probe.axmrender
  ${CMAKE_CURRENT_BINARY_DIR}/contract-probe.axmrender
  COPYONLY
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/examples/contract-flat.axmrender
  ${CMAKE_CURRENT_BINARY_DIR}/contract-flat.axmrender
  COPYONLY
)

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
add_test(
  NAME render-request-reference-render
  COMMAND render_request_smoke ${CMAKE_CURRENT_SOURCE_DIR}/examples/reference.axmrender
)
add_test(
  NAME render-request-rejects-unsupported-backend
  COMMAND axm-render --request ${CMAKE_CURRENT_SOURCE_DIR}/examples/unsupported-backend.axmrender --self-test
)
set_tests_properties(render-request-rejects-unsupported-backend PROPERTIES WILL_FAIL TRUE)
add_test(
  NAME render-receipt-round-trip
  COMMAND render_receipt_smoke
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/reference.axmrender
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/reference.axmscene
    ${CMAKE_CURRENT_BINARY_DIR}/receipt-smoke.ppm
    ${CMAKE_CURRENT_BINARY_DIR}/receipt-smoke.axmreceipt
)
add_test(
  NAME render-request-cli-writes-receipt
  COMMAND axm-render
    --request ${CMAKE_CURRENT_BINARY_DIR}/reference.axmrender
    --receipt ${CMAKE_CURRENT_BINARY_DIR}/cli-reference.axmreceipt
)
add_test(
  NAME contract-only-adapter-probe
  COMMAND contract_adapter_probe ${CMAKE_CURRENT_SOURCE_DIR}/examples/contract-probe.axmrender
)
add_test(
  NAME contract-only-adapter-rejects-native-backend
  COMMAND contract_adapter_probe ${CMAKE_CURRENT_SOURCE_DIR}/examples/reference.axmrender
)
set_tests_properties(contract-only-adapter-rejects-native-backend PROPERTIES WILL_FAIL TRUE)
add_test(
  NAME contract-flat-renderer-writes-pixels-and-receipt
  COMMAND contract_flat_renderer
    ${CMAKE_CURRENT_BINARY_DIR}/contract-flat.axmrender
    ${CMAKE_CURRENT_BINARY_DIR}/contract-flat.axmreceipt
)
add_test(
  NAME contract-flat-renderer-rejects-native-backend
  COMMAND contract_flat_renderer
    ${CMAKE_CURRENT_BINARY_DIR}/reference.axmrender
    ${CMAKE_CURRENT_BINARY_DIR}/contract-flat-wrong-backend.axmreceipt
)
set_tests_properties(contract-flat-renderer-rejects-native-backend PROPERTIES WILL_FAIL TRUE)
add_test(NAME state-native-residency-equivalence COMMAND state_residency_bench --self-test)
