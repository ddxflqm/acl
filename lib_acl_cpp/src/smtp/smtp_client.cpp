#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/stream/istream.hpp"
#include "acl_cpp/stream/polarssl_conf.hpp"
#include "acl_cpp/stream/polarssl_io.hpp"
#include "acl_cpp/smtp/smtp_client.hpp"

namespace acl {

smtp_client::smtp_client(const char* addr, int conn_timeout, int rw_timeout)
{
	dbuf_ = new dbuf_pool;
	addr_ = dbuf_->dbuf_strdup(addr);
	conn_timeout_ = conn_timeout;
	rw_timeout_ = rw_timeout;
	client_ = NULL;

	ehlo_ = true;
	auth_user_ = NULL;
	auth_pass_ = NULL;
	from_ = NULL;
	ssl_conf_ = NULL;
}

smtp_client::~smtp_client()
{
	if (client_)
	{
		// 将 SMTP_CLIENT 对象的流置空，以避免内部再次释放，
		// 因为该流对象会在下面 stream_.close() 时被释放
		client_->conn = NULL;
		smtp_close(client_);
	}

	// 当 socket 流对象打开着，则关闭之，同时将依附于其的 SSL 对象释放
	if (stream_.opened())
		stream_.close();
	dbuf_->destroy();
}

smtp_client& smtp_client::set_ssl(polarssl_conf* ssl_conf)
{
	ssl_conf_ = ssl_conf;
	return *this;
}

smtp_client& smtp_client::set_auth(const char* user, const char* pass)
{
	if (user && *user && pass && *pass)
	{
		auth_user_ = dbuf_->dbuf_strdup(user);
		auth_pass_ = dbuf_->dbuf_strdup(pass);
	}

	return *this;
}

smtp_client& smtp_client::set_from(const char* from)
{
	if (from && *from)
		from_ = dbuf_->dbuf_strdup(from);

	return *this;
}

smtp_client& smtp_client::add_recipients(const char* recipients)
{
	string buf(recipients);
	std::list<string>& tokens = buf.split(" \t;,");
	std::list<string>::const_iterator cit;
	for (cit = tokens.begin(); cit != tokens.end(); ++cit)
		(void) add_to((*cit).c_str());

	return *this;
}

smtp_client& smtp_client::add_to(const char* to)
{
	if (to && *to)
		recipients_.push_back(dbuf_->dbuf_strdup(to));

	return *this;
}

int smtp_client::get_smtp_code() const
{
	return client_ == NULL ? -1 : client_->smtp_code;
}

const char* smtp_client::get_smtp_status() const
{
	return client_ == NULL ? "unknown" : client_->buf;
}

bool smtp_client::send_envelope()
{
	if (open() == false)
		return false;
	if (get_banner() == false)
		return false;
	if (greet() == false)
		return false;
	if (auth_login() == false)
		return false;
	if (mail_from() == false)
		return false;
	if (to_recipients() == false)
		return false;
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

	// 打开流对象
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

bool smtp_client::auth_login()
{
	if (auth_user_ == NULL || auth_pass_ == NULL)
	{
		logger_error("auth_user: %s, auth_pass: %s",
			auth_user_ ? "not null" : "null",
			auth_pass_ ? "not null" : "null");
		return false;
	}
	return smtp_auth(client_, auth_user_, auth_pass_) == 0 ? true : false;
}

bool smtp_client::mail_from()
{
	if (from_ == NULL || *from_ == 0)
	{
		logger_error("from null");
		return false;
	}
	return smtp_mail(client_, from_) == 0 ? true : false;
}

bool smtp_client::to_recipients()
{
	std::vector<char*>::const_iterator cit;
	for (cit = recipients_.begin(); cit != recipients_.end(); ++cit)
	{
		if (smtp_rcpt(client_, *cit) != 0)
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

bool smtp_client::send(const char* filepath)
{
	return smtp_send_file(client_, filepath) == 0 ? true : false;
}

bool smtp_client::send(istream& in)
{
	return smtp_send_stream(client_, in.get_vstream()) == 0
		? true : false;
}

} // namespace acl
