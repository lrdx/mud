// Copyright (c) 2005 Krodo
// Part of Bylins http://www.mud.ru

#include "boards.h"

#include "logger.hpp"
#include "boards.types.hpp"
#include "boards.changelog.loaders.hpp"
#include "boards.constants.hpp"
#include "boards.formatters.hpp"
#include "obj.hpp"
#include "house.h"
#include "screen.h"
#include "comm.h"
#include "privilege.hpp"
#include "char.hpp"
#include "modify.h"
#include "room.hpp"
#include "utils.h"
#include "conf.h"
#include "interpreter.h"

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <fstream>
#include <sstream>
#include <iomanip>

namespace Boards 
{
	// ����� ������ �����
	using boards_list_t = std::deque<Board::shared_ptr>;
	boards_list_t board_list;

	std::string dg_script_text;

	void set_last_read(CHAR_DATA *ch, BoardTypes type, time_t date)
	{
		if (ch->get_board_date(type) < date)
		{
			ch->set_board_date(type, date);
		}
	}

	bool lvl_no_write(CHAR_DATA *ch)
	{
		if (GET_LEVEL(ch) < MIN_WRITE_LEVEL && GET_REMORT(ch) <= 0)
		{
			return true;
		}
		return false;
	}

	void message_no_write(CHAR_DATA *ch)
	{
		if (lvl_no_write(ch))
		{
			send_to_char(ch,
				"��� ����� ���������� %d ������, ����� ������ � ���� ������.\r\n",
				MIN_WRITE_LEVEL);
		}
		else
		{
			send_to_char("� ��� ��� ����������� ������ � ���� ������.\r\n", ch);
		}
	}

	void message_no_read(CHAR_DATA *ch, const Board &board)
	{
		std::string out("� ��� ��� ����������� ������ ���� ������.\r\n");
		if (board.is_special())
		{
			std::string name = board.get_name();
			lower_convert(name);
			out += "��� ��������� � ������� ������� ������ � ������ �����������: " + name + " ������ <���������>.\r\n";
			if (!board.get_alias().empty())
			{
				out += "������� ��� �������� ���������� ���������: "
					+ board.get_alias()
					+ " <����� ��������� � ���� ������>.\r\n";
			}
		}
		send_to_char(out, ch);
	}

	void add_server_message(const std::string& subj, const std::string& text)
	{
		const auto board_it = std::find_if(
			Boards::board_list.begin(),
			Boards::board_list.end(),
			[](const Board::shared_ptr p)
		{
			return p->get_type() == Boards::GODBUILD_BOARD;
		});

		if (Boards::board_list.end() == board_it)
		{
			log("SYSERROR: can't find builder board (%s:%d)", __FILE__, __LINE__);
			return;
		}

		const auto temp_message = std::make_shared<Message>();
		temp_message->author = "������";
		temp_message->subject = subj;
		temp_message->text = text;
		temp_message->date = time(0);
		// ����� ��� ������� �� �������
		temp_message->unique = 1;
		temp_message->level = 1;

		(*board_it)->add_message(temp_message);
	}

	void dg_script_message()
	{
		if (!dg_script_text.empty())
		{
			const std::string subj = "������� � ��������";
			add_server_message(subj, dg_script_text);
			dg_script_text.clear();
		}
	}

	void changelog_message()
	{
		std::ifstream file(runtime_config.changelog_file_name());
		if (!file.is_open())
		{
			log("SYSERROR: can't open changelog file (%s:%d)", __FILE__, __LINE__);
			return;
		}

		const auto coder_board = std::find_if(
			Boards::board_list.begin(),
			Boards::board_list.end(),
			[](const Board::shared_ptr p)
		{
			return p->get_type() == Boards::CODER_BOARD;
		});

		if (Boards::board_list.end() == coder_board)
		{
			log("SYSERROR: can't find coder board (%s:%d)", __FILE__, __LINE__);
			return;
		}

		const auto loader = ChangeLogLoader::create(runtime_config.changelog_format(), *coder_board);
		loader->load(file);
	}

	bool is_spamer(CHAR_DATA *ch, const Board& board)
	{
		if (IS_IMMORTAL(ch) || Privilege::check_flag(ch, Privilege::BOARDS))
		{
			return false;
		}
		if (board.get_lastwrite() != GET_UNIQUE(ch))
		{
			return false;
		}
		switch (board.get_type())
		{
		case CLAN_BOARD:
		case CLANNEWS_BOARD:
		case PERS_BOARD:
		case ERROR_BOARD:
		case MISPRINT_BOARD:
		case SUGGEST_BOARD:
			return false;
		default:
			break;
		}
		return true;
	}

	void DoBoard(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
	{
		if (!ch->desc)
		{
			return;
		}

		if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLIND))
		{
			send_to_char("�� ���������!\r\n", ch);
			return;
		}
		
