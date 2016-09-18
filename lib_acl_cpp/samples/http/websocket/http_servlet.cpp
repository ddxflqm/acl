#include "stdafx.h"
#include "acl_cpp/http/websocket.hpp"
#include "http_servlet.h"

http_servlet::http_servlet(acl::redis_client_cluster& cluster, size_t max_conns)
{
	// ���� session �洢����
	session_ = new acl::redis_session(cluster, max_conns);
}

http_servlet::~http_servlet(void)
{
	delete session_;
}

bool http_servlet::doUnknown(acl::HttpServletRequest&,
	acl::HttpServletResponse& res)
{
	res.setStatus(400);
	res.setContentType("text/html; charset=");
	// ���� http ��Ӧͷ
	if (res.sendHeader() == false)
		return false;
	// ���� http ��Ӧ��
	acl::string buf("<root error='unkown request method' />\r\n");
	(void) res.getOutputStream().write(buf);
	return false;
}

bool http_servlet::doGet(acl::HttpServletRequest& req,
	acl::HttpServletResponse& res)
{
	const char* ptr = req.getHeader("Connection");
	if (ptr == NULL)
	{
		printf("no connectioin\r\n");
		return false;
	}
	printf("Connection: %s\r\n", ptr);
	if (strcasestr(ptr, "Upgrade") == NULL)
	{
		printf("invalid Connection: %s\r\n", ptr);
		return false;
	}

	ptr = req.getHeader("Upgrade");
	if (ptr == NULL)
	{
		printf("no upgrade\r\n");
		return false;
	}
	printf("Upgrade: %s\r\n", ptr);
	if (strcasestr(ptr, "websocket") == NULL)
	{
		printf("invalid Upgrade: %s\r\n", ptr);
		return false;
	}

	const char* key = req.getHeader("Sec-WebSocket-Key");
	if (key == NULL || *key == 0)
	{
		printf("no Sec-WebSocket-Key\r\n");
		return false;
	}

	printf("Sec-WebSocket-Key: %s\r\n", key);
	acl::http_header& header = res.getHttpHeader();
	header.set_upgrade("websocket");
	header.set_ws_accept(key);
	if (res.sendHeader() == false)
	{
		printf("sendHeader error\r\n");
		return false;
	}

	printf("-------------------------------------------------------\r\n");

	return doWebsocket(req, res);
}

bool http_servlet::doWebsocket(acl::HttpServletRequest& req,
	acl::HttpServletResponse&)
{
	acl::socket_stream& ss = req.getSocketStream();
	acl::websocket in(ss), out(ss);

	while (true)
	{
		if (in.read_frame_head() == false)
		{
			printf("read_frame_head error\r\n");
			return false;
		}

		unsigned long long len = in.get_frame_payload_len();
		if (len == 0)
		{
			printf("invalid len: %llu\r\n", len);
			return false;
		}

		out.reset()
			.set_frame_fin(true)
			.set_frame_opcode(acl::FRAME_TEXT)
			.set_frame_payload_len(len);

		char buf[8192];
		while (true)
		{
			int ret = in.read_frame_data(buf, sizeof(buf) - 1);
			if (ret == 0)
				break;
			if (ret < 0)
			{
				printf("read_frame_data error\r\n");
				return false;
			}

			buf[ret] = 0;
			printf("read: %s\r\n", buf);
			if (out.send_frame_data(buf, ret) == false)
			{
				printf("send_frame_data error\r\n");
				return false;
			}
		}

		sleep(1);
		char info[256];
		snprintf(info, sizeof(info), "hello world!");
		out.reset()
			.set_frame_fin(true)
			.set_frame_opcode(acl::FRAME_TEXT)
			.set_frame_payload_len(strlen(info));
		if (out.send_frame_data(info, strlen(info)) == false)
		{
			printf("send_frame_data error\r\n");
			return false;
		}

		sleep(1);
		snprintf(info, sizeof(info), "hello zsx!");
		out.reset()
			.set_frame_fin(true)
			.set_frame_masking_key(12345671)
			.set_frame_opcode(acl::FRAME_TEXT)
			.set_frame_payload_len(strlen(info));
		if (out.send_frame_data(info, strlen(info)) == false)
		{
			printf("send_frame_data error\r\n");
			return false;
		}

		sleep(1);
		snprintf(info, sizeof(info), "GoodBye!");
		out.reset()
			.set_frame_fin(true)
			.set_frame_masking_key(12345671)
			.set_frame_opcode(acl::FRAME_TEXT)
			.set_frame_payload_len(strlen(info));
		if (out.send_frame_data(info, strlen(info)) == false)
		{
			printf("send_frame_data error\r\n");
			return false;
		}
	}

	return false;
}

bool http_servlet::doPost(acl::HttpServletRequest&,
	acl::HttpServletResponse& res)
{
	res.setContentType("text/xml; charset=utf-8")	// ������Ӧ�ַ���
		.setContentEncoding(true)		// �����Ƿ�ѹ������
		.setChunkedTransferEncoding(false);	// ���� chunk ���䷽ʽ

	printf("error\r\n");
	acl::string buf("error");

	// ���� http ��Ӧ�壬��Ϊ������ chunk ����ģʽ��������Ҫ�����һ��
	// res.write ������������Ϊ 0 �Ա�ʾ chunk �������ݽ���
	return res.write(buf) && res.write(NULL, 0);
}
