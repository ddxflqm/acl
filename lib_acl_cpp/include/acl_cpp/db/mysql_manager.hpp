#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/connpool/connect_manager.hpp"

namespace acl {

class ACL_CPP_API mysql_manager : public connect_manager
{
public:
	/**
	 * ���� mysql ���ݿ�ʱ�Ĺ��캯��
	 * @param dbaddr {const char*} mysql ��������ַ����ʽ��IP:PORT��
	 *  �� UNIX ƽ̨�¿���Ϊ UNIX ���׽ӿ�
	 * @param dbname {const char*} ���ݿ���
	 * @param dbuser {const char*} ���ݿ��û�
	 * @param dbpass {const char*} ���ݿ��û�����
	 * @param dblimit {int} ���ݿ����ӳص��������������
	 * @param dbflags {unsigned long} mysql ���λ
	 * @param auto_commit {bool} �Ƿ��Զ��ύ
	 * @param conn_timeout {int} �������ݿⳬʱʱ��(��)
	 * @param rw_timeout {int} �����ݿ�ͨ��ʱ��IOʱ��(��)
	 */
	mysql_manager(const char* dbaddr, const char* dbname,
		const char* dbuser, const char* dbpass,
		int dblimit = 64, unsigned long dbflags = 0,
		bool auto_commit = true, int conn_timeout = 60,
		int rw_timeout = 60);
	~mysql_manager();

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
	char* dbaddr_;		// ���ݿ������ַ
	char* dbname_;          // ���ݿ���
	char* dbuser_;          // ���ݿ��˺�
	char* dbpass_;          // ���ݿ��˺�����
	int   dblimit_;         // ���ݿ����ӳ�����������
	unsigned long dbflags_; // �����ݿ�ʱ�ı�־λ
	bool  auto_commit_;     // �Ƿ��Զ��ύ�޸ĺ������
	int   conn_timeout_;    // �������ݿ�ĳ�ʱʱ��
	int   rw_timeout_;      // �����ݿ�ͨ�ŵĳ�ʱʱ��
};

} // namespace acl
