add_library(blaze_protobuf_codec RpcCodec.cc ProtobufCodecLite.cc)
set_target_properties(blaze_protobuf_codec PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(blaze_protobuf_codec blaze protobuf z)

add_subdirectory(tests)