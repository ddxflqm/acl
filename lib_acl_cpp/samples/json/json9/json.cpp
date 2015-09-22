#include "stdafx.h"

int main()
{
	const char* sss =
		"{\"DataKey1\": \"BindRule\", \"DataValue\": {\"waittime\": \"7\"}, \"null_key\": null}\r\n"
		"{\"DataKey2\": \"BindRule\", \"DataValue\": {\"waittime\": \"7\"}, \"null_key\": null}\r\n"
		"{\"hello world\"}\r\n";

	acl::json json;
	const char* ptr = json.update(sss);

	printf("src:\r\n%s\r\n", sss);

	printf("-------------------------------------------------------\r\n");

	printf("json finish: %s, left char: %s\r\n",
		json.finish() ? "yes" : "no", ptr);

	return 0;
}
