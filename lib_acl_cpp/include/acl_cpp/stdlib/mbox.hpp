#pragma once
#include "acl_cpp/acl_cpp_define.hpp"

struct ACL_MBOX;

namespace acl
{

class ACL_CPP_API mobj
{
public:
	mobj(void) {}
	virtual ~mobj(void) {}
};

class ACL_CPP_API mbox
{
public:
	mbox(void);
	~mbox(void);

	/**
	 * ������Ϣ����
	 * @param o {mobj*} �ǿ���Ϣ����
	 * @return {bool} �����Ƿ�ɹ�
	 */
	bool send(mobj* o);

	/**
	 * ������Ϣ����
	 * @param timeout {int} ���� 0 ʱ���ö��ȴ���ʱʱ��(��)��������Զ�ȴ��ߵ�����
	 *  ��Ϣ��������
	 * @param success {bool*} �������ڸ���ȷ���������Ƿ�ɹ�
	 * @return {mobj*} �� NULL ��ʾ����һ����Ϣ����Ϊ NULL ʱ������ͨ�� success
	 *  �����ķ���ֵ�������Ƿ�ɹ�
	 */
	mobj* read(int timeout = 0, bool* success = NULL);

	/**
	 * ͳ�Ƶ�ǰ�Ѿ����͵���Ϣ��
	 * @return {size_t}
	 */
	size_t sent_count(void) const;

	/**
	 * ͳ�Ƶ�ǰ�Ѿ����յ�����Ϣ��
	 * @return {size_t}
	 */
	size_t read_count(void) const;

private:
	ACL_MBOX* mbox_;
};

} // namespace acl
