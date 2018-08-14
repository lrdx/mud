// description.cpp
// Copyright (c) 2006 Krodo
// Part of Bylins http://www.mud.ru

#include "description.h"

#include "logger.hpp"
#include "conf.h"

#include <stdexcept>

std::vector<std::string> RoomDescription::_desc_list;
RoomDescription::reboot_map_t RoomDescription::_reboot_map;

/**
* ���������� �������� � ������ � ��������� �� ������������
* \param text - �������� �������
* \return ����� �������� � ���������� �������
*/
size_t RoomDescription::add_desc(const std::string &text)
{
	const auto it = _reboot_map.find(text);
	if (it != _reboot_map.end())
	{
		return it->second;
	}
	else
	{
		_desc_list.push_back(text);
		_reboot_map[text] = _desc_list.size();
		return _desc_list.size();
	}
}

const static std::string empty_string;

/**
* ����� �������� �� ��� ����������� ������ � �������
* \param desc_num - ���������� ����� �������� (descripton_num � room_data)
* \return ������ �������� ��� ������ ������ � ������ ����������� ������
*/
const std::string& RoomDescription::show_desc(size_t desc_num)
{
	try
	{
		return _desc_list.at(--desc_num);
	}
	catch (const std::out_of_range&)
	{
		log("SYSERROR : bad room description num '%zd' (%s %s %d)", desc_num, __FILE__, __func__, __LINE__);
		return empty_string;
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
