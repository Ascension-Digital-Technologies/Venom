if(NOT DEFINED PROBE OR NOT DEFINED DIST)
  message(FATAL_ERROR "PROBE and DIST are required")
endif()
file(GLOB packages
  "${DIST}/assets/app/app.*.vbc")
list(LENGTH packages package_count)
if(NOT package_count EQUAL 1)
  message(FATAL_ERROR "expected one app package in ${DIST}, found ${package_count}: ${packages}")
endif()
list(GET packages 0 package)
execute_process(
  COMMAND "${PROBE}" "${package}" "/"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0)
  message(STATUS "${out}")
  message(STATUS "${err}")
  message(FATAL_ERROR "venom_runtime_probe failed with ${rc}")
endif()
message(STATUS "${out}")
