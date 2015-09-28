#include "stdafx.h"

static bool test(const char* in, const char* tag, bool once,
	acl::string& out, acl::string* left = NULL)
{
	acl::json json;
	const char* ptr = NULL, *p1 = in;
	char  buf[2];
       
	if (once)
		ptr = json.update(p1);
	else
	{
		while (*p1)
		{
			buf[0] = *p1;
			buf[1] = 0;
			ptr = json.update(buf);
			p1++;
		}
	}

	const std::vector<acl::json_node*> &receiver = 
		json.getElementsByTagName(tag);

	std::vector<acl::json_node*>::const_iterator cit;
	for (cit = receiver.begin(); cit != receiver.end(); ++cit)
	{
		acl::json_node* node = (*cit)->get_obj();
		if (node == NULL)
		{
			printf("get_obj null\r\n");
			break;
		}

		printf("--------list all elements of the array --------\r\n");

		acl::json_node* child = node->first_child();
		while (child)
		{
			const char* txt = child->get_text();
			if (txt && *txt)
				printf("%s\r\n", txt);
			else
				printf("null\r\n");
			child = node->next_child();
		}

		printf("----------------- list end --------------------\r\n");
	}

	printf("-------------------------------------------------------\r\n");

	printf("%s\r\n", in);

	printf("-------------------------------------------------------\r\n");

	printf("json finish: %s, left char: %s\r\n",
		json.finish() ? "yes" : "no", ptr);
	if (left)
		*left = ptr;

	printf(">>>to string: %s\r\n", json.to_string().c_str());
	out = json.to_string();

	return true;
}

int main()
{
	const char* ptr =
		"{\r\n"
		"	\"data\" : \"dGVzdHFxcQ==\", \r\n"
		"	\"receiver_id\" : [\r\n"
		"		\"1442365683\"\r\n"
		"	],\r\n"
		"	\"extra\" : \"\",\r\n"
		"	\"group_id\" : \"\"\r\n"
		"}\r\n";

	acl::string buf;

	if (test(ptr, "receiver_id", false, buf) == false)
		return 1;

	const char* s = "{\"data\": \"dGVzdHFxcQ==\", \"receiver_id\": [\"1442365683\"], \"extra\": \"\", \"group_id\": \"\"}";
	if (buf == s)
		printf("----OK----\r\n");
	else
	{
		printf("----Error----\r\n");
		return 1;
	}

	printf("Enter any key to continue ...");
	fflush(stdout);
	getchar();

	ptr =   "{ 'forward': [\r\n"
		"  { 'disable': 'false' },\r\n"
		"  { 'disable': false },\r\n"
		"  { 'status': [ true, false, true ]},\r\n"
		"  { 'status': [ 'true', 'false', 'true' ]},\r\n"
		"  { 'status': [ 'true', false, 'true' ]},\r\n"
		"  { 'number': 123456 },\r\n"
		"  { 'number': '123456' },\r\n"
		"  { 'number': [ 1, 2, 3, 4, 5, 6 ]},\r\n"
		"  { 'number': [ '1', '2', '3', '4', '5', '6' ]},\r\n"
		"  { 'number': [ '1', 2, '3', 4, '5', 6 ]},\r\n"
		"  { 'url' : 'http://127.0.0.1/' },\r\n"
		"  {\r\n"
		"    'deny': [\r\n"
		"         {\r\n"
		"           'host': [\r\n"
		"              'baidu.com',\r\n"
		"              'sina.com'\r\n"
		"           ]\r\n"
		"         }\r\n"
		"    ]\r\n"
		"  },\r\n"
		"  {\r\n"
		"    'allow': [\r\n"
		"         '111',\r\n"
		"         '222'\r\n"
		"      ]\r\n"
		"  }\r\n"
		"]}\r\n"
		"{ 'user': { 'name': 'test', age: 111, male: true }\r\n";

	acl::string left;

	if (test(ptr, "number", true, buf, &left) == false)
		return 1;

	s = "{\"forward\": [{\"disable\": \"false\"}, "
		"{\"disable\": false}, "
		"{\"status\": [true, false, true]}, "
		"{\"status\": [\"true\", \"false\", \"true\"]}, "
		"{\"status\": [\"true\", false, \"true\"]}, "
		"{\"number\": 123456}, "
		"{\"number\": \"123456\"}, "
		"{\"number\": [1, 2, 3, 4, 5, 6]}, "
		"{\"number\": [\"1\", \"2\", \"3\", \"4\", \"5\", \"6\"]}, "
		"{\"number\": [\"1\", 2, \"3\", 4, \"5\", 6]}, "
		"{\"url\": \"http://127.0.0.1/\"}, "
		"{\"deny\": [{\"host\": [\"baidu.com\", \"sina.com\"]}]}, "
		"{\"allow\": [\"111\", \"222\"]}"
	    "]}";

	if (buf == s)
		printf("----OK----\r\n");
	else
	{
		printf("----Error----\r\n");
		printf("%s\r\n", s);
		printf("%s\r\n", buf.c_str());
		return 1;
	}

	if (!left.empty())
	{
		printf("Enter any key to continue ...");
		fflush(stdout);
		getchar();

		if (test(left.c_str(), "number", false, buf) == false)
		{
			printf("----Error----\r\n");
			return 1;
		}

		printf("----OK----\r\n");
	}

	return 0;
}
