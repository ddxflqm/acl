#include <getopt.h>
#include "acl_cpp/lib_acl.hpp"

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -s server_addr[127.0.0.1:8194] -n max_loop\r\n", procname);
}

int main(int argc, char* argv[])
{
	int  ch, max = 10;
	acl::string addr("127.0.0.1:8194");

	while ((ch = getopt(argc, argv, "hs:n:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'n':
			max = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::log::stdout_open(true);

	acl::string buf(1024);
	for (size_t i = 0; i < 1024; i++)
		buf << 'X';

	acl::http_request req(addr, 10, 10);
	acl::string tmp(1024);

	for (int i = 0; i < max; i++)
	{
		acl::http_header& header = req.request_header();
		//header.set_method(acl::HTTP_METHOD_POST);
		header.set_url("/");
		header.set_keep_alive(true);
		header.accept_gzip(true);
		//header.set_content_length(buf.length());

		acl::string hdr;
		header.build_request(hdr);
		printf("request header:\r\n%s\r\n", hdr.c_str());

		//if (req.request(buf.c_str(), buf.length()) == false)
		if (req.request(NULL, 0) == false)
		{
			printf("send request error\n");
			break;
		}

		printf("send request body ok\r\n");

		tmp.clear();

		int  size = 0, real_size = 0, n;
		while (true)
		{
			int ret = req.read_body(tmp, false, &n);
			if (ret < 0) {
				printf("read_body error\n");
				return 1;
			}
			else if (ret == 0)
				break;
			size += ret;
			real_size += n;
		}
		printf("read body size: %d, real_size: %d, %s\n",
			size, real_size, tmp.c_str());

		printf("===============================================\r\n");
	}

	return 0;
}
