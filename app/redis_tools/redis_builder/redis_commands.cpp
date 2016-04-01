#include "stdafx.h"
#include "redis_commands.h"

#define LIMIT	40

redis_commands::redis_commands(const char* addr, const char* passwd,
	int conn_timeout, int rw_timeout)
	: addr_(addr)
	, conn_timeout_(conn_timeout)
	, rw_timeout_(rw_timeout)
	, conn_(addr, conn_timeout, rw_timeout)
{
	conns_.set(addr_, conn_timeout_, rw_timeout_);
	conns_.set_all_slot(addr, 0);

	if (passwd && *passwd)
	{
		passwd_ = passwd;
		conn_.set_password(passwd);
		conns_.set_password("default", passwd);
	}
}

redis_commands::~redis_commands(void)
{
}

void redis_commands::help(void)
{
	printf("> keys pattern limit\r\n");
	printf("> get [:limit] parameter ...\r\n");
	printf("> getn parameter limit\r\n");
	printf("> hgetall parameter\r\n");
	printf("> remove pattern\r\n");
	printf("> type parameter ...\r\n");
	printf("> ttl parameter ...\r\n");
	printf("> dbsize\r\n");
}

const std::map<acl::string, acl::redis_node*>* redis_commands::get_masters(
	acl::redis& redis)
{
	const std::map<acl::string, acl::redis_node*>* masters =
		redis.cluster_nodes();
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
		if (buf.equal("quit", false) || buf.equal("exit", false)
			|| buf.equal("q", false))
		{
			printf("Bye!\r\n");
			break;
		}

		if (buf.empty() || buf.equal("help", false))
		{
			help();
			continue;
		}

		std::vector<acl::string>& tokens = buf.split2(" \t");
		acl::string& cmd = tokens[0];
		cmd.lower();

		if (cmd == "date")
			show_date();
		else if (cmd == "keys")
			get_keys(tokens);
		else if (cmd == "get")
			get(tokens);
		else if (cmd == "getn")
			getn(tokens);
		else if (cmd == "hgetall")
			hash_get(tokens);
		else if (cmd == "remove" || cmd == "rm")
			pattern_remove(tokens);
		else if (cmd == "type")
			check_type(tokens);
		else if (cmd == "ttl")
			check_ttl(tokens);
		else if (cmd == "dbsize")
			get_dbsize(tokens);
		else
			help();
	}
}

void redis_commands::show_date(void)
{
	char buf[256];
	acl::rfc822 rfc;
	rfc.mkdate_cst(time(NULL), buf, sizeof(buf));
	printf("Date: %s\r\n", buf);
}

void redis_commands::get_keys(const std::vector<acl::string>& tokens)
{
	if (tokens.size() < 2)
	{
		printf("> usage: keys parameter\r\n");
		return;
	}

	acl::redis redis(&conns_);
	const std::map<acl::string, acl::redis_node*>* masters =
		get_masters(redis);
	if (masters == NULL)
	{
		printf("no masters!\r\n");
		return;
	}

	const char* pattern = tokens[1].c_str();
	int  max;
	if (tokens.size() >= 3)
	{
		max = atoi(tokens[2].c_str());
		if (max < 0)
			max = 10;
	}
	else
		max = 10;

	int  n = 0;
	for (std::map<acl::string, acl::redis_node*>::const_iterator cit =
		masters->begin(); cit != masters->end(); ++cit)
	{
		n += get_keys(cit->second->get_addr(), pattern, max);
	}

	printf("-----keys %s: total count: %d----\r\n", tokens[1].c_str(), n);
}

int redis_commands::get_keys(const char* addr, const char* pattern, int max)
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
	acl::redis_key redis(&conn);
	if (redis.keys_pattern(pattern, &res) <= 0)
		return 0;

	int n = 0;
	for (std::vector<acl::string>::const_iterator cit = res.begin();
		cit != res.end(); ++cit)
	{
		printf("%s\r\n", (*cit).c_str());
		n++;
		if (n >= max)
			break;
	}

	printf("--- Addr: %s, Total: %d, Limit: %d, Show: %d ---\r\n",
		addr, (int) res.size(), max, n);

	return (int) res.size();
}

