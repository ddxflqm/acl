#include "stdafx.h"
#include "redis_commands.h"

redis_commands::redis_commands(const char* addr, const char* passwd,
	int conn_timeout, int rw_timeout)
	: addr_(addr)
	, conn_timeout_(conn_timeout)
	, rw_timeout_(rw_timeout)
	, conn_(addr, conn_timeout, rw_timeout)
	, redis_(&conn_)
{
	conns_.set(addr_, conn_timeout_, rw_timeout_);
	if (passwd && *passwd)
	{
		passwd_ = passwd;
		conn_.set_password(passwd);
		conns_.set_password("default", passwd);
		conns_.set_all_slot(addr, 0);
	}
}

redis_commands::~redis_commands(void)
{
}

void redis_commands::help(void)
{
	printf("> keys pattern\r\n");
	printf("> hgetall parameter\r\n");
	printf("> pattern_remove pattern\r\n");
}

const std::map<acl::string, acl::redis_node*>* redis_commands::get_masters(void)
{
	redis_.clear(false);
	const std::map<acl::string, acl::redis_node*>* masters =
		redis_.cluster_nodes();
	if (masters == NULL)
		printf("masters NULL\r\n");
	return masters;
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
			get_keys(tokens);
		else if (cmd == "hgetall")
			hgetall(tokens);
		else if (cmd == "pattern_remove")
			pattern_remove(tokens);
		else
			help();
	}
}

void redis_commands::get_keys(const std::vector<acl::string>& tokens)
{
	if (tokens.size() < 2)
	{
		printf("> usage: keys parameter\r\n");
		return;
	}

	const std::map<acl::string, acl::redis_node*>* masters = get_masters();
	if (masters == NULL)
	{
		printf("no masters!\r\n");
		return;
	}

	int  n = 0;
	for (std::map<acl::string, acl::redis_node*>::const_iterator cit =
		masters->begin(); cit != masters->end(); ++cit)
	{
		n += get_keys(cit->second->get_addr(), tokens[1]);
	}

	printf("-----keys %s: total count: %d----\r\n", tokens[1].c_str(), n);
}

int redis_commands::get_keys(const char* addr, const char* pattern)
{
	if (addr == NULL || *addr == 0)
	{
		printf("addr NULL\r\nEnter any key to continue ...\r\n");
		getchar();
		return 0;
	}

	acl::redis_client conn(addr, conn_timeout_, rw_timeout_);
	if (passwd_.empty() == false)
		conn.set_password(passwd_);

	std::vector<acl::string> res;
	acl::redis_key cmd(&conn);
	if (cmd.keys_pattern(pattern, &res) <= 0)
		return 0;

	if (res.size() >= 40)
	{
		printf("Total: %d, do you want to show them all[y/n]?\r\n",
			(int) res.size());
		acl::stdin_stream in;
		acl::string buf;
		if (in.gets(buf) == false || !buf.equal("y", false))
			return (int) res.size();
	}

	for (std::vector<acl::string>::const_iterator cit = res.begin();
		cit != res.end(); ++cit)
	{
		printf("%s\r\n", (*cit).c_str());
	}

	return (int) res.size();
}

void redis_commands::hgetall(const std::vector<acl::string>& tokens)
{
	if (tokens.size() < 2)
	{
		printf("> usage: hgetall key\r\n");
		return;
	}

	const char* key = tokens[1].c_str();
	redis_.clear(false);
	redis_.set_cluster(&conns_, 0);

	std::map<acl::string, acl::string> res;

	if (redis_.hgetall(key, res) == false)
	{
		printf("hgetall error: %s, key: %s\r\n",
			redis_.result_error(), key);
		return;
	}

	printf("key: %s\r\n", key);
	for (std::map<acl::string, acl::string>::const_iterator cit
		= res.begin(); cit != res.end(); ++cit)
	{
		printf("%s: %s\r\n", cit->first.c_str(), cit->second.c_str());
	}
}

void redis_commands::pattern_remove(const std::vector<acl::string>& tokens)
{
	if (tokens.size() < 2)
	{
		printf("> usage: pattern_remove pattern\r\n");
		return;
	}

	const char* pattern = tokens[1].c_str();

	const std::map<acl::string, acl::redis_node*>* masters = get_masters();
	if (masters == NULL)
	{
		printf("no masters!\r\n");
		return;
	}

	acl::stdin_stream in;
	acl::string buf;

	int deleted = 0;
	std::vector<acl::string> res;

	for (std::map<acl::string, acl::redis_node*>::const_iterator cit =
		masters->begin(); cit != masters->end(); ++cit)
	{
		const char* addr = cit->second->get_addr();
		if (addr == NULL || *addr == 0)
		{
			printf("addr NULL, skip it\r\n");
			continue;
		}

		acl::redis_client conn(addr, conn_timeout_, rw_timeout_);
		if (!passwd_.empty())
			conn.set_password(passwd_);

		redis_.clear(false);
		redis_.set_client(&conn);
		redis_.keys_pattern(pattern, &res);

		printf("addr: %s, pattern: %s, total: %d\r\n",
			addr, pattern, (int) res.size());

		printf("Do you want to delete them all in %s [y/n]?", addr);
		fflush(stdout);

		if (in.gets(buf) && buf.equal("y", false))
		{
			int ret = remove(res);
			if (ret > 0)
				deleted += ret;
		}
	}

	printf("pattern: %s, total: %d\r\n", pattern, deleted);
}

int redis_commands::remove(const std::vector<acl::string>& keys)
{
	redis_.set_cluster(&conns_, 0);

	int  deleted = 0, error = 0, notfound = 0;

	for (std::vector<acl::string>::const_iterator cit = keys.begin();
		cit != keys.end(); ++cit)
	{
		redis_.clear(false);
		int ret = redis_.del_one((*cit).c_str());
		if (ret < 0)
		{
			printf("del_one error: %s, key: %s\r\n",
				redis_.result_error(), (*cit).c_str());
			error++;
		}
		else if (ret == 0)
		{
			printf("not exist, key: %s\r\n", (*cit).c_str());
			notfound++;
		}
		else
		{
			printf("Delete ok, key: %s\r\n", (*cit).c_str());
			deleted++;
		}
	}

	printf("Remove over, deleted: %d, error: %d, not found: %d\r\n",
		deleted, error, notfound);
	return deleted;
}
