#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include <stdio.h>
#include "stdlib/acl_mystring.h"
#include "code/acl_xmlcode.h"
#include "xml/acl_xml2.h"

#endif

#define IS_DOCTYPE(ptr) ((*(ptr) == 'd' || *(ptr) == 'D')  \
	&& (*(ptr + 1) == 'o' || *(ptr + 1) == 'O')  \
	&& (*(ptr + 2) == 'c' || *(ptr + 2) == 'C')  \
	&& (*(ptr + 3) == 't' || *(ptr + 3) == 'T')  \
	&& (*(ptr + 4) == 'y' || *(ptr + 4) == 'Y')  \
	&& (*(ptr + 5) == 'p' || *(ptr + 5) == 'P')  \
	&& (*(ptr + 5) == 'E' || *(ptr + 6) == 'E'))

#define IS_ID(ptr) ((*(ptr) == 'i' || *(ptr) == 'I')  \
	&& (*(ptr + 1) == 'd' || *(ptr + 1) == 'D'))

#define IS_QUOTE(x) ((x) == '\"' || (x) == '\'')
#define IS_SPACE(c) ((c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n')
#define SKIP_WHILE(cond, ptr) { while(*(ptr) && (cond)) (ptr)++; }
#define SKIP_SPACE(ptr) { while(IS_SPACE(*(ptr))) (ptr)++; }

/* 状态机数据结构类型 */

struct XML_STATUS_MACHINE {
	/**< 状态码 */
	int   status;

	/**< 状态机处理函数 */
	const char *(*callback) (ACL_XML2*, const char*);
};

static const char *xml_parse_next_left_lt(ACL_XML2 *xml, const char *data)
{
	SKIP_SPACE(data);
	SKIP_WHILE(*data != '<', data);
	if (*data == 0)
		return NULL;
	data++;
	xml->curr_node->status = ACL_XML2_S_LLT;
	return data;
}

static const char *xml_parse_left_lt(ACL_XML2 *xml, const char *data)
{
	xml->curr_node->status = ACL_XML2_S_LCH;
	return data;
}

static void xml_parse_check_self_closed(ACL_XML2 *xml)
{
	if ((xml->curr_node->flag & ACL_XML2_F_LEAF) == 0) {
		/* if (acl_xml2_tag_leaf(STR(xml->curr_node->ltag))) */
		if (acl_xml2_tag_leaf(xml->curr_node->ltag))
			xml->curr_node->flag |= ACL_XML2_F_LEAF;
	}

	if ((xml->curr_node->flag & ACL_XML2_F_SELF_CL) == 0) {
		/*
		if (xml->curr_node->last_ch == '/'
		    || acl_xml2_tag_selfclosed(STR(xml->curr_node->ltag)))
		*/
		if (xml->curr_node->last_ch == '/'
		    || acl_xml2_tag_selfclosed(xml->curr_node->ltag))
		{
			xml->curr_node->flag |= ACL_XML2_F_SELF_CL;
		}
	}
}

static const char *xml_parse_left_gt(ACL_XML2 *xml, const char *data)
{
	xml->curr_node->last_ch = 0;
	xml->curr_node->status = ACL_XML2_S_TXT;

	return data;
}

static const char *xml_parse_left_ch(ACL_XML2 *xml, const char *data)
{
	int  ch = *data;

	if (ch == '!') {
		xml->curr_node->meta[0] = ch;
		xml->curr_node->status = ACL_XML2_S_LEM;
		data++;
	} else if (ch == '?') {
		xml->curr_node->meta[0] = ch;
		xml->curr_node->flag |= ACL_XML2_F_META_QM;
		xml->curr_node->status = ACL_XML2_S_MTAG;
		data++;
	} else
		xml->curr_node->status = ACL_XML2_S_LTAG;

	return data;
}

