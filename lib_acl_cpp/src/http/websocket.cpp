#include "acl_stdafx.hpp"
//#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/http/websocket.hpp"
//#endif

namespace acl
{

websocket::websocket(socket_stream& client)
	: client_(client)
{
}

websocket::~websocket(void)
{
}

static bool is_big_endian(void)
{
	const int n = 1;

	if (*(char*) &n)
		return false;
	else
		return true;
}

#define swap64(val) (((val) >> 56) | \
	(((val) & 0x00ff000000000000ll) >> 40) | \
	(((val) & 0x0000ff0000000000ll) >> 24) | \
	(((val) & 0x000000ff00000000ll) >> 8)  | \
	(((val) & 0x00000000ff000000ll) << 8)  | \
	(((val) & 0x0000000000ff0000ll) << 24) | \
	(((val) & 0x000000000000ff00ll) << 40) | \
	(((val) << 56)))

#define	hton64(val) is_big_endian() ? val : swap64(val)
#define	ntoh64(val) hton64(val)

bool websocket::read_frame_head(void)
{
	memset(&header_, 0, sizeof(header_));

	int  ret;
	char buf[8];

	if (client_.read(buf, 2) == -1)
	{
		logger_error("read first two char error: %s", last_serror());
		return false;
	}

	header_.fin = (((unsigned char) buf[0]) >> 7) & 0x01;
	header_.rsv1 = (((unsigned char) buf[0]) >> 6) & 0x01;
	header_.rsv2 = (((unsigned char) buf[0]) >> 5) & 0x01;
	header_.rsv3 = (((unsigned char) buf[0]) >> 4) & 0x01;
	header_.opcode = ((unsigned char) buf[0]) & 0x0f;

	header_.mask = (((unsigned char) buf[1]) >> 7) & 0x01;
	unsigned char payload_len = ((unsigned char) buf[1]) & 0x7f;
	if (payload_len <= 125)
		header_.payload_len = payload_len;

	// payload_len == 126 | 127
	else if ((ret = client_.read(buf, payload_len == 126 ? 2 : 8)) == -1)
	{
		logger_error("read ext_payload_len error %s", last_serror());
		return false;
	}
	else if (ret == 2)
	{
		unsigned int n;
		memcpy(&n, buf, ret);
		header_.payload_len = ntohl(n);
	}
	else	// ret == 8
	{
		memcpy(&header_.payload_len, buf, ret);
		header_.payload_len = ntoh64(header_.payload_len);
	}

	if (!header_.mask)
		return true;

	if (client_.read(&header_.masking_key, sizeof(unsigned int)) == -1)
	{
		logger_error("read masking_key error %s", last_serror());
		return false;
	}

	return true;
}

} // namespace acl
