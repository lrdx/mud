// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2008 Krodo
// Part of Bylins http://www.mud.ru

#include "glory.hpp"

#include "conf.h"
#include "logger.hpp"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "password.hpp"
#include "genchar.h"
#include "screen.h"
#include "top.h"
#include "char.hpp"
#include "char_player.hpp"
#include "glory_misc.hpp"
#include "interpreter.h"

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include <sstream>
#include <fstream>

namespace Glory
{

// ������� ��������� �����
struct glory_time
{
	glory_time() : glory(0), timer(0), stat(0) {};
	int glory; // ���-�� �����
	int timer; // ������� ����� ��������
	int stat; // � ����� ���� ������� (����� G_STR ... G_CHA)
};

typedef std::shared_ptr<struct glory_time> GloryTimePtr;
typedef std::list<GloryTimePtr> GloryTimeType;
class GloryNode;
typedef std::shared_ptr<GloryNode> GloryNodePtr;
typedef std::map<long, GloryNodePtr> GloryListType; // first - ���

class GloryNode
{
public:
	GloryNode() : free_glory(0), spend_glory(0), denial(0), hide(false), freeze(false) {};

	GloryNode &operator= (const GloryNode&);
	void copy_glory(const GloryNodePtr k);

	int free_glory; // ��������� ����� �� �����
	int spend_glory; // ����������� ����� � ������
	GloryTimeType timers; // ������� ��������� �����
	int denial; // ������ ����������� �� �������������� ��������� �����
	std::string name; // ��� ����
	bool hide; // ���������� ��� ��� � ���� �������������
	bool freeze; // ��������� ����� (������� �� ������)

private:
	void copy_stat(const GloryTimePtr k);
};

class spend_glory
{
public:
	spend_glory() : olc_str(0), olc_dex(0), olc_int(0), olc_wis(0), olc_con(0),
			olc_cha(0), olc_add_str(0), olc_add_dex(0), olc_add_int(0), olc_add_wis(0),
			olc_add_con(0), olc_add_cha(0), olc_add_spend_glory(0) {};

	int olc_str; // �����
	int olc_dex;
	int olc_int;
	int olc_wis;
	int olc_con;
	int olc_cha; // --//--
	int olc_add_str; // ��������� ����� ������ ��� �������� ������ ��� �������������� � ���
	int olc_add_dex; // ��� ������� ����� ������������ ������� � ������� ���������, � �� ��� �����
	int olc_add_int;
	int olc_add_wis;
	int olc_add_con;
	int olc_add_cha; // --//--
	GloryNodePtr olc_node; // ����� � ������ ������
	int olc_add_spend_glory; // ����������� ����� � ������ ������� ��������� � ���
	// ��� ��������� ������ � ��� ������ ������ ��, � �� spend_glory � olc_node
	// ������������ � ����������� ��������� spend_glory � olc_node, ����� ������ ���� ����������

