#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include <vector>

struct SMTP_CLIENT;

namespace acl {

class istream;
class polarssl_conf;
class mail_message;

class ACL_CPP_API smtp_client
{
public:
	smtp_client(const char* addr, int conn_timeout, int rw_timeout);
	~smtp_client();

	/**
	 * ���ñ����������ʼ��������ʼ������
	 * @param message {const mail_messsage&} �ʼ������Ϣ��������ǰ������
	 * @param email {const char*} �ǿ�ʱ������ʹ�ô��ļ���Ϊ�ʼ������ݷ���
	 * @
	 */
	bool send(const mail_message& message, const char* email = NULL);

	/**
	 * ���� SSL ���ݴ���ģʽ
	 * @param ssl_conf {polarssl_conf*} �ǿ�ʱ��ָ������ SSL ����ģʽ
	 * @return {smtp_client&}
	 */
	smtp_client& set_ssl(polarssl_conf* ssl_conf);

	/**
	 * ����ϴ� SMTP �������̷���˷��ص�״̬��
	 * @return {int}
	 */
	int get_code() const;

	/**
	 * ����ϴ� SMTP �������̷���˷��ص�״̬��Ϣ
	 * @return {const char*}
	 */
	const char* get_status() const;

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

	/////////////////////////////////////////////////////////////////////

	// �����Ǵ����Ӻͷ����ŷ�ķֲ�����
	bool open();
	bool get_banner();
	bool greet();
	bool auth_login(const char* user, const char* pass);
	bool mail_from(const char* from);
	bool rcpt_to(const char* to);

	/**
	 * ����һ���������ʼ�����Ҫ�����ʼ��洢�ڴ����ϵ�·��
	 * @param filepath {const char*} �ʼ��ļ�·��
	 * @return {bool} ��������Ƿ�ɹ�
	 */
	bool send_email(const char* filepath);

	/**
	 * ��ʼ�����ʼ���������:DATA
	 * @return {bool} ��������Ƿ�ɹ�
	 */
	bool data_begin();
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

private:
	polarssl_conf* ssl_conf_;
	char* addr_;
	int   conn_timeout_;
	int   rw_timeout_;
	SMTP_CLIENT* client_;
	socket_stream stream_;
	bool  ehlo_;

	bool to_recipients(const std::vector<rfc822_addr*>& recipients);
};

} // namespace acl