void redis_commands::getn(const std::vector<acl::string>& tokens)
{
	if (tokens.size() < 2)
	{
		printf("> usage: getn key limit\r\n");
		return;
	}

	const char* key = tokens[1].c_str();

	int count;
	if (tokens.size() >= 3)
	{
		count = atoi(tokens[2].c_str());
		if (count < 0)
			count = 10;
	}
	else
		count = 10;

	get(key, count);
}

void redis_commands::get(const std::vector<acl::string>& tokens)
{
	if (tokens.size() < 2) // xxx
		return;

	std::vector<acl::string>::const_iterator cit = tokens.begin();
	++cit;

	int  max;
	const char* ptr = (*cit).c_str();
	if (*ptr == ':' && *(ptr + 1) != 0)
	{
		ptr++;
		max = atoi(ptr);
		if (max < 0)
			max = 10;
		++cit;
	}
	else
		max = 10;

	for (; cit != tokens.end(); ++cit)
	{
		const char* key = (*cit).c_str();
		get(key, max);
	}
}

void redis_commands::get(const char* key, int max)
{
	acl::redis cmd(&conns_);
	acl::redis_key_t type = cmd.type(key);

	switch (type)
	{
		case acl::REDIS_KEY_NONE:
			break;
		case acl::REDIS_KEY_STRING:
			string_get(key);
			break;
		case acl::REDIS_KEY_HASH:
			hash_get(key, max);
			break;
		case acl::REDIS_KEY_LIST:
			list_get(key, max);
			break;
		case acl::REDIS_KEY_SET:
			set_get(key, max);
			break;
		case acl::REDIS_KEY_ZSET:
			zset_get(key, max);
			break;
		default:
			printf("%s: unknown type: %d\r\n", key, (int) type);
			break;
	}
}

void redis_commands::hash_get(const std::vector<acl::string>& tokens)
{
	if (tokens.empty())  // xxx
		return;

	std::vector<acl::string>::const_iterator cit = tokens.begin();
	for (++cit; cit != tokens.end(); ++cit)
	{
		hash_get((*cit).c_str(), 0);
		printf("-----------------------------------------------\r\n");
	}
}

void redis_commands::hash_get(const char* key, size_t max)
{
	std::map<acl::string, acl::string> res;
	acl::redis cmd(&conns_);

	if (cmd.hgetall(key, res) == false)
	{
		printf("hgetall error: %s, key: %s\r\n",
			cmd.result_error(), key);
		return;
	}

	size_t n = 0, count = res.size();
	printf("HASH KEY: %s, COUNT: %d, MAX: %d\r\n",
		key, (int) count, (int) max);

	for (std::map<acl::string, acl::string>::const_iterator cit2
		= res.begin(); cit2 != res.end(); ++cit2)
	{
		printf("%s: %s\r\n", cit2->first.c_str(),
			cit2->second.c_str());
		n++;
		if (max > 0 && n >= max)
			break;
	}
	printf("HASH KEY: %s, COUNT: %d, MAX: %d, SHOW: %d\r\n",
		key, (int) count, (int) max, (int) n);
}

void redis_commands::string_get(const std::vector<acl::string>& tokens)
{
	if (tokens.empty())  // xxx
		return;

	std::vector<acl::string>::const_iterator cit = tokens.begin();
	for (++cit; cit != tokens.end(); ++cit)
	{
		string_get((*cit).c_str());
		printf("-----------------------------------------------\r\n");
	}
}

void redis_commands::string_get(const char* key)
{
	acl::string buf;
	acl::redis cmd(&conns_);

	if (cmd.get(key, buf) == false)
	{
		printf("get error: %s, key: %s\r\n", cmd.result_error(), key);
		return;
	}

	printf("STRING KEY: %s, VALUE: %s\r\n", key, buf.c_str());
}

void redis_commands::list_get(const std::vector<acl::string>& tokens)
{
	if (tokens.empty())  // xxx
		return;

	std::vector<acl::string>::const_iterator cit = tokens.begin();
	for (++cit; cit != tokens.end(); ++cit)
	{
		list_get((*cit).c_str(), 0);
		printf("-----------------------------------------------\r\n");
	}
}

