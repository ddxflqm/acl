// xml.cpp : 定义控制台应用程序的入口点。
//
#include "stdafx.h"
#include <sys/time.h>

#define	CHECK(x, y) do {  \
	if ((x))  \
		printf("OK, reset to %d\r\n", y);  \
	else  \
		printf("Error, rreset to %d\r\n", y);  \
} while(0)

int main()
{
	acl::log::stdout_open(true);

	acl::dbuf_pool* dbuf = new acl::dbuf_pool;

	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(2048);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(10240);

	CHECK(dbuf->dbuf_reset(32999), 32999);
	CHECK(dbuf->dbuf_reset(22999), 22999);
	CHECK(dbuf->dbuf_reset(12999), 12999);
	CHECK(dbuf->dbuf_reset(8192), 8192);
	CHECK(dbuf->dbuf_reset(819), 819);
	CHECK(dbuf->dbuf_reset(19), 19);
	CHECK(dbuf->dbuf_reset(9), 9);
	CHECK(dbuf->dbuf_reset(0), 9);

	dbuf->destroy();
	return 0;
}
