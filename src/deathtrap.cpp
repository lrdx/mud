// deathtrap.cpp
// Copyright (c) 2006 Krodo
// Part of Bylins http://www.mud.ru

#include "deathtrap.hpp"

#include "constants.h"
#include "db.h"
#include "spells.h"
#include "handler.h"
#include "house.h"
#include "char.hpp"
#include "corpse.hpp"
#include "room.hpp"
#include "fight.h"
#include "fight_stuff.hpp"
#include "logger.hpp"
#include "utils.h"
#include "conf.h"

#include <list>
#include <algorithm>

extern int has_boat(CHAR_DATA * ch);
extern void death_cry(CHAR_DATA * ch, CHAR_DATA * killer);
extern void reset_affects(CHAR_DATA *ch);

namespace DeathTrap
{

// ������ ������� ����-�� � ����
std::list<ROOM_DATA*> room_list;

void log_death_trap(CHAR_DATA * ch);
void remove_items(CHAR_DATA *ch);

} // namespace DeathTrap

// * ������������� ������ ��� �������� ���� ��� �������������� ������ � ���
void DeathTrap::load()
{
	// �� ������ �������, ������� ������ ����
	room_list.clear();

	for (int i = FIRST_ROOM; i <= top_of_world; ++i)
		if (ROOM_FLAGGED(i, ROOM_SLOWDEATH) || ROOM_FLAGGED(i, ROOM_ICEDEATH))
			room_list.push_back(world[i]);
}

/**
* ���������� ����� ������� � ��������� �� �����������
* \param room - �������, ���. ���������
*/
void DeathTrap::add(ROOM_DATA* room)
{
	const auto it = std::find(room_list.begin(), room_list.end(), room);
	if (it == room_list.end())
		room_list.push_back(room);
}

/**
* �������� ������� �� ������ ����-��
* \param room - �������, ���. �������
*/
void DeathTrap::remove(ROOM_DATA* room)
{
	room_list.remove(room);
}

/// �������� ���������� ��, ��������� ������ 2 ������� � ��������.
/// ��� ������ �������� ��� �������, ����� � ������ ������� ������� ���� ���,
/// � ������ ��� ������� � ���������������� ������ �� ch->next_in_room
/// � ������ ���� ���������� ����� �� ���������� ��������� ��������.
void DeathTrap::activity()
{
	for (const auto& it : room_list)
	{
		const auto people = it->people; // make copy of people in the room
		for (const auto i : people)
		{
			if (i->purged() || IS_NPC(i))
			{
				continue;
			}
			std::string name = i->get_name_str();

			Damage dmg(SimpleDmg(TYPE_ROOMDEATH), MAX(1, GET_REAL_MAX_HIT(i) >> 2), FightSystem::UNDEF_DMG);
			dmg.flags.set(FightSystem::NO_FLEE);

			if (dmg.process(i, i) < 0)
			{
				char buf_[MAX_INPUT_LENGTH];
				snprintf(buf_, sizeof(buf_),
					"Player %s died in slow DT (room %d)",
					name.c_str(), it->number);
				mudlog(buf_, LGH, LVL_IMMORT, SYSLOG, TRUE);
			}
		}
	}
}

namespace OneWayPortal
{

// ������ ������������� �������� <���� ���������, ������ ���������>
std::map<ROOM_DATA*, ROOM_DATA*> portal_list;

/**
* ���������� ������� � ������
* \param to_room - ���� �������� �����
* \param from_room - ������ ��������
*/
void add(ROOM_DATA* to_room, ROOM_DATA* from_room)
{
	portal_list[to_room] = from_room;
}

/**
* �������� ������� �� ������
* \param to_room - ���� ��������� �����
*/
void remove(ROOM_DATA* to_room)
{
	const auto it = portal_list.find(to_room);
	if (it != portal_list.end())
		portal_list.erase(it);
}

/**
* �������� �� ������� ������� � ������
* \param to_room - ���� ��������� �����
* \return ��������� �� �������� �����
*/
ROOM_DATA * get_from_room(ROOM_DATA* to_room)
{
	const auto it = portal_list.find(to_room);
	if (it != portal_list.end())
		return it->second;
	return 0;
}

} // namespace OneWayPortal

// * ����������� � ��������� ���� ������ � �� ��� �������� � �� ����������.
void DeathTrap::log_death_trap(CHAR_DATA * ch)
{
	const char *filename = "../log/death_trap.log";
	static FILE *file = 0;
	if (!file)
	{
		file = fopen(filename, "a");
		if (!file)
		{
			log("SYSERR: can't open %s!", filename);
			return;
		}
		opened_files.push_back(file);
	}
	write_time(file);
	fprintf(file, "%s hit death trap #%d (%s)\n", GET_NAME(ch), GET_ROOM_VNUM(ch->in_room), world[ch->in_room]->name);
	fflush(file);
}