		Board::shared_ptr board_ptr;
		for (const auto& board_i : board_list)
		{
			if (board_i->get_type() == subcmd
				&& Static::can_see(ch, board_i))
			{
				board_ptr = board_i;
				break;
			}
		}

		if (!board_ptr)
		{
			send_to_char("����?\r\n", ch);
			return;
		}
		const Board& board = *board_ptr;

		std::string buffer, buffer2 = argument;
		GetOneParam(buffer2, buffer);
		boost::trim_if(buffer2, boost::is_any_of(std::string(" \'")));
		// ������ �������� � buffer, ������ � buffer2

		if (CompareParam(buffer, "������") || CompareParam(buffer, "list"))
		{
			Static::do_list(ch, board_ptr);
		}
		else if (buffer.empty()
			|| (buffer2.empty()
				&& (CompareParam(buffer, "������")
					|| CompareParam(buffer, "read"))))
		{
			// ��� ������ ������� ��� '������' ��� ����� - ���������� ������
			// ������������� ���������, ���� ����� ����
			if (!Static::can_read(ch, board_ptr))
			{
				message_no_read(ch, board);
				return;
			}
			time_t const date = ch->get_board_date(board.get_type());

			// ������� ���� � ��������� ��������
			if ((board.get_type() == NEWS_BOARD
				|| board.get_type() == GODNEWS_BOARD
				|| board.get_type() == CODER_BOARD)
				&& !PRF_FLAGGED(ch, PRF_NEWS_MODE))
			{
				std::ostringstream body;
				const auto formatter = FormattersBuilder::create(board.get_type(), body, ch, date);
				board.format_board(formatter);
				set_last_read(ch, board.get_type(), board.last_message_date());
				page_string(ch->desc, body.str());
				return;
			}
			// ��������� � ��������� ��������
			if (board.get_type() == CLANNEWS_BOARD && !PRF_FLAGGED(ch, PRF_NEWS_MODE))
			{
				std::ostringstream body;
				const auto formatter = FormattersBuilder::create(board.get_type(), body, ch, date);
				board.format_board(formatter);
				set_last_read(ch, board.get_type(), board.last_message_date());
				page_string(ch->desc, body.str());
				return;
			}

			std::ostringstream body;
			time_t last_date = date;
			const auto formatter = FormattersBuilder::single_message(body, ch, date, board.get_type(), last_date);
			board.format_board_in_reverse(formatter);
			set_last_read(ch, board.get_type(), last_date);
			page_string(ch->desc, body.str());
			if (last_date == date)
			{
				send_to_char("� ��� ��� ������������� ���������.\r\n", ch);
			}
		}
		else if (is_number(buffer.c_str())
			|| ((CompareParam(buffer, "������") || CompareParam(buffer, "read"))
				&& is_number(buffer2.c_str())))
		{
			// ���� ����� ������� ����� ����� ��� '������ �����' - ��������
			// �������� ��� �������, ��� ��������� - ����� �� ��������� '������ �����...' ��� ������
			size_t num = 0;
			if (!Static::can_read(ch, board_ptr))
			{
				message_no_read(ch, board);
				return;
			}
			if (CompareParam(buffer, "������"))
			{
				num = atoi(buffer2.c_str());
			}
			else
			{
				num = atoi(buffer.c_str());
			}

			if (num <= 0
				|| num > board.messages.size())
			{
				send_to_char("��� ��������� ����� ��� ������ ����������.\r\n", ch);
				return;
			}
			std::ostringstream out;
			const auto messages_index = num - 1;
			special_message_format(out, board.get_message(messages_index));
			page_string(ch->desc, out.str());
			set_last_read(ch, board.get_type(), board.messages[messages_index]->date);
		}
		else if (CompareParam(buffer, "������")
			|| CompareParam(buffer, "write"))
		{
			if (!Static::can_write(ch, board_ptr))
			{
				message_no_write(ch);
				return;
			}
			if (is_spamer(ch, board))
			{
				send_to_char("�� �� ���� ������ ��� ������ ����.\r\n", ch);
				return;
			}

			if (subcmd == Boards::CLAN_BOARD)
			{
				if (!CLAN(ch)->check_write_board(ch))
				{
					send_to_char("��� ��������� ���� ������!\r\n", ch);
					return;
				}
			}

			if (board.is_special() && board.messages.size() >= MAX_REPORT_MESSAGES)
			{
				send_to_char(constants::OVERFLOW_MESSAGE, ch);
				return;
			}
			/// ��������� �������� �� ������� �����
			std::string name = GET_NAME(ch);
			if (PRF_FLAGGED(ch, PRF_CODERINFO)
				&& (board.get_type() == NEWS_BOARD
					|| board.get_type() == NOTICE_BOARD))
			{
				GetOneParam(buffer2, buffer);
				if (buffer.empty())
				{
					send_to_char("������� ���� �� ���, �� ���� ����� ������������ ���������.\r\n", ch);
					return;
				}
				name = buffer;
			}
			/// ������� �������
			const auto tempMessage = std::make_shared<Message>();
			tempMessage->author = name;
			tempMessage->unique = GET_UNIQUE(ch);
			// ��� ����� ����� �������� � ������������ ����� ����� ������ (��� ��������� ������� ���-��)
			PRF_FLAGGED(ch, PRF_CODERINFO) ? tempMessage->level = LVL_IMPL : tempMessage->level = GET_LEVEL(ch);

			// �������� ��� ����
			if (CLAN(ch))
			{
				tempMessage->rank = CLAN_MEMBER(ch)->rank_num;
			}
			else
			{
				tempMessage->rank = 0;
			}

			// ��������� �����
			boost::trim(buffer2);
			if (buffer2.length() > 40)
			{
				buffer2.erase(40, std::string::npos);
				send_to_char(ch, "���� ��������� ��������� �� '%s'.\r\n", buffer2.c_str());
			}
			// ��� ������ �������� ������� ����� �������, ��� ����� �����������
			std::string subj;
			if (subcmd == ERROR_BOARD || subcmd == MISPRINT_BOARD)
			{
				subj += "[" + std::to_string(GET_ROOM_VNUM(ch->in_room)) + "] ";
			}
			subj += buffer2;
			tempMessage->subject = subj;
			// ���� ����� � ����� ����� ��� �� ����� ����������
			ch->desc->message = tempMessage;
			ch->desc->board = board_ptr;

			send_to_char(ch, "������ ������ ���������.  (/s �������� /h ������)\r\n");
			STATE(ch->desc) = CON_WRITEBOARD;
			const AbstractStringWriter::shared_ptr writer(new StdStringWriter());
			string_write(ch->desc, writer, MAX_MESSAGE_LENGTH, 0, NULL);
		}
		else if (CompareParam(buffer, "��������") || CompareParam(buffer, "remove"))
		{
			if (!is_number(buffer2.c_str()))
			{
				send_to_char("������� ���������� ����� ���������.\r\n", ch);
				return;
			}
			const size_t message_number = atoi(buffer2.c_str());
			const auto messages_index = message_number - 1;
			if (messages_index >= board.messages.size())
			{
				send_to_char("��� ��������� ����� ��� ������ ����������.\r\n", ch);
				return;
			}
			set_last_read(ch, board.get_type(), board.messages[messages_index]->date);
			// ��� �� ����� �������� ����� ������� (�� ������/�����), ��� ������ ����
			if (!Static::full_access(ch, board_ptr))
			{
				if (board.messages[messages_index]->unique != GET_UNIQUE(ch))
				{
					send_to_char("� ��� ��� ����������� ������� ��� ���������.\r\n", ch);
					return;
				}
			}
			else if (board.get_type() != CLAN_BOARD
				&& board.get_type() != CLANNEWS_BOARD
				&& board.get_type() != PERS_BOARD
				&& !PRF_FLAGGED(ch, PRF_CODERINFO)
				&& GET_LEVEL(ch) < board.messages[messages_index]->level)
			{
				// ��� ������� ����� ������� ������ (��� �������� �����)
				// �������� ����, � ������������ ������ ���
				send_to_char("� ��� ��� ����������� ������� ��� ���������.\r\n", ch);
				return;
			}
			else if (board.get_type() == CLAN_BOARD
				|| board.get_type() == CLANNEWS_BOARD)
			{
				// � ���� ���������� �� �������, �� ����� ������� ����� �����, ���� ���� ������ ����� �� ��� ����
				if (CLAN_MEMBER(ch)->rank_num > board.messages[messages_index]->rank)
				{
					send_to_char("� ��� ��� ����������� ������� ��� ���������.\r\n", ch);
					return;
				}
			}
			// ���������� ������� � ����������� ����� �������
			board_ptr->erase_message(messages_index);
			if (board.get_lastwrite() == GET_UNIQUE(ch))
			{
				board_ptr->set_lastwrite_uid(0);
			}
			board_ptr->Save();
			send_to_char("��������� �������.\r\n", ch);
		}
		else
		{
			send_to_char("�������� ������ �������, ������������ �� '������� �����'.\r\n", ch);
		}
	}

	bool act_board(CHAR_DATA *ch, int vnum, char *buf_)
	{
		switch (vnum)
		{
		case GENERAL_BOARD_OBJ:
			DoBoard(ch, buf_, 0, GENERAL_BOARD);
			break;
		case GODGENERAL_BOARD_OBJ:
			DoBoard(ch, buf_, 0, GODGENERAL_BOARD);
			break;
		case GODPUNISH_BOARD_OBJ:
			DoBoard(ch, buf_, 0, GODPUNISH_BOARD);
			break;
		case GODBUILD_BOARD_OBJ:
			DoBoard(ch, buf_, 0, GODBUILD_BOARD);
			break;
		case GODCODE_BOARD_OBJ:
			DoBoard(ch, buf_, 0, CODER_BOARD);
			break;
		default:
			return false;
		}

		return true;
	}

	std::string print_access(const std::bitset<Boards::ACCESS_NUM> &acess_flags)
	{
		std::string access;
		if (acess_flags.test(ACCESS_FULL))
		{
			access = "������+";
		}
		else if (acess_flags.test(ACCESS_CAN_READ)
			&& acess_flags.test(ACCESS_CAN_WRITE))
		{
			access = "������";
		}
		else if (acess_flags.test(ACCESS_CAN_READ))
		{
			access = "������";
		}
		else if (acess_flags.test(ACCESS_CAN_WRITE))
		{
			access = "������";
		}
		else if (acess_flags.test(ACCESS_CAN_SEE))
		{
			access = "���";
		}
		else
		{
			access = "";
		}
		return access;
	}
	// ����� �� ������������ ����� �������� ������ �� ������ ����� � ����� ����������
	int Static::Special(CHAR_DATA* ch, void* me, int cmd, char* argument)
	{
		OBJ_DATA *board = (OBJ_DATA *)me;
		if (!ch->desc)
		{
			return 0;
		}
		// ������ ���������
		std::string buffer = argument, buffer2;
		GetOneParam(buffer, buffer2);
		boost::trim_if(buffer, boost::is_any_of(std::string(" \'")));

		if ((CMD_IS("��������") || CMD_IS("���������") || CMD_IS("look") || CMD_IS("examine")
			|| CMD_IS("������") || CMD_IS("read")) && (CompareParam(buffer2, "�����") || CompareParam(buffer2, "board")))
		{
			// ��� ������ ������ ��� ������ '�� �� �����' � �������� ��������� � ��� �����
			if (buffer2.empty()
				|| (buffer.empty() && !CompareParam(buffer2, "�����") && !CompareParam(buffer2, "board"))
				|| (!buffer.empty() && !CompareParam(buffer, "�����") && !CompareParam(buffer2, "board")))
			{
				return 0;
			}

			char buf_[MAX_INPUT_LENGTH];
			snprintf(buf_, sizeof(buf_), "%s", "������");

			if (act_board(ch, GET_OBJ_VNUM(board), buf_))
			{
				return 1;
			}
			else
			{
				return 0;
			}
		}

		// ��� �����
		if ((CMD_IS("������") || CMD_IS("read")) && !buffer2.empty() && a_isdigit(buffer2[0]))
		{
			if (buffer2.find('.') != std::string::npos)
			{
				return 0;
			}
		}

		// ����� ��������� �������� ��������� �������� ���������
		if (((CMD_IS("������") || CMD_IS("read")) && !buffer2.empty() && a_isdigit(buffer2[0]))
			|| CMD_IS("������") || CMD_IS("write") || CMD_IS("��������") || CMD_IS("remove"))
		{
			// ������������� ������� ���� '������ ���' ���� �� ����� � ������ �� ����
			if ((CMD_IS("������") || CMD_IS("write")) && !buffer2.empty())
			{
				for (auto i = board_list.begin(); i != board_list.end(); ++i)
				{
					if (isname(buffer2, (*i)->get_name()))
					{
						send_to_char(ch,
							"������ ����� ������ ��������� ��������� � ��������� ����� �� ����� ���������,\r\n"
							"�� ��������� ������������� �������������� �������� '<���-�����> ������ <���������>'.\r\n");
						return 1;
					}
				}
			}
			// ����� �����
			char buf_[MAX_INPUT_LENGTH];
			snprintf(buf_, sizeof(buf_), "%s%s", cmd_info[cmd].command, argument);

			if (act_board(ch, GET_OBJ_VNUM(board), buf_))
			{
				return 1;
			}
			else
			{
				return 0;
			}
		}

		return 0;
	}

	// ������� ��� ������ � ���� ���� � ����� ���������� �� ������
	void Static::LoginInfo(CHAR_DATA* ch)
	{
		std::ostringstream buffer, news;
		bool has_message = false;
		buffer << "\r\n��� ������� ���������:\r\n";

		for (auto board = board_list.begin(); board != board_list.end(); ++board)
		{
			// ����� �� ����� ��� ����� ������ ������, �������� ��� �� ������
			if (!can_read(ch, *board)
				|| ((*board)->get_type() == MISPRINT_BOARD
					&& !PRF_FLAGGED(ch, PRF_MISPRINT))
				|| (*board)->get_type() == CODER_BOARD)
			{
				continue;
			}
			const int unread = (*board)->count_unread(ch->get_board_date((*board)->get_type()));
			if (unread > 0)
			{
				has_message = true;
				if ((*board)->get_type() == NEWS_BOARD
					|| (*board)->get_type() == GODNEWS_BOARD
					|| (*board)->get_type() == CLANNEWS_BOARD)
				{
					news << std::setw(4) << unread << " � ������� '" << (*board)->get_description() << "' "
						<< CCWHT(ch, C_NRM) << "(" << (*board)->get_name() << ")" << CCNRM(ch, C_NRM) << ".\r\n";
				}
				else
				{
					buffer << std::setw(4) << unread << " � ������� '" << (*board)->get_description() << "' "
						<< CCWHT(ch, C_NRM) << "(" << (*board)->get_name() << ")" << CCNRM(ch, C_NRM) << ".\r\n";
				}
			}
		}

		if (has_message)
		{
			buffer << news.str();
			send_to_char(buffer.str(), ch);
		}
	}

	Boards::Board::shared_ptr Static::create_board(BoardTypes type, const std::string &name, const std::string &desc, const std::string &file)
	{
		const auto board = std::make_shared<Board>(type);
		board->set_name(name);
		board->set_description(desc);
		board->set_file_name(file);
		board->Load();

		if (board->get_type() == ERROR_BOARD)
		{
			board->set_alias("������");
		}
		else if (board->get_type() == MISPRINT_BOARD)
		{
			board->set_alias("��������");
		}
		else if (board->get_type() == SUGGEST_BOARD)
		{
			board->set_alias("�����");
		}

		switch (board->get_type())
		{
		case ERROR_BOARD:
		case MISPRINT_BOARD:
		case SUGGEST_BOARD:
		case CODER_BOARD:
			board->set_blind(true);
			break;

		default:
			break;
		}

		board_list.push_back(board);
		return board;
	}

	void Static::do_list(CHAR_DATA* ch, const Board::shared_ptr board_ptr)
	{
		if (!can_read(ch, board_ptr))
		{
			const auto& board = *board_ptr;
			message_no_read(ch, board);
			return;
		}

		std::ostringstream body;
		body << "��� �����, �� ������� ������ ������ �������� �������� ���� IMHO.\r\n"
			<< "������: ������/�������� <����� ���������>, ������ <���� ���������>.\r\n";
		if (board_ptr->empty())
		{
			body << "����� ���� �� ���������, ����� �����.\r\n";
			send_to_char(body.str(), ch);
			return;
		}
		else
		{
			body << "����� ���������: " << board_ptr->messages_count() << "\r\n";
		}

		const auto date = ch->get_board_date(board_ptr->get_type());
		const auto formatter = FormattersBuilder::messages_list(body, ch, date);
		board_ptr->format_board(formatter);
		page_string(ch->desc, body.str());
	}

	bool Static::can_see(CHAR_DATA *ch, const Board::shared_ptr board)
	{
		auto access_ = get_access(ch, board);
		return access_.test(ACCESS_CAN_SEE);
	}

	bool Static::can_read(CHAR_DATA *ch, const Board::shared_ptr board)
	{
		auto access_ = get_access(ch, board);
		return access_.test(ACCESS_CAN_READ);
	}

	bool Static::can_write(CHAR_DATA *ch, const Board::shared_ptr board)
	{
		auto access_ = get_access(ch, board);
		return access_.test(ACCESS_CAN_WRITE);
	}

	bool Static::full_access(CHAR_DATA *ch, const Board::shared_ptr board)
	{
		auto access_ = get_access(ch, board);
		return access_.test(ACCESS_FULL);
	}

	void Static::clan_delete_message(const std::string &name, int vnum)
	{
		const std::string subj = "���������� �������";
		const std::string text = boost::str(boost::format(
			"������� %1% ���� ������������� �������.\r\n"
			"����� ����: %2%\r\n") % name % vnum);
		add_server_message(subj, text);
	}

	void Static::new_message_notify(const Board::shared_ptr board)
	{
		if (board->get_type() != PERS_BOARD
			&& board->get_type() != CODER_BOARD
			&& !board->empty())
		{
			const Message &msg = *board->get_last_message();
			char buf_[MAX_INPUT_LENGTH];
			snprintf(buf_, sizeof(buf_),
				"����� ��������� � ������� '%s' �� %s, ����: %s\r\n",
				board->get_name().c_str(), msg.author.c_str(),
				msg.subject.c_str());
			// ��������� ���� ��� ��� � ������� ������
			for (DESCRIPTOR_DATA *f = descriptor_list; f; f = f->next)
			{
				if (f->character
					&& STATE(f) == CON_PLAYING
					&& PRF_FLAGGED(f->character, PRF_BOARD_MODE)
					&& can_read(f->character.get(), board)
					&& (board->get_type() != MISPRINT_BOARD
						|| PRF_FLAGGED(f->character, PRF_MISPRINT)))
				{
					send_to_char(buf_, f->character.get());
				}
			}
		}
	}

	// �������� ���� �����, ����� �������� � ������������
	void Static::BoardInit()
	{
		board_list.clear();

		create_board(GENERAL_BOARD, "����", "�������� �������", ETC_BOARD"general.board");
		create_board(NEWS_BOARD, "�������", "������ � ������� �����", ETC_BOARD"news.board");
		create_board(IDEA_BOARD, "����", "���� � �� ����������", ETC_BOARD"idea.board");
		create_board(ERROR_BOARD, "����", "��������� �� ������� � ����", ETC_BOARD"error.board");
		create_board(GODNEWS_BOARD, "GodNews", "������������ �������", ETC_BOARD"god-news.board");
		create_board(GODGENERAL_BOARD, "��������", "������������ �������� �������", ETC_BOARD"god-general.board");
		create_board(GODBUILD_BOARD, "������", "������� ��������", ETC_BOARD"god-build.board");
		create_board(GODPUNISH_BOARD, "���������", "����������� � ����������", ETC_BOARD"god-punish.board");
		create_board(NOTICE_BOARD, "������", "��������� �� �������������", ETC_BOARD"notice.board");
		create_board(MISPRINT_BOARD, "��������", "�������� � ������� ��������", ETC_BOARD"misprint.board");
		create_board(SUGGEST_BOARD, "��������", "��� ���� � ��������� ������", ETC_BOARD"suggest.board");
		create_board(CODER_BOARD, "�����", "��������� � ���� �����", "");

		dg_script_message();
		changelog_message();
	}

	// ����/������ �������� �����
	void Static::ClanInit()
	{
		const auto erase_predicate = [](const auto& board)
		{
			return board->get_type() == CLAN_BOARD
				|| board->get_type() == CLANNEWS_BOARD;
		};
		// ������ ����-����� ��� �������
		board_list.erase(
			std::remove_if(board_list.begin(), board_list.end(), erase_predicate),
			board_list.end());

		for (const auto& clan : Clan::ClanList)
		{
			std::string name = clan->GetAbbrev();
			CreateFileName(name);

			{
				// ������ �������� �����
				const auto board = std::make_shared<Board>(CLAN_BOARD);
				board->set_name("������");
				std::string description = "�������� ������ ������� ";
				description += clan->GetAbbrev();
				board->set_description(description);
				board->set_clan_rent(clan->GetRent());
				board->set_file_name(LIB_HOUSE + name + "/" + name + ".board");
				board->Load();
				board_list.push_back(board);
			}

			{
				// ������ �������� �������
				const auto board = std::make_shared<Board>(CLANNEWS_BOARD);
				board->set_name("���������");
				std::string description = "������� ������� ";
				description += clan->GetAbbrev();
				board->set_description(description);
				board->set_clan_rent(clan->GetRent());
				board->set_file_name(LIB_HOUSE + name + "/" + name + "-news.board");
				board->Load();
				board_list.push_back(board);
			}
		}
	}

	// ������ ��� �������
	void Static::clear_god_boards()
	{
		const auto erase_predicate = [](const auto& board) { return board->get_type() == PERS_BOARD; };
		board_list.erase(
			std::remove_if(board_list.begin(), board_list.end(), erase_predicate),
			board_list.end());
	}

	// ������� ������� ����
	void Static::init_god_board(long uid, const std::string& name)
	{
		const auto board = std::make_shared<Board>(PERS_BOARD);
		board->set_name("�������");
		board->set_description("��� ������ ��� �������");
		board->set_pers_unique(uid);
		board->set_pers_name(name);
		// ������� ��� � ������ �� �����������
		std::string tmp_name = name;
		CreateFileName(tmp_name);
		board->set_file_name(ETC_BOARD + tmp_name + ".board");
		board->Load();
		board_list.push_back(board);
	}

	// * ������ ���� ����� �����.
	void Static::reload_all()
	{
		BoardInit();
		Privilege::load_god_boards();
		ClanInit();
	}