static const char *xml_parse_left_em(ACL_XML2 *xml, const char *data)
{
	if (*data == '-') {
		if (xml->curr_node->meta[1] != '-') {
			xml->curr_node->meta[1] = '-';
		} else if (xml->curr_node->meta[2] != '-') {
			xml->curr_node->meta[0] = 0;
			xml->curr_node->meta[1] = 0;
			xml->curr_node->meta[2] = 0;
			xml->curr_node->flag |= ACL_XML2_F_META_CM;
			xml->curr_node->status = ACL_XML2_S_MCMT;
		}
		data++;
	} else {
		if (xml->curr_node->ltag == xml->addr)
			xml->curr_node->ltag = xml->ptr;

		if (xml->curr_node->meta[1] == '-') {
			/* ADDCH(xml->curr_node->ltag, '-'); */
			*xml->ptr++ = '-';
			xml->curr_node->meta[1] = 0;
		}
		xml->curr_node->flag |= ACL_XML2_F_META_EM;
		xml->curr_node->status = ACL_XML2_S_MTAG;
	}

	/* ACL_VSTRING_TERMINATE(xml->curr_node->ltag); */
	return data;
}

static const char *xml_parse_meta_tag(ACL_XML2 *xml, const char *data)
{
	int   ch;

	if (xml->curr_node->ltag == xml->addr)
		xml->curr_node->ltag = xml->ptr;

	while ((ch = *data) != 0) {
		data++;
		if (IS_SPACE(ch) || ch == '>') {
			*xml->ptr++ = 0;
			xml->curr_node->status = ACL_XML2_S_MTXT;
			break;
		}
		/* ADDCH(xml->curr_node->ltag, ch); */
		*xml->ptr++ = ch;
	}
	/* ACL_VSTRING_TERMINATE(xml->curr_node->ltag); */
	return data;
}

static const char *xml_meta_attr_name(ACL_XML2_ATTR *attr, const char *data)
{
	int   ch;
	ACL_XML2 *xml = attr->node->xml;

	if (attr->name == xml->addr)
		attr->name = xml->ptr;

	while ((ch = *data) != 0) {
		if (ch == '=') {
			data++;
			/* ACL_VSTRING_TERMINATE(attr->name); */
			*xml->ptr++ = 0;
			break;
		}
		if (!IS_SPACE(ch))
			/* ADDCH(attr->name, ch); */
			*xml->ptr++ = ch;
		data++;
	}
	return data;
}

static const char *xml_meta_attr_value(ACL_XML2_ATTR *attr, const char *data)
{
	ACL_XML2 *xml = attr->node->xml;
	int   ch;

	SKIP_SPACE(data);
	if (IS_QUOTE(*data))
		attr->quote = *data++;
	if (*data == 0)
		return data;

	if (attr->value == xml->addr)
		attr->value = xml->ptr;

	while ((ch = *data) != 0) {
		if (attr->quote) {
			if (ch == attr->quote) {
				data++;
				*xml->ptr++ = 0;
				break;
			}
			/* ADDCH(attr->value, ch); */
			*xml->ptr++ = ch;
		} else if (IS_SPACE(ch)) {
			data++;
			*xml->ptr++ = 0;
			break;
		} else {
			/* ADDCH(attr->value, ch); */
			*xml->ptr++ = ch;
		}
		data++;
	}

	/* ACL_VSTRING_TERMINATE(attr->value); */
	return data;
}

static void xml_meta_attr(ACL_XML2_NODE *node)
{
	ACL_XML2_ATTR *attr;
	const char *ptr;
	int   ch;

	if (node->text == node->xml->addr || *node->text == 0)
		return;

	/* ptr = STR(node->text); */
	ptr = node->text;
	SKIP_SPACE(ptr);	/* 略过 ' ', '\t' */
	if (*ptr == 0)
		return;

	while ((ch = *ptr) != 0) {
		attr = acl_xml2_attr_alloc(node);
		ptr = xml_meta_attr_name(attr, ptr);
		if (*ptr == 0)
			break;
		ptr = xml_meta_attr_value(attr, ptr);
		if (*ptr == 0)
			break;
	}
}

