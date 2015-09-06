#include "acl_stdafx.hpp"
#include "acl_cpp/smtp/mail_attach.hpp"

namespace acl {

mail_attach::mail_attach(const char* filepath, const char* content_type)
	: filepath_(filepath)
	, content_type_(content_type)
	, content_id_(64)
{
	filename_.basename(filepath);
}

mail_attach::~mail_attach()
{
}

mail_attach& mail_attach::set_content_id(const char* id)
{
	if (id && *id)
		content_id_.format("<%s>", id);
	return *this;
}

} // namespace acl
