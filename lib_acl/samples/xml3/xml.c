#include <sys/mman.h>
#include <sys/stat.h>
#include "lib_acl.h"

static const char *__data1 =
	"<?xml version=\"1.0\"?>\r\n"
	"<?xml-stylesheet type=\"text/xsl\"\r\n"
	"\thref=\"http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl\"?>\r\n"
	"\t<!DOCTYPE refentry PUBLIC \"-//OASIS//DTD DocBook XML V4.1.2//EN\"\r\n"
	"\t\"http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd\" [\r\n"
	"	<!ENTITY xmllint \"<command>xmllint</command>\">\r\n"
	"]>\r\n"
	"<root name='root1' id='root_id_1'>\r\n"
	"	<user name='user11\"' value='zsx11' id='id11'> user zsx11 </user>\r\n"
	"	<user name='user12' value='zsx12' id='id12'> user zsx12 \r\n"
	"		<age year='1972'>my age</age>\r\n"
	"		<other><email name='zsxxsz@263.net'/>"
	"			<phone>"
	"				<mobile number='111111'> mobile number </mobile>"
	"				<office number='111111'> mobile number </office>"
	"			</phone>"
	"		</other>"
	"	</user>\r\n"
	"	<user name='user13' value='zsx13' id='id13'> user zsx13 </user>\r\n"
	"</root>\r\n"
	"<root name='root2' id='root_id_2'>\r\n"
	"	<user name='user21' value='zsx21' id='id21'> user zsx21 </user>\r\n"
	"	<user name='user22' value='zsx22' id='id22'> user zsx22 \r\n"
	"		<!-- date should be the date of the latest change or the release version -->\r\n"
	"		<age year='1972'>my age</age>\r\n"
	"	</user>\r\n"
	"	<user name='user23' value='zsx23' id='id23'> user zsx23 </user>\r\n"
	"</root>\r\n"
	"<root name = 'root3' id = 'root_id_3'>\r\n"
	"	<user name = 'user31' value = 'zsx31' id = 'id31'> user zsx31 </user>\r\n"
	"	<user name = 'user32' value = 'zsx32' id = 'id32'> user zsx32 </user>\r\n"
	"	<user name = 'user33' value = 'zsx33' id = 'id33'> user zsx33 \r\n"
	"		<age year = '1978' month = '12' day = '11'> bao bao </age>\r\n"
	"	</user>\r\n"
	"	<!-- still a bit buggy output, will talk to docbook-xsl upstream to fix this -->\r\n"
	"	<!-- <releaseinfo>This is release 0.5 of the xmllint Manual.</releaseinfo> -->\r\n"
	"	<!-- <edition>0.5</edition> -->\r\n"
	"	<user name = 'user34' value = 'zsx34' id = 'id34'> user zsx34 </user>\r\n"
	"</root>\r\n";

static const char *__data2 =
	"<?xml version=\"1.0\"?>\r\n"
	"<?xml-stylesheet type=\"text/xsl\"\r\n"
	"	href=\"http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl\"?>\r\n"
	"<!DOCTYPE refentry PUBLIC \"-//OASIS//DTD DocBook XML V4.1.2//EN\"\r\n"
	"	\"http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd\" [\r\n"
	"	<!ENTITY xmllint \"<command>xmllint</command>\">\r\n"
	"]>\r\n"
	"<root>test\r\n"
	"	<!-- <edition> - <!--0.5--> - </edition> -->\r\n"
	"	<user>zsx\r\n"
	"		<age>38</age>\r\n"
	"	</user>\r\n"
	"</root>\r\n"
	"<!-- <edition><!-- 0.5 --></edition> -->\r\n"
	"<!-- <edition>0.5</edition> -->\r\n"
	"<!-- <edition> -- 0.5 -- </edition> -->\r\n"
	"<root name='root' id='root_id'>test</root>\r\n";

static const char *__data3 = "<root id='tt' >hello <root2> hi </root2></root><br/>\r\n";

static const char* __data4 = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
	"<request action=\"get_location\" sid=\"YOU_CAN_GEN_SID\" user=\"admin@test.com\">\r\n"
	"	<tags1>\r\n"
	"		<module name=\"mail_ud_user\" />\r\n"
	"	</tags1>\r\n"
	"	<tags2>\r\n"
	"		<tags21>\r\n"
	"		<tags22>\r\n"
	"		<tags23>\r\n"
	"		<module name=\"mail_ud_user\" />\r\n"
	"	</tags2>\r\n"
	"	<tag3>\r\n"
	"		<module name=\"mail_ud_user\">\r\n"
	"	</tag3>\r\n"
	"	<tag4>\r\n"
	"		<module name=\"mail_ud_user\">\r\n"
	"	</tag4>\r\n"
	"</request>\r\n";

