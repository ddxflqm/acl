#include "stdafx.h"
#include "redis_util.h"
#include "redis_cmdump.h"

//////////////////////////////////////////////////////////////////////////////

class qitem : public acl::thread_qitem
{
public:
	qitem(const acl::string& addr, const acl::string& msg)
	{
		time_t now = time(NULL);
		char buf[128];
		acl::rfc822 rfc;
		rfc.mkdate_cst(now, buf, sizeof(buf));

		msg_.format("%s: %s--%s\r\n", buf, addr.c_str(), msg.c_str());
	}

	~qitem(void)
	{
	}

	const acl::string& get_msg(void) const
	{
		return msg_;
	}

private:
	acl::string msg_;
};

//////////////////////////////////////////////////////////////////////////////

class dump_thread : public acl::thread
{
public:
	dump_thread(const acl::string& addr, acl::thread_queue& queue)
		: stopped_(false)
		, addr_(addr)
		, queue_(queue)
		, conn_timeout_(0)
		, rw_timeout_(0)
	{
	}

	~dump_thread(void)
	{
	}

	void* run(void)
	{
		acl::redis_client client(addr_, conn_timeout_, rw_timeout_);
		acl::redis redis(&client);
		if (redis.monitor() == false)
		{
			printf("redis monitor error: %s, addr: %s\r\n",
				redis.result_error(), addr_.c_str());
			return NULL;
		}

		acl::string buf;
		while (!stopped_)
		{
			if (redis.get_command(buf) == false)
			{
				printf("redis get_command error: %s\r\n",
					redis.result_error());
				break;
			}

			qitem* item = new qitem(addr_, buf);
			queue_.push(item);

			buf.clear();
		}

		return NULL;
	}

	void stop(void)
	{
		stopped_ = true;
		printf("Thread(%lu) stopping now\r\n", thread_id());
	}

private:
	bool stopped_;
	acl::string addr_;
	acl::thread_queue& queue_;
	int conn_timeout_;
	int rw_timeout_;
};

//////////////////////////////////////////////////////////////////////////////

redis_cmdump::redis_cmdump(const char* addr, int conn_timeout, int rw_timeout,
	const char* passwd, bool use_slave)
	: addr_(addr)
	, conn_timeout_(conn_timeout)
	, rw_timeout_(rw_timeout)
	, use_slave_(use_slave)
{
	if (passwd && *passwd)
		passwd_ = passwd;
}

redis_cmdump::~redis_cmdump(void)
{
}

void redis_cmdump::get_masters(std::vector<acl::string>& addrs)
{
	acl::redis_client client(addr_, conn_timeout_, rw_timeout_);
	acl::redis redis(&client);
	const std::map<acl::string, acl::redis_node*>* masters =
		redis_util::get_masters(redis);
	if (masters == NULL)
	{
		printf("masters NULL!\r\n");
		return;
	}

	const std::vector<acl::redis_node*>* slaves;
	const char* addr;

	for (std::map<acl::string, acl::redis_node*>::const_iterator cit
		= masters->begin(); cit != masters->end(); ++cit)
	{
		// 优先使用从节点
		if (use_slave_ && (slaves = cit->second->get_slaves()) != NULL
			&& !slaves->empty())
		{
			addr = (*slaves)[0]->get_addr();
			if (addr == NULL || *addr == 0)
				addr = cit->second->get_addr();
		}
		else
			addr = cit->second->get_addr();

		if (addr == NULL || *addr == 0)
		{
			printf("addr null\r\n");
			continue;
		}

		addrs.push_back(addr);
	}
}

void redis_cmdump::saveto(const char* filepath, bool dump_all)
{
	if (filepath == NULL || *filepath == 0)
	{
		printf("filepath null\r\n");
		return;
	}

	std::vector<acl::string> addrs;
	if (dump_all)
	{
		get_masters(addrs);
		if (addrs.empty())
		{
			printf("no master available!\r\n");
			return;
		}
	}
	else
		addrs.push_back(addr_);

	acl::ofstream out;
	if (out.open_append(filepath) == false)
	{
		printf("open %s error: %s\r\n", filepath, acl::last_serror());
		return;
	}

	acl::thread_queue queue;

	std::vector<dump_thread*> threads;
	for (std::vector<acl::string>::const_iterator cit = addrs.begin();
		cit != addrs.end(); ++cit)
	{
		dump_thread* thread = new dump_thread((*cit), queue);
		threads.push_back(thread);
		thread->set_detachable(false);
		thread->start();
	}

	while (true)
	{
		qitem* item = (qitem*) queue.pop();
		if (item == NULL)
		{
			printf("queue pop error\r\n");
			break;
		}

		const acl::string& msg = item->get_msg();
		if (msg.empty())
		{
			delete item;
			continue;
		}

		if (out.write(msg) == -1)
		{
			printf("write to %s error %s\r\n", filepath,
				acl::last_serror());
			delete item;
			break;
		}

		delete item;
	}

	for (std::vector<dump_thread*>::iterator it = threads.begin();
		it != threads.end(); ++it)
	{
		(*it)->stop();
	}

	for (std::vector<dump_thread*>::iterator it = threads.begin();
		it != threads.end(); ++it)
	{
		(*it)->wait();
		delete *it;
	}
}
