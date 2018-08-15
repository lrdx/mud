/************************************************************************
 *  OasisOLC - redit.c						v1.5	*
 *  Copyright 1996 Harvey Gilpin.					*
 *  Original author: Levork						*
* 									  *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                      *
 ************************************************************************/

#include "obj.hpp"
#include "comm.h"
#include "db.h"
#include "olc.h"
#include "dg_olc.h"
#include "constants.h"
#include "im.h"
#include "description.h"
#include "deathtrap.hpp"
#include "char.hpp"
#include "room.hpp"
#include "house.h"
#include "world.characters.hpp"
#include "logger.hpp"
#include "utils.h"
#include "structs.h"
#include "sysdep.h"
#include "conf.h"

#include <vector>

//***************************************************************************
#define  W_EXIT(room, num) (world[(room)]->dir_option[(num)])
//***************************************************************************

void redit_setup(DESCRIPTOR_DATA * d, int real_num)
/*++
   ���������� ������ ��� �������������� �������.
      d        - OLC ����������
      real_num - RNUM �������� �������, ����� -1
--*/
{
	ROOM_DATA *room = new ROOM_DATA;
	if (real_num == NOWHERE)
	{
		room->name = str_dup("������������ �������.\r\n");
		room->temp_description = str_dup("�� ��������� � �������, ����������� ��������� ���������� ������ �������.\r\n");
	}
	else
	{
		room_copy(room, world[real_num]);
		// temp_description ���������� ������ �� ����� �������������� ������� � ���
		room->temp_description = str_dup(RoomDescription::show_desc(world[real_num]->description_num).c_str());
	}

	OLC_ROOM(d) = room;
	OLC_ITEM_TYPE(d) = WLD_TRIGGER;
	redit_disp_menu(d);
	OLC_VAL(d) = 0;
}

//------------------------------------------------------------------------

#define ZCMD (zone_table[zone].cmd[cmd_no])

