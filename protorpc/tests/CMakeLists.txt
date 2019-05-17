add_executable(rpc_codec_test RpcCodec_test.cc ../rpc.pb.cc)
# NOTE: yout must link pthread
target_link_libraries(rpc_codec_test blaze_protobuf_codec pthread)