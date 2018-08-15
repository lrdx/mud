// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#include "noob.hpp"

#include "char.hpp"
#include "obj.hpp"
#include "db.h"
#include "comm.h"
#include "pugixml.hpp"
#include "parse.hpp"
#include "room.hpp"
#include "birth_places.hpp"
#include "handler.h"
#include "skills.h"
#include "logger.hpp"
#include "utils.h"
#include "structs.h"
#include "conf.h"

#include <array>
#include <vector>
#include <sstream>

namespace Noob
{

const char *CONFIG_FILE = LIB_MISC"noob_help.xml";
// ���� ������� ����, ������� ��������� ����� (is_noob � ���� � ������, �� CONFIG_FILE)
int MAX_LEVEL = 0;
// ������ ������� (�� id) �� �������� ������ (vnum) � ������ (�� CONFIG_FILE)
std::array<std::vector<int>, NUM_PLAYER_CLASSES> class_list;

///
/// ������ ������� �� misc/noob_help.xml (CONFIG_FILE)
///
void init()
{
	// ��� ������� �� ������ ������ ��� ������
	std::array<std::vector<int>, NUM_PLAYER_CLASSES> tmp_class_list;

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(CONFIG_FILE);
	if (!result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

    const pugi::xml_node root_node = Parse::get_child(doc, "noob_help");
    if (!root_node) return;

	// <noob max_lvl="" max_money="" wait_period="" />
	pugi::xml_node cur_node = Parse::get_child(root_node, "noob");
    if (cur_node)
	{
		MAX_LEVEL = Parse::attr_int(cur_node, "max_lvl");
	}

	// <all_classes>
	cur_node = Parse::get_child(root_node, "all_classes");
    if (!cur_node) return;

	for (pugi::xml_node obj_node = cur_node.child("obj");
		obj_node; obj_node = obj_node.next_sibling("obj"))
	{
		int vnum = Parse::attr_int(obj_node, "vnum");
		if (Parse::valid_obj_vnum(vnum))
		{
			for (auto i = tmp_class_list.begin(); i != tmp_class_list.end(); ++i)
			{
				i->push_back(vnum);
			}
		}
	}

	// <class id="">
	for (cur_node = root_node.child("class");
		cur_node; cur_node = cur_node.next_sibling("class"))
	{
		std::string id_str = Parse::attr_str(cur_node, "id");
		if (id_str.empty()) return;

		const int id = TextId::to_num(TextId::CHAR_CLASS, id_str);
		if (id == CLASS_UNDEFINED)
		{
			snprintf(buf, MAX_STRING_LENGTH, "...<class id='%s'> convert fail", id_str.c_str());
			mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
			return;
		}

		for (pugi::xml_node obj_node = cur_node.child("obj");
			obj_node; obj_node = obj_node.next_sibling("obj"))
		{
			int vnum = Parse::attr_int(obj_node, "vnum");
			if (Parse::valid_obj_vnum(vnum))
			{
				tmp_class_list[id].push_back(vnum);
			}
		}
	}

	class_list = tmp_class_list;
}

///
/// \return true - ���� ch � ���� ��������� ����� � �����-�� ���������� �� ������
///
bool is_noob(const CHAR_DATA *ch)
{
	if (ch->get_level() > MAX_LEVEL || ch->get_remort() > 0)
	{
		return false;
	}
	return true;
}

///
/// ������ �������, ����� �� �������� ������ � ��������� ������ � �����
///
int outfit(CHAR_DATA* /*ch*/, void* /*me*/, int/* cmd*/, char* /*argument*/)
{
	return 0;
}

///
/// \return ������ � ������� ��������� ��������� ���������
/// ����� ��� ������ (%actor.noob_outfit%)
///
std::string print_start_outfit(CHAR_DATA *ch)
{
	std::stringstream out;
	std::vector<int> tmp(get_start_outfit(ch));
	for (const auto &item : tmp)
	{
		out << item << " ";
	}
	return out.str();
}

///
/// \return ������ ������ ��������� ������ �� noob_help.xml
/// + ������, ��������� �� ��������������� ���� �� birthplaces.xml
///
std::vector<int> get_start_outfit(CHAR_DATA *ch)
{
	// ���� �� noob_help.xml
	std::vector<int> out_list;
	const int ch_class = ch->get_class();
	if (ch_class < NUM_PLAYER_CLASSES)
	{
		out_list.insert(out_list.end(),
			class_list.at(ch_class).begin(), class_list.at(ch_class).end());
	}
	// ���� �� birthplaces.xml (����� �������)
	int birth_id = BirthPlace::GetIdByRoom(GET_ROOM_VNUM(ch->in_room));
	if (birth_id >= 0)
	{
		std::vector<int> tmp = BirthPlace::GetItemList(birth_id);
		out_list.insert(out_list.end(), tmp.begin(), tmp.end());
	}
	return out_list;
}

///
/// \return ��������� �� ����-������� � ������ ������� ��� 0
///
CHAR_DATA * find_renter(int room_rnum)
{
	for (const auto tch : world[room_rnum]->people)
	{
		if (GET_MOB_SPEC(tch) == receptionist)
		{
			return tch;
		}
	}

	return nullptr;
}

///
/// �������� ��� ����� � ���� ���� �� �����, ��� ������������� ������
/// ��������� � ����������� �������� ��������� ����� � ����������.
/// ��������� ������� �� birthplaces.xml ��� ��������� �� BirthPlace::GetRentHelp
///
void check_help_message(CHAR_DATA *ch)
{
	if (Noob::is_noob(ch)
		&& GET_HIT(ch) <= 1
		&& IS_CARRYING_N(ch) <= 0
		&& IS_CARRYING_W(ch) <= 0)
	{
		int birth_id = BirthPlace::GetIdByRoom(GET_ROOM_VNUM(ch->in_room));
		if (birth_id >= 0)
		{
			CHAR_DATA *renter = find_renter(ch->in_room);
			std::string text = BirthPlace::GetRentHelp(birth_id);
			if (renter && !text.empty())
			{
				act("\n\\u$n �������$g ��� � ������ �� ���.", TRUE, renter, 0, ch, TO_VICT);
				act("$n ���������$g �� $N3.", TRUE, renter, 0, ch, TO_NOTVICT);
				tell_to_char(renter, ch, text.c_str());
			}
		}
	}
}

///
/// �������������� ��������� � ���������� ������ ��� ���������� ���� ��� �����
/// � ���� �� ��������� ������. ��� ��������� ����� ���� �������� �� �����.
/// ��������� ��� ��������� �������� ������� �������� ���.
///
void equip_start_outfit(CHAR_DATA *ch, OBJ_DATA *obj)
{
	if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_ARMOR)
	{
		int where = find_eq_pos(ch, obj, 0);
		if (where >= 0)
		{
			equip_char(ch, obj, where);
			// ��������� � ��������� ����� �������� ��� ������ �����
			if (where == WEAR_HANDS && GET_CLASS(ch) == CLASS_WARRIOR)
			{
				ch->set_skill(SKILL_PUNCH, 10);
			}
		}
	}
	else if (GET_OBJ_TYPE(obj) == OBJ_DATA::ITEM_WEAPON)
	{
		if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_WIELD)
			&& !GET_EQ(ch, WEAR_WIELD))
		{
			equip_char(ch, obj, WEAR_WIELD);
			ch->set_skill(static_cast<ESkill>(GET_OBJ_SKILL(obj)), 10);
		}
		else if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_BOTHS)
			&& !GET_EQ(ch, WEAR_BOTHS))
		{
			equip_char(ch, obj, WEAR_BOTHS);
			ch->set_skill(static_cast<ESkill>(GET_OBJ_SKILL(obj)), 10);
		}
		else if (CAN_WEAR(obj, EWearFlag::ITEM_WEAR_HOLD)
			&& !GET_EQ(ch, WEAR_HOLD))
		{
			equip_char(ch, obj, WEAR_HOLD);
			ch->set_skill(static_cast<ESkill>(GET_OBJ_SKILL(obj)), 10);
		}
	}
}

} // namespace Noob

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
