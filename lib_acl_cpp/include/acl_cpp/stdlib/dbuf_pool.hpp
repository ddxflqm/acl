#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>

struct ACL_DBUF_POOL;

namespace acl
{

/**
 * 会话类的内存链管理类，该类仅提供内存分配函数，在整个类对象被析构时该内存链
 * 会被一次性地释放，该类适合于需要频繁分配一些大小不等的小内存的应用；
 * 该类实际上是封装了 lib_acl 中的 ACL_DBUF_POOL 结构及方法
 */

class ACL_CPP_API dbuf_pool
{
public:
	/**
	 * 该类对象必须动态创建
	 */
	dbuf_pool();

	/**
	 * 该类对象必须要动态创建，所以隐藏了析构函数，使用者需要调用 destroy
	 * 函数来销毁动态对象
	 */
	void destroy();

	/**
	 * 重载 new/delete 操作符，使 dbuf_pool 对象本身也创建在内存池上，
	 * 从而减少了 malloc/free 的次数
	 */
	void *operator new(size_t size);
	void operator delete(void* ptr);

	/**
	 * 重置内存池的状态以便于重复使用该内存池对象
	 * @param reserve {size_t} 若该值 > 0，则需要指定额外保留的内存大小，
	 *  该大小必须小于等于已经在该内存池对象分配的大小
	 * @return {bool} 如果输入参数非法，则返回 false
	 */
	bool dbuf_reset(size_t reserve = 0);

	/**
	 * 分配指定长度的内存
	 * @param len {size_t} 需要分配的内存长度，当内存比较小时(小于构造函数
	 *  中的 block_size)时，所分配的内存是在 dbuf_pool 所管理的内存链上，
	 *  当内存较大时会直接使用 malloc 进行分配
	 * @return {void*} 新分配的内存地址
	 */
	void* dbuf_alloc(size_t len);

	/**
	 * 分配指定长度的内存并将内存区域清零
	 * @param len {size_t} 需要分配的内存长度
	 * @return {void*} 新分配的内存地址
	 */
	void* dbuf_calloc(size_t len);

	/**
	 * 根据输入的字符串动态创建新的内存并将字符串进行复制，类似于 strdup
	 * @param s {const char*} 源字符串
	 * @return {char*} 新复制的字符串地址
	 */
	char* dbuf_strdup(const char* s);

	/**
	 * 根据输入的字符串动态创建新的内存并将字符串进行复制，类似于 strdup
	 * @param s {const char*} 源字符串
	 * @param len {size_t} 限制所复制字符串的最大长度
	 * @return {char*} 新复制的字符串地址
	 */
	char* dbuf_strndup(const char* s, size_t len);

	/**
	 * 根据输入的内存数据动态创建内存并将数据进行复制
	 * @param addr {const void*} 源数据内存地址
	 * @param len {size_t} 源数据长度
	 * @return {void*} 新复制的数据地址
	 */
	void* dbuf_memdup(const void* addr, size_t len);

	/**
	 * 归还由内存池分配的内存
	 * @param addr {const void*} 由内存池分配的内存地址
	 * @return {bool} 如果该内存地址非内存池分配或释放多次，则返回 false
	 */
	bool dbuf_free(const void* addr);

	/**
	 * 保留由内存池分配的某段地址，以免当调用 dbuf_reset 时被提前释放掉
	 * @param addr {const void*} 由内存池分配的内存地址
	 * @return {bool} 如果该内存地址非内存池分配，则返回 false
	 */
	bool dbuf_keep(const void* addr);

	/**
	 * 取消保留由内存池分配的某段地址，以便于调用 dbuf_reset 时被释放掉
	 * @param addr {const void*} 由内存池分配的内存地址
	 * @return {bool} 如果该内存地址非内存池分配，则返回 false
	 */
	bool dbuf_unkeep(const void* addr);

	/**
	 * 获得内部 ACL_DBUF_POOL 对象，以便于操作 C 接口的内存池对象
	 * @return {ACL_DBUF_POOL*}
	 */
	ACL_DBUF_POOL *get_dbuf()
	{
		return pool_;
	}

private:
	ACL_DBUF_POOL* pool_;
	size_t mysize_;

public:
	~dbuf_pool();
};

//////////////////////////////////////////////////////////////////////////////

class dbuf_guard;

/**
 * 在会话内存池对象上分配的对象基础类
 */
class ACL_CPP_API dbuf_obj
{
public:
	/**
	 * 构造函数
	 * @param guard {dbuf_guard*} 该参数非空时，则本类的子类对象会被
	 *  dbuf_guard 类对象自动管理，统一销毁；如果该参数为空，则应用应
	 *  调用 dbuf_guard::push_back 方法将子类对象纳入统一管理
	 */
	dbuf_obj(dbuf_guard* guard = NULL);

