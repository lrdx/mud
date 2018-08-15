#ifndef __ALIAS_HPP__
#define __ALIAS_HPP__

class CHAR_DATA;	// forward declaration to avoid inclusion of char.hpp and any dependencies of that header.

struct alias_data
{
	char *alias;
	char *replacement;
	int type;
	struct alias_data *next;
};

// public functions in alias.cpp
void write_aliases(CHAR_DATA * ch);
void read_aliases(CHAR_DATA * ch);
alias_data* find_alias(alias_data *alias_list, const char *str);
void free_alias(alias_data *a);

#endif	//__ALIAS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
