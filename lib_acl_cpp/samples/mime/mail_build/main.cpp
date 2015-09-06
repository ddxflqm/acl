#include <string>
#include "acl_cpp/lib_acl.hpp"

int main(void)
{
	acl::mail_message message("gbk");

	message.set_from("zsxxsz@263.net")
		.set_sender("zsx1@263.net")
		.set_replyto("zsx2@263.net")
		.add_to("\"֣����1\" <zsx1@sina.com>; \"֣����2\" <zsx2@sina.com>")
		.add_cc("\"֣����3\" <zsx1@163.com>; \"֣����4\" <zsx2@163.com>")
		.set_subject("���⣺�й��������У�");
	message.add_attachment("main.cpp", "text/plain")
		.add_attachment("Makefile", "text/plain");

	const char* text = "�й��������� TEXT ��ʽ";
	const char* html = "<html><body>�й��������� HTML ��ʽ</body></html>";
	acl::mail_body body("gbk");
	body.build(text, strlen(text), html, strlen(html));
	message.set_body(&body);

	const char* filepath = "./test.eml";
	if (message.compose(filepath) == false)
		printf("compose %s error: %s\r\n", filepath, acl::last_serror());
	else
		printf("compose %s ok\r\n", filepath);

#if defined(_WIN32) || defined(_WIN64)
	printf("Enter any key to exit...\r\n");
	getchar();
#endif

	return (0);
}