static const char *xml_parse_meta_text(ACL_XML2 *xml, const char *data)
{
	int   ch;

	/* if (LEN(xml->curr_node->text) == 0) { */
	if (xml->curr_node->text == xml->addr)
		SKIP_SPACE(data);

	if (*data == 0)
		return NULL;

	if (xml->curr_node->text == xml->addr)
		xml->curr_node->text = xml->ptr;

	while ((ch = *data) != 0) {
		if (xml->curr_node->quote) {
			if (ch == xml->curr_node->quote) {
				xml->curr_node->quote = 0;
			}
			/* ADDCH(xml->curr_node->text, ch); */
			*xml->ptr++ = ch;
		} else if (IS_QUOTE(ch)) {
			if (xml->curr_node->quote == 0) {
				xml->curr_node->quote = ch;
			}
			/* ADDCH(xml->curr_node->text, ch); */
			*xml->ptr++ = ch;
		} else if (ch == '<') {
			xml->curr_node->nlt++;
			/* ADDCH(xml->curr_node->text, ch); */
			*xml->ptr++ = ch;
		} else if (ch != '>') {
			/* ADDCH(xml->curr_node->text, ch); */
			*xml->ptr++ = ch;
		} else if (xml->curr_node->nlt == 0) {
			char *last;
			/* size_t off; */

			data++;
			xml->curr_node->status = ACL_XML2_S_MEND;
			*xml->ptr++ = 0;
			if ((xml->curr_node->flag & ACL_XML2_F_META_QM) == 0)
				break;

			/*
			last = acl_vstring_end(xml->curr_node->text) - 1;
			if (last < STR(xml->curr_node->text) || *last != '?')
				break;
			off = ACL_VSTRING_LEN(xml->curr_node->text) - 1;
			if (off == 0)
				break;
			ACL_VSTRING_AT_OFFSET(xml->curr_node->text, (int) off);
			ACL_VSTRING_TERMINATE(xml->curr_node->text);
			*/
			last = xml->ptr;
			while (last > xml->curr_node->text) {
				if (*last == '?') {
					*last = 0;
					break;
				}
				last--;
			}
			if (last == xml->curr_node->text)
				break;

			xml_meta_attr(xml->curr_node);
			break;
		} else {
			xml->curr_node->nlt--;
			/* ADDCH(xml->curr_node->text, ch); */
			*xml->ptr++ = ch;
		}
		data++;
	}

	/* ACL_VSTRING_TERMINATE(xml->curr_node->text); */
	return data;
}

static const char *xml_parse_meta_comment(ACL_XML2 *xml, const char *data)
{
	int   ch;

	/* if (LEN(xml->curr_node->text) == 0) { */
	if (xml->curr_node->text == xml->addr)
		SKIP_SPACE(data);

	if (*data == 0)
		return NULL;

	if (xml->curr_node->text == xml->addr)
		xml->curr_node->text = xml->ptr;

	while ((ch = *data) != 0) {
		if (xml->curr_node->quote) {
			if (ch == xml->curr_node->quote) {
				xml->curr_node->quote = 0;
			} else {
				/* ADDCH(xml->curr_node->text, ch); */
				*xml->ptr++ = ch;
			}
		} else if (IS_QUOTE(ch)) {
			if (xml->curr_node->quote == 0) {
				xml->curr_node->quote = ch;
			} else {
				/* ADDCH(xml->curr_node->text, ch); */
				*xml->ptr++ = ch;
			}
		} else if (ch == '<') {
			xml->curr_node->nlt++;
			/* ADDCH(xml->curr_node->text, ch); */
			*xml->ptr++ = ch;
		} else if (ch == '>') {
			if (xml->curr_node->nlt == 0
				&& xml->curr_node->meta[0] == '-'
				&& xml->curr_node->meta[1] == '-')
			{
				data++;
				*xml->ptr++ = 0;
				xml->curr_node->status = ACL_XML2_S_MEND;
				break;
			}
			xml->curr_node->nlt--;
			/* ADDCH(xml->curr_node->text, ch); */
			*xml->ptr++ = ch;
		} else if (xml->curr_node->nlt > 0) {
			/* ADDCH(xml->curr_node->text, ch); */
			*xml->ptr++ = ch;
		} else if (ch == '-') {
			if (xml->curr_node->meta[0] != '-') {
				xml->curr_node->meta[0] = '-';
			} else if (xml->curr_node->meta[1] != '-') {
				xml->curr_node->meta[1] = '-';
			}
		} else {
			if (xml->curr_node->meta[0] == '-') {
				/* ADDCH(xml->curr_node->text, '-'); */
				*xml->ptr++ = '-';
				xml->curr_node->meta[0] = 0;
			}
			if (xml->curr_node->meta[1] == '-') {
				/* ADDCH(xml->curr_node->text, '-'); */
				*xml->ptr++ = '-';
				xml->curr_node->meta[1] = 0;
			}
			/* ADDCH(xml->curr_node->text, ch); */
			*xml->ptr++ = ch;
		}
		data++;
	}

	/* ACL_VSTRING_TERMINATE(xml->curr_node->text); */
	return data;
}

