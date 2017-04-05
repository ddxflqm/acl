#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

class myobj : public acl::mobj
{
public:
	myobj(void) : n_(0) {}
	~myobj(void) {}

	void run(void)
	{
		int i = 5;
		printf("thread-%lu sleep %d seconds\r\n",
			acl::thread::thread_self(), i);
		sleep(i);
		printf("thread-%lu wakeup\r\n", acl::thread::thread_self());

		n_ ++;
	}

	int get_result(void) const
	{
		return n_;
	}

private:
	int n_;
};

class mythread : public acl::thread
{
public:
	mythread(acl::mbox& mb, myobj& o) : mb_(mb), o_(o) {}
	~mythread(void) {}

protected:
	// @override
	void* run(void)
	{
		o_.run();
		mb_.push(&o_);
		return NULL;
	}

private:
	acl::mbox& mb_;
	myobj& o_;
};

class myfiber : public acl::fiber
{
public:
	myfiber(void) {}
	~myfiber(void) {}

protected:
	// @override
	void run(void)
	{
		printf("fiber-%u: wait result from thread\r\n", get_id());

		myobj mo;
		mythread thr(mb_, mo);
		thr.start();

		myobj* o = (myobj*) mb_.pop();
		assert(o == &mo);
		printf("fiber-%u: result = %d\r\n", get_id(), o->get_result());

		delete this;

		acl::fiber::schedule_stop();
	}

private:
	acl::mbox mb_;
};

//////////////////////////////////////////////////////////////////////////////

class sleepy_fiber : public acl::fiber
{
public:
	sleepy_fiber(void) {}
	~sleepy_fiber(void) {}

protected:
	void run(void)
	{
		int n = 0;
		while (true)
		{
			printf("fiber-%u sleep %d second\r\n", get_id(), ++n);
			sleep(1);
		}
	}
};

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n", procname);
}

int main(int argc, char *argv[])
{
	int  ch;

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "h")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			break;
		}
	}

	acl::fiber* f = new myfiber();
	f->start();

	sleepy_fiber fb;
	fb.start();

	acl::fiber::schedule();

	return 0;
}