// * ��������� � ������� ��.
int DeathTrap::check_death_trap(CHAR_DATA * ch)
{
	if (ch->in_room != NOWHERE && !PRF_FLAGGED(ch, PRF_CODERINFO))
	{
		if ((ROOM_FLAGGED(ch->in_room, ROOM_DEATH)
			&& !IS_IMMORTAL(ch))
			|| (real_sector(ch->in_room) == SECT_FLYING && !IS_NPC(ch)
				&& !IS_GOD(ch)
				&& !AFF_FLAGGED(ch, EAffectFlag::AFF_FLY))
			|| (real_sector(ch->in_room) == SECT_WATER_NOSWIM && !IS_NPC(ch)
				&& !IS_GOD(ch)
				&& !has_boat(ch)))
		{
			OBJ_DATA *corpse;
			DeathTrap::log_death_trap(ch);

			if (check_tester_death(ch, nullptr))
			{
				sprintf(buf1, "Player %s died in DT (room %d) but zone is under construction.", GET_NAME(ch), GET_ROOM_VNUM(ch->in_room));
				mudlog(buf1, LGH, LVL_IMMORT, SYSLOG, TRUE);
				return FALSE;
			}

			sprintf(buf1, "Player %s died in DT (room %d)", GET_NAME(ch), GET_ROOM_VNUM(ch->in_room));
			mudlog(buf1, LGH, LVL_IMMORT, SYSLOG, TRUE);
			death_cry(ch, NULL);
			//29.11.09 ��� ����� ���������� ����� (�) ��������
			GET_RIP_DT(ch) = GET_RIP_DT(ch) + 1;
			GET_RIP_DTTHIS(ch) = GET_RIP_DTTHIS(ch) + 1;
			//����� ������ (�) ��������
			corpse = make_corpse(ch);
			if (corpse != NULL)
			{
				obj_from_room(corpse);	// ��� ����, ����� ���������� ��� ����������
				extract_obj(corpse);
			}
			GET_HIT(ch) = GET_MOVE(ch) = 0;
			if (RENTABLE(ch))
			{
				die(ch, NULL);
			}
			else
				extract_char(ch, TRUE);
			return TRUE;
		}
	}
	return FALSE;
}

bool DeathTrap::is_slow_dt(int rnum)
{
	if (ROOM_FLAGGED(rnum, ROOM_SLOWDEATH) || ROOM_FLAGGED(rnum, ROOM_ICEDEATH))
		return true;
	return false;
}

/// �������� ���� �� ����� � �������, ���� �� ������� � ������� room_rnum
/// \return ���� > 0, �� �������� ������,
/// ����� - ���� � tunnel_damage() �� ��������
int calc_tunnel_dmg(CHAR_DATA *ch, int room_rnum)
{
	if (!IS_NPC(ch)
		&& !IS_IMMORTAL(ch)
		&& RENTABLE(ch)
		&& ROOM_FLAGGED(room_rnum, ROOM_TUNNEL))
	{
		return std::max(20, GET_REAL_MAX_HIT(ch) >> 3);
	}
	return 0;
}

/// \return true - ���� ����� ����� ����� ��� ����� � ������
/// �������������� �� ������� ���� �� ������ ������
bool DeathTrap::check_tunnel_death(CHAR_DATA *ch, int room_rnum)
{
	const int dam = calc_tunnel_dmg(ch, room_rnum);
	if (dam > 0 && GET_HIT(ch) <= dam * 2)
	{
		return true;
	}
	return false;
}

/// ����� ����� � �� � ���-����� ��� � 2 ������� (SECS_PER_PLAYER_AFFECT)
/// � ������ �� ����� ����� (char_to_room), ����� �� ��� ����� �������
bool DeathTrap::tunnel_damage(CHAR_DATA *ch)
{
	const int dam = calc_tunnel_dmg(ch, ch->in_room);
	if (dam > 0)
	{
		const int room_rnum = ch->in_room;
		const std::string name = ch->get_name_str();
		Damage dmg(SimpleDmg(TYPE_TUNNERLDEATH), dam, FightSystem::UNDEF_DMG);
		dmg.flags.set(FightSystem::NO_FLEE);

		if (dmg.process(ch, ch) < 0)
		{
			char buf_[MAX_INPUT_LENGTH];
			snprintf(buf_, sizeof(buf_),
				"Player %s died in tunnel room (room %d)",
				name.c_str(), GET_ROOM_VNUM(room_rnum));
			mudlog(buf_, NRM, LVL_IMMORT, SYSLOG, TRUE);
			return true;
		}
	}
	return false;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