static const char *xml_parse_meta_end(ACL_XML2 *xml, const char *data)
{
	/* meta 标签是自关闭类型，直接跳至右边 '>' 处理位置 */
	xml->curr_node->status = ACL_XML2_S_RGT;
	return data;
}

static const char *xml_parse_left_tag(ACL_XML2 *xml, const char *data)
{
	int   ch;

	/* if (LEN(xml->curr_node->ltag) == 0) { */
	if (xml->curr_node->ltag == xml->addr)
		SKIP_SPACE(data);

	if (*data == 0)
		return NULL;

	if (xml->curr_node->ltag == xml->addr)
		xml->curr_node->ltag = xml->ptr;

	while ((ch = *data) != 0) {
		data++;
		if (ch == '>') {
			*xml->ptr++ = 0;
			xml->curr_node->status = ACL_XML2_S_LGT;
			xml_parse_check_self_closed(xml);
			if ((xml->curr_node->flag & ACL_XML2_F_SELF_CL)
				&& xml->curr_node->last_ch == '/')
			{
				/*
				acl_vstring_truncate(xml->curr_node->ltag,
					LEN(xml->curr_node->ltag) - 1);
				*/
				char *ptr = xml->curr_node->ltag;
				ptr += strlen(ptr) - 1;
				*ptr = 0;
			}
			break;
		} else if (IS_SPACE(ch)) {
			*xml->ptr++ = 0;
			xml->curr_node->status = ACL_XML2_S_ATTR;
			xml->curr_node->last_ch = ch;
			break;
		} else {
			/* ADDCH(xml->curr_node->ltag, ch); */
			*xml->ptr++ = ch;
			xml->curr_node->last_ch = ch;
		}
	}

	/* ACL_VSTRING_TERMINATE(xml->curr_node->ltag); */
	return data;
}

static const char *xml_parse_attr(ACL_XML2 *xml, const char *data)
{
	int   ch;
	ACL_XML2_ATTR *attr = xml->curr_node->curr_attr;

	/* if (attr == NULL || LEN(attr->name) == 0) { */
	if (attr == NULL || attr->name == xml->addr) {
		SKIP_SPACE(data);	/* 略过 ' ', '\t' */
		SKIP_WHILE(*data == '=', data);
	}

	if (*data == 0)
		return NULL;

	if (*data == '>') {
		xml->curr_node->status = ACL_XML2_S_LGT;
		xml_parse_check_self_closed(xml);
		xml->curr_node->curr_attr = NULL;
		data++;
		return data;
	}

	xml->curr_node->last_ch = *data;
	if (*data == '/') {
		data++;
		return data;
	}

	if (attr == NULL) {
		attr = acl_xml2_attr_alloc(xml->curr_node);
		xml->curr_node->curr_attr = attr;
		attr->name = xml->ptr;
	}

	while ((ch = *data) != 0) {
		xml->curr_node->last_ch = ch;
		if (ch == '=') {
			*xml->ptr++ = 0;
			xml->curr_node->status = ACL_XML2_S_AVAL;
			data++;
			break;
		}
		if (!IS_SPACE(ch))
			/* ADDCH(attr->name, ch); */
			*xml->ptr++ = ch;
		data++;
	}

	/* ACL_VSTRING_TERMINATE(attr->name); */
	return data;
}

