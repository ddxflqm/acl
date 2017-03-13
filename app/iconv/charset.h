#pragma once

class radar 
{
public:
	radar(void);
	~radar(void);

	/**
	 * 识别给定字符串的字符集
	 * @param data 需要识别的字符串
	 * @param len  字符串长度
	 * @param charset_result  识别的字符集
	 * @return {bool} 是否识别成功
	 */
	bool detact(const char *data, int len, acl::string &charset_result);
	bool detact(acl::string &data, acl::string &charset_result);

	/*
	 * 设置是否开启调试模式
	 */
	void setDebugMode(bool flag)
	{
		___debug_mode = flag;
	}

private:
	bool ___debug_mode;
};

//bool format_utf8(const char *str, int len, acl::string &out);