std::string Static::print_stats(CHAR_DATA* ch, const Board::shared_ptr board, int num)
{
	const std::string access = print_access(get_access(ch, board));
	if (access.empty())
	{
		return "";
	}

	std::string out;
	if (IS_IMMORTAL(ch)
		|| PRF_FLAGGED(ch, PRF_CODERINFO)
		|| !board->get_blind())
	{
		const int unread = board->count_unread(ch->get_board_date(board->get_type()));
		out += boost::str(boost::format
			(" %2d)  %10s   [%3d|%3d]   %40s  %6s\r\n")
			% num % board->get_name() % unread % board->messages_count()
			% board->get_description() % access);
	}
	else
	{
		std::string tmp = board->get_description();
		if (!board->get_alias().empty())
		{
			tmp += " (" + board->get_alias() + ")";
		}
		out += boost::str(boost::format(" %2d)  %10s   [ - | - ]   %40s  %6s\r\n")
			% num % board->get_name() % tmp % access);
	}
	return out;
}

std::bitset<ACCESS_NUM> Static::get_access(CHAR_DATA *ch, const Board::shared_ptr board)
{
	std::bitset<ACCESS_NUM> access;

	switch (board->get_type())
	{
	case GENERAL_BOARD:
	case IDEA_BOARD:
		// ��� ������, ����� � ���.������, 32 � �� ���������� ������
		if (IS_GOD(ch) || Privilege::check_flag(ch, Privilege::BOARDS))
		{
			access.set();
		}
		else
		{
			access.set(ACCESS_CAN_SEE);
			access.set(ACCESS_CAN_READ);
			access.set(ACCESS_CAN_WRITE);
		}
		break;
	case ERROR_BOARD:
	case MISPRINT_BOARD:
		// ��� ����� � ���.������, 34 � �� ���������� ������
		if (IS_IMPL(ch)
			|| Privilege::check_flag(ch, Privilege::BOARDS)
			|| Privilege::check_flag(ch, Privilege::MISPRINT))
		{
			access.set();
		}
		else
		{
			access.set(ACCESS_CAN_SEE);
			access.set(ACCESS_CAN_WRITE);
			if (IS_GOD(ch)) access.set(ACCESS_CAN_READ);
		}
		break;
	case NEWS_BOARD:
		// ��� ������, 34 � �� ���������� ������
		if (IS_IMPL(ch) || Privilege::check_flag(ch, Privilege::BOARDS))
		{
			access.set();
		}
		else
		{
			access.set(ACCESS_CAN_SEE);
			access.set(ACCESS_CAN_READ);
		}
		break;
	case GODNEWS_BOARD:
		// 32 ������, 34 � �� ���������� ������
		if (IS_IMPL(ch) || Privilege::check_flag(ch, Privilege::BOARDS))
		{
			access.set();
		}
		else if (IS_GOD(ch))
		{
			access.set(ACCESS_CAN_SEE);
			access.set(ACCESS_CAN_READ);
		}
		break;
	case GODGENERAL_BOARD:
	case GODPUNISH_BOARD:
		// 32 ������/�����, 34 ������
		if (IS_IMPL(ch) || Privilege::check_flag(ch, Privilege::BOARDS))
		{
			access.set();
		}
		else if (IS_GOD(ch))
		{
			access.set(ACCESS_CAN_SEE);
			access.set(ACCESS_CAN_READ);
			access.set(ACCESS_CAN_WRITE);
		}
		break;
	case GODBUILD_BOARD:
	case GODCODE_BOARD:
		// 33 ������/�����, 34 � �� ���������� ������
		if (IS_IMPL(ch) || Privilege::check_flag(ch, Privilege::BOARDS))
		{
			access.set();
		}
		else if (IS_GRGOD(ch))
		{
			access.set(ACCESS_CAN_SEE);
			access.set(ACCESS_CAN_READ);
			access.set(ACCESS_CAN_WRITE);
		}
		break;
	case PERS_BOARD:
		if (IS_GOD(ch) && board->get_pers_uniq() == GET_UNIQUE(ch)
			&& CompareParam(board->get_pers_name(), GET_NAME(ch), true))
		{
			access.set();
		}
		break;
	case CLAN_BOARD:
		// �� ����-�������� ���������� ���, ��� ������ ����� ��� ������
		if (CLAN(ch) && CLAN(ch)->GetRent() == board->get_clan_rent())
		{
			// �������
			if (CLAN(ch)->CheckPrivilege(CLAN_MEMBER(ch)->rank_num, ClanSystem::MAY_CLAN_NEWS))
			{
				access.set();
			}
			else
			{
				access.set(ACCESS_CAN_SEE);
				access.set(ACCESS_CAN_READ);
				access.set(ACCESS_CAN_WRITE);
			}
		}
		break;
	case CLANNEWS_BOARD:
		// ���������� �� �����, �������� ����� ������ ���, ������ ����� �� ����������, ������� ����� ������� �����
		if (CLAN(ch) && CLAN(ch)->GetRent() == board->get_clan_rent())
		{
			if (CLAN(ch)->CheckPrivilege(CLAN_MEMBER(ch)->rank_num, ClanSystem::MAY_CLAN_NEWS))
			{
				access.set();
			}
			else
			{
				access.set(ACCESS_CAN_SEE);
				access.set(ACCESS_CAN_READ);
			}
		}
		break;
	case NOTICE_BOARD:
		// 34+ � �� ���������� ������, 32+ �����/������, ��������� ������ ������
		if (IS_IMPL(ch) || Privilege::check_flag(ch, Privilege::BOARDS))
		{
			access.set();
		}
		else if (IS_GOD(ch))
		{
			access.set(ACCESS_CAN_SEE);
			access.set(ACCESS_CAN_READ);
			access.set(ACCESS_CAN_WRITE);
		}
		else
		{
			access.set(ACCESS_CAN_SEE);
			access.set(ACCESS_CAN_READ);
		}
		break;
	case SUGGEST_BOARD:
		// �� ���������� boards/suggest � 34 ������, ��������� ������ ������ � ��� ������/�����
		if (IS_IMPL(ch)
			|| Privilege::check_flag(ch, Privilege::BOARDS)
			|| Privilege::check_flag(ch, Privilege::SUGGEST))
		{
			access.set();
		}
		else
		{
			access.set(ACCESS_CAN_SEE);
			access.set(ACCESS_CAN_WRITE);
		}
		break;
	case CODER_BOARD:
		access.set(ACCESS_CAN_SEE);
		access.set(ACCESS_CAN_READ);
		break;

	default:
		log("Error board type! (%s %s %d)", __FILE__, __func__, __LINE__);
	}

	// ��������� �������, ������� ������ ����� ������ �� ����-�����
	if (!IS_IMMORTAL(ch)
		&& (PLR_FLAGGED(ch, PLR_HELLED)
			|| PLR_FLAGGED(ch, PLR_NAMED)
			|| PLR_FLAGGED(ch, PLR_DUMB)
			|| lvl_no_write(ch))
		&& (board->get_type() != CLAN_BOARD && board->get_type() != CLANNEWS_BOARD))
	{
		access.reset(ACCESS_CAN_WRITE);
		access.reset(ACCESS_FULL);
	}

	return access;
}

