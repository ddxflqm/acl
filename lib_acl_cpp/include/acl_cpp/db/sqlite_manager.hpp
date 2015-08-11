#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/connpool/connect_manager.hpp"

namespace acl {

class ACL_CPP_API sqlite_manager : public connect_manager
{
public:
	/**
	 * ���캯��
	 * @param dbfile {const char*} sqlite ���ݿ�������ļ�
	 * @param dblimit {int} ���ݿ����ӳ��������������
	 */
	sqlite_manager(const char* dbfile, int dblimit = 64);
	~sqlite_manager();

protected:
	/**
	 * ���� connect_manager �麯����ʵ��
	 * @param addr {const char*} ������������ַ����ʽ��ip:port
	 * @param count {int} ���ӳصĴ�С����
	 * @param idx {size_t} �����ӳض����ڼ����е��±�λ��(�� 0 ��ʼ)
	 * @return {connect_pool*} ���ش��������ӳض���
	 */
	connect_pool* create_pool(const char* addr, int count, size_t idx);

private:
	// sqlite �����ļ���
	char* dbfile_;
	int   dblimit_;
};

} // namespace acl
