#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stream/istream.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/smtp/smtp_client.hpp"

namespace acl {

smtp_client::smtp_client(const char* addr, int conn_timeout,
	int rw_timeout, bool use_ssl /* = false */)
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
	use_ssl_ = use_ssl;
}

smtp_client::~smtp_client()
{
	stream_.unbind();
	if (client_)
		smtp_close(client_);
	dbuf_->destroy();
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
	client_ = smtp_open(addr_, conn_timeout_, rw_timeout_, 1024);
	if (client_ == NULL)
	{
		logger_error("connect %s error: %s", addr_, last_serror());
		return false;
	}
	// 打开流对象
	stream_.open(client_->conn);
	if (use_ssl_)
	{

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