static const char* __data5 = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
	"<request action=\"get_location\" sid=\"YOU_CAN_GEN_SID\" user=\"admin@test.com\">\r\n"
	"	<tag3>\r\n"
	"		<module name=\"mail_ud_user\" />\r\n"
	"	</tag3>\r\n"
	"	<tag4>\r\n"
	"		<module name=\"mail_ud_user\" />\r\n"
	"	</tag4>\r\n"
	"</request>\r\n";

static const char* __data6 = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
	"<request action=\"get_location\" sid=\"YOU_CAN_GEN_SID\" user=\"admin@test.com\">\r\n"
	"	<tags2>\r\n"
	"		<tags21>\r\n"
	"		<tags22>\r\n"
	"		<tags23 />\r\n"
	"		<tags24 />\r\n"
	"		<tags25>\r\n"
	"		<tags26/>\r\n"
	"		<tags27>\r\n"
	"		<tags28/>\r\n"
	"		<tags29>\r\n"
	"		<tags30>\r\n"
	"	</tags2>\r\n"
	"</request>\r\n";

static const char* __data7 = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
	"<request action=\"get_location\" sid=\"YOU_CAN_GEN_SID\" user=\"admin@test.com\">\r\n"
	"	<tags2>\r\n"
	"		<tags22>\r\n"
	"		<tags23>\r\n"
	"		<tags24>\r\n"
	"		<tags25/>\r\n"
	"		<tags26/>\r\n"
	"		<tags27>\r\n"
	"		<tags28>\r\n"
	"		<tags29/>\r\n"
	"		<tags30>\r\n"
	"		<tags31>\r\n"
	"	</tags2>\r\n"
	"</request>\r\n";

static char *mmap_addr(size_t len)
{
	const char *filepath = "./local.map";
	int fd = open(filepath, O_RDWR | O_CREAT, 0600);
	char *ptr;

	if (fd == -1)
	{
		printf("open %s error %s\r\n", filepath, acl_last_serror());
		exit (1);
	}

	ptr = (char*) mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (ptr == NULL) {
		printf("mmap %s error %s\r\n", filepath, acl_last_serror());
		exit (1);
	}

	lseek(fd, len, SEEK_SET);
	write(fd, "\0", 1);
	close(fd);

	return ptr;
}