	virtual ~dbuf_obj() {}
};

/**
 * 会话内存池管理器，由该类对象管理 dbuf_pool 对象及在其上分配的对象，当该类
 * 对象销毁时，dbuf_pool 对象及在上面均被释放。
 */
class ACL_CPP_API dbuf_guard
{
public:
	/**
	 * 构造函数
	 * @param dbuf {dbuf_pool*} 当该内存池对象非空时，dbuf 将由 dbuf_guard
	 *  接管，如果为空，则本构造函数内部将会自动创建一个 dbuf_pool 对象
	 */
	dbuf_guard(dbuf_pool* dbuf = NULL);

	/**
	 * 析构函数，在析构函数内部将会自动销毁由构造函数传入的 dbuf_pool 对象
	 */
	~dbuf_guard();

	/**
	 * 调用 dbuf_pool::dbuf_reset
	 * @param reserve {size_t}
	 * @return {bool}
	 */
	bool reset(size_t reserve = 0)
	{
		return dbuf_->dbuf_reset(reserve);
	}

	/**
	 * 调用 dbuf_pool::dbuf_alloc
	 * @param len {size_t}
	 * @return {void*}
	 */
	void* alloc(size_t len)
	{
		return dbuf_->dbuf_alloc(len);
	}

	/**
	 * 调用 dbuf_pool::dbuf_calloc
	 * @param len {size_t}
	 * @return {void*}
	 */
	void* calloc(size_t len)
	{
		return dbuf_->dbuf_calloc(len);
	}

	/**
	 * 调用 dbuf_pool::dbuf_strdup
	 * @param s {const char*}
	 * @return {char*}
	 */
	char* strdup(const char* s)
	{
		return dbuf_->dbuf_strdup(s);
	}

	/**
	 * 调用 dbuf_pool::dbuf_strndup
	 * @param s {const char*}
	 * @param len {size_t}
	 * @return {char*}
	 */
	char* strndup(const char* s, size_t len)
	{
		return dbuf_->dbuf_strndup(s, len);
	}

	/**
	 * 调用 dbuf_pool::dbuf_memdup
	 * @param addr {const void*}
	 * @param len {size_t}
	 * @return {void*}
	 */
	void* memdup(const void* addr, size_t len)
	{
		return dbuf_->dbuf_memdup(addr, len);
	}

	/**
	 * 调用 dbuf_pool::dbuf_free
	 * @param addr {const void*}
	 * @return {bool}
	 */
	bool free(const void* addr)
	{
		return dbuf_->dbuf_free(addr);
	}

	/**
	 * 调用 dbuf_pool::dbuf_keep
	 * @param addr {const void*}
	 * @return {bool}
	 */
	bool keep(const void* addr)
	{
		return dbuf_->dbuf_keep(addr);
	}

	/**
	 * 调用 dbuf_pool::dbuf_unkeep
	 * @param addr {const void*}
	 * @return {bool}
	 */
	bool unkeep(const void* addr)
	{
		return dbuf_->dbuf_unkeep(addr);
	}

	/**
	 * 获得 dbuf_pool 对象
	 * @return {acl::dbuf_pool&}
	 */
	acl::dbuf_pool& get_dbuf() const
	{
		return *dbuf_;
	}

	/**
	 * 可以手动调用本函数，将在 dbuf_pool 上分配的 dbuf_obj 子类对象交给
	 * dbuf_guard 对象统一进行销毁管理
	 * @param obj {dbuf_obj*}
	 * @return {int} 返回 obj 被添加后其在 dbuf_obj 对象数组中的下标位置，
	 *  如果返回值 < 0 则说明输入参数非法
	 */
	int push_back(dbuf_obj* obj);

	/**
	 * 获得当前内存池中管理的对象数量
	 * @return {size_t}
	 */
	size_t size() const
	{
		return objs_.size();
	}

	/**
	 * 获得当前内存池中管理的对象集合
	 * @return {std::vector<dbuf_obj*>&}
	 */
	const std::vector<dbuf_obj*>& get_objs() const
	{
		return objs_;
	}

	/**
	 * 返回指定下标的对象
	 * @param pos {size_t} 指定对象的下标位置，不应越界
	 * @return {dbuf_obj*} 当下标位置越界时返回 NULL
	 */
	dbuf_obj* operator[](size_t pos) const;

private:
	dbuf_pool* dbuf_;
	std::vector<dbuf_obj*> objs_;
};

/**
 * samples:
 *
 * class myobj : public acl::dbuf_obj
 * {
 * public:
 * 	myobj(acl::dbuf_guard* guard) : dbuf_obj(guard) {}
 *
 * 	void doit()
 * 	{
 * 		printf("hello world!\r\n");
 * 	}
 *
 * private:
 * 	~myobj() {}
 * };
 *
 * void test()
 * {
 * 	acl::dbuf_guard guard;
 * 	for (int i = 0; i < 100; i++)
 * 	{
 * 		myobj* obj = new (guard.get_dbuf().dbuf_alloc(sizeof(myobj)))
 * 			myobj(&guard);
 * 		obj->doit();
 * 	}
 * }
 */
} // namespace acl