static const char *xml_parse_attr_val(ACL_XML2 *xml, const char *data)
{
	int   ch;
	ACL_XML2_ATTR *attr = xml->curr_node->curr_attr;

	/* if (LEN(attr->value) == 0 && !attr->quote) { */
	if (attr->value == xml->addr && !attr->quote) {
		SKIP_SPACE(data);
		if (IS_QUOTE(*data))
			attr->quote = *data++;
	}

	if (*data == 0)
		return NULL;

	if (attr->value == xml->addr)
		attr->value = xml->ptr;

	while ((ch = *data) != 0) {
		if (attr->quote) {
			if (ch == attr->quote) {
				*xml->ptr++ = 0;
				xml->curr_node->status = ACL_XML2_S_ATTR;
				xml->curr_node->last_ch = ch;
				data++;
				break;
			}
			/* ADDCH(attr->value, ch); */
			*xml->ptr++ = ch;
			xml->curr_node->last_ch = ch;
		} else if (ch == '>') {
			*xml->ptr++ = 0;
			xml->curr_node->status = ACL_XML2_S_LGT;
			xml_parse_check_self_closed(xml);
			data++;
			break;
		} else if (IS_SPACE(ch)) {
			*xml->ptr++ = 0;
			xml->curr_node->status = ACL_XML2_S_ATTR;
			xml->curr_node->last_ch = ch;
			data++;
			break;
		} else {
			/* ADDCH(attr->value, ch); */
			*xml->ptr++ = ch;
			xml->curr_node->last_ch = ch;
		}
		data++;
	}

	/* ACL_VSTRING_TERMINATE(attr->value); */

	if (xml->curr_node->status != ACL_XML2_S_AVAL) {
		/*
		if (LEN(attr->value) > 0 && xml->decode_buf != NULL) {
			ACL_VSTRING_RESET(xml->decode_buf);
			acl_xml_decode(STR(attr->value), xml->decode_buf);
			if (LEN(xml->decode_buf) > 0)
				STRCPY(attr->value, STR(xml->decode_buf));
		}
		*/

		/* 将该标签ID号映射至哈希表中，以便于快速查询 */
		/* if (IS_ID(STR(attr->name)) && LEN(attr->value) > 0) { */
		if (IS_ID(attr->name) && *attr->value != 0) {
			/* const char *ptr = STR(attr->value); */
			const char *ptr = attr->value;

			/* 防止重复ID被插入现象 */
			if (acl_htable_find(xml->id_table, ptr) == NULL) {
				acl_htable_enter(xml->id_table, ptr, attr);

				/* 当该属性被加入哈希表后才会赋于节点 id */
				xml->curr_node->id = attr->value;
			}
		}

		/* 必须将该节点的当前属性对象置空，以便于继续解析时
		 * 可以创建新的属性对象
		 */
		xml->curr_node->curr_attr = NULL;
	}

	return data;
}

static const char *xml_parse_text(ACL_XML2 *xml, const char *data)
{
	int   ch;

	/* if (LEN(xml->curr_node->text) == 0) { */
	if (xml->curr_node->text == xml->addr)
		SKIP_SPACE(data);

	if (*data == 0)
		return NULL;

	if (xml->curr_node->text == xml->addr)
		xml->curr_node->text = xml->ptr;

	while ((ch = *data) != 0) {
		if (ch == '<') {
			data++;
			*xml->ptr++ = 0;
			xml->curr_node->status = ACL_XML2_S_RLT;
			break;
		}
		/* ADDCH(xml->curr_node->text, ch); */
		*xml->ptr++ = ch;
		data++;
	}

	/* ACL_VSTRING_TERMINATE(xml->curr_node->text); */

	if (xml->curr_node->status != ACL_XML2_S_RLT)
		return data;

	if ((xml->curr_node->flag & ACL_XML2_F_SELF_CL)) {
		/* 如果该标签是自关闭类型，则应使父节点直接跳至右边 '/' 处理
		 * 位置, 同时使本节点跳至右边 '>' 处理位置
		 */
		ACL_XML2_NODE *parent = acl_xml2_node_parent(xml->curr_node);
		if (parent != xml->root)
			parent->status = ACL_XML2_S_RLT;
		xml->curr_node->status = ACL_XML2_S_RGT;
	}

	/* if (LEN(xml->curr_node->text) == 0 || xml->decode_buf == NULL) */
	if (*xml->curr_node->text == 0 || xml->decode_buf == NULL)
		return data;

#if 0
	/* ACL_VSTRING_RESET(xml->decode_buf); */
	acl_xml_decode(STR(xml->curr_node->text), xml->decode_buf);
	if (LEN(xml->decode_buf) > 0)
		STRCPY(xml->curr_node->text, STR(xml->decode_buf));
#endif

	return data;
}