// * ��������� ����� ������� � ������
void redit_save_internally(DESCRIPTOR_DATA * d)
{
	int j, room_num, zone, cmd_no;
	OBJ_DATA *temp_obj;
	DESCRIPTOR_DATA *dsc;

	room_num = real_room(OLC_ROOM(d)->number);
	// ������ temp_description ��� ����� �� ���������, �������� ������� ��� ������ ����� �����
	OLC_ROOM(d)->description_num = RoomDescription::add_desc(OLC_ROOM(d)->temp_description);
	// * Room exists: move contents over then free and replace it.
	if (room_num != NOWHERE)
	{
		log("[REdit] Save room to mem %d", room_num);
		// ������ ���������� ������
		room_free(world[room_num]);
		// ������� ��������� ������� �� ����������, �������� ��������������� ������
		room_copy(world[room_num], OLC_ROOM(d));
		// ������ ������ ������� OLC_ROOM(d) � ��� ����� ������
		room_free(OLC_ROOM(d));
		// �������� "��������" ���������� � olc_cleanup
	}
	else
	{
		// ���� ������� �� ���� - ��������� �����
		auto it = world.cbegin();
		advance(it, FIRST_ROOM);
		int i = FIRST_ROOM;

		for (; it != world.cend(); ++it, ++i)
		{
			if ((*it)->number > OLC_NUM(d))
			{
				break;
			}
		}

		ROOM_DATA *new_room = new ROOM_DATA;
		room_copy(new_room, OLC_ROOM(d));
		new_room->number = OLC_NUM(d);
		new_room->zone = OLC_ZNUM(d);
		new_room->func = NULL;
		room_num = i; // ���� ����� �������

		if (it != world.cend())
		{
			world.insert(it, new_room);
			// ���� ������� ��������� �����, �� �� ���� ���������� � �����/����� � ���� ��������
			for (i = room_num; i <= top_of_world; i++)
			{
				for (const auto temp_ch : world[i]->people)
				{
					if (temp_ch->in_room != NOWHERE)
					{
						temp_ch->in_room = i;
					}
				}

				for (temp_obj = world[i]->contents; temp_obj; temp_obj = temp_obj->get_next_content())
				{
					if (temp_obj->get_in_room() != NOWHERE)
					{
						temp_obj->set_in_room(i);
					}
				}
			}
		}
		else
		{
			world.push_back(new_room);
		}

		fix_ingr_chest_rnum(room_num);//������ ������� �������� � �������

		// Copy world table over to new one.
		top_of_world++;

// ��������������

		// Update zone table.
		for (zone = 0; zone <= top_of_zone_table; zone++)
		{
			for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
			{
				switch (ZCMD.command)
				{
				case 'M':
				case 'O':
					if (ZCMD.arg3 >= room_num)
						ZCMD.arg3++;
					break;

				case 'F':
				case 'R':
				case 'D':
					if (ZCMD.arg1 >= room_num)
						ZCMD.arg1++;
					break;

				case 'T':
					if (ZCMD.arg1 == WLD_TRIGGER && ZCMD.arg3 >= room_num)
						ZCMD.arg3++;
					break;

				case 'V':
					if (ZCMD.arg2 >= room_num)
						ZCMD.arg2++;
					break;
				}
			}
		}

		// * Update load rooms, to fix creeping load room problem.
		if (room_num <= r_mortal_start_room)
			r_mortal_start_room++;
		if (room_num <= r_immort_start_room)
			r_immort_start_room++;
		if (room_num <= r_frozen_start_room)
			r_frozen_start_room++;
		if (room_num <= r_helled_start_room)
			r_helled_start_room++;
		if (room_num <= r_named_start_room)
			r_named_start_room++;
		if (room_num <= r_unreg_start_room)
			r_unreg_start_room++;


		// ���� in_room ��� �������� � ���������� ��� ��������
		for (const auto& temp_ch : character_list)
		{
			room_rnum temp_room = temp_ch->get_was_in_room();
			if (temp_room >= room_num)
			{
				temp_ch->set_was_in_room(++temp_room);
			}
		}

		// �������, ������
		for (i = FIRST_ROOM; i < top_of_world + 1; i++)
		{
			if (world[i]->portal_room >= room_num)
				world[i]->portal_room++;
			for (j = 0; j < NUM_OF_DIRS; j++)
				if (W_EXIT(i, j))
					if (W_EXIT(i, j)->to_room >= room_num)
						W_EXIT(i, j)->to_room++;
		}

		// * Update any rooms being edited.
		for (dsc = descriptor_list; dsc; dsc = dsc->next)
			if (dsc->connected == CON_REDIT)
				for (j = 0; j < NUM_OF_DIRS; j++)
					if (OLC_ROOM(dsc)->dir_option[j])
						if (OLC_ROOM(dsc)->dir_option[j]->to_room >= room_num)
							OLC_ROOM(dsc)->dir_option[j]->to_room++;
	}

	check_room_flags(room_num);
	// ���� �� �� ������� ������� ����� ��� - ������� ����
	// � ��� � ������ �������� ���� ����� ��������� ��������� ��� ������ ����-�� � ����
	if (ROOM_FLAGGED(room_num, ROOM_SLOWDEATH) || ROOM_FLAGGED(room_num, ROOM_ICEDEATH))
		DeathTrap::add(world[room_num]);
	else
		DeathTrap::remove(world[room_num]);

	// ������� ����� �������� ��������
	SCRIPT(world[room_num])->cleanup();
	assign_triggers(world[room_num], WLD_TRIGGER);
	olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_ROOM);
}

//------------------------------------------------------------------------

