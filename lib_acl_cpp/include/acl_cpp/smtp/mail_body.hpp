#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stdlib/string.hpp"

namespace acl {

class ACL_CPP_API mail_body
{
public:
	mail_body();
	~mail_body();

	mail_body& set_text(const char* text);
	mail_body& set_html(const char* html);
	mail_body& set_charset(const char* charset);
	mail_body& set_content_type(const char* content_type);
	mail_body& set_transfer_encoding(const char* transfer_encoding);

	const char* get_text() const
	{
		return text_encoded_ ? text_encoded_->c_str() : "";
	}

	const char* get_html() const
	{
		return html_encoded_ ? html_encoded_->c_str() : "";
	}

	const char* get_charset() const
	{
		return charset_.c_str();
	}

	const char* get_content_type() const
	{
		return content_type_.c_str();
	}

	const char* get_transfer_encoding() const
	{
		return transfer_encoding_.c_str();
	}

	void build();

private:
	string  text_;
	string  html_;
	string* text_encoded_;
	string* html_encoded_;
	string  charset_;
	string  content_type_;
	string  transfer_encoding_;
};

} // namespace acl

