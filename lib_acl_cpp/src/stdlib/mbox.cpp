#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include <assert.h>
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/mbox.hpp"
#endif

namespace acl
{

static void free_callback(void *ctx)
{
	mbox* o = (mbox*) ctx;
	delete o;
}

mbox::mbox(void)
{
	mbox_ = acl_mbox_create();
}

mbox::~mbox(void)
{
	acl_mbox_free(mbox_, free_callback);
}

bool mbox::send(mobj* o)
{
	return acl_mbox_send(mbox_, o) == 0;
}

mobj* mbox::read(int timeout /* = 0 */, bool* success /* = NULL */)
{
	int ok;
	mobj* o = (mobj*) acl_mbox_read(mbox_, timeout, &ok);
	if (success)
		*success = ok ? true : false;
	return o;
}

size_t mbox::sent_count(void) const
{
	return acl_mbox_nsend(mbox_);
}

size_t mbox::read_count(void) const
{
	return acl_mbox_nread(mbox_);
}

} // namespace acl
