set( root ${CMAKE_SOURCE_DIR}/test )

file( GLOB tests *.sh )

add_custom_target( functional
  COMMAND cp ${CMAKE_SOURCE_DIR}/test/lib .
  COMMAND cp ${CMAKE_SOURCE_DIR}/test/util .
  COMMAND cp ${CMAKE_SOURCE_DIR}/test/check .
  COMMAND ${WIBBLE_WRAP_TESTS} sh ${CMAKE_SOURCE_DIR}/test/harness
    "TOOLS='${lart_BINARY_DIR}/tools'"
    "WIN32='${WIN32}'"
    "LLVMCONFIG='${LLVM_CONFIG_EXECUTABLE}'"
    ${tests}
)

add_dependencies( functional lart )