void DoBoardList(CHAR_DATA *ch, char* /*argument*/, int/* cmd*/, int/* subcmd*/)
{
	if (IS_NPC(ch))
		return;

	std::string out(
		" ���         ���  �����|�����                                  ��������  ������\r\n"
		" ===  ==========  ===========  ========================================  ======\r\n");
	int num = 1;
	for (const auto& board : board_list)
	{
		if (!board->get_blind())
		{
			out += Static::print_stats(ch, board, num++);
		}
	}
	// ��� ����� ��� �������� ��������� ��� ���������
	out += " ---  ----------  -----------  ----------------------------------------  ------\r\n";
	for (const auto& board : board_list)
	{
		if (board->get_blind())
		{
			out += Static::print_stats(ch, board, num++);
		}
	}

	send_to_char(out, ch);
}

void report_on_board(CHAR_DATA *ch, char *argument, int/* cmd*/, int subcmd)
{
	if (IS_NPC(ch)) return;
	skip_spaces(&argument);
	delete_doubledollar(argument);

	if (!*argument)
	{
		send_to_char("������ ��������� ���������.\r\n", ch);
		return;
	}

	const auto predicate = [&](const auto board) { return board->get_type() == subcmd; };
	const auto board = std::find_if(board_list.begin(), board_list.end(), predicate);

	if (board == board_list.end())
	{
		send_to_char("����� ���� �� �������... :/\r\n", ch);
		return;
	}
	if (!Static::can_write(ch, *board))
	{
		message_no_write(ch);
		return;
	}
	if ((*board)->is_special()
		&& (*board)->messages.size() >= MAX_REPORT_MESSAGES)
	{
		send_to_char(constants::OVERFLOW_MESSAGE, ch);
		return;
	}
	// ������� ������� (TODO: �������� � ��������� �� �����, ���� �� �������)
	const auto temp_message = std::make_shared<Message>();
	temp_message->author = GET_NAME(ch) ? GET_NAME(ch) : "����������";
	temp_message->unique = GET_UNIQUE(ch);
	// ��� ����� ����� �������� � ������������ ����� ����� ������ (��� ��������� ������� ���-��)
	temp_message->level = GET_LEVEL(ch);
	temp_message->rank = 0;
	temp_message->subject = "[" + std::to_string(GET_ROOM_VNUM(ch->in_room)) + "]";
	temp_message->text = argument;
	temp_message->date = time(0);

	(*board)->add_message(temp_message);
	send_to_char(ch,
		"����� ���������:\r\n"
		"%s\r\n\r\n"
		"��������. ������� ����������.\r\n"
		"                        ����.\r\n", argument);
}

} // namespace Boards

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
