//
// Created by xi on 19-5-15.
//

#include <stdio.h>
#include <protorpc/RpcCodec.h>
#include <protorpc/ProtobufCodecLite.h>
#include <protorpc/rpc.pb.h>
#include <blaze/net/Buffer.h>

using namespace blaze;
using namespace blaze::net;

void RpcMessageCallback(const TcpConnectionPtr& conn,
                        const RpcMessagePtr& message,
                        Timestamp receive_time)
{
}

MessagePtr g_msgptr;
void MessageCallback(const TcpConnectionPtr& conn,
                     const MessagePtr& message,
                     Timestamp receive_time)
{
    g_msgptr = message;
}

void Print(const Buffer& buf)
{
    printf("encoded to %zd bytes\n", buf.ReadableBytes());
    for (size_t i = 0; i < buf.ReadableBytes(); ++i)
    {
        unsigned char ch = static_cast<unsigned char>(buf.Peek()[i]);
        printf("%2zd: 0x%02x %c\n", i, ch, isgraph(ch) ? ch : ' ');
    }
}

char rpc_tag[] = "RPC0";

int main()
{
    RpcMessage message;
    message.set_type(REQUEST);
    message.set_id(1);
//    char wire[] = "\0\0\0\x13" "RPC0" "\x08\x01\x11\x02\0\0\0\0\0\0\0" "\x0f\xef\x01\x32";
//    std::string expected(wire, sizeof(wire) - 1);
    std::string s1, s2;
    Buffer buf1, buf2;
    {
        RpcCodec codec(RpcMessageCallback);
        codec.FillEmptyBuffer(&buf1, message);
        Print(buf1);
        s1 = buf1.ToString();
    }

    {
        ProtobufCodecLite codec(&RpcMessage::default_instance(), "RPC0", ::MessageCallback);
        codec.FillEmptyBuffer(&buf2, message);
        Print(buf2);
        s2 = buf2.ToString();
        codec.OnMessage(TcpConnectionPtr(), &buf1, Timestamp::Now());
        assert(g_msgptr);
        assert(g_msgptr->DebugString() == message.DebugString());
        g_msgptr->DebugString();
        message.DebugString();
        g_msgptr.reset();
    }
    assert(s1 == s2);
//    assert(s1 == expected);
//    assert(s2 == expected);

    {
        Buffer buf;
        ProtobufCodecLite codec(&RpcMessage::default_instance(), "XYZ", ::MessageCallback);
        codec.FillEmptyBuffer(&buf, message);
        Print(buf);
        s2 = buf.ToString();
        codec.OnMessage(TcpConnectionPtr(), &buf, Timestamp::Now());
        assert(g_msgptr);
        assert(g_msgptr->DebugString() == message.DebugString());
    }
    google::protobuf::ShutdownProtobufLibrary();
}