void redis_commands::list_get(const char* key, size_t max)
{
	acl::string buf;
	acl::redis cmd(&conns_);

	int len = cmd.llen(key), count = len;
	if (len < 0)
	{
		printf("llen error: %s, key: %s\r\n", cmd.result_error(), key);
		return;
	}
	if (len > LIMIT)
	{
		printf("Do you show all %d elements for key %s ? [y/n] ",
			len, key);
		fflush(stdout);

		acl::stdin_stream in;
		if (in.gets(buf) == false || !buf.equal("y", false))
			return;
	}

	if (max > 0 && max > (size_t) len)
		len = (int) max;

	printf("LIST KEY: %s, COUNT: %d, MAX: %d, SHOW: %d\r\n",
		key, count, (int) max, len);

	for (int i = 0; i < len; i++)
	{
		buf.clear();
		cmd.clear(false);
		if (cmd.lindex(key, i, buf) == false)
		{
			printf("lindex error: %s, key: %s, idx: %d\r\n",
				cmd.result_error(), key, i);
			return;
		}
		printf("%s\r\n", buf.c_str());
	}

	printf("LIST KEY: %s, COUNT: %d, MAX: %d, SHOW: %d\r\n",
		key, count, (int) max, len);
}

void redis_commands::set_get(const std::vector<acl::string>& tokens)
{
	if (tokens.empty())  // xxx
		return;

	std::vector<acl::string>::const_iterator cit = tokens.begin();
	for (++cit; cit != tokens.end(); ++cit)
	{
		set_get((*cit).c_str(), 0);
		printf("-----------------------------------------------\r\n");
	}
}

void redis_commands::set_get(const char* key, size_t max)
{
	acl::string buf;
	acl::redis cmd(&conns_);
	int len = cmd.scard(key), count = len;
	if (len < 0)
	{
		printf("scard error: %s, key: %s\r\n", cmd.result_error(), key);
		return;
	}
	if (len > LIMIT)
	{
		printf("Do you show all %d elements for key %s ? [y/n] ",
			len, key);
		fflush(stdout);

		acl::stdin_stream in;
		if (in.gets(buf) == false || !buf.equal("y", false))
			return;
	}

	if (max > 0 && max > (size_t) len)
		len = (int) max;

	printf("SET KEY: %s, COUNT: %d\r\n", key, len);

	for (int i = 0; i < len; i++)
	{
		buf.clear();
		cmd.clear(false);
		if (cmd.spop(key, buf) == false)
		{
			printf("spop error: %s, key: %s, idx: %d\r\n",
				cmd.result_error(), key, i);
			return;
		}
		printf("%s\r\n", buf.c_str());
	}

	printf("SET KEY: %s, COUNT: %d, MAX: %d, SHOW: %d\r\n",
		key, count, (int) max, len);
}

void redis_commands::zset_get(const std::vector<acl::string>& tokens)
{
	if (tokens.empty())  // xxx
		return;

	std::vector<acl::string>::const_iterator cit = tokens.begin();
	for (++cit; cit != tokens.end(); ++cit)
	{
		zset_get((*cit).c_str(), 0);
		printf("-----------------------------------------------\r\n");
	}
}

void redis_commands::zset_get(const char* key, size_t max)
{
	acl::string buf;
	acl::redis cmd(&conns_);
	int len = cmd.zcard(key), count = len;
	if (len < 0)
	{
		printf("zcard error: %s, key: %s\r\n", cmd.result_error(), key);
		return;
	}
	if (len > LIMIT)
	{
		printf("Do you show all %d elements for key %s ? [y/n] ",
			len, key);
		fflush(stdout);

		acl::stdin_stream in;
		if (in.gets(buf) == false || !buf.equal("y", false))
			return;
	}

	if (max > 0 && max > (size_t) len)
		len = (int) max;

	std::vector<acl::string> res;
	printf("ZSET KEY: %s, COUNT: %d, MAX: %d, SHOW: %d\r\n",
		key, count, (int) max, len);

	for (int i = 0; i < len; i++)
	{
		buf.clear();
		cmd.clear(false);
		res.clear();
		int ret = cmd.zrange(key, i, i + 1, &res);
		if (ret < 0)
		{
			printf("zrange error: %s, key: %s, idx: %d\r\n",
				cmd.result_error(), key, i);
			return;
		}

		if (res.empty())
			continue;

		for (std::vector<acl::string>::const_iterator cit
			= res.begin(); cit != res.end(); ++cit)
		{
			printf("%s\r\n", (*cit).c_str());
		}
	}

	printf("ZSET KEY: %s, COUNT: %d, MAX: %d, SHOW: %d\r\n",
		key, count, (int) max, len);
}