void redit_save_to_disk(int zone_num)
{
	int counter, counter2, realcounter;
	FILE *fp;
	ROOM_DATA *room;

	if (zone_num < 0 || zone_num > top_of_zone_table)
	{
		log("SYSERR: redit_save_to_disk: Invalid real zone passed!");
		return;
	}

	sprintf(buf, "%s/%d.new", WLD_PREFIX, zone_table[zone_num].number);
	if (!(fp = fopen(buf, "w+")))
	{
		mudlog("SYSERR: OLC: Cannot open room file!", BRF, LVL_BUILDER, SYSLOG, TRUE);
		return;
	}
	for (counter = zone_table[zone_num].number * 100; counter < zone_table[zone_num].top; counter++)
	{
		if ((realcounter = real_room(counter)) != NOWHERE)
		{
			if (counter % 100 == 99)
				continue;
			room = world[realcounter];

#if defined(REDIT_LIST)
			sprintf(buf1, "OLC: Saving room %d.", room->number);
			log(buf1);
#endif

			// * Remove the '\r\n' sequences from description.
			strcpy(buf1, RoomDescription::show_desc(room->description_num).c_str());
			strip_string(buf1);

			// * Forget making a buffer, lets just write the thing now.
			*buf2 = '\0';
			room->flags_tascii(4, buf2);
			fprintf(fp, "#%d\n%s~\n%s~\n%d %s %d\n", counter,
					room->name ? room->name : "������������", buf1,
					zone_table[room->zone].number, buf2, room->sector_type);

			// * Handle exits.
			for (counter2 = 0; counter2 < NUM_OF_DIRS; counter2++)
			{
				if (room->dir_option[counter2])
				{
					// * Again, strip out the garbage.
					if (!room->dir_option[counter2]->general_description.empty())
					{
						const std::string& description = room->dir_option[counter2]->general_description;
						strcpy(buf1, description.c_str());
						strip_string(buf1);
					}
					else
					{
						*buf1 = 0;
					}

					// * Check for keywords.
					if (room->dir_option[counter2]->keyword)
					{
						strcpy(buf2, room->dir_option[counter2]->keyword);
					}

					// ����� � ����������� ������ ������� ���� �� ����� ;
					if (room->dir_option[counter2]->vkeyword)
					{
						strcpy(buf2 + strlen(buf2), "|");
						strcpy(buf2 + strlen(buf2), room->dir_option[counter2]->vkeyword);
					}
					else
						*buf2 = '\0';

					//����� ����������� ����� ������� �� �����-�� ������� ���������
					//�������� ��, ����� �� �������� ����������� ����
					byte old_exit_info = room->dir_option[counter2]->exit_info;

					REMOVE_BIT(room->dir_option[counter2]->exit_info, EX_CLOSED);
					REMOVE_BIT(room->dir_option[counter2]->exit_info, EX_LOCKED);
					REMOVE_BIT(room->dir_option[counter2]->exit_info, EX_BROKEN);
					// * Ok, now wrote output to file.
					fprintf(fp, "D%d\n%s~\n%s~\n%d %d %d %d\n",
						counter2, buf1, buf2,
						room->dir_option[counter2]->exit_info, room->dir_option[counter2]->key,
						room->dir_option[counter2]->to_room != NOWHERE ?
						world[room->dir_option[counter2]->to_room]->number : NOWHERE,
						room->dir_option[counter2]->lock_complexity);

					//����������� ����� ����������� � ������
					room->dir_option[counter2]->exit_info = old_exit_info;
				}
			}
			// * Home straight, just deal with extra descriptions.
			if (room->ex_description)
			{
				for (auto ex_desc = room->ex_description; ex_desc; ex_desc = ex_desc->next)
				{
					strcpy(buf1, ex_desc->description);
					strip_string(buf1);
					fprintf(fp, "E\n%s~\n%s~\n", ex_desc->keyword, buf1);
				}
			}
			fprintf(fp, "S\n");
			script_save_to_disk(fp, room, WLD_TRIGGER);
			im_inglist_save_to_disk(fp, room->ing_list);
		}
	}
	// * Write final line and close.
	fprintf(fp, "$\n$\n");
	fclose(fp);
	sprintf(buf2, "%s/%d.wld", WLD_PREFIX, zone_table[zone_num].number);
	// * We're fubar'd if we crash between the two lines below.
	remove(buf2);
	rename(buf, buf2);

	olc_remove_from_save_list(zone_table[zone_num].number, OLC_SAVE_ROOM);
}


// *************************************************************************
// * Menu functions                                                        *
// *************************************************************************

// * For extra descriptions.
void redit_disp_extradesc_menu(DESCRIPTOR_DATA * d)
{
	auto extra_desc = OLC_DESC(d);

	sprintf(buf,
#if defined(CLEAR_SCREEN)
		"[H[J"
#endif
		"%s1%s) ����: %s%s\r\n"
		"%s2%s) ��������:\r\n%s%s\r\n"
		"%s3%s) ��������� ��������: ",
		grn, nrm, yel,
		extra_desc->keyword ? extra_desc->keyword : "<NONE>", grn, nrm,
		yel, extra_desc->description ? extra_desc->description : "<NONE>", grn, nrm);

	strcat(buf, !extra_desc->next ? "<NOT SET>\r\n" : "Set.\r\n");
	strcat(buf, "Enter choice (0 to quit) : ");
	send_to_char(buf, d->character.get());
	OLC_MODE(d) = REDIT_EXTRADESC_MENU;
}

