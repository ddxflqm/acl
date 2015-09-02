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
	 * �����ʼ��ŷ���̣��������δ�򿪣����Զ������ӣ�Ȼ�����������
	 * ehlo/helo, auth login, mail from, rcpt to, data
	 * �ú�����ʵ���������ĺ�����ϣ�open, get_banner, greet, auth_login,
	 * mail_from, to_recipients
	 * @return {bool}
	 */
	bool send_envelope();

	/**
	 * ��ʼ�����ʼ���������:DATA
	 * @return {bool} ��������Ƿ�ɹ�
	 */
	bool data_begin();

	/**
	 * �����ʼ������ݣ�����ѭ�����ñ����������������ݱ������ϸ���ʼ���ʽ
	 * @param data {const char*} �ʼ�����
	 * @param len {size_t} data �ʼ����ݳ���
	 * @return {bool} ��������Ƿ�ɹ�
	 */
	bool write(const char* data, size_t len);

	/**
	 * �����ʼ������ݣ�����ѭ�����ñ����������������ݱ������ϸ���ʼ���ʽ
	 * @param fmt {const char*} ��θ�ʽ
	 * @return {bool} ��������Ƿ�ɹ�
	 */
	bool format(const char* fmt, ...);

	/**
	 * �����ʼ������ݣ�����ѭ�����ñ����������������ݱ������ϸ���ʼ���ʽ
	 * @param fmt {const char*} ��θ�ʽ
	 * @param ap {va_list}
	 * @return {bool} ��������Ƿ�ɹ�
	 */
	bool vformat(const char* fmt, va_list ap);

	/**
	 * ����һ���������ʼ�����Ҫ�����ʼ��洢�ڴ����ϵ�·��
	 * @param filepath {const char*} �ʼ��ļ�·��
	 * @return {bool} ��������Ƿ�ɹ�
	 */
	bool send(const char* filepath);

	/**
	 * ����һ���������ʼ�����Ҫ�����ʼ��ļ�������������
	 * @param in {istream&} �ʼ�����������
	 * @return {bool} ��������Ƿ�ɹ�
	 */
	bool send(istream& in);

	/**
	 * �ʼ�������Ϻ���������ñ����������ʼ��������������ݽ���
	 * @return {bool} ��������Ƿ�ɹ�
	 */
	bool data_end();

	/**
	 * �Ͽ����ʼ�������������
	 * @return {bool} ��������Ƿ�ɹ�
	 */
	bool quit();

	/**
	 * NOOP ����
	 * @return {bool} ��������Ƿ�ɹ�
	 */
	bool noop();

	/**
	 * �������ʼ�������������״̬
	 * @return {bool} ��������Ƿ�ɹ�
	 */
	bool reset();

	/////////////////////////////////////////////////////////////////////
	// �����Ǵ����Ӻͷ����ŷ�ķֲ�����
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
