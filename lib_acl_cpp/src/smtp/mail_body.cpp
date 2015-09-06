#include "acl_stdafx.hpp"
#include "acl_cpp/mime/mime_base64.hpp"
#include "acl_cpp/mime/mime_code.hpp"
#include "acl_cpp/mime/mime_quoted_printable.hpp"
#include "acl_cpp/mime/mime_uucode.hpp"
#include "acl_cpp/mime/mime_xxcode.hpp"
#include "acl_cpp/smtp/mail_body.hpp"

namespace acl {

mail_body::mail_body()
	: text_encoded_(NULL)
	, html_encoded_(NULL)
	, charset_(32)
	, content_type_(32)
	, transfer_encoding_(32)
{
}

mail_body::~mail_body()
{
	if (text_encoded_ != &text_)
		delete text_encoded_;
	if (html_encoded_ != &html_)
		delete html_encoded_;
}

mail_body& mail_body::set_text(const char* text)
{
	text_ = text;
	return *this;
}

mail_body& mail_body::set_html(const char* html)
{
	html_ = html;
	return *this;
}

mail_body& mail_body::set_charset(const char* charset)
{
	charset_ = charset;
	return *this;
}

mail_body& mail_body::set_content_type(const char* content_type)
{
	content_type_ = content_type;
	return *this;
}

mail_body& mail_body::set_transfer_encoding(const char* transfer_encoding)
{
	transfer_encoding_ = transfer_encoding;
	return *this;
}

void mail_body::build()
{
	mime_code* coder;
	if (transfer_encoding_.compare("base64", false) == 0)
		coder = NEW mime_base64(true, true);
	else if (transfer_encoding_.compare("qp", false) == 0)
		coder = NEW mime_quoted_printable(true, true);
	else if (transfer_encoding_.compare("uucode", false) == 0)
		coder = NEW mime_uucode(true, true);
	else if (transfer_encoding_.compare("xxcode", false) == 0)
		coder = NEW mime_xxcode(true, true);
	else
		coder = NULL;

	if (coder == NULL)
	{
		text_encoded_ = &text_;
		html_encoded_ = &html_;
		return;
	}

	text_encoded_ = NEW string(text_.size() * 4 / 3 + 32);
	html_encoded_ = NEW string(html_.size() * 4 / 3 + 32);

	coder->encode_update(text_, (int) text_.size(), text_encoded_);
	coder->encode_finish(text_encoded_);

	coder->reset();
	coder->encode_update(html_, (int) html_.size(), html_encoded_);
	coder->encode_finish(html_encoded_);

	delete coder;
}

} // namespace acl
