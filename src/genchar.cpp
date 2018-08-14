/* ************************************************************************
*   File: genchar.cpp                                   Part of Bylins    *
*  Usage: functions for character generation                              *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#include "genchar.h"

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "logger.hpp"
#include "utils.h"
#include "spells.h"
#include "char.hpp"
#include "char_player.hpp"

const char *genchar_help =
	"\r\n ������ �� ������ ������� ���� ��������������. � ����������� �� ����, ���\r\n"
	"�� �� ������������, �� ������ ����� �������� ����� ������ ���������.\r\n"
	"� ������ '����� ��������' �� ������ ����� �������, ������� ����� ������������\r\n"
	"�� ��������������� ������ ���������. �������� ��������������, �� ����������\r\n"
	"���� ���� ���, ���������� - ��������.\r\n"
	"����� �� ���������, ����� � ��� � ���� ������ ���������� ����� 0, �� �������\r\n"
	"��������� ��������� � ����� � ����.\r\n";

int max_stats[][6] =
	// Str Dex Int Wis Con Cha
{ {14, 13, 24, 25, 15, 10},	// ������ //
	{14, 12, 25, 23, 13, 16},	// ������ //
	{19, 25, 12, 12, 17, 16},	// ��� //
	{25, 11, 15, 15, 25, 10},	// �������� //
	{22, 24, 14, 14, 17, 12},	// ������� //
	{23, 17, 14, 14, 23, 12},	// ��������� //
	{14, 12, 25, 23, 13, 16},	// �������� //
	{14, 12, 25, 23, 13, 16},	// ��������� //
	{15, 13, 25, 23, 14, 12},	// ������������ //
	{22, 13, 16, 19, 18, 17},	// ������ //
	{25, 21, 16, 16, 18, 16},	// ������� //
	{25, 17, 13, 15, 20, 16},	// ������ //
	{21, 17, 14, 13, 20, 17},	// ����� //
	{18, 12, 24, 18, 15, 12}	// ����� //
};

int min_stats[][6] =
	// Str Dex Int Wis Con Cha //
{ {11, 10, 19, 20, 12, 10},	// ������ //
	{10,  9, 20, 18, 10, 13},	// ������ //
	{16, 22,  9,  9, 14, 13},	// ��� //
	{21,  8, 11, 11, 22, 10},	// �������� //
	{17, 19, 11, 11, 14, 12},	// ������� //
	{20, 14, 10, 10, 17, 12},	// ��������� //
	{10,  9, 20, 18, 10, 13},	// �������� //
	{10,  9, 20, 18, 10, 13},	// ��������� //
	{ 9,  9, 20, 20, 11, 10},	// ������������ //
	{19, 10, 12, 15, 14, 13},	// ������ //
	{19, 15, 11, 11, 14, 11},	// ������� //
	{20, 14, 10, 11, 14, 12},	// ������ //
	{18, 14, 10, 10, 14, 13},	// ����� //
	{15, 10, 19, 15, 12, 12}	// ����� //
};

int auto_stats[][6] =
	// Str Dex Int Wis Con Cha //
{ {11, 10, 24, 25, 15, 10},	// ������ //
	{12,  9, 25, 23, 13, 13},	// ������ //
	{19, 25,  12,  9, 17, 13},	// ��� //
	{25,  8, 15, 12, 25, 10},	// �������� //
	{22, 24, 11, 11, 15, 12},	// ������� //
	{23, 17, 10, 10, 23, 12},	// ��������� //
	{10,  9, 25, 22, 13, 16},	// �������� //
	{10,  9, 25, 22, 13, 16},	// ��������� //
	{14,  9, 25, 23, 14, 10},	// ������������ //
	{22, 13, 12, 17, 18, 13},	// ������ //
	{25, 21, 11, 11, 16, 11},	// ������� //
	{25, 17, 10, 11, 20, 12},	// ������ //
	{18, 17, 14, 11, 18, 17},	// ����� //
	{16, 10, 24, 18, 15, 12}	// ����� //
};


const char *default_race[] = { 
	"�������", //������
	"������", //������
    "������", //���
    "�������� ��� ������", //��������
	"������", //�������
	"������ ��� ������", // ���������
	"������", // ��������
	"������", //���������
	"������ ��� �������", //������������
	"������", //������
	"������", //�������
	"��������", //������
	"������ ��� ������", // �����
	"������" //�����
};



void genchar_disp_menu(CHAR_DATA * ch)
{
	char buf[MAX_STRING_LENGTH];

	sprintf(buf,
			"\r\n              -      +\r\n"
			"  ����     : (�) %2d (�)        [%2d - %2d]\r\n"
			"  �������� : (�) %2d (�)        [%2d - %2d]\r\n"
			"  ��       : (�) %2d (�)        [%2d - %2d]\r\n"
			"  �������� : (�) %2d (�)        [%2d - %2d]\r\n"
			"  �������� : (�) %2d (�)        [%2d - %2d]\r\n"
			"  �������  : (�) %2d (�)        [%2d - %2d]\r\n"
			"\r\n"
			"  ����� ��������: %3d \r\n"
			"\r\n"
			"  �) ������ (��� ���. ���������� �������� '������� ��������������')\r\n"
			"  �) &W������������� ����������� (������������� ��� ��������)&n\r\n",
			ch->get_inborn_str(), MIN_STR(ch), MAX_STR(ch),
			ch->get_inborn_dex(), MIN_DEX(ch), MAX_DEX(ch),
			ch->get_inborn_int(), MIN_INT(ch), MAX_INT(ch),
			ch->get_inborn_wis(), MIN_WIS(ch), MAX_WIS(ch),
			ch->get_inborn_con(), MIN_CON(ch), MAX_CON(ch),
			ch->get_inborn_cha(), MIN_CHA(ch), MAX_CHA(ch), SUM_ALL_STATS - SUM_STATS(ch));
	send_to_char(buf, ch);
	if (SUM_ALL_STATS == SUM_STATS(ch))
		send_to_char("  �) ��������� ���������\r\n", ch);
	send_to_char(" ��� �����: ", ch);
}

int genchar_parse(CHAR_DATA* ch, const char* arg)
{
	int tmp_class;
	switch (*arg)
	{
	case '�':
	case '�':
		ch->set_str(MAX(ch->get_inborn_str() - 1, MIN_STR(ch)));
		break;
	case '�':
	case '�':
		ch->set_dex(MAX(ch->get_inborn_dex() - 1, MIN_DEX(ch)));
		break;
	case '�':
	case '�':
		ch->set_int(MAX(ch->get_inborn_int() - 1, MIN_INT(ch)));
		break;
	case '�':
	case '�':
		ch->set_wis(MAX(ch->get_inborn_wis() - 1, MIN_WIS(ch)));
		break;
	case '�':
	case '�':
		ch->set_con(MAX(ch->get_inborn_con() - 1, MIN_CON(ch)));
		break;
	case '�':
	case '�':
		ch->set_cha(MAX(ch->get_inborn_cha() - 1, MIN_CHA(ch)));
		break;
	case '�':
	case '�':
		ch->set_str(MIN(ch->get_inborn_str() + 1, MAX_STR(ch)));
		break;
	case '�':
	case '�':
		ch->set_dex(MIN(ch->get_inborn_dex() + 1, MAX_DEX(ch)));
		break;
	case '�':
	case '�':
		ch->set_int(MIN(ch->get_inborn_int() + 1, MAX_INT(ch)));
		break;
	case '�':
	case '�':
		ch->set_wis(MIN(ch->get_inborn_wis() + 1, MAX_WIS(ch)));
		break;
	case '�':
	case '�':
		ch->set_con(MIN(ch->get_inborn_con() + 1, MAX_CON(ch)));
		break;
	case '�':
	case '�':
		ch->set_cha(MIN(ch->get_inborn_cha() + 1, MAX_CHA(ch)));
		break;
	case '�':
	case '�':
		send_to_char(genchar_help, ch);
		break;
	case '�':
	case '�':
		if (SUM_STATS(ch) != SUM_ALL_STATS)
			break;
		// �� ������ �������� ��������� ��������� ��������� �����
		ch->set_start_stat(G_STR, ch->get_inborn_str());
		ch->set_start_stat(G_DEX, ch->get_inborn_dex());
		ch->set_start_stat(G_INT, ch->get_inborn_int());
		ch->set_start_stat(G_WIS, ch->get_inborn_wis());
		ch->set_start_stat(G_CON, ch->get_inborn_con());
		ch->set_start_stat(G_CHA, ch->get_inborn_cha());
		return GENCHAR_EXIT;
	case '�':
	case '�':
		tmp_class = GET_CLASS(ch);
		ch->set_str(auto_stats[tmp_class][0]);
		ch->set_dex(auto_stats[tmp_class][1]);
		ch->set_int(auto_stats[tmp_class][2]);
		ch->set_wis(auto_stats[tmp_class][3]);
		ch->set_con(auto_stats[tmp_class][4]);
		ch->set_cha(auto_stats[tmp_class][5]);
		ch->set_start_stat(G_STR, ch->get_inborn_str());
		ch->set_start_stat(G_DEX, ch->get_inborn_dex());
		ch->set_start_stat(G_INT, ch->get_inborn_int());
		ch->set_start_stat(G_WIS, ch->get_inborn_wis());
		ch->set_start_stat(G_CON, ch->get_inborn_con());
		ch->set_start_stat(G_CHA, ch->get_inborn_cha());
		return GENCHAR_EXIT;
	default:
		break;
	}
	return GENCHAR_CONTINUE;
}

/*
 * Roll the 6 stats for a character... each stat is made of the sum of
 * the best 3 out of 4 rolls of a 6-sided die.  Each class then decides
 * which priority will be given for the best to worst stats.
 */
