// deathtrap.hpp
// Copyright (c) 2006 Krodo
// Part of Bylins http://www.mud.ru

#ifndef DEATHTRAP_HPP_INCLUDED
#define DEATHTRAP_HPP_INCLUDED

#include "structs.h"

struct ROOM_DATA;	// forward declaration to avoid inclusion of room.hpp and any dependencies of that header.

/**
* ������ ����-�� (������� ������������ ��� ���), ����� �� ������ ������ 2 ������� �� 64� ��������.
* ���������/�������� ��� ������, ������ � ��� � ������������ ��� ���
*/
namespace DeathTrap
{

// ������������� ������ ��� �������� ���� ��� �������������� ������ � ���
void load();
// ���������� ����� ������� � ��������� �� �����������
void add(ROOM_DATA* room);
// �������� ������� �� ������ ����-��
void remove(ROOM_DATA* room);
// �������� ���������� ��, ��������� ������ 2 ������� � ��������
void activity();
// ��������� ������� ��
int check_death_trap(CHAR_DATA * ch);
// �������� ������� �� �������������� � ��������� ��
bool is_slow_dt(int rnum);
// \return true - ���� ����� ����� ����� ��� ����� � ������
bool check_tunnel_death(CHAR_DATA *ch, int room_rnum);
// ����� ����� � �� � ���-�����, \return true - ���� �����
bool tunnel_damage(CHAR_DATA *ch);

} // namespace DeathTrap

/**
* ������ ������������� �������� (�� �������� � ������), ������������ ���� - �� ���������� ���
* ������� ��� ������ ���� ��� �������� ���� ���� �� �� �������, ��� �������� ����� ��������.
*/
namespace OneWayPortal
{

void add(ROOM_DATA* to_room, ROOM_DATA* from_room);
void remove(ROOM_DATA* to_room);
ROOM_DATA* get_from_room(ROOM_DATA* to_room);

} // namespace OneWayPortal

#endif // DEATHTRAP_HPP_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
