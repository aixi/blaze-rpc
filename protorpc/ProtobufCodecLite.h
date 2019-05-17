//
// Created by xi on 19-5-15.
//

#ifndef BLAZE_PROTORPC_PROTOBUFCODECLITE_H
#define BLAZE_PROTORPC_PROTOBUFCODECLITE_H

#include <memory>
#include <type_traits>
#include <string_view>
#include <blaze/utils/Types.h>
#include <blaze/utils/noncopyable.h>
#include <blaze/utils/Timestamp.h>
#include <blaze/net/Callbacks.h>

// forward declarations avoid include google/protobuf/message.h
namespace google::protobuf
{
class Message;
} // namespace google::protobuf

namespace blaze::net
{
using MessagePtr = std::shared_ptr<google::protobuf::Message>;

//wire format
//
//Field      Length      Content
//size       4-byte      M+N+4
//tag        M-byte      "RPC0", etc.
//payload    N-byte      protobuf message binary
//checksum   4-byte      adler32 of tag + payload

// Internal use only, use ProtobufCodecT.
class ProtobufCodecLite : public noncopyable
{
public:
    const static int kHeaderLen = sizeof(int32_t);
    const static int kChecksumLen = sizeof(int32_t);
    const static int kMaxMessageLen = 64 * 1024 * 1024; // codec_stream.h kDefaultTotalBytesLimit

    enum class ErrorCode
    {
        NoError,
        InvalidLength,
        ChecksumError,
        InvalidNameLength,
        UnknownMessageType,
        ProtobufParseError
    };

    // when obtain a complete protobuf message
    using ProtobufMessageCallback = std::function<void (const TcpConnectionPtr&,
                                                        const MessagePtr&,
                                                        Timestamp)>;

    using ErrorCallback = std::function<void (const TcpConnectionPtr&,
                                              Buffer*,
                                              Timestamp,
                                              ErrorCode)>;

    ProtobufCodecLite(const google::protobuf::Message* prototype,
                      std::string_view tag,
                      ProtobufMessageCallback message_cb,
                      ErrorCallback error_cb = DefaultErrorCallback) :
        prototype_(prototype),
        tag_(tag),
        message_cb_(std::move(message_cb)),
        error_cb_(std::move(error_cb)),
        kMinMessageLen(static_cast<int>(tag_.size() + kChecksumLen))
    {
    }

    virtual ~ProtobufCodecLite() = default;

    const std::string& tag() const { return tag_; }

    void Send(const TcpConnectionPtr& conn, const google::protobuf::Message& message);

    void OnMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receive_time);

    virtual bool ParseFromBuffer(std::string_view buf, google::protobuf::Message* message);

    virtual int SerializeToBuffer(const google::protobuf::Message& message, Buffer* buf);

    // buf point to tag field
    ErrorCode Parse(const char* buf, int len, google::protobuf::Message* message);

    void FillEmptyBuffer(Buffer* buf, const google::protobuf::Message& message);

    static std::string ErrorCodeToString(ErrorCode error_code);

    static int32_t ComputeAdler32(const void* buf, int len);

    // buf point to tag field
    static bool ValidateChecksum(const char* buf, int len);

    static void DefaultErrorCallback(const TcpConnectionPtr& conn,
                                     Buffer* buf,
                                     Timestamp receive_time,
                                     ErrorCode error_code);

private:
    const google::protobuf::Message* prototype_; // prototype_->New()
    const std::string tag_;
    ProtobufMessageCallback message_cb_;
    ErrorCallback error_cb_;
    const int kMinMessageLen;
};

// TAG must be a variable with external linkage, not a string literal
// template <typename MSG, const char*(*TAG)(), typename CODEC = ProtobufCodecLite>
// string literal can not be used as a template non type parameter
// use function pointer to work around
template <typename MSG, const char* TAG, typename CODEC = ProtobufCodecLite>
class ProtobufCodecLiteT
{
    static_assert(std::is_base_of_v<ProtobufCodecLite, CODEC>, "CODEC must be derived from ProtobufCodecLite");
public:
    using ConcreteMessagePtr = std::shared_ptr<MSG>;
    using ConcreteMessageCallback = std::function<void (const TcpConnectionPtr&,
                                                        const ConcreteMessagePtr&,
                                                        Timestamp)>;
    using ErrorCallback = ProtobufCodecLite::ErrorCallback;

    explicit ProtobufCodecLiteT(ConcreteMessageCallback message_cb,
                                const ErrorCallback& error_cb = ProtobufCodecLite::DefaultErrorCallback) :
        message_cb_(std::move(message_cb)),
        codec_(&MSG::default_instance(),
               TAG,
               std::bind(&ProtobufCodecLiteT::OnRpcMessage, this, _1, _2, _3),
               error_cb)
    {
    }

    const std::string& tag() const { return codec_.tag(); }

    void Send(const TcpConnectionPtr& conn, const MSG& message)
    {
        codec_.Send(conn, message);
    }

    void OnMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receive_time)
    {
        codec_.OnMessage(conn, buf, receive_time);
    }

    // internal use only
    void OnRpcMessage(const TcpConnectionPtr& conn, const MessagePtr& message, Timestamp receive_time)
    {
        message_cb_(conn, blaze::down_pointer_cast<MSG>(message), receive_time);
    }

    void FillEmptyBuffer(Buffer* buf, const MSG& message)
    {
        codec_.FillEmptyBuffer(buf, message);
    }
private:
    ConcreteMessageCallback message_cb_;
    CODEC codec_;
};

} // namespace blaze::net

#endif //BLAZE_PROTORPC_PROTOBUFCODECLITE_H
