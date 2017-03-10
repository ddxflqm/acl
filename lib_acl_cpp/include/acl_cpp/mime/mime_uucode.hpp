#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/mime/mime_code.hpp"

namespace acl {

class ACL_CPP_API mime_uucode : public mime_code
{
public:
	/**
	 * 构造函数
	 * @param addCrlf {bool} 非流式编码时是否在末尾添加 "\r\n"
	 * @param addInvalid {bool} 流式解码时是否遇到非法字符是否原样拷贝
	 */
	mime_uucode(bool addCrlf = false, bool addInvalid  = false);
	~mime_uucode();

	/**
	 * 静态编码函数，直接将输入数据进行编码同时存入用户缓冲区
	 * 用户缓冲区
	 * @param in {const char*} 输入数据地址
	 * @param n {int} 输入数据长度
	 * @param out {string*} 存储结果的缓冲区
	 */
	static void encode(const char* in, int n, string* out);

	/**
	 * 静态解码函数，直接将输入数据进行解析并存入用户缓冲区
	 * @param in {const char*} 输入数据地址
	 * @param n {int} 数据长度
	 * @param out {string*} 存储解析结果
	 */
	static void decode(const char* in, int n, string* out);

protected:
private:
};

} // namespace acl
