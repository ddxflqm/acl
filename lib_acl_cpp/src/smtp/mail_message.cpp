#include "acl_stdafx.hpp"
#include <list>
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/stream/ofstream.hpp"
#include "acl_cpp/stdlib/thread.hpp"
#include "acl_cpp/mime/rfc822.hpp"
#include "acl_cpp/mime/rfc2047.hpp"
#include "acl_cpp/smtp/mail_message.hpp"

namespace acl
{

mail_message::mail_message()
{
	dbuf_ = new dbuf_pool;
	auth_user_ = NULL;
	auth_pass_ = NULL;
	from_ = NULL;
	sender_ = NULL;
	replyto_ = NULL;
	subject_ = NULL;
	charset_ = NULL;
	body_ = NULL;
	body_len_ = 0;
	filepath_ = NULL;
}

mail_message::~mail_message()
{
	dbuf_->destroy();
}

mail_message& mail_message::set_auth(const char* user, const char* pass)
{
	if (user && *user && pass && *pass)
	{
		auth_user_ = dbuf_->dbuf_strdup(user);
		auth_pass_ = dbuf_->dbuf_strdup(pass);
	}
	return *this;
}

mail_message& mail_message::set_from(const char* from, const char* name)
{
	if (from == NULL || *from == 0)
		return *this;
	from_ = (rfc822_addr*) dbuf_->dbuf_alloc(sizeof(rfc822_addr));
	from_->addr = dbuf_->dbuf_strdup(from);
	if (name && *name)
		from_->comment = dbuf_->dbuf_strdup(name);
	else
		from_->comment = NULL;
	return *this;
}

mail_message& mail_message::set_sender(const char* sender, const char* name)
{
	if (sender == NULL || *sender == 0)
		return *this;
	sender_ = (rfc822_addr*) dbuf_->dbuf_alloc(sizeof(rfc822_addr));
	sender_->addr = dbuf_->dbuf_strdup(sender);
	if (name && *name)
		sender_->comment = dbuf_->dbuf_strdup(name);
	else
		sender_->comment = NULL;
	return *this;
}

mail_message& mail_message::set_replyto(const char* replyto, const char* name)
{
	if (replyto == NULL || *replyto == 0)
		return *this;
	replyto_ = (rfc822_addr*) dbuf_->dbuf_alloc(sizeof(rfc822_addr));
	replyto_->addr = dbuf_->dbuf_strdup(replyto);
	if (name && *name)
		replyto_->comment = dbuf_->dbuf_strdup(name);
	else
		replyto_->comment = NULL;
	return *this;
}

void mail_message::add_addrs(const char* in, std::vector<rfc822_addr*>& out)
{
	rfc822 rfc;
	const std::list<rfc822_addr*>& addrs = rfc.parse_addrs(in, "utf-8");
	std::list<rfc822_addr*>::const_iterator cit = addrs.begin();
	for (; cit != addrs.end(); ++cit)
	{
		rfc822_addr* addr = (rfc822_addr* )
			dbuf_->dbuf_alloc(sizeof(rfc822_addr));
		if ((*cit)->addr == NULL)
			continue;
		addr->addr = dbuf_->dbuf_strdup((*cit)->addr);
		if ((*cit)->comment)
			addr->comment = dbuf_->dbuf_strdup((*cit)->comment);
		out.push_back(addr);
	}
}

mail_message& mail_message::add_recipients(const char* recipients)
{
	string buf(recipients);
	std::list<string>& tokens = buf.split(" \t;,");
	std::list<string>::const_iterator cit;
	for (cit = tokens.begin(); cit != tokens.end(); ++cit)
		(void) add_to((*cit).c_str());
	return *this;
}

mail_message& mail_message::add_to(const char* to)
{
	if (to && *to)
	{
		add_addrs(to, to_list_);
		add_addrs(to, recipients_);
	}
	return *this;
}

mail_message& mail_message::add_cc(const char* cc)
{
	if (cc && *cc)
	{
		add_addrs(cc, to_list_);
		add_addrs(cc, recipients_);
	}
	return *this;
}

mail_message& mail_message::add_bcc(const char* bcc)
{
	if (bcc && *bcc)
	{
		add_addrs(bcc, to_list_);
		add_addrs(bcc, recipients_);
	}
	return *this;
}

mail_message& mail_message::set_subject(const char* subject,
	const char* charset /* = "UTF-8" */)
{
	if (subject && *subject)
		subject_ = dbuf_->dbuf_strdup(subject_);
	if (charset && *charset)
		charset_ = dbuf_->dbuf_strdup(charset);
	return *this;
}

mail_message& mail_message::add_header(const char* name, const char* value)
{
	if (name == NULL || *name == 0 || value == NULL || *value == 0)
		return *this;
	char* n = dbuf_->dbuf_strdup(name);
	char* v = dbuf_->dbuf_strdup(value);
	headers_.push_back(std::make_pair(n, v));
	return *this;
}

mail_message& mail_message::set_body(const char* body)
{
	if (body && *body)
	{
		body_len_ = strlen(body);
		body_ = (char*) dbuf_->dbuf_memdup(body, body_len_);
	}

	return *this;
}

mail_message& mail_message::add_attachment(const char* filepath)
{
	if (filepath && *filepath)
		attachments_.push_back(dbuf_->dbuf_strdup(filepath));
	return *this;
}

#if defined(_WIN32) || defined(_WIN64)
#include <process.h>
#define PID	_getpid
#else
#include <unistd.h>
#define PID	getpid
#endif // defined(_WIN32) || defined(_WIN64)

void mail_message::create_boundary()
{
	boundary_.format("---=_aclPart_%lu_%lu_%lu",
		(unsigned long) PID(), thread::thread_self(),
		(unsigned long) time(NULL));
}

bool mail_message::append_addr(ofstream& fp, const char* name,
	const rfc822_addr* addr, const char* to_charset)
{
	if (fp.format("%s: ", name) == -1)
	{
		logger_error("write to file %s error: %s",
			fp.file_path(), last_serror());
		return false;
	}

	string buf;

	if (addr->comment && rfc2047::encode(addr->comment,
		strlen(addr->comment), &buf, to_charset) == false)
	{
		logger_error("rfc2047::encode(%s) error", addr->comment);
		return false;
	}

	if (!buf.empty() && fp.write(buf) == -1)
	{
		logger_error("write comment(%s) error: %s",
			buf.c_str(), last_serror());
		return false;
	}

	if (fp.format("<%s>\r\n", addr->addr) == -1)
	{
		logger_error("write addr(%s) error: %s",
			addr->addr, last_serror());
		return false;
	}

	return true;
}

bool mail_message::append_message_id(ofstream& fp)
{
	string id;
	id.format("<%lu.%lu.%lu.acl@localhost>", (unsigned long) PID(),
		thread::thread_self(), (unsigned long) time(NULL));
	if (fp.write(id) == -1)
	{
		logger_error("write message_id to %s error: %s",
			fp.file_path(), last_serror());
		return false;
	}
	return true;
}

bool mail_message::compose_header(ofstream& fp, const char* to_charset)
{
	std::vector<std::pair<char*, char*> >::const_iterator cit;
	for (cit = headers_.begin(); cit != headers_.end(); ++cit)
	{
		if (fp.format("%s: %s\r\n", (*cit).first, (*cit).second) == -1)
		{
			logger_error("write one header to %s error: %s",
				fp.file_path(), last_serror());
			return false;
		}
	}

	if (sender_ && sender_->addr
		&& append_addr(fp, "Sender", sender_, to_charset) == false)
	{
		return false;
	}

	if (replyto_ && replyto_->addr
		&& append_addr(fp, "ReplyTo", replyto_, to_charset) == false)
	{
		return false;
	}

	if (from_ && from_->addr
		&& append_addr(fp, "From", from_, to_charset) == false)
	{
		return false;
	}

	if (append_message_id(fp) == false)
		return false;

	return true;
}

bool mail_message::append_multipart(ofstream& fp, const char* to_charset)
{
	const char *prompt = "This is a multi-part message in MIME format.";

	create_boundary();
	
	if (fp.format("Mime-version: 1.0\r\n"
		"Content-Type: multipart/mixed;\r\n"
		"	charset=\"%s\";\r\n"
		"	boundary=\"%s\"\r\n\r\n",
		to_charset, boundary_.c_str()) == -1)
	{
		logger_error("write to %s error: %s",
			fp.file_path(), last_serror());
		return false;
	}

	if (fp.format("%s\r\n\r\n", prompt) == -1)
	{
		logger_error("write mime prompt to %s error %s",
			fp.file_path(), last_serror());
		return false;
	}

	if (fp.format("--%s\r\n", boundary_.c_str()) == -1)
	{
		logger_error("write boundary to %s error %s",
			fp.file_path(), last_serror());
		return false;
	}

	if (fp.write(body_, body_len_) == -1)
	{
		logger_error("write body to %s error %s",
			fp.file_path(), last_serror());
		return false;
	}

	return true;
}

bool mail_message::compose(const char* filepath,
	const char* to_charset /* = "utf-8" */)
{
	ofstream fp;
	if (fp.open_write(filepath) == false)
	{
		logger_error("open %s error: %s", filepath, last_serror());
		return false;
	}

	if (to_charset == NULL)
		to_charset = "utf-8";

	filepath_ = dbuf_->dbuf_strdup(filepath);

	if (compose_header(fp, to_charset) == false)
		return false;

	return false;
}

} // namespace acl
