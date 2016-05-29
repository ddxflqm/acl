#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lib_fiber.h"

static void echo_client(ACL_VSTREAM *cstream)
{
	char  buf[8192];
	int   ret;

	while (1) {
		ret = acl_vstream_gets(cstream, buf, sizeof(buf) - 1);
		if (ret == ACL_VSTREAM_EOF) {
			printf("gets error\r\n");
			break;
		}
		buf[ret] = 0;
		printf("gets line: %s", buf);

		if (acl_vstream_writen(cstream, buf, ret) == ACL_VSTREAM_EOF) {
			printf("write error\r\n");
			break;
		}
	}

	acl_vstream_close(cstream);
}

int main(void)
{
	const char *addr = "0.0.0.0:8089";
	ACL_VSTREAM *sstream = acl_vstream_listen(addr, 128);
	ACL_VSTREAM *cstream;

	//fiber_init();
	if (sstream == NULL) {
		printf("acl_vstream_listen error %s\r\n", acl_last_serror());
		return 1;
	}

	acl_non_blocking(ACL_VSTREAM_SOCK(sstream), ACL_NON_BLOCKING);

	for (;;) {
		cstream = acl_vstream_accept(sstream, NULL, 0);
		if (cstream == NULL) {
			printf("acl_vstream_accept error %s\r\n",
				acl_last_serror());
			return 1;
		}

		echo_client(cstream);
	}

	printf("call fiber_schedule\r\n");
	fiber_schedule();

	return 0;
}