void roll_real_abils(CHAR_DATA * ch)
{
	int i;

	switch (ch->get_class())
	{
	case CLASS_CLERIC:
		ch->set_cha (10);
		do
		{
			ch->set_con(12 + number(0, 3));
			ch->set_wis(18 + number(0, 5));
			ch->set_int(18 + number(0, 5));
		}		// 57/48 roll 13/9
		while (ch->get_con() + ch->get_wis() + ch->get_int() != 57);
		do
		{
			ch->set_str(11 + number(0, 3));
			ch->set_dex(10 + number(0, 3));
		}		// 92/88 roll 6/4
		while (ch->get_str() + ch->get_con() + ch->get_dex() +
				ch->get_wis() + ch->get_int() + ch->get_cha() != 92);
		// ADD SPECIFIC STATS
		ch->set_wis(ch->get_wis() + 2);
		ch->set_int(ch->get_int() + 1);
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 170) : number(120, 180);
		break;

	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
		do
		{
			ch->set_str(10 + number(0, 4));
			ch->set_wis(17 + number(0, 5));
			ch->set_int(18 + number(0, 5));
		}		// 55/45 roll 14/10
		while (ch->get_str() + ch->get_wis()+ ch->get_int()!= 55);
		do
		{
			ch->set_con(10 + number(0, 3));
			ch->set_dex(9 + number(0, 3));
			ch->set_cha(13 + number(0, 3));
		}		// 92/87 roll 9/5
		while (ch->get_str() + ch->get_con() + ch->get_dex() +
				ch->get_wis()+ ch->get_int()+ ch->get_cha()!= 92);
		// ADD SPECIFIC STATS
		ch->set_wis(ch->get_wis() + 1);
		ch->set_int(ch->get_int() + 2);
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 170) : number(150, 180);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 150) : number(120, 180);
		break;

	case CLASS_NECROMANCER:
		do
		{
			ch->set_cha(10 + number(0, 2));
			ch->set_wis(20 + number(0, 3));
			ch->set_int(18 + number(0, 5));
		}	// 58/48
		while (ch->get_cha()+ ch->get_wis()+ ch->get_int()!= 55);
		do
		{
			ch->set_str(9 + number(0, 6));
			ch->set_dex(9 + number(0, 4));
			ch->set_con(11 + number(0, 2));
		}	// 96/84
		while (ch->get_str() + ch->get_con() + ch->get_dex() +
				ch->get_wis()+ ch->get_int()+ ch->get_cha()!= 92);
		// ADD SPECIFIC STATS
		ch->set_con(ch->get_con() + 1);
		ch->set_int(ch->get_int() + 2);
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 170) : number(150, 180);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 150) : number(120, 180);
		break;

	case CLASS_THIEF:
		do
		{
			ch->set_str(16 + number(0, 3));
			ch->set_con(14 + number(0, 3));
			ch->set_dex(20 + number(0, 3));
		}	// 59/50
		while (ch->get_str() + ch->get_con() + ch->get_dex() != 57);
		do
		{
			ch->set_wis(9 + number(0, 3));
			ch->set_cha(13 + number(0, 2));
			ch->set_int(9 + number(0, 3));
		}	// 96/88
		while (ch->get_str() + ch->get_con() + ch->get_dex() +
				ch->get_wis()+ ch->get_int()+ ch->get_cha()!= 92);
		// ADD SPECIFIC STATS
		ch->set_dex(ch->get_dex() + 2);
		ch->set_cha(ch->get_cha()+ 1);
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 190);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 170) : number(120, 180);
		break;

	case CLASS_WARRIOR:
		ch->set_cha(10);
		do
		{
			ch->set_str(20 + number(0, 4));
			ch->set_dex(8 + number(0, 3));
			ch->set_con(20 + number(0, 3));
		}		// 55/48 roll 10/7
		while (ch->get_str() + ch->get_con() + ch->get_dex() != 55);
		do
		{
			ch->set_int(11 + number(0, 4));
			ch->set_wis(11 + number(0, 4));
		}		// 92/87 roll 8/5
		while (ch->get_str() + ch->get_con() + ch->get_dex() +
				ch->get_wis()+ ch->get_int()+ ch->get_cha()!= 92);
		// ADD SPECIFIC STATS
		ch->set_str(ch->get_str() + 1);
		ch->set_con(ch->get_con() + 2);
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(165, 180) : number(170, 200);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(160, 180) : number(170, 200);
		break;

	case CLASS_ASSASINE:
		ch->set_cha(12);
		do
		{
			ch->set_str(16 + number(0, 5));
			ch->set_dex(18 + number(0, 5));
			ch->set_con(14 + number(0, 2));
		}	// 60/48
		while (ch->get_str() + ch->get_con() + ch->get_dex() != 55);
		do
		{
			ch->set_int(11 + number(0, 3));
			ch->set_wis(11 + number(0, 3));
		}	// 95/89
		while (ch->get_str() + ch->get_con() + ch->get_dex() +
				ch->get_wis()+ ch->get_int()+ ch->get_cha()!= 92);
		// ADD SPECIFIC STATS
		ch->set_str(ch->get_str() + 1);
		ch->set_con(ch->get_con() + 1);
		ch->set_dex(ch->get_dex() + 1);
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 180) : number(150, 200);
		break;

	case CLASS_GUARD:
		ch->set_cha(12);
		do
		{
			ch->set_str(19 + number(0, 3));
			ch->set_dex(13 + number(0, 3));
			ch->set_con(16 + number(0, 5));
		}		// 55/48 roll 11/7
		while (ch->get_str() + ch->get_con() + ch->get_dex() != 55);
		do
		{
			ch->set_int(10 + number(0, 4));
			ch->set_wis(10 + number(0, 4));
		}		// 92/87 roll 8/5
		while (ch->get_str() + ch->get_con() + ch->get_dex() +
				ch->get_wis()+ ch->get_int()+ ch->get_cha()!= 92);
		// ADD SPECIFIC STATS
		ch->set_str(ch->get_str() + 1);
		ch->set_dex(ch->get_dex() + 1);
		ch->set_con(ch->get_con() + 1);
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(140, 170) : number(160, 200);
		break;

	case CLASS_PALADINE:
		do
		{
			ch->set_str(18 + number(0, 3));
			ch->set_wis(14 + number(0, 4));
			ch->set_con(14 + number(0, 4));
		}		// 53/46 roll 11/7
		while (ch->get_str() + ch->get_con() + ch->get_wis()!= 53);
		do
		{
			ch->set_int(12 + number(0, 4));
			ch->set_dex(10 + number(0, 3));
			ch->set_cha(12 + number(0, 4));
		}		// 92/87 roll 11/5
		while (ch->get_str() + ch->get_con() + ch->get_dex() +
				ch->get_wis()+ ch->get_int()+ ch->get_cha()!= 92);
		// ADD SPECIFIC STATS
		ch->set_str(ch->get_str() + 1);
		ch->set_cha(ch->get_cha() + 1);
		ch->set_wis(ch->get_wis() + 1);
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(140, 175) : number(140, 190);
		break;

	case CLASS_RANGER:

		do
		{
			ch->set_str(18 + number(0, 6));
			ch->set_dex(13 + number(0, 6));
			ch->set_con(14 + number(0, 4));
		}		// 53/46 roll 12/7
		while (ch->get_str() + ch->get_con() + ch->get_dex() != 53);
		do
		{
			ch->set_int(11 + number(0, 5));
			ch->set_wis(11 + number(0, 5));
			ch->set_cha(11 + number(0, 5));
		}		// 92/85 roll 10/7
		while (ch->get_str() + ch->get_con() + ch->get_dex() +
				ch->get_wis()+ ch->get_int()+ ch->get_cha()!= 92);
		// ADD SPECIFIC STATS
		ch->set_str(ch->get_str() + 1);
		ch->set_dex(ch->get_dex() + 2);
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 200);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 180) : number(120, 200);
		break;

	case CLASS_SMITH:
		do
		{
			ch->set_str(18 + number(0, 5));
			ch->set_dex(14 + number(0, 3));
			ch->set_con(14 + number(0, 6));
		}		// 53/46 roll 11/7
		while (ch->get_str() + ch->get_dex() + ch->get_con() != 55);
		do
		{
			ch->set_int(10 + number(0, 3));
			ch->set_wis(11 + number(0, 4));
			ch->set_cha(11 + number(0, 4));
		}		// 92/85 roll 11/7
		while (ch->get_str() + ch->get_con() + ch->get_dex() +
				ch->get_wis()+ ch->get_int()+ ch->get_cha()!= 92);
		// ADD SPECIFIC STATS
		ch->set_str(ch->get_str() + 2);
		ch->set_cha(ch->get_cha() + 1);
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(160, 180) : number(170, 200);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(170, 200);
		break;

	case CLASS_MERCHANT:
		do
		{
			ch->set_str(18 + number(0, 3));
			ch->set_con(12 + number(0, 6));
			ch->set_dex(14 + number(0, 3));
		}		// 55/48 roll 9/7
		while (ch->get_str() + ch->get_con() + ch->get_dex() != 55);
		do
		{
			ch->set_wis(10 + number(0, 3));
			ch->set_cha(12 + number(0, 4));
			ch->set_int(10 + number(0, 4));
		}		// 92/87 roll 9/5
		while (ch->get_str() + ch->get_con() + ch->get_dex() +
				ch->get_wis()+ ch->get_int()+ ch->get_cha()!= 92);
		// ADD SPECIFIC STATS
		ch->set_con(ch->get_con() + 2);
		ch->set_cha(ch->get_cha() + 1);
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 170) : number(150, 190);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 180) : number(120, 200);
		break;

	case CLASS_DRUID:
		ch->set_cha(12);
		do
		{
			ch->set_con(12 + number(0, 3));
			ch->set_wis(15 + number(0, 3));
			ch->set_int(17 + number(0, 5));
		}		// 53/45 roll 12/8
		while (ch->get_con() + ch->get_wis()+ ch->get_int()!= 53);
		do
		{
			ch->set_str(14 + number(0, 3));
			ch->set_dex(10 + number(0, 2));
		}		// 92/89 roll 5/3
		while (ch->get_str() + ch->get_con() + ch->get_dex() +
				ch->get_wis()+ ch->get_int()+ ch->get_cha()!= 92);
		// ADD SPECIFIC STATS
		ch->set_str(ch->get_str() + 1);
		ch->set_int(ch->get_int() + 2);
		GET_HEIGHT(ch) = IS_FEMALE(ch) ? number(150, 180) : number(150, 190);
		GET_WEIGHT(ch) = IS_FEMALE(ch) ? number(120, 170) : number(120, 180);
		for (i = 1; i <= MAX_SPELLS; i++)
			GET_SPELL_TYPE(ch, i) = SPELL_RUNES;
		break;

	default:
		log("SYSERROR : ATTEMPT STORE ABILITIES FOR UNKNOWN CLASS (Player %s)", GET_NAME(ch));
	};
}

