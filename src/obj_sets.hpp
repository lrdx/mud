// Copyright (c) 2014 Krodo
// Part of Bylins http://www.mud.ru

#ifndef OBJ_SETS_HPP_INCLUDED
#define OBJ_SETS_HPP_INCLUDED

#include "obj.hpp"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"

#include <array>
#include <vector>
#include <set>

/// ������ ������� ������� �� �������� ����������� �������� �� ����, � �� ��
/// ���������� ��������, �������� �� ������ ����� + ��� ����� ����������� ���
namespace obj_sets
{
int normalize_vnum(int vnum);
struct idx_node
{
	// ������ ����
	size_t set_idx;
	// �������� �� ���� �� ������� ����
	std::vector<OBJ_DATA *> obj_list;
	// ���-�� ��� �������������� ������ (��� ��������� ���������/�����������)
	int activated_cnt;
	// ���������� ��� ��� ��������� ���������
	bool show_msg;
};

/// ���������� ������ ��������� � ��������� �� �����
/// ��� ����� ������� ��������� � ���� � affect_total()
class WornSets
{
public:
	/// ������� ����� ������ ��������������
	void clear();
	/// ���������� ������ ����� idx_list_ ��� ������ ��������� �� ����
	void add(OBJ_DATA *obj);
	/// �������� ����������� (��� ����� �����)
	void check(CHAR_DATA *ch);

private:
	std::array<idx_node, NUM_WEARS> idx_list_;
};

struct bonus_type
{
	bonus_type() : phys_dmg(0), mage_dmg(0) {};

	int phys_dmg;
	int mage_dmg;

	bool operator!=(const bonus_type &r) const;
	bool operator==(const bonus_type &r) const;
	bonus_type& operator+=(const bonus_type &r);
	bool empty() const;
};

struct ench_type
{
	ench_type() : weight(0), ndice(0), sdice(0) {};

	int weight;
	int ndice;
	int sdice;

	bool operator!=(const ench_type &r) const;
	bool operator==(const ench_type &r) const;
	ench_type& operator+=(const ench_type &r);
	bool empty() const;
};

struct activ_node;

struct activ_sum
{
	activ_sum()
	{
		affects = clear_flags;
	};

	// ������������ �����������
	activ_sum& operator+=(const activ_node *r);

	bool operator!=(const activ_sum &r) const;
	bool operator==(const activ_sum &r) const;
	bool empty() const;
	void clear();

	void update(CHAR_DATA *ch);
	void apply_affects(CHAR_DATA *ch) const;

	int get_skill(const ESkill num) const;
	int calc_phys_dmg(int dam) const;
	int calc_mage_dmg(int dam) const;

	// ������� (obj_flags.affects)
	FLAG_DATA affects;
	// APPLY_XXX ������� (affected[MAX_OBJ_AFFECT])
	std::vector<obj_affected_type> apply;
	// +����� � ����� �������� ������ ������
	CObjectPrototype::skills_t skills;
	// �������� ������� ������
	bonus_type bonus;
	// ���� ������ - ������� � �����
	std::map<int, ench_type> enchants;
};

void load();
void save();
void print_off_msg(CHAR_DATA *ch, OBJ_DATA *obj);
void print_identify(CHAR_DATA *ch, const OBJ_DATA *obj);
void init_xhelp();
std::set<int> vnum_list_add(int vnum);
std::string get_name(size_t idx);
bool is_set_item(OBJ_DATA *obj);

} // namespace obj_sets

namespace obj_sets_olc
{

void parse_input(CHAR_DATA *ch, const char *arg);

} // namespace obj_sets_olc

void do_slist(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
void do_sedit(CHAR_DATA *ch, char *argument, int cmd, int subcmd);

#endif // OBJ_SETS_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
