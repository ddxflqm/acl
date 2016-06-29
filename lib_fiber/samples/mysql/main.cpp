#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

static int __max_fibers = 2;
static int __cur_fibers = 2;

class myfiber : public acl::fiber
{
public:
	myfiber(acl::db_pool& dbp, const char* oper, int count)
		: dbp_(dbp), oper_(oper), count_(count) {}
	~myfiber(void) {}

protected:
	// @override
	void run(void)
	{
		printf("fiber-%d-%d running\r\n", get_id(), acl::fiber::self());

		for (int i = 0; i < count_; i++)
		{
			acl::db_handle* db = dbp_.peek_open();
			if (db == NULL)
			{
				printf("peek db connection error\r\n");
				break;
			}

			if (oper_ == "insert")
				db_insert(*db, i);
			else if (oper_ == "get")
				db_get(*db, i);
			else
				printf("unknown oper: %s\r\n", oper_.c_str());

			dbp_.put(db);
		}

		delete this;

		if (--__cur_fibers == 0)
		{
			printf("All fibers Over\r\n");
			acl::fiber::stop();
		}
	}

private:
	bool db_insert(acl::db_handle& db, int n)
	{
		acl::query query;
		query.create_sql("insert into group_tbl(group_name, uvip_tbl,"
			" update_date) values(:group, :test, :date)")
			.set_format("group", "group:%d", n)
			.set_parameter("test", "test")
			.set_date("date", time(NULL), "%Y-%m-%d");
		if (db.exec_update(query) == false)
		{
			printf("exec_update error: %s\r\n", db.get_error());
			return false;
		}

		return true;
	}

	bool db_get(acl::db_handle& db, int n)
	{
		acl::query query;
		query.create_sql("select * from group_tbl"
			" where group_name=:group"
			" and uvip_tbl=:test")
			.set_format("group", "group:%d", n)
			.set_format("test", "test");
		if (db.exec_select(query) == false)
		{
			printf("exec_select error: %s\r\n", db.get_error());
			return false;
		}

		const acl::db_rows* result = db.get_result();
		if (result)
		{
			const std::vector<acl::db_row*>& rows =
				result->get_rows();
			for (size_t i = 0; i < rows.size(); i++)
			{
				if (n > 100)
					continue;
				const acl::db_row* row = rows[i];
				for (size_t j = 0; j < row->length(); j++)
					printf("%s, ", (*row)[j]);
				printf("\r\n");
			}
		}

		db.free_result();
		return true;
	}

private:
	acl::db_pool& dbp_;
	acl::string oper_;
	int count_;
};

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -c fibers_count\r\n"
		" -n oper_count\r\n"
		" -o db_oper[insert|get]\r\n"
		" -f mysqlclient_path\r\n"
		" -u dbuser\r\n"
		" -p dbpass\r\n",
		procname);
}

int main(int argc, char *argv[])
{
	int  ch, count = 10;
	acl::string mysql_path("../../lib/libmysqlclient_r.so");
	acl::string dbaddr("127.0.0.1:3306"), dbname("acl_db");
	acl::string dbuser("root"), dbpass(""), oper("get");

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hc:n:f:u:p:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			__max_fibers = atoi(optarg);
			break;
		case 'n':
			count = atoi(optarg);
			break;
		case 'f':
			mysql_path = optarg;
			break;
		case 'u':
			dbuser = optarg;
			break;
		case 'p':
			dbpass = optarg;
			break;
		case 'o':
			oper = optarg;
			break;
		default:
			break;
		}
	}


	acl::db_handle::set_loadpath(mysql_path);

	acl::mysql_conf dbconf(dbaddr, dbname);
	dbconf.set_dbuser(dbuser)
		.set_dbpass(dbpass)
		.set_dblimit(__max_fibers)
		.set_conn_timeout(1)
		.set_rw_timeout(1);

	acl::mysql_pool dbpool(dbconf);

	__cur_fibers = __max_fibers;

	for (int i = 0; i < __max_fibers; i++)
	{
		acl::fiber* f = new myfiber(dbpool, oper, count);
		f->start();
	}

	acl::fiber::schedule();

	printf("---- exit now ----\r\n");

	return 0;
}
