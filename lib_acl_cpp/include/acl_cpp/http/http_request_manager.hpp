#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/connpool/connect_manager.hpp"

namespace acl
{

/**
 * HTTP �ͻ����������ӳع�����
 */
class ACL_CPP_API http_request_manager : public acl::connect_manager
{
public:
	/**
	 * ���캯��
	 * @param conn_timeout {int} ���ӳ�ʱʱ��(��)
	 * @param rw_timeout {int} ���� IO ��д��ʱʱ��(��)
	 */
	http_request_manager(int conn_timeout = 30, int rw_timeout = 30);
	virtual ~http_request_manager();

protected:
	/**
	 * ���ി�麯���������������ӳض��󣬸ú������غ��ɻ����������ӳص���������
	 * ������ IO �ĳ�ʱʱ��
	 * @param addr {const char*} ������������ַ����ʽ��ip:port
	 * @param count {size_t} ���ӳصĴ�С���ƣ�����ֵΪ 0 ʱ��û������
	 * @param idx {size_t} �����ӳض����ڼ����е��±�λ��(�� 0 ��ʼ)
	 * @return {connect_pool*} ���ش��������ӳض���
	 */
	connect_pool* create_pool(const char* addr, size_t count, size_t idx);
};

} // namespace acl
