if(NOT DEFINED HARNESS OR NOT DEFINED RENDERER OR NOT DEFINED SCENE OR NOT DEFINED WORK_DIR)
  message(FATAL_ERROR "HARNESS, RENDERER, SCENE, and WORK_DIR are required")
endif()

file(MAKE_DIRECTORY "${WORK_DIR}")
get_filename_component(renderer_name "${RENDERER}" NAME)
set(renderer_copy "${WORK_DIR}/guard-${renderer_name}")
set(request_path "${WORK_DIR}/guard.axmrender")
set(receipt_path "${WORK_DIR}/guard.axmreceipt")
set(capabilities_path "${WORK_DIR}/guard.axmcaps")
set(safe_output_name "guard-output.ppm")
set(safe_output_path "${WORK_DIR}/${safe_output_name}")

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E copy "${RENDERER}" "${renderer_copy}"
  RESULT_VARIABLE copy_status
)
if(NOT copy_status EQUAL 0)
  message(FATAL_ERROR "failed to copy renderer executable for continuity guard test")
endif()

get_filename_component(renderer_copy_name "${renderer_copy}" NAME)
file(WRITE "${request_path}"
  "AXM_RENDER_REQUEST 1\n"
  "scene ${SCENE}\n"
  "backend axm.contract.cpu.flat\n"
  "width 32\n"
  "height 18\n"
  "format ppm-rgb8\n"
  "output ${renderer_copy_name}\n"
)

execute_process(
  COMMAND "${HARNESS}"
    --renderer "${renderer_copy}"
    --request "${request_path}"
    --receipt "${receipt_path}"
    --capabilities "${capabilities_path}"
  WORKING_DIRECTORY "${WORK_DIR}"
  RESULT_VARIABLE guard_status
  OUTPUT_VARIABLE guard_stdout
  ERROR_VARIABLE guard_stderr
)

if(guard_status EQUAL 0)
  message(FATAL_ERROR "renderer self-clobber request unexpectedly succeeded")
endif()
if(NOT EXISTS "${renderer_copy}")
  message(FATAL_ERROR "renderer executable was removed by a declared artifact collision")
endif()
if(NOT guard_stderr MATCHES "render output path must differ from renderer executable path")
  message(FATAL_ERROR
    "renderer self-clobber request failed for an unexpected reason: ${guard_stderr}")
endif()

file(WRITE "${request_path}"
  "AXM_RENDER_REQUEST 1\n"
  "scene ${SCENE}\n"
  "backend axm.contract.cpu.flat\n"
  "width 32\n"
  "height 18\n"
  "format ppm-rgb8\n"
  "output ${safe_output_name}\n"
)

execute_process(
  COMMAND "${HARNESS}"
    --renderer "${renderer_copy}"
    --request "${request_path}"
    --receipt "${receipt_path}"
    --capabilities "${capabilities_path}"
  WORKING_DIRECTORY "${WORK_DIR}"
  RESULT_VARIABLE safe_status
  OUTPUT_VARIABLE safe_stdout
  ERROR_VARIABLE safe_stderr
)

if(NOT safe_status EQUAL 0)
  message(FATAL_ERROR "safe external dispatch failed: ${safe_stderr}")
endif()
if(NOT EXISTS "${safe_output_path}" OR NOT EXISTS "${receipt_path}" OR NOT EXISTS "${capabilities_path}")
  message(FATAL_ERROR "safe external dispatch did not produce all expected artifacts")
endif()
if(NOT safe_stdout MATCHES "renderer_process_continuity=PASS")
  message(FATAL_ERROR "safe external dispatch did not report renderer process continuity evidence")
endif()
if(NOT safe_stdout MATCHES "renderer_process_digest64=0x[0-9a-f]+")
  message(FATAL_ERROR "safe external dispatch did not report a renderer process digest")
endif()
if(NOT safe_stdout MATCHES "external_process_dispatch=PASS")
  message(FATAL_ERROR "safe external dispatch did not complete verified dispatch")
endif()

message(STATUS "renderer_process_self_clobber_guard=PASS")
message(STATUS "renderer_process_continuity_evidence=PASS")
