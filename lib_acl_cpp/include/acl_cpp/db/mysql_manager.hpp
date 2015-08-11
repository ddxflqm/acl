#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/connpool/connect_manager.hpp"

namespace acl {

class ACL_CPP_API mysql_manager : public connect_manager
{
public:
	/**
	 * 采用 mysql 数据库时的构造函数
	 * @param dbaddr {const char*} mysql 服务器地址，格式：IP:PORT，
	 *  在 UNIX 平台下可以为 UNIX 域套接口
	 * @param dbname {const char*} 数据库名
	 * @param dbuser {const char*} 数据库用户
	 * @param dbpass {const char*} 数据库用户密码
	 * @param dblimit {int} 数据库连接池的最大连接数限制
	 * @param dbflags {unsigned long} mysql 标记位
	 * @param auto_commit {bool} 是否自动提交
	 * @param conn_timeout {int} 连接数据库超时时间(秒)
	 * @param rw_timeout {int} 与数据库通信时的IO时间(秒)
	 */
	mysql_manager(const char* dbaddr, const char* dbname,
		const char* dbuser, const char* dbpass,
		int dblimit = 64, unsigned long dbflags = 0,
		bool auto_commit = true, int conn_timeout = 60,
		int rw_timeout = 60);
	~mysql_manager();

protected:
	/**
	 * 基类 connect_manager 虚函数的实现
	 * @param addr {const char*} 服务器监听地址，格式：ip:port
	 * @param count {int} 连接池的大小限制
	 * @param idx {size_t} 该连接池对象在集合中的下标位置(从 0 开始)
	 * @return {connect_pool*} 返回创建的连接池对象
	 */
	connect_pool* create_pool(const char* addr, int count, size_t idx);

private:
	char* dbaddr_;		// 数据库监听地址
	char* dbname_;          // 数据库名
	char* dbuser_;          // 数据库账号
	char* dbpass_;          // 数据库账号密码
	int   dblimit_;         // 数据库连接池连接数上限
	unsigned long dbflags_; // 打开数据库时的标志位
	bool  auto_commit_;     // 是否自动提交修改后的数据
	int   conn_timeout_;    // 连接数据库的超时时间
	int   rw_timeout_;      // 与数据库通信的超时时间
};

} // namespace acl
