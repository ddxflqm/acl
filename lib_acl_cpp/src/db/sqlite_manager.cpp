#include "acl_stdafx.hpp"
#include "acl_cpp/db/sqlite_pool.hpp"
#include "acl_cpp/db/sqlite_manager.hpp"

namespace acl {

sqlite_manager::sqlite_manager(const char* dbfile, int dblimit /* = 64 */)
{
	acl_assert(dbfile && *dbfile);
	dbfile_ = acl_mystrdup(dbfile);
	dblimit_ = dblimit;
}

sqlite_manager::~sqlite_manager()
{
	acl_myfree(dbfile_);
}

connect_pool* sqlite_manager::create_pool(const char*, int, size_t)
{
	sqlite_pool* dbpool = NEW sqlite_pool(dbfile_, dblimit_);
	return dbpool;
}

} // namespace acl