// ������� ��� ��������� ����� �� �������.
// ����� ������ ���� ������� ���������� � ������ �������.
// name - ��� � ������������ ������
// sex - ��� (SEX_MALE ��� SEX_FEMALE)
// caseNum - ����� ������ (0 - 5)
//  0 - ������������ (���? ���?)
//  1 - ����������� (����? ����?)
//  2 - ��������� (����? ����?)
//  3 - ����������� (����? ���?)
//  4 - ������������ (���? ���?)
//  5 - ���������� (� ���? � ���?)
// result - ���������
void GetCase(const char *name, const ESex sex, int caseNum, char *result)
{
	const size_t len = strlen(name);

	if (strchr("��������������������", name[len - 1]) != NULL
		&& sex == ESex::SEX_MALE)
	{
		strcpy(result, name);
		if (caseNum == 1)
			strcat(result, "�"); // �����
		else if (caseNum == 2)
			strcat(result, "�"); // �����
		else if (caseNum == 3)
			strcat(result, "�"); // �����
		else if (caseNum == 4)
			strcat(result, "��"); // ������, �������
		else if (caseNum == 5)
			strcat(result, "�"); // �����
	}
	else if (name[len - 1] == '�')
	{
		strncpy(result, name, len - 1);
		result[len - 1] = '\0';
		if (caseNum == 1)
			strcat(result, "�"); // ���, ����
		else if (caseNum == 2)
			strcat(result, "�"); // ���, ����
		else if (caseNum == 3)
			strcat(result, "�"); // ���, ����
		else if (caseNum == 4)
			strcat(result, "��"); // ����, �����
		else if (caseNum == 5)
			strcat(result, "�"); // ���, ����
		else
			strcat(result, "�"); // ���, ����
	}
	else if (name[len - 1] == '�'
		&& sex == ESex::SEX_MALE)
	{
		strncpy(result, name, len - 1);
		result[len - 1] = '\0';
		if (caseNum == 1)
			strcat(result, "�"); // �������
		else if (caseNum == 2)
			strcat(result, "�"); // �������
		else if (caseNum == 3)
			strcat(result, "�"); // �������
		else if (caseNum == 4)
			strcat(result, "��"); // ��������
		else if (caseNum == 5)
			strcat(result, "�"); // �������
		else
			strcat(result, "�"); // �������
	}
	else if (name[len - 1] == '�')
	{
		strncpy(result, name, len - 1);
		result[len - 1] = '\0';
		if (caseNum == 1)
		{
			if (strchr("����", name[len - 2]) != NULL)
				strcat(result, "�"); // ����, ����
			else
				strcat(result, "�"); // ����
		}
		else if (caseNum == 2)
			strcat(result, "�"); // ����, ����
		else if (caseNum == 3)
			strcat(result, "�"); // ����, ����
		else if (caseNum == 4)
		{
			if (strchr("���", name[len - 2]) != NULL)
				strcat(result, "��"); // �����, �����
			else
				strcat(result, "��"); // �����, ������
		}
		else if (caseNum == 5)
			strcat(result, "�"); // ����, ����
		else
			strcat(result, "�"); // ����, ����
	}
	else
	{
		// ��������� �������� ���� �� ����������, ���� ����� (��������, ������������ �� �)
		strcpy(result, name);
	}
	CAP(result);
	return;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