	int check_spend_glory; // ��� ������ spend_glory � olc_node, �� ���-�� �� ������� ���� ��
	int check_free_glory; // � �����������, ��� ������ ����������� ����� �� � ����� ��� �����
	// ��������� ���� + ���� check_free_glory ��� free_glory, �.�. � olc_node ��� ��������
	// ���� �����, ����� ������ � ����� ������� ��������� �� ����� �������������� � ���
};

const int MAX_STATS_BY_GLORY = 10; // ������� ������ ����� ��������� ������
const int MAX_GLORY_TIMER = 267840; // ������ ��������� ����� (� �������)
const int DISPLACE_TIMER = 20160; // ������ �� ������������� ��������� ����� ����� ������� (� �������)
GloryListType glory_list; // ����� ������ ����� �� �����, � ��� ����� ������ � �����

// * ����������� ���� �����, ����� �� �������� ������ � ������������ ����������.
GloryNode & GloryNode::operator= (const GloryNode &t)
{
	free_glory = t.free_glory;
	spend_glory = t.spend_glory;
	denial = t.denial;
	name = t.name;
	hide = t.hide;
	freeze = t.freeze;
	timers.clear();
	for (const auto& tm_it : t.timers)
	{
		const GloryTimePtr temp_timer(new glory_time);
		temp_timer->stat = tm_it->stat;
		temp_timer->glory = tm_it->glory;
		temp_timer->timer = tm_it->timer;
		timers.push_back(temp_timer);
	}
	return *this;
}

// * ����������� ���� � ������ ������ �� k � ��������� �� ����_����.
void GloryNode::copy_stat(const GloryTimePtr k)
{
	if (spend_glory < MAX_STATS_BY_GLORY)
	{
		const GloryTimePtr tmp_node(new glory_time);
		*tmp_node = *k;
		tmp_node->glory = MIN(k->glory, MAX_STATS_BY_GLORY - spend_glory);
		spend_glory += tmp_node->glory;
		timers.push_back(tmp_node);
	}
}

// * ����������� ��������� � ������ (�� �����������) ����� �� k.
void GloryNode::copy_glory(const GloryNodePtr k)
{
	free_glory += k->free_glory;
	for (GloryTimeType::const_iterator i = k->timers.begin(); i != k->timers.end(); ++i)
	{
		copy_stat(*i);
	}
}

// * �������� ������ ������ �����, �������� �� ���������� ����, �����, ������ ��5 �����.
void load_glory()
{
	const char *glory_file = LIB_PLRSTUFF"glory.lst";
	std::ifstream file(glory_file);
	if (!file.is_open())
	{
		log("Glory: �� ������� ������� ���� �� ������: %s", glory_file);
		return;
	}

	long all_sum = 0;
	std::string buffer;
	bool checksum = false;
	while (file >> buffer)
	{
		if (buffer == "<Node>")
		{
			GloryNodePtr temp_node(new GloryNode);
			long uid = 0;
			int free_glory = 0, denial = 0;
			bool hide = false, freeze = false;

			if (!(file >> uid >> free_glory >> denial >> hide >> freeze))
			{
				log("Glory: ������ ������ uid: %ld, free_glory: %d, denial: %d, hide: %d, freeze: %d",
					uid, free_glory, denial, hide, freeze);
				break;
			}
			temp_node->free_glory = free_glory;
			temp_node->denial = denial;
			temp_node->hide = hide;
			temp_node->freeze = freeze;
			all_sum += uid + free_glory + denial + hide + freeze;

			file >> buffer;
			if (buffer != "<Glory>")
			{
				log("Glory: ������ ������ ������ <Glory>");
				break;
			}


			while (file >> buffer)
			{
				if (buffer == "</Glory>") break;

				int glory = 0, stat = 0, timer = 0;

				try
				{
					glory = std::stoi(buffer, nullptr, 10);
				}
				catch (const std::invalid_argument &)
				{
					log("Glory: ������ ������ glory (%s)", buffer.c_str());
					break;
				}

				all_sum += glory;

				if (!(file >> stat >> timer))
				{
					log("Glory: ������ ������: %d, %d", stat, timer);
					break;
				}

				const GloryTimePtr temp_glory_timers(new glory_time);
				temp_glory_timers->glory = glory;
				temp_glory_timers->timer = timer;
				temp_glory_timers->stat = stat;
				// ���� ���� ������� ���� ����� ������ - ��������� ������ ������ 10 ��������� ������
				// � ���� ��� ����� ������ �� ���������� �������� - �� � ��� � ���, ��� �������� ��� ���
				if (temp_node->spend_glory + glory <= MAX_STATS_BY_GLORY)
				{
					temp_node->timers.push_back(temp_glory_timers);
					temp_node->spend_glory += glory;
				}
				else
				{
					log("Glory: ������������ ���-�� ��������� ������, %d %d %d ��������� (uid: %ld).", glory, stat, timer, uid);
				}

				all_sum += stat + timer;
			}
			if (buffer != "</Glory>")
			{
				log("Glory: ������ ������ ������ </Glory>.");
				break;
			}

			file >> buffer;
			if (buffer != "</Node>")
			{
				log("Glory: ������ ������ </Node>: %s", buffer.c_str());
				break;
			}

			// ��������� ���� �� ��� ����� ��� ������
			std::string name = GetNameByUnique(uid);
			if (name.empty())
			{
				log("Glory: UID %ld - ��������� �� ����������.", uid);
				continue;
			}
			temp_node->name = name;

			if (!temp_node->free_glory && !temp_node->spend_glory)
			{
				log("Glory: UID %ld - �� �������� ��������� ��� ��������� �����.", uid);
				continue;
			}

			// ��������� � ������
			glory_list[uid] = temp_node;
		}
		else if (buffer == "<End>")
		{
			// �������� ��5
			file >> buffer;
			const int result = Password::compare_password(buffer, std::to_string(all_sum));
			checksum = true;
			if (!result)
			{
				// FIXME ��� ���� ������ ����������, �� �����
				log("Glory: ������������ ����� ����������� ����� (%s).", buffer.c_str());
			}
		}
		else
		{
			log("Glory: ����������� ���� ��� ������ �����: %s", buffer.c_str());
			break;
		}
	}
	if (!checksum)
	{
		// FIXME ��� ���� ������ ����������, �� �����
		log("Glory: �������� ����� ����������� ����� �� �������������.");
	}
}

// * ���������� ������ ������ �����, ������ ��5 �����.
void save_glory()
{
	long all_sum = 0;
	std::stringstream out;

	for (GloryListType::const_iterator it = glory_list.begin(); it != glory_list.end(); ++it)
	{
		out << "<Node>\n" << it->first << " " << it->second->free_glory << " " << it->second->denial << " " << it->second->hide << " " << it->second->freeze << "\n<Glory>\n";
		all_sum += it->first + it->second->free_glory + it->second->denial + it->second->hide + it->second->freeze;
		for (GloryTimeType::const_iterator gl_it = it->second->timers.begin(); gl_it != it->second->timers.end(); ++gl_it)
		{
			out << (*gl_it)->glory << " " << (*gl_it)->stat << " " << (*gl_it)->timer << "\n";
			all_sum += (*gl_it)->stat + (*gl_it)->glory + (*gl_it)->timer;
		}
		out << "</Glory>\n</Node>\n";
	}
	out << "<End>\n";
	// TODO: � file_crc ������� ��� ����
	out << Password::generate_md5_hash(std::to_string(all_sum)) << "\n";

	const char *glory_file = LIB_PLRSTUFF"glory.lst";
	std::ofstream file(glory_file);
	if (!file.is_open())
	{
		log("Glory: �� ������� ������� ���� �� ������: %s", glory_file);
		return;
	}
	file << out.rdbuf();
	file.close();
}

// * ������ ������� ������� GET_GLORY().
int get_glory(long uid)
{
	int glory = 0;
	const auto it = glory_list.find(uid);

	if (it != glory_list.end())
		glory = it->second->free_glory;

	return glory;
}

// * ���������� ����� ����, �������� ����� ������ ��� �������������, �����������, ���� ��� ������.
void add_glory(long uid, int amount)
{
	if (uid <= 0 || amount <= 0) return;

	GloryListType::iterator it = glory_list.find(uid);
	if (it != glory_list.end())
	{
		it->second->free_glory += amount;
	}
	else
	{
		const GloryNodePtr temp_node(new GloryNode);
		temp_node->free_glory = amount;
		glory_list[uid] = temp_node;
	}
	save_glory();
	DESCRIPTOR_DATA *d = DescByUID(uid);
	if (d)
		send_to_char(d->character.get(), "�� ��������� %d %s �����.\r\n",
                  amount, desc_count(amount, WHAT_POINT));
}

/**
* �������� ����� � ���� (���������), ���� ���� ���� �� �����.
* \return 0 - ������ �� �������, ����� > 0 - ������� ������� �������
*/
int remove_glory(long uid, int amount)
{
	if (uid <= 0 || amount <= 0) return 0;
	int real_removed = amount;

	GloryListType::iterator it = glory_list.find(uid);
	if (it != glory_list.end())
	{
		// ���� ���� ��-��� �� ��������
		if (it->second->free_glory >= amount)
			it->second->free_glory -= amount;
		else
		{
			real_removed = it->second->free_glory;
			it->second->free_glory = 0;
		}
		// ������ ������ ������ ��������� �� ������ ���
		if (!it->second->free_glory && !it->second->spend_glory)
			glory_list.erase(it);

		save_glory();
	}
	else
	{
		real_removed = 0;
	}
	return real_removed;
}

// * ������, ����� �� ����������� � ������ ������ ���� � ����.
void print_denial_message(CHAR_DATA *ch, int denial)
{
	send_to_char(ch, "�� �� ������� �������� ��� ��������� ���� ����� (%s).\r\n", time_format(denial).c_str());
}

/**
* �������� �� ������ �������������� ������ ��� �������������� � ���.
* \return 0 - ���������, 1 - ���������
*/
bool parse_denial_check(CHAR_DATA *ch, int stat)
{
	// ��� ���������� ������� ����� ���� ��� ��������� �� ����������
	if (ch->desc->glory->olc_node->denial)
	{
		bool stop = false;
		switch (stat)
		{
		case G_STR:
			if (ch->get_inborn_str() == ch->desc->glory->olc_str)
				stop = true;
			break;
		case G_DEX:
			if (ch->get_inborn_dex() == ch->desc->glory->olc_dex)
				stop = true;
			break;
		case G_INT:
			if (ch->get_inborn_int() == ch->desc->glory->olc_int)
				stop = true;
			break;
		case G_WIS:
			if (ch->get_inborn_wis() == ch->desc->glory->olc_wis)
				stop = true;
			break;
		case G_CON:
			if (ch->get_inborn_con() == ch->desc->glory->olc_con)
				stop = true;
			break;
		case G_CHA:
			if (ch->get_inborn_cha() == ch->desc->glory->olc_cha)
				stop = true;
			break;
		default:
			log("Glory: ���������� ����� ����: %d (uid: %d, name: %s)", stat, GET_UNIQUE(ch), GET_NAME(ch));
			stop = true;
		}
		if (stop)
		{
			print_denial_message(ch, ch->desc->glory->olc_node->denial);
			return false;
		}
	}
	return true;
}

/**
* ��������� ��� ���:
* stat = -1 - �������������� ������, ������ ��������, ���� ���� ������ ��������
* timer = 0 - ����� ��������� ���� ��� ��������������
* timer > 0 - ��������� ����, ������� ����� �� ������ ��� ��������������
*/
bool parse_remove_stat(CHAR_DATA *ch, int stat)
{
	if (!parse_denial_check(ch, stat))
		return false;

	for (auto it = ch->desc->glory->olc_node->timers.begin(); it != ch->desc->glory->olc_node->timers.end(); ++it)
	{
		if ((*it)->stat == stat)
		{
			(*it)->glory -= 1;

			if ((*it)->glory > 0)
			{
				// ���� �� ������ ������ ����� ������, �� ���� ��������� ������ ���� �����
				// ��, ��� ��� ������ � ���� ���� ��������� ��� � ������ �����/������ ���
				const GloryTimePtr temp_timer(new glory_time);
				temp_timer->stat = -1;
				temp_timer->timer = (*it)->timer;
				// � ������ ������, ����� ��� �������� �� ���� ������
				ch->desc->glory->olc_node->timers.push_front(temp_timer);

			}
			else if ((*it)->glory == 0 && (*it)->timer != 0)
			{
				// ���� � ����� �� �������� ������ (��� ��������������), �� ����������� ���� �����,
				// �� ��������� ������ � ���������� ������ � ������ ������, ����� ������������ ��� ������ ��� ��������
				(*it)->stat = -1;
				ch->desc->glory->olc_node->timers.push_front(*it);
				ch->desc->glory->olc_node->timers.erase(it);
			}
			else if ((*it)->glory == 0 && (*it)->timer == 0)
			{
				// ������� ������� � �������� ������ �������� ���
				ch->desc->glory->olc_node->timers.erase(it);
			}
			ch->desc->glory->olc_add_spend_glory -= 1;
			ch->desc->glory->olc_node->free_glory += 1000;
			return true;
		}
	}
	log("Glory: �� ������ ���� ��� ��������� � ��� (stat: %d, uid: %d, name: %s)", stat, GET_UNIQUE(ch), GET_NAME(ch));
	return true;
}

// * ���������� ����� � ���, �������� �� ��������� ������ � ���������� ��������, ����� ������ �������������� ������.
void parse_add_stat(CHAR_DATA *ch, int stat)
{
	// � ����� ������ ���� ���������
	ch->desc->glory->olc_add_spend_glory += 1;
	ch->desc->glory->olc_node->free_glory -= 1000;

	for (GloryTimeType::iterator it = ch->desc->glory->olc_node->timers.begin();
			it != ch->desc->glory->olc_node->timers.end(); ++it)
	{
		// ���� ���� �����-�� ����������� ���� (�����������), �� � ������ ������� ������� ���,
		// ����� ������ ��� �������. ����� ���������� ��� ���� �������� ����� �����
		if ((*it)->stat == -1)
		{
			// ��� ������ ��������, �� ��� � ����� �� ���� ��� ����, � ��� ��� �������� ����������
			// ���� ��� ��������� ����� �� ���� � ��� �� ��������, ����� �� ������� ������ ������
			for (GloryTimeType::const_iterator tmp_it = ch->desc->glory->olc_node->timers.begin();
					tmp_it != ch->desc->glory->olc_node->timers.end(); ++tmp_it)
			{
				if ((*tmp_it)->stat == stat && (*tmp_it)->timer == (*it)->timer)
				{
					(*tmp_it)->glory += 1;
					ch->desc->glory->olc_node->timers.erase(it);
					return;
				}
			}
			// ���� ������ � ��� �� �������� ��� - ����� ��� ��� � ���� ��������� �������
			(*it)->stat = stat;
			(*it)->glory += 1;
			return;
		}

		// ���� ��� ���� �������� � ��� �� ���� � ������� ������ ��������������
		// - ���������� � ���� ����, ������ ��� ������� ����� ����������
		if ((*it)->stat == stat && (*it)->timer == 0)
		{
			(*it)->glory += 1;
			return;
		}
	}
	// ���� ���� �� ������ - ������ ����� ������
	const GloryTimePtr temp_timer(new glory_time);
	temp_timer->stat = stat;
	temp_timer->glory = 1;
	// ��������� � ������ ������, ����� ��� ��������� ������� ������� ��������� � ������� ������ �����
	ch->desc->glory->olc_node->timers.push_front(temp_timer);
}

// * ���� ��� ���� '�����'.
bool parse_spend_glory_menu(CHAR_DATA* ch, const char* arg)
{
	switch (*arg)
	{
	case '�':
	case '�':
		if (ch->desc->glory->olc_add_str >= 1)
		{
			if (!parse_remove_stat(ch, G_STR)) break;
			ch->desc->glory->olc_str -= 1;
			ch->desc->glory->olc_add_str -= 1;
		}
		break;
	case '�':
	case '�':
		if (ch->desc->glory->olc_add_dex >= 1)
		{
			if (!parse_remove_stat(ch, G_DEX)) break;
			ch->desc->glory->olc_dex -= 1;
			ch->desc->glory->olc_add_dex -= 1;
		}
		break;
	case '�':
	case '�':
		if (ch->desc->glory->olc_add_int >= 1)
		{
			if (!parse_remove_stat(ch, G_INT)) break;
			ch->desc->glory->olc_int -= 1;
			ch->desc->glory->olc_add_int -= 1;
		}
		break;
	case '�':
	case '�':
		if (ch->desc->glory->olc_add_wis >= 1)
		{
			if (!parse_remove_stat(ch, G_WIS)) break;
			ch->desc->glory->olc_wis -= 1;
			ch->desc->glory->olc_add_wis -= 1;
		}
		break;
	case '�':
	case '�':
		if (ch->desc->glory->olc_add_con >= 1)
		{
			if (!parse_remove_stat(ch, G_CON)) break;
			ch->desc->glory->olc_con -= 1;
			ch->desc->glory->olc_add_con -= 1;
		}
		break;
	case '�':
	case '�':
		if (ch->desc->glory->olc_add_cha >= 1)
		{
			if (!parse_remove_stat(ch, G_CHA)) break;
			ch->desc->glory->olc_cha -= 1;
			ch->desc->glory->olc_add_cha -= 1;
		}
		break;
	case '�':
	case '�':
		if (ch->desc->glory->olc_node->free_glory >= 1000 && ch->desc->glory->olc_add_spend_glory < MAX_STATS_BY_GLORY)
		{
			parse_add_stat(ch, G_STR);
			ch->desc->glory->olc_str += 1;
			ch->desc->glory->olc_add_str += 1;
		}
		break;
	case '�':
	case '�':
		if (ch->desc->glory->olc_node->free_glory >= 1000 && ch->desc->glory->olc_add_spend_glory < MAX_STATS_BY_GLORY)
		{
			parse_add_stat(ch, G_DEX);
			ch->desc->glory->olc_dex += 1;
			ch->desc->glory->olc_add_dex += 1;
		}
		break;
	case '�':
	case '�':
		if (ch->desc->glory->olc_node->free_glory >= 1000 && ch->desc->glory->olc_add_spend_glory < MAX_STATS_BY_GLORY)
		{
			parse_add_stat(ch, G_INT);
			ch->desc->glory->olc_int += 1;
			ch->desc->glory->olc_add_int += 1;
		}
		break;
	case '�':
	case '�':
		if (ch->desc->glory->olc_node->free_glory >= 1000 && ch->desc->glory->olc_add_spend_glory < MAX_STATS_BY_GLORY)
		{
			parse_add_stat(ch, G_WIS);
			ch->desc->glory->olc_wis += 1;
			ch->desc->glory->olc_add_wis += 1;
		}
		break;
	case '�':
	case '�':
		if (ch->desc->glory->olc_node->free_glory >= 1000 && ch->desc->glory->olc_add_spend_glory < MAX_STATS_BY_GLORY)
		{
			parse_add_stat(ch, G_CON);
			ch->desc->glory->olc_con += 1;
			ch->desc->glory->olc_add_con += 1;
		}
		break;
	case '�':
	case '�':
		if (ch->desc->glory->olc_node->free_glory >= 1000 && ch->desc->glory->olc_add_spend_glory < MAX_STATS_BY_GLORY)
		{
			parse_add_stat(ch, G_CHA);
			ch->desc->glory->olc_cha += 1;
			ch->desc->glory->olc_add_cha += 1;
		}
		break;
	case '�':
	case '�':
	{
		// ��������, ����� �� ���������� ���, � ������ ��� ���������
		// � ����� ������ ���� �� ����� ����� ��������
		if ((ch->desc->glory->olc_str == ch->get_inborn_str()
				&& ch->desc->glory->olc_dex == ch->get_inborn_dex()
				&& ch->desc->glory->olc_int == ch->get_inborn_int()
				&& ch->desc->glory->olc_wis == ch->get_inborn_wis()
				&& ch->desc->glory->olc_con == ch->get_inborn_con()
				&& ch->desc->glory->olc_cha == ch->get_inborn_cha())
				|| ch->desc->glory->olc_add_spend_glory < ch->desc->glory->olc_node->spend_glory)
		{
			return false;
		}

		const auto it = glory_list.find(GET_UNIQUE(ch));
		if (it == glory_list.end()
				|| ch->desc->glory->check_spend_glory != it->second->spend_glory
				|| ch->desc->glory->check_free_glory != it->second->free_glory
				|| ch->desc->glory->olc_node->hide != it->second->hide
				|| ch->desc->glory->olc_node->freeze != it->second->freeze)
		{
			// ������ ����� ���������� �������� ���� � ���� � ���� ������ ������������ �����
			// ��������� - �� ����� ������� � ��� ���-�� ����������, ���� �����, ������������ ����� � �.�.
			ch->desc->glory.reset();
			STATE(ch->desc) = CON_PLAYING;
			send_to_char("�������������� �������� ��-�� ������� ���������.\r\n", ch);
			return true;
		}

		// �������� ������, ���� ���� ����������� ������
		if (ch->get_inborn_str() < ch->desc->glory->olc_str
				|| ch->get_inborn_dex() < ch->desc->glory->olc_dex
				|| ch->get_inborn_int() < ch->desc->glory->olc_int
				|| ch->get_inborn_wis() < ch->desc->glory->olc_wis
				|| ch->get_inborn_con() < ch->desc->glory->olc_con
				|| ch->get_inborn_cha() < ch->desc->glory->olc_cha)
		{
			ch->desc->glory->olc_node->denial = DISPLACE_TIMER;
		}

		ch->set_str(ch->desc->glory->olc_str);
		ch->set_dex(ch->desc->glory->olc_dex);
		ch->set_con(ch->desc->glory->olc_con);
		ch->set_wis(ch->desc->glory->olc_wis);
		ch->set_int(ch->desc->glory->olc_int);
		ch->set_cha(ch->desc->glory->olc_cha);

		// ����������� �������, ������ ��� � ��� ������� ����� ������� ��� ����� ������
		for (GloryTimeType::const_iterator tmp_it = ch->desc->glory->olc_node->timers.begin();
				tmp_it != ch->desc->glory->olc_node->timers.end(); ++tmp_it)
		{
			if ((*tmp_it)->timer == 0)
				(*tmp_it)->timer = MAX_GLORY_TIMER;
		}

		ch->desc->glory->olc_node->spend_glory = ch->desc->glory->olc_add_spend_glory;
		*(it->second) = *(ch->desc->glory->olc_node);
		save_glory();

		ch->desc->glory.reset();
		STATE(ch->desc) = CON_PLAYING;
		check_max_hp(ch);
		send_to_char("���� ��������� ���������.\r\n", ch);
		return true;
	}
	case '�':
	case '�':
		ch->desc->glory.reset();
		STATE(ch->desc) = CON_PLAYING;
		send_to_char("�������������� ��������.\r\n", ch);
		return true;
	default:
		break;
	}
	return false;
}

// * ���������� ��� ����.
void spend_glory_menu(CHAR_DATA *ch)
{
	std::ostringstream out;
	out << "\r\n              -      +\r\n"
	<< "  ����     : ("
	<< CCIGRN(ch, C_SPR) << "�" << CCNRM(ch, C_SPR)
	<< ") " << ch->desc->glory->olc_str << " ("
	<< CCIGRN(ch, C_SPR) << "�" << CCNRM(ch, C_SPR)
	<< ")";
	if (ch->desc->glory->olc_add_str)
		out << "    (" << (ch->desc->glory->olc_add_str > 0 ? "+" : "") << ch->desc->glory->olc_add_str << ")";
	out << "\r\n"
	<< "  �������� : ("
	<< CCIGRN(ch, C_SPR) << "�" << CCNRM(ch, C_SPR)
	<< ") " << ch->desc->glory->olc_dex << " ("
	<< CCIGRN(ch, C_SPR) << "�" << CCNRM(ch, C_SPR)
	<< ")";
	if (ch->desc->glory->olc_add_dex)
		out << "    (" << (ch->desc->glory->olc_add_dex > 0 ? "+" : "") << ch->desc->glory->olc_add_dex << ")";
	out << "\r\n"
	<< "  ��       : ("
	<< CCIGRN(ch, C_SPR) << "�" << CCNRM(ch, C_SPR)
	<< ") " << ch->desc->glory->olc_int << " ("
	<< CCIGRN(ch, C_SPR) << "�" << CCNRM(ch, C_SPR)
	<< ")";
	if (ch->desc->glory->olc_add_int)
		out << "    (" << (ch->desc->glory->olc_add_int > 0 ? "+" : "") << ch->desc->glory->olc_add_int << ")";
	out << "\r\n"
	<< "  �������� : ("
	<< CCIGRN(ch, C_SPR) << "�" << CCNRM(ch, C_SPR)
	<< ") " << ch->desc->glory->olc_wis << " ("
	<< CCIGRN(ch, C_SPR) << "�" << CCNRM(ch, C_SPR)
	<< ")";
	if (ch->desc->glory->olc_add_wis)
		out << "    (" << (ch->desc->glory->olc_add_wis > 0 ? "+" : "") << ch->desc->glory->olc_add_wis << ")";
	out << "\r\n"
	<< "  �������� : ("
	<< CCIGRN(ch, C_SPR) << "�" << CCNRM(ch, C_SPR)
	<< ") " << ch->desc->glory->olc_con << " ("
	<< CCIGRN(ch, C_SPR) << "�" << CCNRM(ch, C_SPR)
	<< ")";
	if (ch->desc->glory->olc_add_con)
		out << "    (" << (ch->desc->glory->olc_add_con > 0 ? "+" : "") << ch->desc->glory->olc_add_con << ")";
	out << "\r\n"
	<< "  �������  : ("
	<< CCIGRN(ch, C_SPR) << "�" << CCNRM(ch, C_SPR)
	<< ") " << ch->desc->glory->olc_cha << " ("
	<< CCIGRN(ch, C_SPR) << "�" << CCNRM(ch, C_SPR)
	<< ")";
	if (ch->desc->glory->olc_add_cha)
		out << "    (" << (ch->desc->glory->olc_add_cha > 0 ? "+" : "") << ch->desc->glory->olc_add_cha << ")";
	out << "\r\n\r\n"
	<< "  ����� ��������: "
	<< MIN((MAX_STATS_BY_GLORY - ch->desc->glory->olc_add_spend_glory), ch->desc->glory->olc_node->free_glory / 1000)
	<< "\r\n\r\n";

	const int diff = ch->desc->glory->olc_node->spend_glory - ch->desc->glory->olc_add_spend_glory;
	if (diff > 0)
	{
		out << "  �� ������ ������������ ��������� ����� " << diff << " " << desc_count(diff, WHAT_POINT) << "\r\n";
	}
	else if (ch->desc->glory->olc_add_spend_glory > ch->desc->glory->olc_node->spend_glory
			 || ch->desc->glory->olc_str != ch->get_inborn_str()
			 || ch->desc->glory->olc_dex != ch->get_inborn_dex()
			 || ch->desc->glory->olc_int != ch->get_inborn_int()
			 || ch->desc->glory->olc_wis != ch->get_inborn_wis()
			 || ch->desc->glory->olc_con != ch->get_inborn_con()
			 || ch->desc->glory->olc_cha != ch->get_inborn_cha())
	{
		out << "  "
		<< CCIGRN(ch, C_SPR) << "�" << CCNRM(ch, C_SPR)
		<< ") ��������� ����������\r\n";
	}

	out << "  "
	<< CCIGRN(ch, C_SPR) << "�" << CCNRM(ch, C_SPR)
	<< ") ����� ��� ����������\r\n"
	<< " ��� �����: ";
	send_to_char(out.str(), ch);
}

// * ���������� '����� ����������'.
void print_glory(CHAR_DATA *ch, GloryListType::iterator &it)
{
	std::ostringstream out;
	for (GloryTimeType::const_iterator tm_it = it->second->timers.begin(); tm_it != it->second->timers.end(); ++tm_it)
	{
		switch ((*tm_it)->stat)
		{
		case G_STR:
			out << "���� : +" << (*tm_it)->glory << " (" << time_format((*tm_it)->timer) << ")\r\n";
			break;
		case G_DEX:
			out << "���� : +" << (*tm_it)->glory << " (" << time_format((*tm_it)->timer) << ")\r\n";
			break;
		case G_INT:
			out << "��   : +" << (*tm_it)->glory << " (" << time_format((*tm_it)->timer) << ")\r\n";
			break;
		case G_WIS:
			out << "���� : +" << (*tm_it)->glory << " (" << time_format((*tm_it)->timer) << ")\r\n";
			break;
		case G_CON:
			out << "���� : +" << (*tm_it)->glory << " (" << time_format((*tm_it)->timer) << ")\r\n";
			break;
		case G_CHA:
			out << "�����: +" << (*tm_it)->glory << " (" << time_format((*tm_it)->timer) << ")\r\n";
			break;
		default:
			log("Glory: ������������ ����� ����� %d (uid: %ld)", (*tm_it)->stat, it->first);
		}
	}
	out << "��������� �����: " << it->second->free_glory << "\r\n";
	send_to_char(out.str(), ch);
	return;
}

// * ������� '�����' - �������� ��������� � ������ ����� � ����� ��� ������� �����.
void do_spend_glory(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	GloryListType::iterator it = glory_list.find(GET_UNIQUE(ch));
	if (it == glory_list.end() || IS_IMMORTAL(ch))
	{
		send_to_char("��� ��� �� �����...\r\n", ch);
		return;
	}

	std::string buffer = argument, buffer2;
	GetOneParam(buffer, buffer2);
	if (CompareParam(buffer2, "����������"))
	{
		send_to_char("���������� � ��������� ���� ����� �����:\r\n", ch);
		print_glory(ch, it);
		return;
	}

	if (it->second->free_glory < 1000)
	{
		if (!it->second->spend_glory)
		{
			send_to_char("� ��� ������������ ����� ����� ��� ������������� ���� �������.\r\n", ch);
			return;
		}
		else if (it->second->denial)
		{
			print_denial_message(ch, it->second->denial);
			return;
		}
	}
	if (it->second->spend_glory >= MAX_STATS_BY_GLORY && it->second->denial)
	{
		send_to_char("� ��� ��� ������� ����������� ���������� ���������� �����.\r\n", ch);
		print_denial_message(ch, it->second->denial);
		return;
	}

	std::shared_ptr<class spend_glory> temp_glory(new spend_glory);
	temp_glory->olc_str = ch->get_inborn_str();
	temp_glory->olc_dex = ch->get_inborn_dex();
	temp_glory->olc_int = ch->get_inborn_int();
	temp_glory->olc_wis = ch->get_inborn_wis();
	temp_glory->olc_con = ch->get_inborn_con();
	temp_glory->olc_cha = ch->get_inborn_cha();

	// � �� ����� ���, ��� ��� ���� �����-��� ����� ���� ��� ��������� �����������
	// � �.�. ������ ���� ������ �������� �� ��� - �������� ������� � ������ �� ���
	const GloryNodePtr olc_node(new GloryNode);
	*olc_node = *(it->second);
	temp_glory->olc_node = olc_node;
	temp_glory->olc_add_spend_glory = it->second->spend_glory;
	// ���� ��� ����������� ������ �� � �����
	temp_glory->check_spend_glory = it->second->spend_glory;
	temp_glory->check_free_glory = it->second->free_glory;

	for (GloryTimeType::const_iterator tm_it = it->second->timers.begin(); tm_it != it->second->timers.end(); ++tm_it)
	{
		switch ((*tm_it)->stat)
		{
		case G_STR:
			temp_glory->olc_add_str += (*tm_it)->glory;
			break;
		case G_DEX:
			temp_glory->olc_add_dex += (*tm_it)->glory;
			break;
		case G_INT:
			temp_glory->olc_add_int += (*tm_it)->glory;
			break;
		case G_WIS:
			temp_glory->olc_add_wis += (*tm_it)->glory;
			break;
		case G_CON:
			temp_glory->olc_add_con += (*tm_it)->glory;
			break;
		case G_CHA:
			temp_glory->olc_add_cha += (*tm_it)->glory;
			break;
		default:
			log("Glory: ������������ ����� ����� %d (uid: %ld)", (*tm_it)->stat, it->first);
		}
	}

	ch->desc->glory = temp_glory;
	STATE(ch->desc) = CON_SPEND_GLORY;
	spend_glory_menu(ch);
}

// * �������� ������ � ����, ���� �� ������.
void remove_stat_online(long uid, int stat, int glory)
{
	DESCRIPTOR_DATA *d = DescByUID(uid);
	if (d)
	{
		switch (stat)
		{
		case G_STR:
			d->character->inc_str(-glory);
			break;
		case G_DEX:
			d->character->inc_dex(-glory);
			break;
		case G_INT:
			d->character->inc_int(-glory);
			break;
		case G_WIS:
			d->character->inc_wis(-glory);
			break;
		case G_CON:
			d->character->inc_con(-glory);
			break;
		case G_CHA:
			d->character->inc_cha(-glory);
			break;
		default:
			log("Glory: ������������ ����� ����� %d (uid: %ld)", stat, uid);
		}
	}
}

// * ������� �������� ����� � ������� ��������������, ������ ������������ ����� � ���� ������.
void timers_update()
{
	for (const auto& it : glory_list)
	{
		if (it.second->freeze)
			continue; // �� ����� ������� ������� �� ������

		bool removed = false; // ������ ��� ��������� � ���������� ����� (����� �� ������� �� ������ ����)
		for (auto tm_it = it.second->timers.begin(); tm_it != it.second->timers.end();)
		{
			if ((*tm_it)->timer > 0)
				(*tm_it)->timer -= 1;
			if ((*tm_it)->timer <= 0)
			{
				removed = true;
				// ���� ��� ������ - ������� � ���� ����
				remove_stat_online(it.first, (*tm_it)->stat, (*tm_it)->glory);
				it.second->spend_glory -= (*tm_it)->glory;
				it.second->timers.erase(tm_it++);
			}
			else
				++tm_it;
		}
		// ��� �� ������ ������������ �� �����������
		if (it.second->denial > 0)
			it.second->denial -= 1;

		if (removed)
		{
			DESCRIPTOR_DATA *d = DescByUID(it.first);
			if (d)
			{
				send_to_char("�� ����� �� ��������� ��������� ������ � ����� ��� ��������...\r\n",
					d->character.get());
			}
		}

	}

	// ��� ��� ���� ������, ��� � ���� ������� �� ����, � ������ ����������� ������ ���������
	// � �������� ����� �����, �� ��� �� ����� - ������ �� ������
	for (DESCRIPTOR_DATA *d = descriptor_list; d; d = d->next)
	{
		if (STATE(d) != CON_SPEND_GLORY || !d->glory) continue;
		for (GloryTimeType::iterator d_it = d->glory->olc_node->timers.begin(); d_it != d->glory->olc_node->timers.end(); ++d_it)
		{
			// ����� �� �� ������ denial � �� ������� ������������ �����, � ������ ���������� �� ���
			if ((*d_it)->timer > 0)
			{
				// ������������ �� ������ ������� > 0, ������ ��� ��������� �� ����� ��������������
				// ����� ����� ������� ������, � ��� ���������� ������ ������� ����� ������ � �������
				(*d_it)->timer -= 1;
				if ((*d_it)->timer <= 0)
				{
					d->glory.reset();
					STATE(d) = CON_PLAYING;
					send_to_char("�������������� �������� ��-�� ������� ���������.\r\n", d->character.get());
					return;
				}
			}
		}
	}
}

/**
* �������� ����� � ���� �����, ��� ��������� � ����� (glory remove).
* \return 0 - ������ �� �����, 1 - ����� ������� �������
*/
bool remove_stats(CHAR_DATA *ch, CHAR_DATA *god, int amount)
{
	const auto it = glory_list.find(GET_UNIQUE(ch));
	if (it == glory_list.end())
	{
		send_to_char(god, "� %s ��� ��������� �����.\r\n", GET_PAD(ch, 1));
		return false;
	}
	if (amount > it->second->spend_glory)
	{
		send_to_char(god, "� %s ��� ������� ��������� �����.\r\n", GET_PAD(ch, 1));
		return false;
	}
	if (amount <= 0)
	{
		send_to_char(god, "������������ ���������� ������ (%d).\r\n", amount);
		return false;
	}

	int removed = 0;
	// ������ �� ������������� ������ ����� ������ ���������� �����
	for (auto tm_it = it->second->timers.begin(); tm_it != it->second->timers.end();)
	{
		if (amount > 0)
		{
			if ((*tm_it)->glory > amount)
			{
				// ���� � ���� �� ��� ������� ������, ��� ��������� �����
				(*tm_it)->glory -= amount;
				it->second->spend_glory -= amount;
				removed += amount;
				// ���� ��� ������ - ������� � ���� �����
				remove_stat_online(it->first, (*tm_it)->stat, amount);
				break;
			}
			else
			{
				// ���� ������� ������ - ������� �� �������� �����
				amount -= (*tm_it)->glory;
				it->second->spend_glory -= (*tm_it)->glory;
				removed += (*tm_it)->glory;
				// ���� ��� ������ - ������� � ���� �����
				remove_stat_online(it->first, (*tm_it)->stat, (*tm_it)->glory);
				it->second->timers.erase(tm_it++);
			}
		}
		else
			break;
	}
	imm_log("(GC) %s sets -%d stats to %s.", GET_NAME(god), removed, GET_NAME(ch));
	send_to_char(god, "� %s ����� %d %s ��������� ����� �����.\r\n", GET_PAD(ch, 1), removed, desc_count(removed, WHAT_POINT));
	// ���� ����������� �� �� ������ ������ � ����
	check_max_hp(ch);
	save_glory();
	return true;
}

/**
* �������� '�������', �.�. � ����������� ���� ��� �������� (����� ��������� �����) � ����� ���,
* ��� ���� � ����������� (��������� ����� ���������).
*/
void transfer_stats(CHAR_DATA *ch, CHAR_DATA *god, const std::string& name, const char *reason)
{
	if (IS_IMMORTAL(ch))
	{
		send_to_char(god, "�������� ����� � ����������� �� ������ ���������� ��������.\r\n");
		return;
	}

	const auto it = glory_list.find(GET_UNIQUE(ch));
	if (it == glory_list.end())
	{
		send_to_char(god, "� %s ��� �����.\r\n", GET_PAD(ch, 1));
		return;
	}

	if (it->second->denial)
	{
		send_to_char(god, "� %s ������� ������, ����������� ���������� ��������� ����� �����.\r\n", GET_PAD(ch, 1));
		return;
	}

	const long vict_uid = GetUniqueByName(name);
	if (!vict_uid)
	{
		send_to_char(god, "������������ ��� ��������� (%s), ������������ �����.\r\n", name.c_str());
		return;
	}

	DESCRIPTOR_DATA *d_vict = DescByUID(vict_uid);
	CHAR_DATA::shared_ptr vict;
	if (d_vict)
	{
		vict = d_vict->character;
	}
	else
	{
		// ����������� �������
		CHAR_DATA::shared_ptr t_vict(new Player); // TODO: ���������� �� ����
		if (load_char(name.c_str(), t_vict.get()) < 0)
		{
			send_to_char(god, "������������ ��� ��������� (%s), ������������ �����.\r\n", name.c_str());
			return;
		}

		vict = t_vict;
	}

	// ������ � ��� ����������� vict � ����� ������
	if (IS_IMMORTAL(vict))
	{
		send_to_char(god, "�������� ����� �� ������������ - ��� �����.\r\n");
		return;
	}

	if (str_cmp(GET_EMAIL(ch), GET_EMAIL(vict)))
	{
		send_to_char(god, "��������� ����� ������ email ������.\r\n");
		return;
	}

	// ���� ������ ������������, ���� �� ��� - �������
	auto vict_it = glory_list.find(vict_uid);
	if (vict_it == glory_list.end())
	{
		const GloryNodePtr temp_node(new GloryNode);
		glory_list[vict_uid] = temp_node;
		vict_it = glory_list.find(vict_uid);
	}
	// vict_it ������ �������� �������� �� ������������
	const int was_stats = vict_it->second->spend_glory;
	vict_it->second->copy_glory(it->second);
	vict_it->second->denial = DISPLACE_TIMER;

	snprintf(buf, MAX_STRING_LENGTH,
			"%s: ���������� (%s -> %s) �����: %d, ������: %d",
			GET_NAME(god), GET_NAME(ch), GET_NAME(vict), it->second->free_glory,
			vict_it->second->spend_glory - was_stats);
	imm_log(buf);
	mudlog(buf, DEF, LVL_IMMORT, SYSLOG, TRUE);
	add_karma(ch, buf, reason);
	GloryMisc::add_log(GloryMisc::TRANSFER_GLORY, 0, buf, std::string(reason), vict.get());

	// ���� ����������� ��� ������ - ����� ����� ��� �����
	if (d_vict)
	{
		// ��������� ����� �������� ������ ���� ���������, ��� �� ��� � ����
		GloryMisc::recalculate_stats(vict.get());
		for (const auto& tm_it : vict_it->second->timers)
		{
			if (tm_it->timer > 0)
			{
				switch (tm_it->stat)
				{
				case G_STR:
					vict->inc_str(tm_it->glory);
					break;
				case G_DEX:
					vict->inc_dex(tm_it->glory);
					break;
				case G_INT:
					vict->inc_int(tm_it->glory);
					break;
				case G_WIS:
					vict->inc_wis(tm_it->glory);
					break;
				case G_CON:
					vict->inc_con(tm_it->glory);
					break;
				case G_CHA:
					vict->inc_cha(tm_it->glory);
					break;
				default:
					log("Glory: ������������ ����� ����� %d (uid: %ld)",
							tm_it->stat, it->first);
				}
			}
		}
	}
	add_karma(vict.get(), buf, reason);
	vict->save_char();

	// ������� ������ ����, � �������� ������������
	glory_list.erase(it);
	// � ���������� ��� ����� ����� (�� �� �������� ��� �������� �����,
	// �� ��� �������� ������� ����� ���������� �����) � ���� �� ��� ������� - ��������� ��� �����
	DESCRIPTOR_DATA *k = DescByUID(GET_UNIQUE(ch));
	if (k)
	{
		GloryMisc::recalculate_stats(k->character.get());
	}

	save_glory();
}

// * ����� ��������� � ��������� ����� � ���� (glory ���).
void show_glory(CHAR_DATA *ch, CHAR_DATA *god)
{
	auto it = glory_list.find(GET_UNIQUE(ch));
	if (it == glory_list.end())
	{
		send_to_char(god, "� %s ������ �� �����.\r\n", GET_PAD(ch, 1));
		return;
	}

	send_to_char(god, "���������� �� ����� ����� %s:\r\n", GET_PAD(ch, 1));
	print_glory(god, it);
}

// * ����� ���� � show stats.
void show_stats(CHAR_DATA *ch)
{
	int free_glory = 0, spend_glory = 0;
	for (const auto& it : glory_list)
	{
		free_glory += it.second->free_glory;
		spend_glory += it.second->spend_glory * 1000;
	}
	send_to_char(ch, "  �����: ������� %d, �������� %d, ����� %d\r\n", spend_glory, free_glory, free_glory + spend_glory);
}

// * ���������� ���� �����. � ����� ������������� ���������� ������ �����, �� �������� � ��� (hide).
void print_glory_top(CHAR_DATA *ch)
{
	std::stringstream out;
	boost::format class_format("\t%-20s %-2d\r\n");
	std::map<int, GloryNodePtr> temp_list;
	std::stringstream hide;

	bool print_hide = false;
	if (IS_IMMORTAL(ch))
	{
		print_hide = true;
		hide << "\r\n���������, ����������� �� ������: ";
	}

	for (const auto& it : glory_list)
	{
		if (!it.second->hide && !it.second->freeze)
			temp_list[it.second->free_glory + it.second->spend_glory * 1000] = it.second;
		else if (print_hide)
			hide << it.second->name << " ";
	}

	out << CCWHT(ch, C_NRM) << "������ �������������:\r\n" << CCNRM(ch, C_NRM);

	int i = 0;
	for (auto t_it = temp_list.rbegin(); t_it != temp_list.rend() && i < MAX_TOP_CLASS; ++t_it, ++i)
	{
		//��� � ��������� �����. �� ����� ������� ���-�� �����...
		t_it->second->name[0] = UPPER(t_it->second->name[0]);
		out << class_format % t_it->second->name % (t_it->second->free_glory + t_it->second->spend_glory * 1000);
	}
	send_to_char(ch, out.str().c_str());

	if (print_hide)
	{
		hide << "\r\n";
		send_to_char(ch, hide.str().c_str());
	}
}

// * ���������/���������� ������ ���� � ���� ����� (glory hide on|off).
void hide_char(CHAR_DATA *vict, CHAR_DATA *god, char const * const mode)
{
	if (!mode || !*mode) 
		return;

	bool ok = true;
	const auto it = glory_list.find(GET_UNIQUE(vict));
	if (it != glory_list.end())
	{
		if (!str_cmp(mode, "on"))
			it->second->hide = true;
		else if (!str_cmp(mode, "off"))
			it->second->hide = true;
		else
			ok = false;
	}
	if (ok)
	{
		std::string text = it->second->hide ? "�������� �� ����" : "������������ � ����";
		send_to_char(god, "%s ������ %s ������������� ����������\r\n", GET_NAME(vict), text.c_str());
		save_glory();
	}
	else
		send_to_char(god, "������������ �������� %s, hide ����� ���� ������ on ��� off.\r\n", mode);
}

// * ��������� �������� �� ��������� �����.
void set_freeze(long uid)
{
	const auto it = glory_list.find(uid);
	if (it != glory_list.end())
		it->second->freeze = true;
	save_glory();
}

// * ��������� �������� �� ��������� �����.
void remove_freeze(long uid)
{
	const auto it = glory_list.find(uid);
	if (it != glory_list.end())
		it->second->freeze = false;
	save_glory();
}

// * �� ��������� ������� ���� ������������� � ������ �������� - ��������� ��� ���� ��� ����� � ����.
void check_freeze(CHAR_DATA *ch)
{
	const auto it = glory_list.find(GET_UNIQUE(ch));
	if (it != glory_list.end())
		it->second->freeze = PLR_FLAGGED(ch, PLR_FROZEN) ? true : false;
}

void set_stats(CHAR_DATA *ch)
{
	auto it = glory_list.find(GET_UNIQUE(ch));
	if (glory_list.end() == it)
	{
		return;
	}

	for (const auto& tm_it : it->second->timers)
	{
		if (tm_it->timer > 0)
		{
			switch (tm_it->stat)
			{
			case G_STR:
				ch->inc_str(tm_it->glory);
				break;
			case G_DEX:
				ch->inc_dex(tm_it->glory);
				break;
			case G_INT:
				ch->inc_int(tm_it->glory);
				break;
			case G_WIS:
				ch->inc_wis(tm_it->glory);
				break;
			case G_CON:
				ch->inc_con(tm_it->glory);
				break;
			case G_CHA:
				ch->inc_cha(tm_it->glory);
				break;
			default:
				log("Glory: ������������ ����� ����� %d (uid: %ld)", tm_it->stat, it->first);
			}
		}
	}
}

int get_spend_glory(CHAR_DATA *ch)
{
	const auto i = glory_list.find(GET_UNIQUE(ch));
	if (glory_list.end() != i)
	{
		return i->second->spend_glory;
	}
	return 0;
}

} // namespace Glory

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
