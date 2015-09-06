#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stdlib/string.hpp"

namespace acl {

class mime_code;

class ACL_CPP_API mail_body
{
public:
	mail_body(const char* charset = "utf-8",
		const char* encoding = "base64");
	~mail_body();

	bool build(const char* text, size_t tlen,
		const char* html, size_t hlen);

	const string& get_body() const
	{
		return buf_;
	}

	const string& get_content_type(bool& is_alternative) const
	{
		is_alternative = is_alternative_;
		return content_type_;
	}

private:
	string  charset_;
	string  content_type_;
	string  transfer_encoding_;
	mime_code* coder_;
	string  buf_;
	string  boundary_;
	bool    is_alternative_;
};

} // namespace acl

