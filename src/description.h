// description.h
// Copyright (c) 2006 Krodo
// Part of Bylins http://www.mud.ru

#ifndef _DESCRIPTION_H_INCLUDED
#define _DESCRIPTION_H_INCLUDED

#include <string>
#include <vector>
#include <map>

/**
* ���������� �����-��������� ���������� �������� ������, ���� �������� 70+�� ������.
* ��������: ������ �������� ������, ������� ������� ���������. � ������� �������
* �� ���� ��������, � ��� ���������� ����� � ���������� ������� ���� ��������.
* ���� � ��� ��������� ����� ������ ���� ����� ���������, �� �� ���������,
* �.�. �� ������ �� ���� �� ����������, �� ������� ���� � ������� �������� ���.
* ��� �������������� � ��� ������ �������� �������� � �������, �.�. ��� ��� �����

* \todo � ����������� ����� ����������� � class room, � ����� ������ ������ �� ����,
* �.�. ���� ���� ����� ���������� ���� � ��������� � ������ ���.
*/
class RoomDescription
{
public:
	static size_t add_desc(const std::string &text);
	static const std::string& show_desc(size_t desc_num);

private:
	RoomDescription();
	~RoomDescription();
	// ������ ������� �������� ��� ������ ����
	static std::vector<std::string> _desc_list;
	// � ��� ����� ��� �� �������� ��� ����. ��-�� ����������� ����������� ����� ���
	// ����� ��������� �� ��� ����� ������ ���� ��� ���, � ��� � ����������� ������� ��������
	typedef std::map<std::string, size_t> reboot_map_t;
	static reboot_map_t _reboot_map;
};

#endif // _DESCRIPTION_H_INCLUDED

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
