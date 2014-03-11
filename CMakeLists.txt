cmake_minimum_required(VERSION 2.8.9)
project( lart )
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" )

include( FindLLVM )
if( NOT LLVM_FOUND )
  message( FATAL_ERROR "could not find LLVM" )
endif()

add_definitions( -fno-rtti )

link_directories( ${LLVM_LIBRARY_DIRS} )
include_directories( ${LLVM_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} )
add_definitions( ${LLVM_COMPILE_FLAGS} )

add_subdirectory( lart )
add_subdirectory( tools )
add_subdirectory( test )