static void parse_xml(int once, const char *data)
{
	ACL_XML2 *xml;
	const char *ptr;
	ACL_ITER iter1;
	int   i, total, left;
	ACL_ARRAY *a;
	ACL_XML2_NODE *pnode;
	char *addr = mmap_addr(81920);

	xml = acl_xml2_alloc(addr);
	ptr = data;

	if (once) {
		/* 一次性地分析完整 xml 数据 */
		ACL_METER_TIME("-------------once begin--------------");
		acl_xml2_parse(xml, ptr);
	} else {
		/* 每次仅输入一个字节来分析 xml 数据 */
		ACL_METER_TIME("-------------stream begin--------------");
		while (*ptr != 0) {
			char  ch2[2];

			ch2[0] = *ptr;
			ch2[1] = 0;
			acl_xml2_parse(xml, ch2);
			ptr++;
		}
	}
	ACL_METER_TIME("-------------end--------------");
	printf("enter any key to continue ...\n");
	getchar();

	if (acl_xml2_is_complete(xml, "root")) {
		printf(">> Yes, the xml complete\n");
	} else {
		printf(">> No, the xml not complete\n");
	}

	total = xml->node_cnt;

	/* 遍历根结点的一级子结点 */
	acl_foreach(iter1, xml->root) {
		ACL_ITER iter2;

		ACL_XML2_NODE *node = (ACL_XML2_NODE*) iter1.data;
		printf("tag> %s, text: %s\n", node->ltag, node->text);

		/* 遍历一级子结点的二级子结点 */
		acl_foreach(iter2, node) {
			ACL_ITER iter3;
			ACL_XML2_NODE *node2 = (ACL_XML2_NODE*) iter2.data;

			printf("\ttag> %s, text: %s\n", node2->ltag, node2->text);

			/* 遍历二级子结点的属性 */
			acl_foreach(iter3, node2->attr_list) {
				ACL_XML2_ATTR *attr = (ACL_XML2_ATTR*) iter3.data;
				printf("\t\tattr> %s: %s\n", attr->name, attr->value);
			}
		}
	}

	printf("----------------------------------------------------\n");

	/* 从根结点开始遍历 xml 对象的所有结点 */

	acl_foreach(iter1, xml) {
		ACL_ITER iter2;
		ACL_XML2_NODE *node = (ACL_XML2_NODE*) iter1.data;

		for (i = 1; i < node->depth; i++) {
			printf("\t");
		}

		printf("tag> %s, text: %s\n", node->ltag, node->text);

		/* 遍历 xml 结点的属性 */
		acl_foreach(iter2, node->attr_list) {
			ACL_XML2_ATTR *attr = (ACL_XML2_ATTR*) iter2.data;

			for (i = 1; i < node->depth; i++) {
				printf("\t");
			}

			printf("\tattr> %s: %s\n", attr->name, attr->value);
		}
	}

	/* 根据标签名获得 xml 结点集合 */

	printf("--------- acl_xml2_getElementsByTagName ----------\n");
	a = acl_xml2_getElementsByTagName(xml, "user");
	if (a) {
		/* 遍历结果集 */
		acl_foreach(iter1, a) {
			ACL_XML2_NODE *node = (ACL_XML2_NODE*) iter1.data;
			printf("tag> %s, text: %s\n", node->ltag, node->text);
		}
		/* 释放数组对象 */
		acl_xml2_free_array(a);
	}


	/* 查询属性名为 name, 属性值为 user2_1 的所有 xml 结点的集合 */

	printf("--------- acl_xml2_getElementsByName ------------\n");
	a = acl_xml2_getElementsByName(xml, "user2_1");
	if (a) {
		/* 遍历结果集 */
		acl_foreach(iter1, a) {
			ACL_XML2_NODE *node = (ACL_XML2_NODE*) iter1.data;
			printf("tag> %s, text: %s\n", node->ltag, node->text);
		}
		/* 释放数组对象 */
		acl_xml2_free_array(a);
	}

	/* 查询属性名为 id, 属性值为 id2_2 的所有 xml 结点集合 */
	printf("----------- acl_xml2_getElementById -------------\n");
	pnode = acl_xml2_getElementById(xml, "id2_2");
	if (pnode) {
		printf("tag> %s, text: %s\n", pnode->ltag, pnode->text);
		/* 遍历该 xml 结点的属性 */
		acl_foreach(iter1, pnode->attr_list) {
			ACL_XML2_ATTR *attr = (ACL_XML2_ATTR*) iter1.data;
			printf("\tattr_name: %s, attr_value: %s\n",
				attr->name, attr->value);
		}

		pnode = acl_xml2_node_next(pnode);
		printf("----------------- the id2_2's next node is ---------------------\n");
		if (pnode) {
			printf("-------------- walk node -------------------\n");
			/* 遍历该 xml 结点的属性 */
			acl_foreach(iter1, pnode->attr_list) {
				ACL_XML2_ATTR *attr = (ACL_XML2_ATTR*) iter1.data;
				printf("\tattr_name: %s, attr_value: %s\n",
					attr->name, attr->value);
			}

		} else {
			printf("-------------- null node -------------------\n");
		}
	}

	pnode = acl_xml2_getElementById(xml, "id2_3");
	if (pnode) {
		int   ndel = 0, node_cnt;

		/* 删除该结点及其子结点 */
		printf(">>>before delete %s, total: %d\n", pnode->ltag, xml->node_cnt);
		ndel = acl_xml2_node_delete(pnode);
		node_cnt = xml->node_cnt;
		printf(">>>after delete id2_3(%d deleted), total: %d\n", ndel, node_cnt);
	}

	acl_foreach(iter1, xml) {
		ACL_XML2_NODE *node = (ACL_XML2_NODE*) iter1.data;
		printf(">>tag: %s\n", node->ltag);
	}

	pnode = acl_xml2_getElementById(xml, "id2_3");
	if (pnode) {
		printf("-------------- walk %s node -------------------\n", pnode->ltag);
		/* 遍历该 xml 结点的属性 */
		acl_foreach(iter1, pnode->attr_list) {
			ACL_XML2_ATTR *attr = (ACL_XML2_ATTR*) iter1.data;
			printf("\tattr_name: %s, attr_value: %s\n",
				attr->name, attr->value);
		}
	} else {
		printf("---- the id2_3 be deleted----\n");
	}

	/* 释放 xml 对象 */
	left = acl_xml2_free(xml);
	printf("free all node ok, total(%d), left is: %d\n", total, left);
}

