#pragma once
#include "acl_cpp/acl_cpp_define.hpp"

struct ACL_DBUF_POOL;

namespace acl
{

/**
 * �ڴ��������࣬������ṩ�ڴ���亯�������������������ʱ���ڴ����ᱻһ��
 * �Ե��ͷţ������ʺ�����ҪƵ������һЩ��С���ȵ�С�ڴ��Ӧ�ã�
 * ����ʵ�����Ƿ�װ�� lib_acl �е� ACL_DBUF_POOL �ṹ������
 */

class ACL_CPP_API dbuf_pool
{
public:
	/**
	 * ���������붯̬����
	 */
	dbuf_pool();

	/**
	 * ����������Ҫ��̬��������������������������ʹ������Ҫ���� destroy
	 * ���������ٶ�̬����
	 */
	void destroy();

	/**
	 * ���� new/delete ��������ʹ dbuf_pool ������Ҳ�������ڴ���ϣ�
	 * �Ӷ������� malloc/free �Ĵ���
	 */
	void *operator new(size_t size);
	void operator delete(void* ptr);

	/**
	 * �����ڴ�ص�״̬�Ա����ظ�ʹ�ø��ڴ�ض���
	 * @param reserve {size_t} ����ֵ > 0������Ҫָ�����Ᵽ�����ڴ��С��
	 *  �ô�С����С�ڵ����Ѿ��ڸ��ڴ�ض������Ĵ�С
	 * @return {bool} �����������Ƿ����򷵻� false
	 */
	bool dbuf_reset(size_t reserve = 0);

	/**
	 * ����ָ�����ȵ��ڴ�
	 * @param len {size_t} ��Ҫ������ڴ泤�ȣ����ڴ�Ƚ�Сʱ(С�ڹ��캯��
	 *  �е� block_size)ʱ����������ڴ����� dbuf_pool ��������ڴ����ϣ�
	 *  ���ڴ�ϴ�ʱ��ֱ��ʹ�� malloc ���з���
	 * @return {void*} �·�����ڴ��ַ
	 */
	void* dbuf_alloc(size_t len);

	/**
	 * ����ָ�����ȵ��ڴ沢���ڴ���������
	 * @param len {size_t} ��Ҫ������ڴ泤��
	 * @return {void*} �·�����ڴ��ַ
	 */
	void* dbuf_calloc(size_t len);

	/**
	 * ����������ַ�����̬�����µ��ڴ沢���ַ������и��ƣ������� strdup
	 * @param s {const char*} Դ�ַ���
	 * @return {char*} �¸��Ƶ��ַ�����ַ
	 */
	char* dbuf_strdup(const char* s);

	/**
	 * ����������ڴ����ݶ�̬�����ڴ沢�����ݽ��и���
	 * @param addr {const void*} Դ�����ڴ��ַ
	 * @param len {size_t} Դ���ݳ���
	 * @return {void*} �¸��Ƶ����ݵ�ַ
	 */
	void* dbuf_memdup(const void* addr, size_t len);

	/**
	 * �黹���ڴ�ط�����ڴ�
	 * @param addr {const void*} ���ڴ�ط�����ڴ��ַ
	 * @return {bool} ������ڴ��ַ���ڴ�ط�����ͷŶ�Σ��򷵻� false
	 */
	bool dbuf_free(const void* addr);

	/**
	 * �������ڴ�ط����ĳ�ε�ַ�����⵱���� dbuf_reset ʱ����ǰ�ͷŵ�
	 * @param addr {const void*} ���ڴ�ط�����ڴ��ַ
	 * @return {bool} ������ڴ��ַ���ڴ�ط��䣬�򷵻� false
	 */
	bool dbuf_keep(const void* addr);

	/**
	 * ȡ���������ڴ�ط����ĳ�ε�ַ���Ա��ڵ��� dbuf_reset ʱ���ͷŵ�
	 * @param addr {const void*} ���ڴ�ط�����ڴ��ַ
	 * @return {bool} ������ڴ��ַ���ڴ�ط��䣬�򷵻� false
	 */
	bool dbuf_unkeep(const void* addr);

private:
	ACL_DBUF_POOL* pool_;
	size_t mysize_;

	~dbuf_pool();
};

} // namespace acl
