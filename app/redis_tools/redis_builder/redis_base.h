#pragma once

class redis_base
{
public:
	redis_base(void);
	virtual ~redis_base(void);

	const std::map<acl::string, acl::redis_node*>* get_masters(acl::redis&);
};
