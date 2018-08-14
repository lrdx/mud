// Copyright (c) 2012 Krodo
// Part of Bylins http://www.mud.ru

#ifndef SETS_DROP_HPP_INCLUDED
#define SETS_DROP_HPP_INCLUDED

#include "structs.h"

#include <map>

namespace SetsDrop
{

// ������ ���������� ������� ����� � ������� ����� (������)
const int SAVE_PERIOD = 27;
// ���� ������� ��� ������ ����
void init();
// ������ ������ ����� � ������������� ������ ������
// ��� ������� ���������� �� ��������� �����
void reload(int zone_vnum = 0);
// ���������� ������� ����� �� �������
void reload_by_timer();
// �������� ����� ������
int check_mob(int mob_rnum);
// ����� ���� ������ ������
void renumber_obj_rnum(const int mob_rnum = -1);
// ���������� ���� � ������� �������
void init_xhelp();
void init_xhelp_full();
// ������ ������� ������ ������� ����� ����� ��������� �������
void print_timer_str(CHAR_DATA *ch);
// ���� ������� ������� ����� � ������
void save_drop_table();
void create_clone_miniset(int vnum);
std::map<int, int> get_unique_mob();

} // namespace SetsDrop

#endif // SETS_DROP_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
