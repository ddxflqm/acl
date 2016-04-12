#pragma once

class redis_cmdump
{
public:
	redis_cmdump(const char* addr, int conn_timeout, int rw_timeout,
		const char* passwd, bool use_slave);
	~redis_cmdump(void);

	void saveto(const char* filepath, bool dump_all);

private:
	acl::string addr_;
	int conn_timeout_;
	int rw_timeout_;
	acl::string passwd_;
	bool use_slave_;

	void get_masters(std::vector<acl::string>& addrs);
};
