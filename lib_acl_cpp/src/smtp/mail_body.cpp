#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/mime/mime_base64.hpp"
#include "acl_cpp/mime/mime_code.hpp"
#include "acl_cpp/mime/mime_quoted_printable.hpp"
#include "acl_cpp/mime/mime_uucode.hpp"
#include "acl_cpp/mime/mime_xxcode.hpp"
#include "acl_cpp/smtp/mail_message.hpp"
#include "acl_cpp/smtp/mail_body.hpp"

namespace acl {

mail_body::mail_body(const char* charset /* = "utf-8" */,
	const char* encoding /* = "base64" */)
	: charset_(charset)
	, transfer_encoding_(encoding)
	, is_alternative_(false)
{
	if (transfer_encoding_.compare("base64", false) == 0)
		coder_ = NEW mime_base64(true, true);
	else if (transfer_encoding_.compare("qp", false) == 0)
		coder_ = NEW mime_quoted_printable(true, true);
	else if (transfer_encoding_.compare("uucode", false) == 0)
		coder_ = NEW mime_uucode(true, true);
	else if (transfer_encoding_.compare("xxcode", false) == 0)
		coder_ = NEW mime_xxcode(true, true);
	else
		coder_ = NULL;
}

mail_body::~mail_body()
{
	delete coder_;
}

bool mail_body::build(const char* text, size_t tlen,
	const char* html, size_t hlen)
{
	int n = 0;
	if (text && tlen > 0)
	{
		n++;
		content_type_.format("text/plain");
	}
	if (html && hlen > 0)
	{
		n++;
		content_type_.format("text/html");
	}

	if (n == 0)
	{
		logger_error("text and html are all NULL");
		return false;
	}
	if (n == 2)
	{
		content_type_.format("multipart/alternative");
		is_alternative_ = true;
	}

	buf_.format("Content-Type: %s\r\n", content_type_.c_str());
	if (n == 1)
	{
		buf_.format_append("\tcharset=\"%s\"\r\n", charset_.c_str());
		buf_.format_append("Content-Transfer-Encoding: %s\r\n\r\n",
			transfer_encoding_.c_str());
		const char* ptr;
		int   len;
		if (text && tlen > 0)
		{
			ptr = text;
			len = (int) tlen;
		}
		else if (html && hlen > 0)
		{
			ptr = html;
			len = (int) hlen;
		}
		else
		{
			logger_error("text and html are all null");
			return false;
		}

		coder_->encode_update(ptr, len, &buf_);
		coder_->encode_finish(&buf_);
	}
	else
	{
		mail_message::create_boundary("0002", boundary_);

		buf_.format_append("\tboundary=\"%s\"\r\n\r\n\r\n",
			boundary_.c_str());

		buf_.format_append("--%s\r\n", boundary_.c_str());
		buf_.format_append("Content-Type: text/plain;\r\n");
		buf_.format_append("\tcharset=\"%s\"\r\n", charset_.c_str());
		buf_.format_append("Content-Transfer-Encoding: %s\r\n\r\n",
			transfer_encoding_.c_str());
		coder_->encode_update(text, (int) tlen, &buf_);
		coder_->encode_finish(&buf_);
		buf_.append("\r\n\r\n");

		buf_.format_append("--%s\r\n", boundary_.c_str());
		buf_.format_append("Content-Type: text/html;\r\n");
		buf_.format_append("\tcharset=\"%s\"\r\n", charset_.c_str());
		buf_.format_append("Content-Transfer-Encoding: %s\r\n\r\n",
			transfer_encoding_.c_str());
		coder_->encode_update(html, (int) hlen, &buf_);
		coder_->encode_finish(&buf_);
		buf_.append("\r\n\r\n");

		buf_.format_append("--%s--\r\n", boundary_.c_str());
	}

	return true;
}

} // namespace acl
