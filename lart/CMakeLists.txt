link_libraries( ${LLVM_LIBRARIES} )

add_library( liblart STATIC aa/pass.cpp )
set_target_properties( liblart PROPERTIES OUTPUT_NAME "lart" )
