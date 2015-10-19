#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/connpool/connect_pool.hpp"

namespace acl
{

/**
 * redis ���ӳ��࣬����̳��� connect_pool���� connect_pool ������ͨ�õ��й�
 * TCP ���ӳص�ͨ�÷�����
 * redis connection pool inherting from connect_pool, which includes
 * TCP connection pool methods.
 */
class ACL_CPP_API redis_client_pool : public connect_pool
{
public:
	/**
	 * ���캯��
	 * constructor
	 * @param addr {const char*} ����˵�ַ����ʽ��ip:port
	 *  the redis-server's listening address, format: ip:port
	 * @param count {size_t} ���ӳص�������������ƣ������ֵΪ 0�������ӳ�
	 *  û���������ơ�
	 *  the max connections for each connection pool. there is
	 *  no connections limit of the pool when the count is 0.
	 * @param idx {size_t} �����ӳض����ڼ����е��±�λ��(�� 0 ��ʼ)
	 *  the subscript of the connection pool in the connection cluster
	 */
	redis_client_pool(const char* addr, size_t count, size_t idx = 0);

	virtual ~redis_client_pool();

	/**
	 * �����������ӳ�ʱʱ�估���� IO ��д��ʱʱ��(��)
	 * set the connect and read/write timeout in seconds
	 * @param conn_timeout {int} ���ӳ�ʱʱ��
	 *  the timeout to connect in seconds
	 * @param rw_timeout {int} ���� IO ��д��ʱʱ��(��)
	 *  the timeout to read/write in seconds
	 * @return {redis_client_pool&}
	 */
	redis_client_pool& set_timeout(int conn_timeout, int rw_timeout);

	/**
	 * �������� redis ����������������
	 * @param pass {const char*} ��������
	 * @return {redis_client_pool&}
	 */
	redis_client_pool& set_password(const char* pass);

protected:
	/**
	 * ���ി�麯��: ���ô˺�����������һ���µ�����
	 * virtual function in class connect_pool to create a new connection
	 * @return {connect_client*}
	 */
	connect_client* create_connect();

private:
	int   conn_timeout_;
	int   rw_timeout_;
	char* pass_;
};

} // namespace acl