// * For exits.
void redit_disp_exit_menu(DESCRIPTOR_DATA * d)
{
	// * if exit doesn't exist, alloc/create it
	if (!OLC_EXIT(d))
	{
		OLC_EXIT(d).reset(new EXIT_DATA());
		OLC_EXIT(d)->to_room = NOWHERE;
	}

	// * Weird door handling!
	if (IS_SET(OLC_EXIT(d)->exit_info, EX_ISDOOR))
	{
		strcpy(buf2, "����� ");
		if (IS_SET(OLC_EXIT(d)->exit_info, EX_PICKPROOF))
			strcat(buf2, "�������������� ");
		sprintf(buf2+strlen(buf2), " (��������� ����� [%d])", OLC_EXIT(d)->lock_complexity);
	}
	else
	{
		strcpy(buf2, "��� �����");
	}

	if (IS_SET(OLC_EXIT(d)->exit_info, EX_HIDDEN))
	{
		strcat(buf2, " (����� �����)");
	}
	
	get_char_cols(d->character.get());
	sprintf(buf,
#if defined(CLEAR_SCREEN)
		"[H[J"
#endif
		"%s1%s) ����� �        : %s%d\r\n"
		"%s2%s) ��������       :-\r\n%s%s\r\n"
		"%s3%s) �������� ����� : %s%s (%s)\r\n"
		"%s4%s) ����� �����    : %s%d\r\n"
		"%s5%s) ����� �����    : %s%s\r\n"
		"%s6%s) �������� �����.\r\n"
		"��� ����� (0 - �����) : ",
		grn, nrm, cyn,
		OLC_EXIT(d)->to_room !=
		NOWHERE ? world[OLC_EXIT(d)->to_room]->number : NOWHERE, grn, nrm,
		yel,
		!OLC_EXIT(d)->general_description.empty() ? OLC_EXIT(d)->general_description.c_str() : "<NONE>",
		grn, nrm, yel,
		OLC_EXIT(d)->keyword ? OLC_EXIT(d)->keyword : "<NONE>",
		OLC_EXIT(d)->vkeyword ? OLC_EXIT(d)->vkeyword : "<NONE>", grn, nrm,
		cyn, OLC_EXIT(d)->key, grn, nrm, cyn, buf2, grn, nrm);

	send_to_char(buf, d->character.get());
	OLC_MODE(d) = REDIT_EXIT_MENU;
}

// * For exit flags.
void redit_disp_exit_flag_menu(DESCRIPTOR_DATA * d)
{
	get_char_cols(d->character.get());
	sprintf(buf,
		"��������! ��������� ����� ����� ����� ������ ������� � �������.\r\n"
		"�������� ��������� ����� �� ��������� ����� ������ ��������� ���� (zedit).\r\n\r\n"
		"%s1%s) [%c]�����\r\n"
		"%s2%s) [%c]��������������\r\n"
		"%s3%s) [%c]������� �����\r\n"
		"%s4%s) [%d]��������� �����\r\n"
		"��� ����� (0 - �����): ",
		grn, nrm, IS_SET(OLC_EXIT(d)->exit_info, EX_ISDOOR) ? 'x' : ' ',
		grn, nrm, IS_SET(OLC_EXIT(d)->exit_info, EX_PICKPROOF) ? 'x' : ' ',
		grn, nrm, IS_SET(OLC_EXIT(d)->exit_info, EX_HIDDEN) ? 'x' : ' ',
		grn, nrm, OLC_EXIT(d)->lock_complexity);
	send_to_char(buf, d->character.get());
}

// * For room flags.
void redit_disp_flag_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0, plane = 0;
	char c;

	get_char_cols(d->character.get());
