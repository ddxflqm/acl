#pragma once
#include "acl_cpp/acl_cpp_define.hpp"

namespace acl
{

class socket_stream;

struct frame_header
{
	unsigned char fin:1;
	unsigned char rsv1:1;
	unsigned char rsv2:1;
	unsigned char rsv3:1;
	unsigned char opcode:4;
	unsigned char mask:1;
	unsigned long long payload_len;
	unsigned int masking_key;
};

class websocket
{
public:
	websocket(socket_stream& client);
	~websocket(void);

	bool read_frame_head(void);

private:
	socket_stream& client_;
	struct frame_header header_;
};

} // namespace acl