static const char *xml_parse_right_lt(ACL_XML2 *xml, const char *data)
{
	ACL_XML2_NODE *node;

	SKIP_SPACE(data);
	if (*data == 0)
		return NULL;

	if (*data == '/') {
		data++;
		*xml->ptr++ = 0;
		xml->curr_node->status = ACL_XML2_S_RTAG;
		return data;
	} else if ((xml->curr_node->flag & ACL_XML2_F_LEAF)) {
		/*
		ADDCH(xml->curr_node->text, '<');
		ADDCH(xml->curr_node->text, *data);
		ACL_VSTRING_TERMINATE(xml->curr_node->text);
		*/
		xml->curr_node->status = ACL_XML2_S_TXT;
		*xml->ptr++ = '<';
		*xml->ptr++ = *data;
		data++;
		return data;
	}

	/* 说明遇到了当前节点的子节点 */

	/* 重新设置当前节点状态，以便于其可以找到 "</" */
	xml->curr_node->status = ACL_XML2_S_TXT;

	/* 创建新的子节点，并将其加入至当前节点的子节点集合中 */

	node = acl_xml2_node_alloc(xml);
	acl_xml2_node_add_child(xml->curr_node, node);
	node->depth = xml->curr_node->depth + 1;
	if (node->depth > xml->depth)
		xml->depth = node->depth;
	xml->curr_node = node;
	xml->curr_node->status = ACL_XML2_S_LLT;
	return data;
}

static const char *xml_parse_right_gt(ACL_XML2 *xml, const char *data)
{
	xml->curr_node = acl_xml2_node_parent(xml->curr_node);
	if (xml->curr_node == xml->root)
		xml->curr_node = NULL;
	return data;
}

/* 因为该父节点其实为叶节点，所以需要更新附属于该伪父节点的
 * 子节点的深度值，都应与该伪父节点相同
 */ 
static void update_children_depth(ACL_XML2_NODE *parent)
{
	ACL_ITER  iter;
	ACL_XML2_NODE *child;

	acl_foreach(iter, parent) {
		child = (ACL_XML2_NODE*) iter.data;
		child->depth = parent->depth;
		update_children_depth(child);
	}
}

static void string_copy(char *to, const char *from)
{
	while (*from)
		*to++ = *from++;
	*to = 0;
}

/* 查找与右标签相同的父节点 */
static int search_match_node(ACL_XML2 *xml)
{
	ACL_XML2_NODE *parent, *node;
	ACL_ARRAY *nodes = acl_array_create(10);
	ACL_ITER iter;

	parent = acl_xml2_node_parent(xml->curr_node);
	if (parent != xml->root)
		acl_array_append(nodes, xml->curr_node);

	while (parent != xml->root) {
		/*
		if (acl_strcasecmp(STR(xml->curr_node->rtag),
			STR(parent->ltag)) == 0)
			acl_vstring_strcpy(parent->rtag,
				STR(xml->curr_node->rtag));
			ACL_VSTRING_RESET(xml->curr_node->rtag);
			ACL_VSTRING_TERMINATE(xml->curr_node->rtag);
			parent->status = ACL_XML2_S_RGT;
			xml->curr_node = parent;
			break;
		}
		*/
		if (acl_strcasecmp(xml->curr_node->rtag, parent->ltag) == 0) {
			parent->rtag = xml->ptr;
			string_copy(xml->ptr, xml->curr_node->rtag);
			parent->status = ACL_XML2_S_RGT;
			xml->curr_node = parent;
			break;
		}

		acl_array_append(nodes, parent);

		parent = acl_xml2_node_parent(parent);
	}

	if (parent == xml->root) {
		acl_array_free(nodes, NULL);
		return 0;
	}

	acl_foreach_reverse(iter, nodes) {
		node = (ACL_XML2_NODE*) iter.data;
		acl_ring_detach(&node->node);
		node->flag |= ACL_XML2_F_LEAF;
		node->depth = parent->depth + 1;
		update_children_depth(node);
		acl_xml2_node_add_child(parent, node);
	}
	acl_array_free(nodes, NULL);
	return 1;
}