#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0, c = 'a' - 1; plane < NUM_PLANES; counter++)
	{
		if (*room_bits[counter] == '\n')
		{
			plane++;
			c = 'a' - 1;
			continue;
		}
		else if (c == 'z')
			c = 'A';
		else
			c++;

		sprintf(buf, "%s%c%d%s) %-20.20s %s", grn, c, plane, nrm,
				room_bits[counter], !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character.get());
	}
	OLC_ROOM(d)->flags_sprint(buf1, ",", true);
	sprintf(buf, "\r\n����� �������: %s%s%s\r\n" "������� ���� ������� (0 - �����) : ", cyn, buf1, nrm);
	send_to_char(buf, d->character.get());
	OLC_MODE(d) = REDIT_FLAGS;
}

// * For sector type.
void redit_disp_sector_menu(DESCRIPTOR_DATA * d)
{
	int counter, columns = 0;

#if defined(CLEAR_SCREEN)
	send_to_char("[H[J", d->character);
#endif
	for (counter = 0; counter < NUM_ROOM_SECTORS; counter++)
	{
		sprintf(buf, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
				sector_types[counter], !(++columns % 2) ? "\r\n" : "");
		send_to_char(buf, d->character.get());
	}
	send_to_char("\r\n��� ����������� � ������� : ", d->character.get());
	OLC_MODE(d) = REDIT_SECTOR;
}

// * The main menu.
void redit_disp_menu(DESCRIPTOR_DATA * d)
{
	ROOM_DATA *room;

	get_char_cols(d->character.get());
	room = OLC_ROOM(d);

	room->flags_sprint(buf1, ",");
	sprinttype(room->sector_type, sector_types, buf2);
	sprintf(buf,
#if defined(CLEAR_SCREEN)
			"[H[J"
#endif
			"-- ������� : [%s%d%s]  	����: [%s%d%s]\r\n"
			"%s1%s) ��������    : &C&q%s&e&Q\r\n"
			"%s2&n) ��������    :\r\n%s&e"
			"%s3%s) �����       : %s%s\r\n"
			"%s4%s) ����������� : %s%s\r\n"
			"%s5%s) �� ������   : %s%d\r\n"
			"%s6%s) �� �������  : %s%d\r\n"
			"%s7%s) �� ���      : %s%d\r\n"
			"%s8%s) �� ������   : %s%d\r\n"
			"%s9%s) ������      : %s%d\r\n"
			"%sA%s) �����       : %s%d\r\n"
			"%sB%s) ���� ��������������\r\n"
			"%s�%s) ����������� : %s%s\r\n"
			"%sS%s) �������     : %s%s\r\n"
			"%sQ%s) Quit\r\n"
			"��� ����� : ",
			cyn, OLC_NUM(d), nrm,
			cyn, zone_table[OLC_ZNUM(d)].number, nrm,
			grn, nrm, room->name,
			grn, room->temp_description,
			grn, nrm, cyn, buf1, grn, nrm, cyn, buf2, grn, nrm, cyn, room->dir_option[NORTH]
			&& room->dir_option[NORTH]->to_room !=
			NOWHERE ? world[room->dir_option[NORTH]->to_room]->
			number : NOWHERE, grn, nrm, cyn, room->dir_option[EAST]
			&& room->dir_option[EAST]->to_room !=
			NOWHERE ? world[room->dir_option[EAST]->to_room]->
			number : NOWHERE, grn, nrm, cyn, room->dir_option[SOUTH]
			&& room->dir_option[SOUTH]->to_room !=
			NOWHERE ? world[room->dir_option[SOUTH]->to_room]->
			number : NOWHERE, grn, nrm, cyn, room->dir_option[WEST]
			&& room->dir_option[WEST]->to_room !=
			NOWHERE ? world[room->dir_option[WEST]->to_room]->number : NOWHERE, grn, nrm, cyn, room->dir_option[UP]
			&& room->dir_option[UP]->to_room !=
			NOWHERE ? world[room->dir_option[UP]->to_room]->number : NOWHERE, grn, nrm, cyn, room->dir_option[DOWN]
			&& room->dir_option[DOWN]->to_room !=
			NOWHERE ? world[room->dir_option[DOWN]->to_room]->
			number : NOWHERE, grn, nrm, grn, nrm, cyn,
			room->ing_list ? "����" : "���", grn, nrm, cyn, !room->proto_script->empty() ? "Set." : "Not Set.", grn, nrm);
	send_to_char(buf, d->character.get());

	OLC_MODE(d) = REDIT_MAIN_MENU;
}

// *************************************************************************
// *  The main loop                                                        *
// *************************************************************************

