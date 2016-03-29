#pragma once

class redis_commands
{
public:
	redis_commands(const char* addr);
	~redis_commands(void);

	void run(void);

private:
	acl::string addr_;

	void help(void);
	void keys(const std::vector<acl::string>& tokens);
};