void redis_commands::pattern_remove(const std::vector<acl::string>& tokens)
{
	if (tokens.size() < 2)
	{
		printf("> usage: pattern_remove pattern\r\n");
		return;
	}

	const char* pattern = tokens[1].c_str();

	acl::redis redis(&conns_);
	const std::map<acl::string, acl::redis_node*>* masters =
		get_masters(redis);
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

		redis.clear(false);
		redis.keys_pattern(pattern, &res);

		printf("addr: %s, pattern: %s, total: %d\r\n",
			addr, pattern, (int) res.size());

		if (res.empty())
			continue;

		printf("Do you want to delete them all in %s ? [y/n]: ", addr);
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
	acl::redis cmd(&conns_);

	int  deleted = 0, error = 0, notfound = 0;

	for (std::vector<acl::string>::const_iterator cit = keys.begin();
		cit != keys.end(); ++cit)
	{
		cmd.clear(false);
		int ret = cmd.del_one((*cit).c_str());
		if (ret < 0)
		{
			printf("del_one error: %s, key: %s\r\n",
				cmd.result_error(), (*cit).c_str());
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

void redis_commands::check_type(const std::vector<acl::string>& tokens)
{
	acl::redis cmd(&conns_);
	std::vector<acl::string>::const_iterator cit = tokens.begin();
	++cit;
	for (; cit != tokens.end(); ++cit)
	{
		cmd.clear(false);
		const char* key = (*cit).c_str();
		acl::redis_key_t type = cmd.type(key);
		switch (type)
		{
		case acl::REDIS_KEY_NONE:
			printf("%s: NONE\r\n", key);
			break;
		case acl::REDIS_KEY_STRING:
			printf("%s: STRING\r\n", key);
			break;
		case acl::REDIS_KEY_HASH:
			printf("%s: HASH\r\n", key);
			break;
		case acl::REDIS_KEY_LIST:
			printf("%s: LIST\r\n", key);
			break;
		case acl::REDIS_KEY_SET:
			printf("%s: SET\r\n", key);
			break;
		case acl::REDIS_KEY_ZSET:
			printf("%s: ZSET\r\n", key);
			break;
		default:
			printf("%s: UNKNOWN\r\n", key);
			break;
		}
	}
}

void redis_commands::check_ttl(const std::vector<acl::string>& tokens)
{
	acl::redis cmd(&conns_);
	std::vector<acl::string>::const_iterator cit = tokens.begin();
	++cit;
	for (; cit != tokens.end(); ++cit)
	{
		cmd.clear(false);
		const char* key = (*cit).c_str();
		int ttl = cmd.ttl(key);
		printf("%s: %d seconds\r\n", key, ttl);
	}
}

void redis_commands::get_dbsize(const std::vector<acl::string>&)
{
	acl::redis redis(&conns_);
	const std::map<acl::string, acl::redis_node*>* masters =
		get_masters(redis);
	if (masters == NULL)
	{
		printf("no masters!\r\n");
		return;
	}

	int total = 0;

	for (std::map<acl::string, acl::redis_node*>::const_iterator cit =
		masters->begin(); cit != masters->end(); ++cit)
	{
		const char* addr = cit->second->get_addr();
		if (addr == NULL || *addr == 0)
		{
			printf("addr NULL\r\n");
			continue;
		}

		acl::redis_client conn(addr, conn_timeout_, rw_timeout_);
		if (passwd_.empty() == false)
			conn.set_password(passwd_);
		acl::redis cmd(&conn);
		int n = cmd.dbsize();
		printf("----- ADDR: %s, DBSIZE: %d -----\r\n", addr, n);
		if (n > 0)
			total += n;
	}

	printf("---- Total DBSIZE: %d -----\r\n", total);
}
