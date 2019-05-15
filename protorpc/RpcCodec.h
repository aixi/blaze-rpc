//
// Created by xi on 19-5-15.
//

#ifndef BLAZE_PROTORPC_RPCCODEC_H
#define BLAZE_PROTORPC_RPCCODEC_H

#include <protorpc/ProtobufCodecLite.h>

namespace blaze::net
{

class RpcMessage;
using RpcMessagePtr = std::shared_ptr<RpcMessage>;

extern const char rpc_tag[]; // = "RPC0"

using RpcCodec = ProtobufCodecLiteT<RpcMessage, rpc_tag>;
}

#endif //BLAZE_PROTORPC_RPCCODEC_H
