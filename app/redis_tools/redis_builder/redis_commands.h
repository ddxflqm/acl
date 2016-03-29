#pragma once

class redis_commands
{
public:
	redis_commands(const char* addr, const char* passwd,
		int conn_timeout, int rw_timeout);
	~redis_commands(void);

	void run(void);

private:
	acl::string addr_;
	acl::string passwd_;
	int conn_timeout_;
	int rw_timeout_;
	acl::redis_client conn_;
	acl::redis redis_;

	void help(void);
	const std::map<acl::string, acl::redis_node*>* get_masters(void);
	void keys(const std::vector<acl::string>& tokens);
	int keys(const char* addr, const char* pattern);
};
