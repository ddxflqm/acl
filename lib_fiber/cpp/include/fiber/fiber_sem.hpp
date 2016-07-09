#pragma once

struct ACL_FIBER_SEM;

namespace acl {

class fiber_sem : public noncopyable
{
public:
	fiber_sem(int max);
	~fiber_sem(void);

	int wait(void);
	int trywait(void);
	int post(void);

private:
	ACL_FIBER_SEM* sem_;
};

} // namespace acl
