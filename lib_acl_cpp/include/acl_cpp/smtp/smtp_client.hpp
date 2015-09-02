#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include <map>

struct SMTP_CLIENT;

namespace acl {

class dbuf_pool;
class istream;

class ACL_CPP_API smtp_client
{
public:
	smtp_client(const char* addr, int conn_timeout, int rw_timeout);
	~smtp_client();

	smtp_client& set_auth(const char* user, const char* pass);
	smtp_client& set_from(const char* from);
	smtp_client& add_to(const char* to);

	int get_smtp_code() const;
	const char* get_smtp_status() const;

	bool send_envelope();

	bool data_begin();
	bool write(const char* data, size_t len);
	bool format(const char* fmt, ...);
	bool vformat(const char* fmt, va_list ap);
	bool send(const char* filepath);
	bool send(istream& in);
	bool data_end();

	bool quit();
	bool noop();
	bool reset();

	/////////////////////////////////////////////////////////////////////
	// 以下是打开连接和发送信封的分步步骤
	bool open();
	bool get_banner();
	bool greet();
	bool auth_login();
	bool mail_from();
	bool to_recipients();
	
private:
	int   status_;
	dbuf_pool* dbuf_;
	char* addr_;
	int   conn_timeout_;
	int   rw_timeout_;
	SMTP_CLIENT* client_;
	socket_stream stream_;
	bool  ehlo_;
	char* auth_user_;
	char* auth_pass_;
	char* from_;
	std::vector<char*> recipients_;
};

} // namespace acl
