#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fiber/lib_fiber.hpp"
#include "user_client.h"

#define	STACK_SIZE	128000

static int __rw_timeout = 0;

static std::map<acl::string, user_client*> __users;

static void remove_user(user_client* uc)
{
	const char* name = uc->get_name();
	if (name == NULL || *name == 0)
	{
		printf("no name!\r\n");
		return;
	}

	std::map<acl::string, user_client*>::iterator it = __users.find(name);
	if (it == __users.end())
		printf("not exist, name: %s\r\n", name);
	else
	{
		__users.erase(it);
		printf("delete user ok, name: %s\r\n", name);
	}
}

static bool client_login(user_client* uc)
{
	acl::string buf;
	if (uc->get_stream().gets(buf) == false)
	{
		printf("gets error %s\r\n", acl::last_serror());
		if (errno == ETIMEDOUT)
			uc->get_stream().write("Login read timeout\r\n");
		return false;
	}

	std::vector<acl::string>& tokens = buf.split2("|");
	if (tokens.size() < 2)
	{
		acl::string tmp;
		tmp.format("invalid argc: %d < 2\r\n", (int) tokens.size());
		printf("%s", tmp.c_str());

		return uc->get_stream().write(tmp) != -1;
	}

	acl::string msg;

	const acl::string& name = tokens[1];
	std::map<acl::string, user_client*>::iterator it = __users.find(name);
	if (it == __users.end())
	{
		__users[name] = uc;
		uc->set_name(name);
		msg.format("user %s login ok\r\n", name.c_str());
	}
	else
		msg.format("user %s already login\r\n", name.c_str());

	printf("%s", msg.c_str());

	return uc->get_stream().write(msg) != -1;
}

static void client_logout(user_client* client)
{
	if (client->already_login())
		remove_user(client);

	if (client->is_reading())
	{
		printf("%s(%d): user: %s, shutdown\r\n",
			__FUNCTION__, __LINE__, client->get_name());
		client->shutdown();
	}
	if (client->is_waiting())
	{
		printf("%s(%d): user: %s, notify logout\r\n",
			__FUNCTION__, __LINE__, client->get_name());
		client->notify(MT_LOGOUT);
	}

	if (!client->is_reading() && !client->is_waiting())
		client->notify_exit();
}

static bool client_chat(user_client* uc, std::vector<acl::string>& tokens)
{
	if (tokens.size() < 3)
	{
		printf("invalid argc: %d < 3\r\n", (int) tokens.size());
		return true;
	}

	const acl::string& to = tokens[1];
	const acl::string& msg = tokens[2];

	std::map<acl::string, user_client*>::iterator it = __users.find(to);
	if (it == __users.end())
	{
		acl::string tmp;
		tmp.format("from user: %s, to user: %s not exist\r\n",
			uc->get_name(), to.c_str());
		printf("%s", tmp.c_str());

		return uc->get_stream().write(tmp) != -1;
	}
	else
	{
		it->second->push(msg);
		it->second->notify(MT_MSG);
		return true;
	}
}

static void fiber_reader(user_client* client)
{
	acl::socket_stream& conn = client->get_stream();
	conn.set_rw_timeout(30);

	if (client_login(client) == false)
	{
		client_logout(client);
		return;
	}

	conn.set_rw_timeout(1);

	client->set_reader();
	client->set_reading(true);
	acl::string buf;

	while (true)
	{
		bool ret = conn.gets(buf);
		if (ret == false)
		{
			printf("%s(%d): user: %s, gets error %s, fiber: %d\r\n",
				__FUNCTION__, __LINE__, client->get_name(),
				acl::last_serror(), acl_fiber_self());

			if (client->existing())
			{
				printf("----existing now----\r\n");
				break;
			}

			if (errno == ETIMEDOUT)
				printf("ETIMEDOUT\r\n");
			else if (errno == EAGAIN)
				printf("EAGAIIN\r\n");
			else {
				printf("gets error: %d, %s\r\n",
					errno, acl::last_serror());
				break;
			}

			continue;
		}

		std::vector<acl::string>& tokens = buf.split2("|");
		if (tokens[0] != "chat")
		{
			printf("invalid data: %s\r\n", buf.c_str());
			continue;
		}

		if (client_chat(client, tokens) == false)
			break;
	}

	client->set_reading(false);
	printf(">>%s(%d), user: %s, logout\r\n", __FUNCTION__, __LINE__,
		client->get_name());
	client_logout(client);
}

static bool client_flush(user_client* client)
{
	acl::socket_stream& conn = client->get_stream();
	acl::string* msg;

	bool ret = true;

	while ((msg = client->pop()) != NULL)
	{
		if (conn.write(*msg) == -1)
		{
			printf("flush to user: %s error\r\n",
				client->get_name());
			delete msg;
			ret = false;
			break;
		}
	}

	return ret;
}

static void fiber_writer(user_client* client)
{
	client->set_waiter();
	client->set_waiting(true);

	while (true)
	{
		int mtype;
		client->wait(mtype);
		if (client_flush(client) == false)
		{
			printf("%s(%d), user: %s, flush error\r\n",
				__FUNCTION__, __LINE__, client->get_name());
			break;
		}
		if (mtype == MT_LOGOUT)
		{
			printf("%s(%d), user: %s, MT_LOGOUT\r\n",
				__FUNCTION__, __LINE__, client->get_name());
			break;
		}
	}

	client->set_waiting(false);
	printf(">>%s(%d), user: %s, logout\r\n", __FUNCTION__, __LINE__,
		client->get_name());
	client_logout(client);
}

static void fiber_client(acl::socket_stream* conn)
{
	user_client* client = new user_client(*conn);
	go[&] {
		fiber_reader(client);
	};

	go[&] {
		fiber_writer(client);
	};

	client->wait_exit();
	printf("----- client %s exit now -----\r\n", client->get_name());
	delete client;
	delete conn;
}

static void fiber_accept(acl::server_socket& ss)
{
	while (true)
	{
		acl::socket_stream* conn = ss.accept();
		if (conn == NULL)
		{
			printf("accept error %s\r\n", acl::last_serror());
			break;
		}

		go[&] {
			fiber_client(conn);
		};
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s listen_addr\r\n"
		" -r rw_timeout\r\n" , procname);
}

int main(int argc, char *argv[])
{
	char addr[64];
	int  ch;

	acl::log::stdout_open(true);
	snprintf(addr, sizeof(addr), "%s", "127.0.0.1:9002");

	while ((ch = getopt(argc, argv, "hs:r:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			snprintf(addr, sizeof(addr), "%s", optarg);
			break;
		case 'r':
			__rw_timeout = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::server_socket ss;
	if (ss.open(addr) == false)
	{
		printf("listen %s error %s\r\n", addr, acl::last_serror());
		return 1;
	}

	printf("listen %s ok\r\n", addr);

	go[&] {
		fiber_accept(ss);
	};

	acl::fiber::schedule();

	return 0;
}
