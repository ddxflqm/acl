#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stdlib/string.hpp"

namespace acl {

class ACL_CPP_API mail_attach
{
public:
	mail_attach(const char* filepath, const char* content_type);
	~mail_attach();

	mail_attach& set_content_id(const char* id);

	const char* get_filepath() const
	{
		return filepath_.c_str();
	}

	const char* get_filename() const
	{
		return filename_.c_str();
	}

	const char* get_content_type() const
	{
		return content_type_.c_str();
	}

	const char* get_content_id() const
	{
		return content_id_.c_str();
	}

private:
	string filepath_;
	string filename_;
	string content_type_;
	string content_id_;
};

} // namespace acl