static void test1(void)
{
	const char* data = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
	"<?xml-stylesheet type=\"text/xsl\"\r\n"
	"\thref=\"http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl\"?>\r\n"
	"<!DOCTYPE refentry PUBLIC \"-//OASIS//DTD DocBook XML V4.1.2//EN\"\r\n"
	"\t\"http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd\" [\r\n"
	"	<!ENTITY xmllint \"<command>xmllint</command>\">\r\n"
	"]>\r\n"
	"<root name1 = \"value1\" name2 = \"val\\ue2\" name3 = \"v\\al'ue3\">hello world!</root>\r\n";
	ACL_XML2 *xml;
	ACL_ITER node_it, attr_it;
	ACL_XML2_NODE *node;
	const char *encoding, *type, *href;
	char *addr = mmap_addr(1024);

	xml = acl_xml2_alloc(addr);

	printf("------------------------------------------------------\r\n");

	printf("%s\r\n", data);

	printf("------------------------------------------------------\r\n");

	acl_xml2_update(xml, data);
	acl_foreach(node_it, xml) {
		ACL_XML2_NODE *tmp = (ACL_XML2_NODE*) node_it.data;
		printf("tag: %s, size: %ld\r\n", tmp->ltag, tmp->ltag_size);
		printf("\ttext: %s, size: %ld\r\n", tmp->text, tmp->text_size);
		acl_foreach(attr_it, tmp->attr_list) {
			ACL_XML2_ATTR *attr = (ACL_XML2_ATTR*) attr_it.data;
			printf("\tattr: %s(%ld)=\"%s\"(%ld)\r\n",
				attr->name, attr->name_size,
				attr->value, attr->value_size);
		}
	}

	printf("------------------------------------------------------\r\n");

	encoding = acl_xml2_getEncoding(xml);
	type = acl_xml2_getType(xml);
	node = acl_xml2_getElementMeta(xml, "xml-stylesheet");
	if (node)
		href = acl_xml2_getElementAttrVal(node, "href");
	else
		href = NULL;

	printf("xml encoding: %s, type: %s, href: %s\r\n",
		encoding ? encoding : "null", type ? type : "null",
		href ? href : "null");

	printf("------------------------------------------------------\r\n");

	acl_xml2_free(xml);
}

static void usage(const char *procname)
{
	printf("usage: %s -h[help]"
		" -s[parse once]\n"
		" -p[print] data1|data2|data3|data4|data5|data6|data7\n"
		" -d[which data] data1|data2|data3|data4|data5|data6|data7\n",
		procname);
}

#ifdef WIN32
#define snprintf _snprintf
#endif

int main(int argc, char *argv[])
{
	int   ch, once = 0;
	const char *data = __data1;

	test1();

	printf("Enter any key to continue ...\r\n");
	getchar();

	while ((ch = getopt(argc, argv, "hsp:d:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 's':
			once = 1;
			break;
		case 'd':
			if (strcasecmp(optarg, "data2") == 0)
				data = __data2;
			else if (strcasecmp(optarg, "data3") == 0)
				data = __data3;
			else if (strcasecmp(optarg, "data4") == 0)
				data = __data4;
			else if (strcasecmp(optarg, "data5") == 0)
				data = __data5;
			else if (strcasecmp(optarg, "data6") == 0)
				data = __data6;
			else if (strcasecmp(optarg, "data7") == 0)
				data = __data7;
			break;
		case 'p':
			if (strcasecmp(optarg, "data1") == 0)
				printf("%s\n", __data1);
			else if (strcasecmp(optarg, "data2") == 0)
				printf("%s\n", __data2);
			else if (strcasecmp(optarg, "data3") == 0)
				printf("%s\n", __data3);
			else if (strcasecmp(optarg, "data4") == 0)
				printf("%s\n", __data4);
			else if (strcasecmp(optarg, "data5") == 0)
				printf("%s\n", __data5);
			else if (strcasecmp(optarg, "data6") == 0)
				printf("%s\n", __data6);
			else if (strcasecmp(optarg, "data7") == 0)
				printf("%s\n", __data7);
			return (0);
		default:
			break;
		}
	}

	parse_xml(once, data);

#ifdef	ACL_MS_WINDOWS
	printf("ok, enter any key to exit ...\n");
	getchar();
#endif
	return 0;
}
