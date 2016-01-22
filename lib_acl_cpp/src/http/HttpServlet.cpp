#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/session/memcache_session.hpp"
#include "acl_cpp/http/http_header.hpp"
#include "acl_cpp/http/HttpSession.hpp"
#include "acl_cpp/http/HttpServletRequest.hpp"
#include "acl_cpp/http/HttpServletResponse.hpp"
#include "acl_cpp/http/HttpServlet.hpp"

namespace acl
{

HttpServlet::HttpServlet(socket_stream* stream, session* session)
: req_(NULL)
, res_(NULL)
, stream_(stream)
{
	dbuf_ = new dbuf_pool;
	init();

	if (session == NULL)
	{
		session_ = new (dbuf_->dbuf_alloc(sizeof(memcache_session)))
			memcache_session("127.0.0.1");
		session_ptr_ = session_;
		reserve_size_ = sizeof(memcache_session);
	}
	else
	{
		session_ = session;
		session_ptr_ = NULL;
		reserve_size_ = 0;
	}
}

HttpServlet::HttpServlet(socket_stream* stream,
	const char* memcache_addr /* = "127.0.0.1:11211" */)
: req_(NULL)
, res_(NULL)
, stream_(stream)
{
	dbuf_ = new dbuf_pool;

	init();

	session_ = new (dbuf_->dbuf_alloc(sizeof(memcache_session)))
		memcache_session(memcache_addr);
	session_ptr_ = session_;
	reserve_size_ = sizeof(memcache_session);
}

HttpServlet::HttpServlet()
{
	dbuf_ = new dbuf_pool;

	init();

	req_ = NULL;
	res_ = NULL;
	stream_ = NULL;
	session_ = NULL;
	session_ptr_ = NULL;
	reserve_size_ = 0;
}

void HttpServlet::init()
{
	first_ = true;
	local_charset_[0] = 0;
	rw_timeout_ = 60;
	parse_body_enable_ = true;
	parse_body_limit_ = 0;
}

HttpServlet::~HttpServlet(void)
{
	if (req_)
		req_->~HttpServletRequest();
	if (res_)
		res_->~HttpServletResponse();
	if (session_ptr_)
		session_ptr_->~session();
	dbuf_->destroy();
}

#define COPY(x, y) ACL_SAFE_STRNCPY((x), (y), sizeof((x)))

HttpServlet& HttpServlet::setLocalCharset(const char* charset)
{
	if (charset && *charset)
		COPY(local_charset_, charset);
	else
		local_charset_[0] =0;
	return *this;
}

HttpServlet& HttpServlet::setRwTimeout(int rw_timeout)
{
	rw_timeout_ = rw_timeout;
	return *this;
}

HttpServlet& HttpServlet::setParseBody(bool on)
{
	parse_body_enable_ = on;
	return *this;
}

HttpServlet& HttpServlet::setParseBodyLimit(int length)
{
	if (length > 0)
		parse_body_limit_ = length;
	return *this;
}

bool HttpServlet::doRun(dbuf_pool* dbuf)
{
	socket_stream* in;
	socket_stream* out;
	bool cgi_mode;

	bool first = first_;
	if (first_)
		first_ = false;

	if (stream_ == NULL)
	{
		// ������Ϊ�գ��� CGI ģʽ��������׼�������
		// ��Ϊ������
		in = NEW socket_stream();
		in->open(ACL_VSTREAM_IN);

		out = NEW socket_stream();
		out->open(ACL_VSTREAM_OUT);
		cgi_mode = true;
	}
	else
	{
		in = out = stream_;
		cgi_mode = false;
	}

	// req/res ����ջ�����������ڴ�������

	res_ = new (dbuf->dbuf_alloc(sizeof(HttpServletResponse)))
		HttpServletResponse(*out, dbuf);
	req_ = new (dbuf->dbuf_alloc(sizeof(HttpServletRequest)))
		HttpServletRequest(*res_, *session_, *in, local_charset_,
			parse_body_enable_, parse_body_limit_, dbuf);

	// ���� HttpServletRequest ����
	res_->setHttpServletRequest(req_);

	if (rw_timeout_ >= 0)
		req_->setRwTimeout(rw_timeout_);

	res_->setCgiMode(cgi_mode);

	string method_s(32);
	http_method_t method = req_->getMethod(&method_s);

	// ���������ֵ�Զ��趨�Ƿ���Ҫ���ֳ�����
	if (!cgi_mode)
		res_->setKeepAlive(req_->isKeepAlive());

	bool  ret;

	switch (method)
	{
	case HTTP_METHOD_GET:
		ret = doGet(*req_, *res_);
		break;
	case HTTP_METHOD_POST:
		ret = doPost(*req_, *res_);
		break;
	case HTTP_METHOD_PUT:
		ret = doPut(*req_, *res_);
		break;
	case HTTP_METHOD_CONNECT:
		ret = doConnect(*req_, *res_);
		break;
	case HTTP_METHOD_PURGE:
		ret = doPurge(*req_, *res_);
		break;
	case HTTP_METHOD_DELETE:
		ret = doDelete(*req_, *res_);
		break;
	case  HTTP_METHOD_HEAD:
		ret = doHead(*req_, *res_);
		break;
	case HTTP_METHOD_OPTION:
		ret = doOptions(*req_, *res_);
		break;
	case HTTP_METHOD_PROPFIND:
		ret = doPropfind(*req_, *res_);
		break;
	case HTTP_METHOD_OTHER:
		ret = doOther(*req_, *res_, method_s.c_str());
		break;
	default:
		ret = false; // �п�����IOʧ�ܻ�δ֪����
		if (req_->getLastError() == HTTP_REQ_ERR_METHOD)
			doUnknown(*req_, *res_);
		else if (first)
			doError(*req_, *res_);
		break;
	}

	if (in != out)
	{
		// ����Ǳ�׼���������������Ҫ�Ƚ����������׼����������
		// Ȼ������ͷ������������������ڲ����Զ��ж�������Ϸ���
		// �������Ա�֤��ͻ��˱��ֳ�����
		in->unbind();
		out->unbind();
		delete in;
		delete out;
	}

	// ���ظ��ϲ�����ߣ�true ��ʾ�������ֳ����ӣ������ʾ��Ͽ�����
	return ret && req_->isKeepAlive()
		&& res_->getHttpHeader().get_keep_alive();
}

bool HttpServlet::doRun()
{
	bool ret = doRun(dbuf_);

	// �����ڴ��״̬
	dbuf_->dbuf_reset(reserve_size_);
	return ret;
}

bool HttpServlet::doRun(session& session, socket_stream* stream /* = NULL */)
{
	stream_ = stream;
	session_ = &session;
	return doRun();
}

bool HttpServlet::doRun(const char* memcached_addr, socket_stream* stream)
{
	memcache_session session(memcached_addr);
	return doRun(session, stream);
}

} // namespace acl
