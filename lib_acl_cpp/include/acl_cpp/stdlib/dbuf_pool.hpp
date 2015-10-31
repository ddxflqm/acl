#pragma once
#include "acl_cpp/acl_cpp_define.hpp"

struct ACL_DBUF_POOL;

namespace acl
{

/**
 * 内存链管理类，该类仅提供内存分配函数，在整个类对象被析构时该内存链会被一次
 * 性地释放，该类适合于需要频繁分配一些大小不等的小内存的应用；
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

private:
	ACL_DBUF_POOL* pool_;
	size_t mysize_;

	~dbuf_pool();
};

} // namespace acl
