#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include <vector>

namespace acl {

class dbuf_pool;
struct rfc822_addr;
class ofstream;

class ACL_CPP_API mail_message
{
public:
	mail_message();
	~mail_message();

	mail_message& set_auth(const char* user, const char* pass);
	mail_message& set_from(const char* from, const char* name = NULL);
	mail_message& set_sender(const char* sender, const char* name = NULL);
	mail_message& set_replyto(const char* replyto, const char* name = NULL);
	mail_message& add_recipients(const char* recipients);
	mail_message& add_to(const char* to);
	mail_message& add_cc(const char* cc);
	mail_message& add_bcc(const char* bcc);
	mail_message& set_subject(const char* subject,
		const char* charset = "UTF-8");
	mail_message& add_header(const char* name, const char* value);
	mail_message& set_body(const char* body);
	mail_message& add_attachment(const char* filepath);

	bool compose(const char* filepath, const char* to_charset = "utf-8");

	const char* get_auth_user() const
	{
		return auth_user_;
	}

	const char* get_auth_pass() const
	{
		return auth_pass_;
	}

	const rfc822_addr* get_from() const
	{
		return from_;
	}

	const rfc822_addr* get_sender() const
	{
		return sender_;
	}

	const rfc822_addr* get_replyto() const
	{
		return replyto_;
	}

	const std::vector<rfc822_addr*>& get_to() const
	{
		return to_list_;
	}

	const std::vector<rfc822_addr*>& get_cc() const
	{
		return cc_list_;
	}

	const std::vector<rfc822_addr*>& get_bcc() const
	{
		return bcc_list_;
	}

	const std::vector<rfc822_addr*>& get_recipients() const
	{
		return recipients_;
	}

	const char* get_filepath() const
	{
		return filepath_;
	}

private:
	dbuf_pool* dbuf_;

	char* auth_user_;
	char* auth_pass_;
	rfc822_addr* from_;
	rfc822_addr* sender_;
	rfc822_addr* replyto_;
	std::vector<rfc822_addr*> to_list_;
	std::vector<rfc822_addr*> cc_list_;
	std::vector<rfc822_addr*> bcc_list_;
	std::vector<rfc822_addr*> recipients_;
	char* subject_;
	char* charset_;
	std::vector<std::pair<char*, char*> > headers_;
	std::vector<char*> attachments_;
	string boundary_;
	char* body_;
	size_t body_len_;
	char* filepath_;

	void add_addrs(const char* in, std::vector<rfc822_addr*>& out);
	bool append_addr(ofstream& fp, const char* name,
		const rfc822_addr* addr, const char* to_charset);
	void create_boundary();
	bool append_message_id(ofstream& fp);

	bool compose_header(ofstream& fp, const char* to_charset);
	bool append_multipart(ofstream& fp, const char* to_charset);
};

} // namespace acl
