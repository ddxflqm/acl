#include "stdafx.h"
#include "redis_commands.h"

redis_commands::redis_commands(const char* addr)
	: addr_(addr)
{
}

redis_commands::~redis_commands(void)
{
}

void redis_commands::help(void)
{
	printf("> keys parameter\r\n");
}

void redis_commands::run(void)
{
	acl::string buf;
	acl::stdin_stream in;

	while (!in.eof())
	{
		printf("> ");
		fflush(stdout);

		if (in.gets(buf) == false)
			break;
		if (buf.equal("quit", false) || buf.equal("exit", false))
		{
			printf("Bye!\r\n");
			break;
		}

		if (buf.equal("help", false))
		{
			help();
			continue;
		}

		std::vector<acl::string>& tokens = buf.split2(" \t");
		acl::string& cmd = tokens[0];
		cmd.lower();
		if (cmd == "keys")
			keys(tokens);
		else
			help();
	}
}

void redis_commands::keys(const std::vector<acl::string>& tokens)
{
	if (tokens.size() < 2)
	{
		printf("usage: keys parameter\r\n");
		return;
	}
}
