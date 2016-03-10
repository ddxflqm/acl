#pragma once

class redis_status
{
public:
	redis_status(const char* addr, int conn_timeout, int rw_timeout);
	~redis_status(void);

	void show_nodes(bool tree_mode = false);
	static void show_nodes(acl::redis& redis, bool tree_mode = false);

	static void show_nodes_tree(
		const std::map<acl::string, acl::redis_node*>&);
	static void show_nodes_tree(const std::vector<acl::redis_node*>& nodes);

	static bool show_nodes(const std::map<acl::string,
		acl::redis_node*>* masters);

	static void show_master_slots(const acl::redis_node* master);
	static void show_slave_nodes(const std::vector<acl::redis_node*>& slaves);

	void show_slots();
	static bool show_slots(acl::redis& redis);
	static void show_slaves_slots(const acl::redis_slot* slot);

	static void sort(const std::map<acl::string, acl::redis_node*>& in,
		std::map<acl::string, std::vector<acl::redis_node*>* >& out);
	static void clear(std::map<acl::string,
		std::vector<acl::redis_node*>* >& nodes);

private:
	acl::string addr_;
	int conn_timeout_;
	int rw_timeout_;
};
