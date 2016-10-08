#pragma once

struct base
{
	std::string string;
	std::string *string_ptr;
	int a;
	int *a_ptr;
	unsigned int b;
	unsigned int *b_ptr;
	int64_t c;
	int64_t *c_ptr;
	unsigned long d;
	unsigned long *d_ptr;
	unsigned long long e;
	unsigned long long *e_ptr;
	long f;
	long *f_ptr;
	long long g;
	long long *g_ptr;
	acl::string acl_string;
	acl::string *acl_string_ptr;

	float h;
	float *h_ptr;
	double i;
	double *i_ptr;

	base()
	{
		a_ptr = NULL;
		b_ptr = NULL;
		c_ptr = NULL;
		d_ptr = NULL;
		e_ptr = NULL;
		f_ptr = NULL;
		g_ptr = NULL;
		acl_string_ptr = NULL;
	}

	~base()
	{
		delete a_ptr;
		delete b_ptr;
		delete c_ptr;
		delete d_ptr;
		delete e_ptr;
		delete f_ptr;
		delete acl_string_ptr;
	}
};

struct list1
{
	list1()
	{
		b_ptr = NULL;
		bases_list_ptr = NULL;
		bases_ptr_list_ptr = NULL;
	}

	~list1()
	{
		delete b_ptr;
		delete bases_list_ptr;
		delete bases_ptr_list_ptr;
	}

	base b;
	base *b_ptr;
	std::list<base> bases_list;
	std::list<base> *bases_list_ptr;
	std::list<base*> *bases_ptr_list_ptr;
};

