#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include "http_servlet.h"

#define	 STACK_SIZE	16000

static void http_server(FIBER *, void *ctx)
{
	acl::socket_stream *conn = (acl::socket_stream *) ctx;

	printf("start one http_server\r\n");

	acl::memcache_session session("127.0.0.1:11211");
	http_servlet servlet(conn, &session);
	servlet.setLocalCharset("gb2312");

	while (true)
	{
		if (servlet.doRun() == false)
			break;
	}

	delete conn;
}

static void fiber_accept(FIBER *, void *)
{
	acl::string addr = "127.0.0.1:9001";
	acl::server_socket server;

	if (server.open(addr) == false)
	{
		printf("open %s error\r\n", addr.c_str());
		exit (1);
	}
	else
		printf("open %s ok\r\n", addr.c_str());

	while (true)
	{
		acl::socket_stream* client = server.accept();
		if (client == NULL)
		{
			printf("accept failed: %s\r\n", acl::last_serror());
			break;
		}

		printf("accept one: %d\r\n", client->sock_handle());
		fiber_create(http_server, client, STACK_SIZE);
	}

	exit (0);
}

int main(void)
{
	acl::acl_cpp_init();
	fiber_create(fiber_accept, NULL, STACK_SIZE);
	fiber_schedule();
}
