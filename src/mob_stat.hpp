// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#ifndef MOB_STAT_HPP_INCLUDED
#define MOB_STAT_HPP_INCLUDED

#include "structs.h"

#include <unordered_map>
#include <array>
#include <list>
#include <string>

/// ���������� �� ��������� �����/������ � �������� �� ������ ������ � ������
namespace mob_stat
{

/// ����. ���-�� ���������� � ������ ����������� � ����������
const int MAX_GROUP_SIZE = 12;
/// ������ ���������� mob_stat.xml (������)
const int SAVE_PERIOD = 27;
/// 0 - ������� ����� �������, 1..MAX_GROUP_SIZE - ������� ���� ��������
typedef std::array<int, MAX_GROUP_SIZE + 1> KillStatType;

struct mob_node
{
	mob_node() : month(0), year(0)
	{
		kills.fill(0);
	};
	// ����� (1..12)
	int month;
	// ��� (����)
	int year;
	// ����� �� ��������� �� ������ �����
	KillStatType kills;
};

/// ������ ����� �� ����� � �������
extern std::unordered_map<int, std::list<mob_node>> mob_list;

/// ���� mob_stat.xml
void load();
/// ���� mob_stat.xml
void save();
/// ����� ���� ���� �� show stats
void show_stats(CHAR_DATA *ch);
/// ���������� ����� �� ����
/// \param members ���� = 0 - ��. KillStatType
void add_mob(CHAR_DATA *mob, int members);
/// ������ ���������� ���� �� ���������� ���� (show mobstat zone_vnum)
void show_zone(CHAR_DATA *ch, int zone_vnum, int months);
/// ������� ����� �� ���� ����� �� ���� zone_vnum
void clear_zone(int zone_vnum);
/// ������� ���-���������� �� ��������� months ������� (0 = ���)
mob_node sum_stat(const std::list<mob_node> &mob_stat, int months);

} // namespace mob_stat

namespace char_stat
{

/// ����� ����� - ��� '����������'
extern int mkilled;
/// ������� ����� - ��� '����������'
extern int pkilled;
/// ���������� ����� �� ������ - ��� '����������'
void add_class_exp(unsigned class_num, int exp);
/// ���������� ����� �� ������ - ��� '����������'
std::string print_class_exp(CHAR_DATA *ch);
/// ���������� ����� ������� ��������� ���������� ����� �� ������
void log_class_exp();

} // namespace char_stat

#endif // MOB_STAT_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
