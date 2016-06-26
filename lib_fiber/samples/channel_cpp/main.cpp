#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

class fiber_consumer : public acl::fiber
{
public:
	fiber_consumer(acl::channel<acl::string*>& chan1,
		acl::channel<int>& chan2, int n)
		: chan1_(chan1), chan2_(chan2), count_(n) {}
	~fiber_consumer(void) {}

protected:
	// @override
	void run(void)
	{
		for (int i = 0; i < count_; i++)
		{
			acl::string* rs = chan1_.pop();
			int n = chan2_.pop();
			if (i < 1000)
				printf(">>read: %s, %d\r\n", rs->c_str(), n);
			delete rs;
		}
	}

private:
	acl::channel<acl::string*>& chan1_;
	acl::channel<int>& chan2_;
	int count_;
};

class fiber_producer : public acl::fiber
{
public:
	fiber_producer(acl::channel<acl::string*>& chan1,
		acl::channel<int>& chan2, int n)
		: chan1_(chan1), chan2_(chan2), count_(n) {}
	~fiber_producer(void) {}

protected:
	// @override
	void run(void)
	{
		for (int i = 0; i < count_; i++)
		{
			if (i < 1000)
				printf(">>send: %d\r\n", i);
			acl::string* buf = new acl::string;
			buf->format("hello-%d", i);
			chan1_ << buf;
			chan2_ << i;
		}
	}

private:
	acl::channel<acl::string*>& chan1_;
	acl::channel<int>& chan2_;
	int count_;
};

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -n count\r\n", procname);
}

int main(int argc, char *argv[])
{
	int  ch, n = 10;

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hn:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			n = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::channel<acl::string*> chan1;
	acl::channel<int> chan2;

	fiber_consumer consumer(chan1, chan2, n);
	consumer.start();

	fiber_producer producer(chan1, chan2, n);
	producer.start();

	acl::fiber::schedule();

	return 0;
}
