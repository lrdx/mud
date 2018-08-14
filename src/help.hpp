// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef HELP_HPP_INCLUDED
#define HELP_HPP_INCLUDED

#include "char_player.hpp"

#include <string>
#include <map>
#include <vector>
#include <array>

/// STATIC �������:
/// 	��� ������� �������
/// 	���������� ����� (������ � �����)
/// 	����-����
/// DYNAMIC �������:
/// 	����-�����
/// 	������� �� ����� �����
namespace HelpSystem
{

extern bool need_update;
enum Flags { STATIC, DYNAMIC, TOTAL_NUM };

// ���������� ����������� �������, ������� ����������� ����������� �����
void add_static(const std::string &key, const std::string &entry,
	int min_level = 0, bool no_immlog = false);
// � ������������ ������� ��� � ���������� no_immlog � 0 min_level
void add_dynamic(const std::string &key, const std::string &entry);
// ���������� �����, ���� � DYNAMIC ������ � ���������� sets_drop_page
void add_sets_drop(const std::string& key_str, const std::string& entry_str);
// ����/������ ����������� ������� �������
void reload(HelpSystem::Flags sort_flag);
// ����/������ ���� �������
void reload_all();
// �������� ��� � ������ ����� �� �������� ������������ ������� (����-�����, �����-����)
void check_update_dynamic();

} // namespace HelpSystem

namespace PrintActivators
{

// ��������� ������ ��� ����� �����
struct clss_activ_node
{
	clss_activ_node() { total_affects = clear_flags; };

	// �������
	FLAG_DATA total_affects;
	// ��������
	std::vector<obj_affected_type> affected;
	// �����
	CObjectPrototype::skills_t skills;
};

std::string print_skill(const CObjectPrototype::skills_t::value_type &skill, bool activ);
std::string print_skills(const CObjectPrototype::skills_t &skills, bool activ, bool header = true);
void sum_skills(CObjectPrototype::skills_t &target, const CObjectPrototype::skills_t::value_type &add);
void sum_skills(CObjectPrototype::skills_t &target, const CObjectPrototype::skills_t &add);

/// l - ������ <obj_affected_type> ���� ���������,
/// r - ������ ���� ��, ������� ��������� � l
template <class T, class N>
void sum_apply(T &l, const N &r)
{
	for (auto ri = r.begin(); ri != r.end(); ++ri)
	{
		if (ri->modifier == 0)
		{
			continue;
		}

		auto li = std::find_if(l.begin(), l.end(),
			[&](const auto& obj_aff)
		{
			return obj_aff.location == ri->location;
		});

		if (li != l.end())
		{
			li->modifier += ri->modifier;
		}
		else
		{
			l.push_back(*ri);
		}
	}
}

template <class T>
void add_pair(T &target, const typename T::value_type &add)
{
	if (add.first > 0)
	{
		auto i = target.find(add.first);
		if (i != target.end())
		{
			i->second += add.second;
		}
		else
		{
			target[add.first] = add.second;
		}
	}
}

template <class T>
void add_map(T &target, const T &add)
{
	for (auto i = add.begin(), iend = add.end(); i != iend; ++i)
	{
		if (i->first > 0)
		{
			auto ii = target.find(i->first);
			if (ii != target.end())
			{
				ii->second += i->second;
			}
			else
			{
				target[i->first] = i->second;
			}
		}
	}
}

} // namespace PrintActivators

void do_help(CHAR_DATA *ch, char *argument, int cmd, int subcmd);

#endif // HELP_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
