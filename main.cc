#include <protorpc/rpc.pb.h>

int main()
{
    blaze::net::RpcMessage message;
    message.set_type(blaze::net::REQUEST);
    message.set_id(0);
    // FIXME: std::system_error
    std::cout << message.DebugString();
    return 0;
}