void redit_parse(DESCRIPTOR_DATA * d, char *arg)
{
	int number, plane, bit;

	switch (OLC_MODE(d))
	{
	case REDIT_CONFIRM_SAVESTRING:
		switch (*arg)
		{
		case 'y':
		case 'Y':
		case '�':
		case '�':
			redit_save_internally(d);
			sprintf(buf, "OLC: %s edits room %d.", GET_NAME(d->character), OLC_NUM(d));
			olc_log("%s edit room %d", GET_NAME(d->character), OLC_NUM(d));
			mudlog(buf, NRM, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), SYSLOG, TRUE);
			// * Do NOT free strings! Just the room structure.
			cleanup_olc(d, CLEANUP_STRUCTS);
			send_to_char("Room saved to memory.\r\n", d->character.get());
			break;

		case 'n':
		case 'N':
		case '�':
		case '�':
			// * Free everything up, including strings, etc.
			cleanup_olc(d, CLEANUP_ALL);
			break;
		default:
			send_to_char("�������� �����!\r\n�� ������� ��������� ������� � ������? : ", d->character.get());
			break;
		}
		return;

	case REDIT_MAIN_MENU:
		switch (*arg)
		{
		case 'q':
		case 'Q':
			if (OLC_VAL(d))  	// Something has been modified.
			{
				send_to_char("�� ������� ��������� ������� � ������? : ", d->character.get());
				OLC_MODE(d) = REDIT_CONFIRM_SAVESTRING;
			}
			else
			{
				cleanup_olc(d, CLEANUP_ALL);
			}
			return;

		case '1':
			send_to_char("������� �������� �������:-\r\n] ", d->character.get());
			OLC_MODE(d) = REDIT_NAME;
			break;

		case '2':
			OLC_MODE(d) = REDIT_DESC;
#if defined(CLEAR_SCREEN)
			SEND_TO_Q("\x1B[H\x1B[J", d);
#endif
			SEND_TO_Q("������� �������� �������: (/s �������� /h ������)\r\n\r\n", d);
			d->backstr = NULL;
			if (OLC_ROOM(d)->temp_description)
			{
				SEND_TO_Q(OLC_ROOM(d)->temp_description, d);
				d->backstr = str_dup(OLC_ROOM(d)->temp_description);
			}
			d->writer.reset(new DelegatedStringWriter(OLC_ROOM(d)->temp_description));
			d->max_str = MAX_ROOM_DESC;
			d->mail_to = 0;
			OLC_VAL(d) = 1;
			break;

		case '3':
			redit_disp_flag_menu(d);
			break;

		case '4':
			redit_disp_sector_menu(d);
			break;

		case '5':
			OLC_VAL(d) = NORTH;
			redit_disp_exit_menu(d);
			break;

		case '6':
			OLC_VAL(d) = EAST;
			redit_disp_exit_menu(d);
			break;

		case '7':
			OLC_VAL(d) = SOUTH;
			redit_disp_exit_menu(d);
			break;

		case '8':
			OLC_VAL(d) = WEST;
			redit_disp_exit_menu(d);
			break;

		case '9':
			OLC_VAL(d) = UP;
			redit_disp_exit_menu(d);
			break;

		case 'a':
		case 'A':
			OLC_VAL(d) = DOWN;
			redit_disp_exit_menu(d);
			break;

		case 'b':
		case 'B':
			// * If the extra description doesn't exist.
			if (!OLC_ROOM(d)->ex_description)
			{
				OLC_ROOM(d)->ex_description.reset(new EXTRA_DESCR_DATA());
			}
			OLC_DESC(d) = OLC_ROOM(d)->ex_description;
			redit_disp_extradesc_menu(d);
			break;

		case 'h':
		case 'H':
		case '�':
		case '�':
			OLC_MODE(d) = REDIT_ING;
			xedit_disp_ing(d, OLC_ROOM(d)->ing_list);
			return;

		case 's':
		case 'S':
			dg_olc_script_copy(d);
			OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
			dg_script_menu(d);
			return;
		default:
			send_to_char("�������� �����!", d->character.get());
			redit_disp_menu(d);
			break;
		}
		olc_log("%s command %c", GET_NAME(d->character), *arg);
		return;

	case OLC_SCRIPT_EDIT:
		if (dg_script_edit_parse(d, arg))
			return;
		break;

	case REDIT_NAME:
		if (OLC_ROOM(d)->name)
			free(OLC_ROOM(d)->name);
		if (strlen(arg) > MAX_ROOM_NAME)
			arg[MAX_ROOM_NAME - 1] = '\0';
		OLC_ROOM(d)->name = str_dup((arg && *arg) ? arg : "������������");
		break;

	case REDIT_DESC:
		// * We will NEVER get here, we hope.
		mudlog("SYSERR: Reached REDIT_DESC case in parse_redit", BRF, LVL_BUILDER, SYSLOG, TRUE);
		break;

	case REDIT_FLAGS:
		number = planebit(arg, &plane, &bit);
		if (number < 0)
		{
			send_to_char("�������� �����!\r\n", d->character.get());
			redit_disp_flag_menu(d);
		}
		else if (number == 0)
			break;
		else
		{
			// * Toggle the bit.
			OLC_ROOM(d)->toggle_flag(plane, 1 << bit);
			redit_disp_flag_menu(d);
		}
		return;

	case REDIT_SECTOR:
		number = atoi(arg);
		if (number < 0 || number >= NUM_ROOM_SECTORS)
		{
			send_to_char("�������� �����!", d->character.get());
			redit_disp_sector_menu(d);
			return;
		}
		else
			OLC_ROOM(d)->sector_type = number;
		break;

	case REDIT_EXIT_MENU:
		switch (*arg)
		{
		case '0':
			break;
		case '1':
			OLC_MODE(d) = REDIT_EXIT_NUMBER;
			send_to_char("������� � ������� N (vnum) : ", d->character.get());
			return;
		case '2':
			OLC_MODE(d) = REDIT_EXIT_DESCRIPTION;
			send_to_char("������� �������� ������ : ", d->character.get());
			return;

		case '3':
			OLC_MODE(d) = REDIT_EXIT_KEYWORD;
			send_to_char("������� �������� ����� : ", d->character.get());
			return;
		case '4':
			OLC_MODE(d) = REDIT_EXIT_KEY;
			send_to_char("������� ����� ����� : ", d->character.get());
			return;
		case '5':
			redit_disp_exit_flag_menu(d);
			OLC_MODE(d) = REDIT_EXIT_DOORFLAGS;
			return;
		case '6':
			// * Delete an exit.
			if (OLC_EXIT(d)->keyword)
				free(OLC_EXIT(d)->keyword);
			if (OLC_EXIT(d)->vkeyword)
				free(OLC_EXIT(d)->vkeyword);
			OLC_EXIT(d).reset();
			break;
		default:
			send_to_char("�������� �����!\r\n��� ����� : ", d->character.get());
			return;
		}
		break;

	case REDIT_EXIT_NUMBER:
		if ((number = atoi(arg)) != NOWHERE)
			if ((number = real_room(number)) == NOWHERE)
			{
				send_to_char("��� ����� ������� - ��������� ���� : ", d->character.get());
				return;
			}
		OLC_EXIT(d)->to_room = number;
		redit_disp_exit_menu(d);
		return;

	case REDIT_EXIT_DESCRIPTION:
		OLC_EXIT(d)->general_description = arg ? arg : "";
		redit_disp_exit_menu(d);
		return;

	case REDIT_EXIT_KEYWORD:
		if (OLC_EXIT(d)->keyword)
			free(OLC_EXIT(d)->keyword);
		if (OLC_EXIT(d)->vkeyword)
			free(OLC_EXIT(d)->vkeyword);

		if (arg && *arg)
		{
			std::string buffer(arg);
			std::string::size_type i = buffer.find('|');
			if (i != std::string::npos)
			{
				OLC_EXIT(d)->keyword = str_dup(buffer.substr(0, i).c_str());
				OLC_EXIT(d)->vkeyword = str_dup(buffer.substr(++i).c_str());
			}
			else
			{
				OLC_EXIT(d)->keyword = str_dup(buffer.c_str());
				OLC_EXIT(d)->vkeyword = str_dup(buffer.c_str());
			}
		}
		else
		{
			OLC_EXIT(d)->keyword = NULL;
			OLC_EXIT(d)->vkeyword = NULL;
		}
		redit_disp_exit_menu(d);
		return;

	case REDIT_EXIT_KEY:
		OLC_EXIT(d)->key = atoi(arg);
		redit_disp_exit_menu(d);
		return;

	case REDIT_EXIT_DOORFLAGS:
		number = atoi(arg);
		if ((number < 0) || (number > 6))
		{
			send_to_char("�������� �����!\r\n", d->character.get());
			redit_disp_exit_flag_menu(d);
		}
		else if (number == 0)
			redit_disp_exit_menu(d);
		else
		{
			if (number == 1)
			{
				if (IS_SET(OLC_EXIT(d)->exit_info, EX_ISDOOR))
				{
					OLC_EXIT(d)->exit_info = 0;
					OLC_EXIT(d)->lock_complexity = 0;
				}
				else
					SET_BIT(OLC_EXIT(d)->exit_info, EX_ISDOOR);
			}
			else if (number == 2)
			{
				TOGGLE_BIT(OLC_EXIT(d)->exit_info, EX_PICKPROOF);
			}
			else if (number == 3)
			{
				TOGGLE_BIT(OLC_EXIT(d)->exit_info, EX_HIDDEN);
			}
			else if (number == 4)
			{
				OLC_MODE(d) = REDIT_LOCK_COMPLEXITY;
				send_to_char("������� ��������� �����, (0-255): ", d->character.get());
				return;
			}
			redit_disp_exit_flag_menu(d);
		}
		return;
	case REDIT_LOCK_COMPLEXITY:
		OLC_EXIT(d)->lock_complexity = ((arg && *arg) ? atoi(arg) : 0);
		OLC_MODE(d) = REDIT_EXIT_DOORFLAGS;
		redit_disp_exit_flag_menu(d);
		return;
	case REDIT_EXTRADESC_KEY:
		OLC_DESC(d)->keyword = ((arg && *arg) ? str_dup(arg) : NULL);
		redit_disp_extradesc_menu(d);
		return;

	case REDIT_EXTRADESC_MENU:
		switch ((number = atoi(arg)))
		{
		case 0:
			// * If something got left out, delete the extra description
			// * when backing out to the menu.
			if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description)
			{
				auto& desc = OLC_DESC(d);
				desc.reset();
			}
			break;

		case 1:
			OLC_MODE(d) = REDIT_EXTRADESC_KEY;
			send_to_char("������� �������� �����, ����������� ��������� : ", d->character.get());
			return;

		case 2:
			OLC_MODE(d) = REDIT_EXTRADESC_DESCRIPTION;
			SEND_TO_Q("������� ��������������: (/s ��������� /h ������)\r\n\r\n", d);
			d->backstr = NULL;
			if (OLC_DESC(d)->description)
			{
				SEND_TO_Q(OLC_DESC(d)->description, d);
				d->backstr = str_dup(OLC_DESC(d)->description);
			}
			d->writer.reset(new DelegatedStringWriter(OLC_DESC(d)->description));
			d->max_str = 4096;
			d->mail_to = 0;
			return;

		case 3:
			if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description)
			{
				send_to_char("�� �� ������ ������������� ��������� ��������������, �� �������� �������.\r\n",
					d->character.get());
				redit_disp_extradesc_menu(d);
			}
			else
			{
				if (OLC_DESC(d)->next)
				{
					OLC_DESC(d) = OLC_DESC(d)->next;
				}
				else
				{
					// * Make new extra description and attach at end.
					EXTRA_DESCR_DATA::shared_ptr new_extra(new EXTRA_DESCR_DATA());
					OLC_DESC(d)->next = new_extra;
					OLC_DESC(d) = new_extra;
				}
				redit_disp_extradesc_menu(d);
			}
			return;
		}
		break;

	case REDIT_ING:
		if (!xparse_ing(d, &OLC_ROOM(d)->ing_list, arg))
		{
			redit_disp_menu(d);
			return;
		}
		OLC_VAL(d) = 1;
		xedit_disp_ing(d, OLC_ROOM(d)->ing_list);
		return;

	default:
		// * We should never get here.
		mudlog("SYSERR: Reached default case in parse_redit", BRF, LVL_BUILDER, SYSLOG, TRUE);
		break;
	}
	// * If we get this far, something has been changed.
	OLC_VAL(d) = 1;
	redit_disp_menu(d);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
