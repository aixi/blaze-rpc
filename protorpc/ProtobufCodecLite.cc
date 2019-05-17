//
// Created by xi on 19-5-15.
//

#include <zlib.h>
#include <google/protobuf/message.h>
#include <blaze/log/Logging.h>
#include <blaze/net/TcpConnection.h>
#include <blaze/net/Endian.h>
#include <protorpc/ProtobufCodecLite.h>

namespace
{
int ProtobufVersionVerify()
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    return 0;
};

int dummy __attribute__((unused)) = ProtobufVersionVerify();

int32_t AsInt32(const char* buf)
{
    int32_t be32 = 0;
    memcpy(&be32, buf, sizeof(be32));
    return blaze::net::sockets::NetworkToHost32(be32);
}

} // anonymous namespace

namespace blaze::net
{

void ProtobufCodecLite::Send(const TcpConnectionPtr& conn, const google::protobuf::Message& message)
{
    Buffer buf;
    FillEmptyBuffer(&buf, message);
    conn->Send(&buf);
}

void ProtobufCodecLite::FillEmptyBuffer(Buffer* buf, const google::protobuf::Message& message)
{
    assert(buf->ReadableBytes() == 0);
    buf->Append(tag_);
    int payload_bytes = SerializeToBuffer(message, buf);
    UnusedVariable(payload_bytes);
    int32_t checksum = ComputeAdler32(buf->Peek(), static_cast<int>(buf->ReadableBytes()));
    buf->AppendInt32(checksum);
    assert(buf->ReadableBytes() == tag_.size() + payload_bytes + kChecksumLen);
    int32_t len = sockets::HostToNetwork32(static_cast<int32_t>(buf->ReadableBytes()));
    buf->Prepend(&len, sizeof(len));
}

void ProtobufCodecLite::OnMessage(const TcpConnectionPtr& conn,
                                  Buffer* buf,
                                  Timestamp receive_time)
{
    // use while because there could be more than one message in buffer
    while (buf->ReadableBytes() >= implicit_cast<uint32_t>(kMinMessageLen + kHeaderLen))
    {
        // Note: do not use ReadInt32, it will retrieve 4 byte
        const int32_t len = buf->PeekInt32();
        if (len < kMinMessageLen || len > kMaxMessageLen)
        {
            error_cb_(conn, buf, receive_time, ErrorCode::InvalidLength);
            break;
        }
        else if (buf->ReadableBytes() >= implicit_cast<size_t>(kHeaderLen + len))
        {
            MessagePtr message(prototype_->New());
            ErrorCode error_code = Parse(buf->Peek() + kHeaderLen, len, get_pointer(message));
            if (error_code == ErrorCode::NoError)
            {
                message_cb_(conn, message, receive_time);
                buf->Retrieve(kHeaderLen + len);
            }
            else
            {
                error_cb_(conn, buf, receive_time, error_code);
                break;
            }
        }
        else
        {
            break;
        }
    }
}

bool ProtobufCodecLite::ParseFromBuffer(std::string_view buf, google::protobuf::Message* message)
{
    return message->ParseFromArray(buf.data(), static_cast<int>(buf.size()));
}

int ProtobufCodecLite::SerializeToBuffer(const google::protobuf::Message& message, Buffer* buf)
{
    int byte_size = message.ByteSize();
    buf->EnsureWritableBytes(byte_size + kChecksumLen);
    uint8_t* start = reinterpret_cast<uint8_t*>(buf->BeginWrite());
    uint8_t* end = message.SerializeWithCachedSizesToArray(start);
    assert(end - start == byte_size);
    UnusedVariable(end);
    buf->HasWritten(byte_size);
    return byte_size;
}

std::string ProtobufCodecLite::ErrorCodeToString(ErrorCode error_code)
{
    switch (error_code)
    {
        case ErrorCode::NoError:
            return "NoError";
        case ErrorCode::InvalidLength:
            return "InvalidLength";
        case ErrorCode::ChecksumError:
            return "ChecksumError";
        case ErrorCode::InvalidNameLength:
            return "InvalidNameLength";
        case ErrorCode::UnknownMessageType:
            return "UnknownMessageType";
        case ErrorCode::ProtobufParseError:
            return "ProtobufParseError";
        default:
            return "UnknownError";
    }
}

void ProtobufCodecLite::DefaultErrorCallback(const TcpConnectionPtr& conn,
                                             Buffer* buf,
                                             blaze::Timestamp receive_time,
                                             blaze::net::ProtobufCodecLite::ErrorCode error_code)
{
    LOG_ERROR << "ProtobufCodecLite::DefaultErrorCallback" << ErrorCodeToString(error_code);
    if (conn && conn->Connected())
    {
        conn->Shutdown();
    }
}

int32_t ProtobufCodecLite::ComputeAdler32(const void* buf, int len)
{
    uLong adler = ::adler32(0L, Z_NULL, 0);
    return static_cast<int32_t>(::adler32(adler, reinterpret_cast<const Bytef*>(buf), len));
}

// buf point to tag field
bool ProtobufCodecLite::ValidateChecksum(const char* buf, int len)
{
    int32_t actual = AsInt32(buf + len - kChecksumLen);
    int32_t checksum = ComputeAdler32(buf, len - kChecksumLen);
    return actual == checksum;
}

// buf point to tag field
ProtobufCodecLite::ErrorCode ProtobufCodecLite::Parse(const char* buf, int len, google::protobuf::Message* message)
{
    ErrorCode error_code = ErrorCode::NoError;
    if (ValidateChecksum(buf, len))
    {
        if (memcmp(buf, tag_.data(), tag_.size()) == 0)
        {
            const char* data = buf + tag_.size();
            int32_t payload_len = len - kChecksumLen - static_cast<int>(tag_.size());
            if (ParseFromBuffer(std::string_view(data, payload_len), message))
            {
                error_code = ErrorCode::NoError;
            }
        }
        else
        {
            error_code = ErrorCode::UnknownMessageType;
        }
    }
    else
    {
        error_code = ErrorCode::ChecksumError;
    }
    return error_code;
}

} // namespace blaze::net