static const char *xml_parse_right_tag(ACL_XML2 *xml, const char *data)
{
	int   ch;
	ACL_XML2_NODE *curr_node = xml->curr_node;

	/* if (LEN(curr_node->rtag) == 0) { */
	if (curr_node->rtag == xml->addr)
		SKIP_SPACE(data);

	if (*data == 0)
		return NULL;

	if (curr_node->rtag == xml->addr)
		curr_node->rtag = xml->ptr;

	while ((ch = *data) != 0) {
		if (ch == '>') {
			data++;
			*xml->ptr++ = 0;
			curr_node->status = ACL_XML2_S_RGT;
			break;
		}

		if (!IS_SPACE(ch))
			/* ADDCH(curr_node->rtag, ch); */
			*xml->ptr++ = ch;
		data++;
	}

	/* ACL_VSTRING_TERMINATE(curr_node->rtag); */

	if (curr_node->status != ACL_XML2_S_RGT)
		return data;

	/*
	if (acl_strcasecmp(STR(curr_node->ltag), STR(curr_node->rtag)) != 0) {
	*/
	if (acl_strcasecmp(curr_node->ltag, curr_node->rtag) != 0) {
		int   ret = 0;

		if ((xml->flag & ACL_XML2_FLAG_IGNORE_SLASH))
			ret = search_match_node(xml);
		if (ret == 0) {
			/* 如果节点标签名与开始标签名不匹配，
			 * 则需要继续寻找真正的结束标签
			 */ 
			/*
			acl_vstring_strcat(curr_node->text,
				STR(curr_node->rtag));
			ACL_VSTRING_RESET(curr_node->rtag);
			ACL_VSTRING_TERMINATE(curr_node->rtag);
			*/
			curr_node->text = xml->ptr;
			string_copy(xml->ptr, curr_node->rtag);

			/* 重新设置当前节点状态，以便于其可以找到 "</" */
			curr_node->status = ACL_XML2_S_TXT;
		}
	}

	return data;
}

static struct XML_STATUS_MACHINE status_tab[] = {
	{ ACL_XML2_S_NXT, xml_parse_next_left_lt },
	{ ACL_XML2_S_LLT, xml_parse_left_lt },
	{ ACL_XML2_S_LGT, xml_parse_left_gt },
	{ ACL_XML2_S_LCH, xml_parse_left_ch },
	{ ACL_XML2_S_LEM, xml_parse_left_em },
	{ ACL_XML2_S_LTAG, xml_parse_left_tag },
	{ ACL_XML2_S_RLT, xml_parse_right_lt },
	{ ACL_XML2_S_RGT, xml_parse_right_gt },
	{ ACL_XML2_S_RTAG, xml_parse_right_tag },
	{ ACL_XML2_S_ATTR, xml_parse_attr },
	{ ACL_XML2_S_AVAL, xml_parse_attr_val },
	{ ACL_XML2_S_TXT, xml_parse_text },
	{ ACL_XML2_S_MTAG, xml_parse_meta_tag },
	{ ACL_XML2_S_MTXT, xml_parse_meta_text },
	{ ACL_XML2_S_MCMT, xml_parse_meta_comment },
	{ ACL_XML2_S_MEND, xml_parse_meta_end },
};

void acl_xml2_update(ACL_XML2 *xml, const char *data)
{
	const char *ptr = data;

	/* XML 解析器状态机循环处理过程 */

	while (ptr && *ptr) {
		if (xml->curr_node == NULL) {
			SKIP_SPACE(ptr);
			if (*ptr == 0)
				break;
			xml->curr_node = acl_xml2_node_alloc(xml);
			acl_xml2_node_add_child(xml->root, xml->curr_node);
			xml->curr_node->depth = xml->root->depth + 1;
			if (xml->curr_node->depth > xml->depth)
				xml->depth = xml->curr_node->depth;
		}
		ptr = status_tab[xml->curr_node->status].callback(xml, ptr);
	}
}
