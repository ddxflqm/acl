#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include <map>

struct SMTP_CLIENT;

namespace acl {

class dbuf_pool;
class istream;
class polarssl_conf;

class ACL_CPP_API smtp_client
{
public:
	smtp_client(const char* addr, int conn_timeout, int rw_timeout);
	~smtp_client();

	smtp_client& set_ssl(polarssl_conf* ssl_conf);
	smtp_client& set_auth(const char* user, const char* pass);
	smtp_client& set_from(const char* from);
	smtp_client& add_recipients(const char* recipients);
	smtp_client& add_to(const char* to);

	int get_smtp_code() const;
	const char* get_smtp_status() const;

	/**
	 * 发送邮件信封过程，如果连接未打开，则自动打开连接，然后发送如下命令：
	 * ehlo/helo, auth login, mail from, rcpt to, data
	 * 该函数其实会调用下面的函数组合：open, get_banner, greet, auth_login,
	 * mail_from, to_recipients
	 * @return {bool}
	 */
	bool send_envelope();

	/**
	 * 开始发送邮件数据命令:DATA
	 * @return {bool} 命令操作是否成功
	 */
	bool data_begin();

	/**
	 * 发送邮件体数据，可以循环调用本函数，但数据内容必须是严格的邮件格式
	 * @param data {const char*} 邮件内容
	 * @param len {size_t} data 邮件数据长度
	 * @return {bool} 命令操作是否成功
	 */
	bool write(const char* data, size_t len);

	/**
	 * 发送邮件体数据，可以循环调用本函数，但数据内容必须是严格的邮件格式
	 * @param fmt {const char*} 变参格式
	 * @return {bool} 命令操作是否成功
	 */
	bool format(const char* fmt, ...);

	/**
	 * 发送邮件体数据，可以循环调用本函数，但数据内容必须是严格的邮件格式
	 * @param fmt {const char*} 变参格式
	 * @param ap {va_list}
	 * @return {bool} 命令操作是否成功
	 */
	bool vformat(const char* fmt, va_list ap);

	/**
	 * 发送一封完整的邮件，需要给出邮件存储于磁盘上的路径
	 * @param filepath {const char*} 邮件文件路径
	 * @return {bool} 命令操作是否成功
	 */
	bool send(const char* filepath);

	/**
	 * 发送一封完整的邮件，需要给出邮件文件的输入流对象
	 * @param in {istream&} 邮件数据输入流
	 * @return {bool} 命令操作是否成功
	 */
	bool send(istream& in);

	/**
	 * 邮件发送完毕后，最后必须调用本函数告诉邮件服务器发送数据结束
	 * @return {bool} 命令操作是否成功
	 */
	bool data_end();

	/**
	 * 断开与邮件服务器的连接
	 * @return {bool} 命令操作是否成功
	 */
	bool quit();

	/**
	 * NOOP 命令
	 * @return {bool} 命令操作是否成功
	 */
	bool noop();

	/**
	 * 重置与邮件服务器的连接状态
	 * @return {bool} 命令操作是否成功
	 */
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
	polarssl_conf* ssl_conf_;
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
