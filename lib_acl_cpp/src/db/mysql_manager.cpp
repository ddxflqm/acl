#include "acl_stdafx.hpp"
#include "acl_cpp/db/mysql_pool.hpp"
#include "acl_cpp/db/mysql_manager.hpp"

namespace acl {

mysql_manager::mysql_manager(const char* dbaddr, const char* dbname,
	const char* dbuser, const char* dbpass, int dblimit /* = 64 */,
	unsigned long dbflags /* = 0 */, bool auto_commit /* = true */,
	int conn_timeout /* = 60 */, int rw_timeout /* = 60 */)
{
	acl_assert(dbaddr && *dbaddr);
	acl_assert(dbname && *dbname);
	dbaddr_ = acl_mystrdup(dbaddr);
	dbname_ = acl_mystrdup(dbname);
	if (dbuser)
		dbuser_ = acl_mystrdup(dbuser);
	else
		dbuser_ = NULL;

	if (dbpass)
		dbpass_ = acl_mystrdup(dbpass);
	else
		dbpass_ = NULL;

	dblimit_ = dblimit;
	dbflags_ = dbflags;
	auto_commit_ = auto_commit;
	conn_timeout_ = conn_timeout;
	rw_timeout_ = rw_timeout;
}

mysql_manager::~mysql_manager()
{
	if (dbaddr_)
		acl_myfree(dbaddr_);
	if (dbname_)
		acl_myfree(dbname_);
	if (dbuser_)
		acl_myfree(dbuser_);
	if (dbpass_)
		acl_myfree(dbpass_);
}

connect_pool* mysql_manager::create_pool(const char*, int, size_t)
{
	mysql_pool* dbpool = NEW mysql_pool(dbaddr_, dbname_, dbuser_,
		dbpass_, dblimit_, dbflags_, auto_commit_,
		conn_timeout_, rw_timeout_);
	return dbpool;
}

} // namespace acl
