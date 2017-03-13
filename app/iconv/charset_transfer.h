#pragma once

class charset_transfer
{
public:
	charset_transfer(void) : utf8_bom_(false) {}
	~charset_transfer(void) {}

	charset_transfer& set_from_charset(const char* charset);
	charset_transfer& set_to_charset(const char* charset);
	charset_transfer& set_from_path(const char* path);
	charset_transfer& set_to_path(const char* path);
	charset_transfer& set_utf8bom(bool yes);

	int run(bool recursive = true);

	bool check_file(const char* filepath, const char* charset);
	int check_charset(const char* charset);

private:
	acl::string from_charset_;
	acl::string to_charset_;
	acl::string from_path_;
	acl::string to_path_;
	bool utf8_bom_;

	bool check(void);
	bool get_filepath(acl::scan_dir& scan, const char* filename,
		acl::string& from_filepath, acl::string& to_filepath,
		acl::string& to_path);
	bool transfer(const char* from_file, const char* to_file);
};
