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
if(NOT safe_stdout MATCHES "renderer_process_resolution=EXPLICIT")
  message(FATAL_ERROR "safe external dispatch did not identify explicit renderer resolution")
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

if(UNIX)
  set(path_bin "${WORK_DIR}/path-bin")
  file(MAKE_DIRECTORY "${path_bin}")
  file(COPY "${RENDERER}" DESTINATION "${path_bin}"
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
  set(path_renderer "${path_bin}/${renderer_name}")
  set(path_request "${WORK_DIR}/path-guard.axmrender")
  set(path_receipt "${WORK_DIR}/path-guard.axmreceipt")
  set(path_capabilities "${WORK_DIR}/path-guard.axmcaps")
  set(path_output_name "path-guard-output.ppm")
  set(path_output "${WORK_DIR}/${path_output_name}")
  set(decoy_path "${WORK_DIR}/${renderer_name}")

  # Deliberately place a same-named regular file in the working directory. A bare
  # execvp-style launch must resolve from PATH, not claim continuity for this decoy.
  file(WRITE "${decoy_path}" "AXM renderer path-resolution decoy; not executable.\n")
  file(WRITE "${path_request}"
    "AXM_RENDER_REQUEST 1\n"
    "scene ${SCENE}\n"
    "backend axm.contract.cpu.flat\n"
    "width 32\n"
    "height 18\n"
    "format ppm-rgb8\n"
    "output ${path_output_name}\n"
  )

  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env "PATH=${path_bin}:$ENV{PATH}"
      "${HARNESS}"
      --renderer "${renderer_name}"
      --request "${path_request}"
      --receipt "${path_receipt}"
      --capabilities "${path_capabilities}"
    WORKING_DIRECTORY "${WORK_DIR}"
    RESULT_VARIABLE path_status
    OUTPUT_VARIABLE path_stdout
    ERROR_VARIABLE path_stderr
  )

  if(NOT path_status EQUAL 0)
    message(FATAL_ERROR "PATH-resolved external dispatch failed: ${path_stderr}")
  endif()
  if(NOT EXISTS "${path_output}" OR NOT EXISTS "${path_receipt}" OR NOT EXISTS "${path_capabilities}")
    message(FATAL_ERROR "PATH-resolved external dispatch did not produce all expected artifacts")
  endif()
  if(NOT path_stdout MATCHES "renderer_process_resolution=PATH")
    message(FATAL_ERROR "bare renderer name was not reported as PATH-resolved: ${path_stdout}")
  endif()
  string(FIND "${path_stdout}" "renderer_process_invocation_path=${path_renderer}" invocation_match)
  if(invocation_match EQUAL -1)
    message(FATAL_ERROR "PATH-resolved dispatch did not pin the PATH executable: ${path_stdout}")
  endif()
  string(FIND "${path_stdout}" "renderer_process_canonical_path=${path_renderer}" canonical_match)
  if(canonical_match EQUAL -1)
    message(FATAL_ERROR "PATH-resolved dispatch continuity evidence points at the wrong file: ${path_stdout}")
  endif()
  if(NOT path_stdout MATCHES "renderer_process_continuity=PASS")
    message(FATAL_ERROR "PATH-resolved dispatch did not report renderer process continuity evidence")
  endif()
  if(NOT path_stdout MATCHES "external_process_dispatch=PASS")
    message(FATAL_ERROR "PATH-resolved dispatch did not complete verified dispatch")
  endif()

  message(STATUS "renderer_process_path_resolution=PASS")
  message(STATUS "renderer_process_path_decoy_ignored=PASS")
endif()

message(STATUS "renderer_process_self_clobber_guard=PASS")
message(STATUS "renderer_process_continuity_evidence=PASS")
