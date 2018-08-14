// Copyright (c) 2012 Krodo
// Part of Bylins http://www.mud.ru

#ifndef OBJ_ENCHANT_HPP_INCLUDED
#define OBJ_ENCHANT_HPP_INCLUDED

#include "structs.h"

#include <string>
#include <vector>

namespace obj_sets
{
	struct ench_type;
}

namespace obj
{

enum
{
	// �� �������� ���� ITEM_ENCHANT
	ENCHANT_FROM_OBJ,
	// �� �������� ������
	ENCHANT_FROM_SET
};

// ������ �������� �� ������-�� ������ ���������
struct enchant
{
	enchant();
	// ���� ���� ������� �� ���������� �������� (ENCHANT_FROM_OBJ)
	enchant(OBJ_DATA *obj);
	// ���������� �������� ��� ���������
	void print(CHAR_DATA *ch) const;
	// ��������� ������ � �������� ��� ����� �������
	std::string print_to_file() const;
	// �������� ������ �� �������
	void apply_to_obj(OBJ_DATA *obj) const;

	// ��� ��������� ��������
	std::string name_;
	// ��� ��������� ��������
	int type_;
	// ������ APPLY �������� (affected[MAX_OBJ_AFFECT])
	std::vector<obj_affected_type> affected_;
	// ������� ������� (obj_flags.affects)
	FLAG_DATA affects_flags_;
	// ������ ������� (obj_flags.extra_flags)
	FLAG_DATA extra_flags_;
	// ������� �� ������� (obj_flags.no_flag)
	FLAG_DATA no_flags_;
	// ��������� ���� (+-)
	int weight_;
	// ������ �� ����� (���� �������� ������ � ������� ��������)
	int ndice_;
	int sdice_;
};

class Enchants
{
public:
	bool empty() const;
	std::string print_to_file() const;
	void print(CHAR_DATA *ch) const;
	bool check(int type) const;
	void add(const enchant &ench);
	// ���� ���������� ������ ��� (������� �������������, � �� ����), �������
	// �� ����������� ������, �.�. ������� ���������, ������� ����� � ������
	void update_set_bonus(OBJ_DATA *obj, const obj_sets::ench_type& ench);
	void remove_set_bonus(OBJ_DATA *obj);

private:
	std::vector<enchant> list_;
};

} // obj

#endif // OBJ_ENCHANT_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
