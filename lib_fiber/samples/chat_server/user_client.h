#pragma once
#include <list>

#define	ERRNO_LOGOUT	100000

enum
{
	MT_MSG,
	MT_LOGOUT,
};

class user_client
{
public:
	user_client(acl::socket_stream& conn) : conn_(conn) {}
	~user_client(void)
	{
		for (std::list<acl::string*>::iterator it = messages_.begin();
			it != messages_.end(); ++it)
		{
			delete *it;
		}
	}

	acl::socket_stream& get_stream(void) const
	{
		return conn_;
	}

	bool already_login(void) const
	{
		return !name_.empty();
	}

	bool empty(void) const
	{
		return messages_.empty();
	}

	acl::string* pop(void)
	{
		if (messages_.empty())
			return NULL;
		acl::string* msg = messages_.front();
		messages_.pop_front();
		return msg;
	}

	void push(const char* msg)
	{
		acl::string* buf = new acl::string(msg);
		(*buf) << "\r\n";
		messages_.push_back(buf);
	}

	void set_name(const char* name)
	{
		name_ = name;
	}

	const char* get_name(void) const
	{
		return name_.c_str();
	}

	void wait(int& mtype)
	{
		chan_.pop(mtype);
	}

	void notify(int mtype)
	{
		chan_.put(mtype);
	}

	void shutdown(void)
	{
		printf(">>>close sock: %d\r\n", conn_.sock_handle());
		if (fiber_reader_ != NULL)
		{
			acl_fiber_set_errno(fiber_reader_, ERRNO_LOGOUT);
			acl_fiber_keep_errno(fiber_reader_, 1);
			acl_fiber_ready(fiber_reader_);
		}
		else if (0)
		{
			::close(conn_.sock_handle());
			ACL_VSTREAM_SET_SOCK(conn_.get_vstream(), -1);
		}
		else
			::shutdown(conn_.sock_handle(), SHUT_RD);
	}

	void set_reading(bool yes)
	{
		reading_ = yes;
	}

	bool is_reading(void) const
	{
		return reading_;
	}

	void set_waiting(bool yes)
	{
		waiting_ = yes;
	}

	bool is_waiting(void) const
	{
		return waiting_;
	}

	void wait_exit(void)
	{
		int mtype;
		chan_exit_.pop(mtype);
	}

	void notify_exit(void)
	{
		int mtype = MT_LOGOUT;
		chan_exit_.put(mtype);
	}

	void set_reader(void)
	{
		fiber_reader_ = acl_fiber_running();
	}

	void set_waiter(void)
	{
		fiber_waiter_ = acl_fiber_running();
	}

private:
	acl::socket_stream& conn_;
	acl::channel<int> chan_;
	acl::channel<int> chan_exit_;
	acl::string name_;
	std::list<acl::string*> messages_;
	bool reading_ = false;
	bool waiting_ = false;
	ACL_FIBER* fiber_reader_ = NULL;
	ACL_FIBER* fiber_waiter_ = NULL;
};
