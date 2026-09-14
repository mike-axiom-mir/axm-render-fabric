# Optional second real external renderer integration.
# This module is included after the core targets/tests so it can reuse the
# existing renderer-neutral contracts and receipt-comparison fixtures.

add_executable(ghostscript_ps_renderer src/ghostscript_ps_renderer.cpp)
target_link_libraries(ghostscript_ps_renderer PRIVATE axm_render_contracts)
target_compile_features(ghostscript_ps_renderer PRIVATE cxx_std_20)
if (MSVC)
  target_compile_options(ghostscript_ps_renderer PRIVATE /W4 /permissive-)
else()
  target_compile_options(ghostscript_ps_renderer PRIVATE -Wall -Wextra -Wpedantic)
endif()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/examples/ghostscript-reference.axmrender
  ${CMAKE_CURRENT_BINARY_DIR}/ghostscript-reference.axmrender
  COPYONLY
)

find_program(AXM_GHOSTSCRIPT_EXECUTABLE NAMES gs gswin64c gswin32c)

if (AXM_GHOSTSCRIPT_EXECUTABLE)
  add_test(
    NAME external-process-ghostscript-dispatch
    COMMAND axm-render-external
      --renderer $<TARGET_FILE:ghostscript_ps_renderer>
      --request ${CMAKE_CURRENT_BINARY_DIR}/ghostscript-reference.axmrender
      --receipt ${CMAKE_CURRENT_BINARY_DIR}/external-ghostscript.axmreceipt
      --capabilities ${CMAKE_CURRENT_BINARY_DIR}/external-ghostscript.axmcaps
  )
  set_tests_properties(
    external-process-ghostscript-dispatch
    PROPERTIES
      FIXTURES_SETUP ghostscript_external_receipt
      ENVIRONMENT "AXM_GHOSTSCRIPT=${AXM_GHOSTSCRIPT_EXECUTABLE}"
  )

  add_test(
    NAME external-process-ghostscript-compares-native-intent
    COMMAND axm-render-compare
      ${CMAKE_CURRENT_BINARY_DIR}/interop-native.axmreceipt
      ${CMAKE_CURRENT_BINARY_DIR}/external-ghostscript.axmreceipt
  )
  set_tests_properties(
    external-process-ghostscript-compares-native-intent
    PROPERTIES FIXTURES_REQUIRED "interop_reference_receipts;ghostscript_external_receipt"
  )

  add_test(
    NAME external-process-ghostscript-rejects-native-backend
    COMMAND axm-render-external
      --renderer $<TARGET_FILE:ghostscript_ps_renderer>
      --request ${CMAKE_CURRENT_BINARY_DIR}/reference.axmrender
      --receipt ${CMAKE_CURRENT_BINARY_DIR}/external-ghostscript-wrong-backend.axmreceipt
      --capabilities ${CMAKE_CURRENT_BINARY_DIR}/external-ghostscript-wrong-backend.axmcaps
  )
  set_tests_properties(
    external-process-ghostscript-rejects-native-backend
    PROPERTIES
      WILL_FAIL TRUE
      ENVIRONMENT "AXM_GHOSTSCRIPT=${AXM_GHOSTSCRIPT_EXECUTABLE}"
  )
endif()
