#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/connpool/connect_pool.hpp"

namespace acl
{

/**
 * http �ͻ������ӳ��࣬���ุ��Ϊ connect_pool������ֻ��ʵ�ָ����е��麯��
 * create_connect ��ӵ�������ӳظ��� connect_pool �Ĺ��ܣ����⣬���ഴ��
 * �����Ӷ����� http_reuqest ���������ڵ��� connect_pool::peek ʱ����
 * �ı��� http_request �࣬��������Ҫ�� peek ���ص������ǿ��תΪ http_request
 * ����󣬱����ʹ�� http_request �������й��ܣ����� http_reuqest ��Ϊ
 * connect_client ������
 */
class ACL_CPP_API http_request_pool : public connect_pool
{
public:
	/**
	 * ���캯��
	 * @param addr {const char*} ������������ַ����ʽ��ip:port(domain:port)
	 * @param count {size_t} ���ӳ�������Ӹ������ƣ�����ֵΪ 0 ʱ��û������
	 * @param idx {size_t} �����ӳض����ڼ����е��±�λ��(�� 0 ��ʼ)
	 */
	http_request_pool(const char* addr, size_t count, size_t idx = 0);
	~http_request_pool();

protected:
	// ���ി�麯�����ú������غ��ɻ������ø����ӳص��������Ӽ����� IO ��ʱʱ��
	virtual connect_client* create_connect();
};

} // namespace acl
