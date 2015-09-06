#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stream/istream.hpp"
#include "acl_cpp/stream/polarssl_conf.hpp"
#include "acl_cpp/stream/polarssl_io.hpp"
#include "acl_cpp/mime/rfc822.hpp"
#include "acl_cpp/smtp/mail_message.hpp"
#include "acl_cpp/smtp/smtp_client.hpp"

namespace acl {

smtp_client::smtp_client(const char* addr, int conn_timeout, int rw_timeout)
{
	addr_ = acl_mystrdup(addr);
	conn_timeout_ = conn_timeout;
	rw_timeout_ = rw_timeout;
	client_ = NULL;

	ehlo_ = true;
	ssl_conf_ = NULL;
}

smtp_client::~smtp_client()
{
	acl_myfree(addr_);

	if (client_)
	{
		// �� SMTP_CLIENT ��������ÿգ��Ա����ڲ��ٴ��ͷţ�
		// ��Ϊ��������������� stream_.close() ʱ���ͷ�
		client_->conn = NULL;
		smtp_close(client_);
	}

	// �� socket ��������ţ���ر�֮��ͬʱ����������� SSL �����ͷ�
	if (stream_.opened())
		stream_.close();
}

smtp_client& smtp_client::set_ssl(polarssl_conf* ssl_conf)
{
	ssl_conf_ = ssl_conf;
	return *this;
}

int smtp_client::get_code() const
{
	return client_ == NULL ? -1 : client_->smtp_code;
}

const char* smtp_client::get_status() const
{
	return client_ == NULL ? "unknown" : client_->buf;
}

bool smtp_client::send(const mail_message& message,
	const char* email /* = NULL */)
{
	if (open() == false)
		return false;
	if (get_banner() == false)
		return false;
	if (greet() == false)
		return false;
	if (!auth_login(message.get_auth_user(), message.get_auth_pass()))
		return false;
	const rfc822_addr* from = message.get_from();
	if (from == NULL || from->addr == NULL)
		return false;
	if (mail_from(from->addr) == false)
		return false;
	if (to_recipients(message.get_recipients()) == false)
		return false;
	if (email != NULL)
	{
		if (send_email(email) == false)
			return false;
		return true;
	}

	email = message.get_email();
	if (email != NULL)
	{
		if (send_email(email) == false)
			return false;
		return true;
	}

	return true;
}

bool smtp_client::open()
{
	if (stream_.opened())
	{
		acl_assert(client_ != NULL);
		acl_assert(client_->conn == stream_.get_vstream());
		return true;
	}

	client_ = smtp_open(addr_, conn_timeout_, rw_timeout_, 1024);
	if (client_ == NULL)
	{
		logger_error("connect %s error: %s", addr_, last_serror());
		return false;
	}

	// ��������
	stream_.open(client_->conn);

	if (ssl_conf_)
	{
		polarssl_io* ssl = new polarssl_io(*ssl_conf_, false);
		if (stream_.setup_hook(ssl) == ssl)
		{
			logger_error("open ssl client error!");
			ssl->destroy();
			return false;
		}
	}
	return true;
}

bool smtp_client::get_banner()
{
	return smtp_get_banner(client_) == 0 ? true : false;
}

bool smtp_client::greet()
{
	return smtp_greet(client_, "localhost", ehlo_ ? 1 : 0)
		== 0 ? true : false;
}

bool smtp_client::auth_login(const char* user, const char* pass)
{
	if (user == NULL || *user == 0)
	{
		logger_error("user null");
		return false;
	}
	if (pass == NULL || *pass == 0)
	{
		logger_error("pass null");
		return false;
	}
	return smtp_auth(client_, user, pass) == 0 ? true : false;
}

bool smtp_client::mail_from(const char* from)
{
	if (from == NULL || *from == 0)
	{
		logger_error("from null");
		return false;
	}
	return smtp_mail(client_, from) == 0 ? true : false;
}

bool smtp_client::rcpt_to(const char* to)
{
	if (to == NULL || *to == 0)
	{
		logger_error("to null");
		return false;
	}
	return smtp_rcpt(client_, to) == 0 ? true : false;
}

bool smtp_client::to_recipients(const std::vector<rfc822_addr*>& recipients)
{
	std::vector<rfc822_addr*>::const_iterator cit;
	for (cit = recipients.begin(); cit != recipients.end(); ++cit)
	{
		if ((*cit)->addr && rcpt_to((*cit)->addr) != 0)
			return false;
	}
	return true;
}

bool smtp_client::data_begin()
{
	return smtp_data(client_) == 0 ? true : false;
}

bool smtp_client::data_end()
{
	return smtp_data_end(client_) == 0 ? true : false;
}

bool smtp_client::quit()
{
	return smtp_quit(client_) == 0 ? true : false;
}

bool smtp_client::noop()
{
	return smtp_noop(client_) == 0 ? true : false;
}

bool smtp_client::reset()
{
	return smtp_rset(client_) == 0 ? true : false;
}

bool smtp_client::write(const char* data, size_t len)
{
	return smtp_send(client_, data, len) == 0 ? true : false;
}

bool smtp_client::format(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	bool ret = vformat(fmt, ap);
	va_end(ap);
	return ret;
}

bool smtp_client::vformat(const char* fmt, va_list ap)
{
	string buf;
	buf.vformat(fmt, ap);
	return write(buf.c_str(), buf.size());
}

bool smtp_client::send_email(const char* filepath)
{
	return smtp_send_file(client_, filepath) == 0 ? true : false;
}

} // namespace acl
