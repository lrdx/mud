// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#include "reset_stats.hpp"

#include "logger.hpp"
#include "utils.h"
#include "genchar.h"
#include "db.h"
#include "screen.h"
#include "parse.hpp"
#include "char.hpp"
#include "features.hpp"
#include "conf.h"

#include <boost/format.hpp>

#include <string>
#include <array>
#include <algorithm>

namespace ResetStats
{

const char *CONFIG_FILE = LIB_MISC"reset_stats.xml";

// ��� ������ reset_prices
struct price_node
{
	int base_price;
	int add_price;
	int max_price;
	// �������� ������������ �������������� ��� �������/�����
	std::string log_text;
};

// ������ ��� ��� ������ ����� ������ ������������� ���� (�� CONFIG_FILE)
std::array<price_node, Type::TOTAL_NUM> reset_prices =
{{
	{ 100000000, 1000000, 1000000000, "main stats" },
	{ 110000000, 1100000, 1100000000, "race" },
	{ 120000000, 1200000, 1200000000, "feats" },
	{ 400000, 200000, 1000000, "religion" }
}};

///
/// ���� ��������� ������ � CONFIG_FILE �� init()
///
void parse_prices(const pugi::xml_node &cur_node, Type type)
{
	if (cur_node)
	{
		reset_prices.at(type).base_price = Parse::attr_int(cur_node, "price");
		reset_prices.at(type).add_price = Parse::attr_int(cur_node, "price_add");
		reset_prices.at(type).max_price = Parse::attr_int(cur_node, "max_price");
	}
}

///
/// ����/������ CONFIG_FILE, ���������� ����� 'reload <��� �����>'
///
void init()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(CONFIG_FILE);
	if (!result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

    pugi::xml_node main_node = doc.child("reset_stats");
    if (!main_node)
    {
		snprintf(buf, MAX_STRING_LENGTH, "...<reset_stats> read fail");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
    }

	pugi::xml_node cur_node = Parse::get_child(main_node, "main_stats");
	parse_prices(cur_node, Type::MAIN_STATS);

	cur_node = Parse::get_child(main_node, "race");
	parse_prices(cur_node, Type::RACE);

	cur_node = Parse::get_child(main_node, "feats");
	parse_prices(cur_node, Type::FEATS);
	cur_node = Parse::get_child(main_node, "religion");
	parse_prices(cur_node, Type::RELIGION);
}

///
/// \return ��������� ���������� ������ ������������� type ����� ����
///
int calc_price(CHAR_DATA *ch, Type type)
{
	int price = reset_prices[type].base_price
		+ ch->get_reset_stats_cnt(type) * reset_prices[type].add_price;
	return std::min(price, reset_prices[type].max_price);
}

///
/// ���������� ���� �� ����� ������������� type:
/// ������ �����, �����������, ������ � �����
///
void reset_stats(CHAR_DATA *ch, Type type)
{
	switch (type)
	{
	case Type::MAIN_STATS:
		ch->set_start_stat(G_STR, 0);
		break;
	case Type::RACE:
		// ������� ����������� �������� ����
		set_race_feats(ch, false);
		// � ����� ��� ������� �� ValidateStats
		ch->set_race(99);
		break;
	case Type::FEATS:
		ch->real_abils.Feats.reset();
		set_natural_feats(ch);
		break;
	case Type::RELIGION:
		ch->player_data.Religion = 2; //RELIGION_MONO + 1
		break;
	default:
		mudlog("SYSERROR: reset_stats() switch", NRM, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

	ch->inc_reset_stats_cnt(type);
	ch->save_char();
}

///
/// ���������� ���� ������ �������������
///
void print_menu(DESCRIPTOR_DATA *d)
{
	const int stats_price = calc_price(d->character.get(), Type::MAIN_STATS);
	const int race_price = calc_price(d->character.get(), Type::RACE);
	const int feats_price = calc_price(d->character.get(), Type::FEATS);
	const int religion_price = calc_price(d->character.get(), Type::RELIGION);

	std::string str = boost::str(boost::format(
		"%s� ������ ������ ����� ��������� ����� ����� ���������� ��� ��������� ����� � ����.%s\r\n\r\n"
		"1) �������� %d %s � ������ ����������������� ��������� �������������.\r\n"
		"2) �������� %d %s � ������� � ������ ����.\r\n"
		"3) �������� %d %s � �������� ����������� (����� ����������).\r\n"
		"4) �������� %d %s � ������� � ������ ���������������.\r\n"
		"5) �������� � ��������� � ������� ����\r\n"
		"\r\n��� �����:")
		% CCIGRN(d->character, C_SPR) % CCNRM(d->character, C_SPR)
		% stats_price % desc_count(stats_price, WHAT_MONEYa)
		% race_price % desc_count(race_price, WHAT_MONEYa)
		% feats_price % desc_count(feats_price, WHAT_MONEYa)
		% religion_price % desc_count(religion_price, WHAT_MONEYa));
	SEND_TO_Q(str.c_str(), d);
}

///
/// ��������� ����������� ���� ������ ������������� �� ����
///
void process(DESCRIPTOR_DATA *d, Type type)
{
	const auto& ch = d->character;
	const int price = calc_price(ch.get(), type);

	if (ch->get_total_gold() < price)
	{
		SEND_TO_Q("\r\n� ��� ��� ����� �����!\r\n", d);
		SEND_TO_Q(MENU, d);
		STATE(d) = CON_MENU;
	}
	else
	{
		char buf_[MAX_INPUT_LENGTH];
		reset_stats(ch.get(), type);

		if ((type == Type::MAIN_STATS || type == Type::RACE || type == Type::RELIGION)
			&& ValidateStats(d))
		{
			// ���� �� ������ ����, ������ ���� �� ������ �� �������� ������
			// ����� �������� � ValidateStats()
			SEND_TO_Q("��������� �����-�� ������, �������� �����!\r\n", d);
			SEND_TO_Q(MENU, d);
			STATE(d) = CON_MENU;
			snprintf(buf_, sizeof(buf_), "%s failed to change %s",
				d->character->get_name().c_str(), reset_prices.at(type).log_text.c_str());
			mudlog(buf_, NRM, LVL_IMMORT, SYSLOG, TRUE);
		}
		else
		{
			// � ����� ������ ������ ��������� ����� ������� ������������
			snprintf(buf_, sizeof(buf_), "changed %s, price=%d",
				reset_prices.at(type).log_text.c_str(), price);
			add_karma(ch.get(), buf_, "auto");

			ch->remove_both_gold(price);
			ch->save_char();

			snprintf(buf_, sizeof(buf_), "%s changed %s, price=%d",
				ch->get_name().c_str(), reset_prices.at(type).log_text.c_str(), price);
			mudlog(buf_, NRM, LVL_BUILDER, SYSLOG, TRUE);
		}

		// ��� ������ �� ����� ValidateStats ����� ���������� ���� ������ � ����
		if (type == Type::FEATS)
		{
			const char *message = "\r\n��������� ������������� ���������!\r\n";
			if (!check_dupes_email(d))
			{
				SEND_TO_Q(message, d);
				STATE(d) = CON_CLOSE;
				return;
			}
			do_entergame(d);
			SEND_TO_Q(message, d);
		}
	}
}

///
/// ��������� ������� � ���� ������ ������������� ��� ������ ����
///
void parse_menu(DESCRIPTOR_DATA *d, const char *arg)
{
	bool result = false;

	if (arg && a_isdigit(*arg))
	{
		const int num = atoi(arg);

		switch (num)
		{
		case 1:
			process(d, Type::MAIN_STATS);
			result = true;
			break;
		case 2:
			process(d, Type::RACE);
			result = true;
			break;
		case 3:
			process(d, Type::FEATS);
			result = true;
			break;
		case 4:
			process(d, Type::RELIGION);
			result = true;
			break;
		}
	}

	if (!result)
	{
		SEND_TO_Q("��������� ���������� ��������� ���� ��������.\r\n", d);
		SEND_TO_Q(MENU, d);
		STATE(d) = CON_MENU;
	}
}

} // namespace ResetStats

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
