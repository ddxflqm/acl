#pragma once

class http_servlet : public acl::HttpServlet
{
public:
	http_servlet(acl::socket_stream* stream);
	~http_servlet();

protected:
	virtual bool doError(acl::HttpServletRequest&,
		acl::HttpServletResponse& res);
	virtual bool doUnknown(acl::HttpServletRequest&,
		acl::HttpServletResponse& res);
	virtual bool doGet(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);
	virtual bool doPost(acl::HttpServletRequest& req,
		acl::HttpServletResponse& res);

private:
	acl::memcache_session session_;
};
