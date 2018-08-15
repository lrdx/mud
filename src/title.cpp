// title.cpp
// Copyright (c) 2006 Krodo
// Part of Bylins http://www.mud.ru

#include "title.hpp"

#include "conf.h"
#include "logger.hpp"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "screen.h"
#include "pk.h"
#include "privilege.hpp"
#include "handler.h"
#include "char.hpp"
#include "char_player.hpp"
#include "interpreter.h"

#include <boost/algorithm/string.hpp>

#include <sstream>
#include <fstream>

namespace TitleSystem
{

// ��� ������ �������, ������ ������������� �������� ��� ��������� ������
struct waiting_title
{
	std::string title;     // �����
	std::string pre_title; // ���������
	long unique;           // ��� ������ (��� ����� �������)
};

const unsigned int MAX_TITLE_LENGTH = 80; // ����.����� ������ ������ (�����+���������)
const int SET_TITLE_COST = 1000;          // ���� �� ������� ��������� ������
const char* TITLE_FILE = LIB_PLRSTUFF"titles.lst"; // ���� ����������/��������� ������ ��������� �������
const char* MORTAL_DO_TITLE_FORMAT =
	"����� - ������� � ������� � ���������� �� ������, ������������ �� ������������ ��� ������� ������ �������������\r\n"
	"����� ���������� <�����> - ��������������� ��������� ������ ������, ������� �������������\r\n"
	"����� ���������� <����� ����������!����� ������> - ����, ��� � � �������\r\n"
	"����� �������� - ������������� � �������� ������ �����\r\n"
	"����� �������� - ������ �������� ����� ������ �� ������ ������������ �����\r\n"
	"����� ������� - ������� ���� ������� ����� � ��������� (������ ������ �������)\r\n";
const char* GOD_DO_TITLE_FORMAT =
	"����� - ����� ������� ������ ��� ���� ������� � ������ �� ����������\r\n"
	"����� <�����> ��������|���������|������� - ��������� � ����� � ������ ������ �� ������, ������ ������ � ������ ������\r\n"
	"����� ���������� <����� ������> - ��������� ������ ������\r\n"
	"����� ���������� <����� ����������!����� ������> - ��������� ������ ���������� � ������\r\n"
	"����� ������� - ������� ���� ������� ����� � ��������� (������ ������ �������)\r\n";
const char* TITLE_SEND_FORMAT = "�� ������ ��������� �������� ��, �������� �����, ��� �������� �������� '����� ��������'\r\n";

enum { TITLE_FIND_CHAR = 0, TITLE_CANT_FIND_CHAR, TITLE_NEED_HELP };
typedef std::shared_ptr<waiting_title> WaitingTitlePtr;
typedef std::map<std::string, WaitingTitlePtr> TitleListType;
TitleListType title_list;
TitleListType temp_title_list;

bool check_title(const std::string& text, CHAR_DATA* ch);
bool check_pre_title(std::string text, CHAR_DATA* ch);
bool check_alphabet(const std::string& text, CHAR_DATA* ch, const std::string& allowed);
bool is_new_petition(CHAR_DATA* ch);
bool manage_title_list(std::string& name, bool action, CHAR_DATA* ch);
void set_player_title(CHAR_DATA* ch, const std::string& pre_title, const std::string& title, const char* god);
const char* print_help_string(CHAR_DATA* ch);
std::string print_agree_string(bool new_petittion);
std::string print_title_string(CHAR_DATA* ch, const std::string& pre_title, const std::string& title);
std::string print_title_string(const std::string& name, const std::string& pre_title, const std::string& title);
void do_title_empty(CHAR_DATA* ch);
DESCRIPTOR_DATA* send_result_message(long unique, bool action);

} // namespace TitleSystem

// * ������� �����, title. ACMD(do_title), ��� ������� � ��� ����� ��� ����� ��������.
void TitleSystem::do_title(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/)
{
	if (IS_NPC(ch)) return;

	if (!Privilege::check_flag(ch, Privilege::TITLE) && PLR_FLAGGED(ch, PLR_NOTITLE))
	{
		send_to_char("��� ��������� ������ � ��������.\r\n", ch);
		return;
	}

	std::string buffer(argument), buffer2;
	GetOneParam(buffer, buffer2);

	if (buffer2.empty())
	{
		do_title_empty(ch);
		return;
	}

	// ����� ���� ������� �� ���������� ������, ����� ��������������� ���������� ��������� ����
	int result = TITLE_NEED_HELP;
	if (IS_GOD(ch) || Privilege::check_flag(ch, Privilege::TITLE))
	{
		boost::trim(buffer);
		if (CompareParam(buffer, "�������"))
		{
			boost::trim(buffer2);
			CHAR_DATA *vict = get_player_pun(ch, buffer2, FIND_CHAR_WORLD);
			if (!vict)
			{
				send_to_char("��� ������ ������.\r\n", ch);
				return;
			}
			if (GET_LEVEL(vict) >= LVL_IMMORT || PRF_FLAGGED(vict, PRF_CODERINFO))
			{
				send_to_char("�� �� ������ ������� �����.\r\n", ch);
				return;
			}
			if (GET_TITLE(vict) != "")
			{
				sprintf(buf, "&c%s ������ ����� ������ %s.&n\r\n", GET_NAME(ch), GET_NAME(vict));
				send_to_gods(buf, true);
				GET_TITLE(vict) = "";
				//send_to_char("����� ������.\r\n", ch);
			} else
				send_to_char("� ������ ��� ������.\r\n", ch);
			return;
		}
		else if (CompareParam(buffer, "��������"))
			result = manage_title_list(buffer2, true, ch);
		else if (CompareParam(buffer, "���������"))
			result = manage_title_list(buffer2, false, ch);
		if (result == TITLE_FIND_CHAR)
			return;
	}

	if (CompareParam(buffer2, "����������"))
	{
		boost::trim(buffer);
		if (buffer.size() > MAX_TITLE_LENGTH)
		{
			if (PLR_FLAGGED(ch, PLR_NOTITLE))
			{
				send_to_char(ch, "��� ��������� ������������� ������ ���� �����.\r\n");
				return;
			}
			send_to_char(ch, "����� ������ ���� �� ������� %d ��������.\r\n", MAX_TITLE_LENGTH);
			return;
		}

		std::string pre_title, title;
		std::string::size_type beg_idx = buffer.find('!');
		if (beg_idx != std::string::npos)
		{
			pre_title = buffer.substr(0, beg_idx);
			beg_idx = buffer.find_first_not_of('!', beg_idx);
			if (beg_idx != std::string::npos)
				title = buffer.substr(beg_idx);
			boost::trim(title);
			boost::trim(pre_title);
			if (!pre_title.empty())
			{
				sprintf(buf2, "%c%s", UPPER(pre_title.substr(0, 1).c_str()[0]), pre_title.substr(1).c_str());
				pre_title = buf2;
			}
			if (!pre_title.empty() && !check_pre_title(pre_title, ch)) return;
			if (!title.empty() && !check_title(title, ch)) return;
		}
		else
		{
			if (!check_title(buffer, ch)) return;
			title = buffer;
		}
		if (IS_GOD(ch) || Privilege::check_flag(ch, Privilege::TITLE))
		{
			set_player_title(ch, pre_title, title, GET_NAME(ch));
			send_to_char("����� ����������\r\n", ch);
			return;
		}
		if (title.empty() && pre_title.empty())
		{
			send_to_char("�� ������ ������ �������� ����� ��� ���������.\r\n", ch);
			return;
		}

		bool new_petition = is_new_petition(ch);
		WaitingTitlePtr temp(new waiting_title);
		temp->title = title;
		temp->pre_title = pre_title;
		temp->unique = GET_UNIQUE(ch);
		temp_title_list[GET_NAME(ch)] = temp;

		std::stringstream out;
		out << "��� ����� ����� ��������� ��������� �������:\r\n" << CCPK(ch, C_NRM, ch);
		out << print_title_string(ch, pre_title, title) << print_agree_string(new_petition);
		send_to_char(out.str(), ch);
	}
	else if (CompareParam(buffer2, "��������"))
	{
		TitleListType::iterator it = temp_title_list.find(GET_NAME(ch));
		if (it != temp_title_list.end())
		{
			if (ch->get_bank() < SET_TITLE_COST)
			{
				send_to_char("�� ����� ����� �� ������� ����� ��� ������ ���� ������.\r\n", ch);
				return;
			}
			ch->remove_bank(SET_TITLE_COST);
			title_list[it->first] = it->second;
			temp_title_list.erase(it);
			send_to_char("���� ������ ���������� ����� � ����� ����������� � ��������� �����.\r\n", ch);
			send_to_char(TITLE_SEND_FORMAT, ch);
		}
		else
			send_to_char("� ������ ������ ��� ������, �� ������� ��������� ���� ��������.\r\n", ch);
	}
	else if (CompareParam(buffer2, "��������"))
	{
		TitleListType::iterator it = title_list.find(GET_NAME(ch));
		if (it != title_list.end())
		{
			title_list.erase(it);
			ch->add_bank(SET_TITLE_COST);
			send_to_char("���� ������ �� ����� ��������.\r\n", ch);
		}
		else
			send_to_char("� ������ ������ ��� ������ ��������.\r\n", ch);
	}
	else if (CompareParam(buffer2, "�������", true))
	{
		if (GET_TITLE(ch) != "")
		{
			GET_TITLE(ch) = "";
		}
		send_to_char("���� ����� � ��������� �������.\r\n", ch);
	}
	else
	{
		// ��� �����, ����� ������� �������� �� ������ ������� �, ���� �� ����� ��� ����, ��������� ������ �������
		if (result == TITLE_CANT_FIND_CHAR)
			send_to_char("� ������ ��� ��������� � ����� ������.\r\n", ch);
		else if (result == TITLE_NEED_HELP)
			send_to_char(print_help_string(ch), ch);
	}
}

/**
* �������� �� ���������� ������ � �������
* ��� ������� 25+ ����� ��� ������� �������, ����� �� ��������
* \return 0 �� �������, 1 �������
*/
bool TitleSystem::check_title(const std::string& text, CHAR_DATA* ch)
{
	if (!check_alphabet(text, ch, " ,.-?��")) return false;

	if (GET_LEVEL(ch) < 25 && !GET_REMORT(ch) && !IS_GOD(ch) && !Privilege::check_flag(ch, Privilege::TITLE))
	{
		send_to_char(ch, "��� ����� �� ����� �� ������ ���������� 25�� ������ ��� ����� ��������������.\r\n");
		return false;
	}

	return true;
}

/**
* �������� �� ���������� ������ � �����������.
* ��� ������� ���������� ������� �������� ��� ������� ����� (����� - ��� 4+ ����) � ����������,
* �� 3 ���� - ������� ��� �������. �� ����� 3 ���� � 3 ���������, ����� �� ��������.
* \return 0 �� �������, 1 �������
*/
bool TitleSystem::check_pre_title(std::string text, CHAR_DATA* ch)
{
	if (!check_alphabet(text, ch, " .-?��")) return false;

	if (IS_GOD(ch) || Privilege::check_flag(ch, Privilege::TITLE)) return true;

	if (!GET_REMORT(ch))
	{
		send_to_char(ch, "�� ������ ����� �� ������� ���� ���� �������������� ��� ����������.\r\n");
		return false;
	}

	int word = 0, prep = 0;
	typedef boost::split_iterator<std::string::iterator> split_it;
	for (split_it it = boost::make_split_iterator(text, boost::first_finder(" ", boost::is_iequal())); it != split_it(); ++it)
	{
		if (boost::copy_range<std::string>(*it).size() > 3)
			++word;
		else
			++prep;
	}
	if (word > 3 || prep > 3)
	{
		send_to_char(ch, "������� ����� ���� � ����������.\r\n");
		return false;
	}
	if (word > GET_REMORT(ch))
	{
		send_to_char(ch, "� ��� ������������ �������������� ��� �������� ���� � ����������.\r\n");
		return false;
	}

	return true;
}

/**
* �������� �� ���������� ������ � ����� �������������� � ������� �������� � �������� �� ������ �� �����
* \param allowed - ������ � ����������� ��������� ������ �������� �������� (��� ������ � ���������� ������)
* \return 0 �� �������, 1 �������
*/
bool TitleSystem::check_alphabet(const std::string& text, CHAR_DATA* ch, const std::string& allowed)
{
	int i = 0;
	std::string::size_type idx;
	for (std::string::const_iterator it = text.begin(); it != text.end(); ++it, ++i)
	{
		unsigned char c = static_cast<char>(*it);
		idx = allowed.find(*it);
		if (c < 192 && idx == std::string::npos)
		{
			send_to_char(ch, "������������ ������ '%c' � ������� %d.\r\n", *it, ++i);
			return false;
		}
	}
	return true;
}

/**
* ��� ������ ��������� ����������� ��������� ������, ���� �� ������
* \param unique - ��� ���������, �������� ���� ���������
* \param action - 0 ���� ���������, 1 ���� ��������
*/
DESCRIPTOR_DATA* TitleSystem::send_result_message(long unique, bool action)
{
	DESCRIPTOR_DATA* d = DescByUID(unique);
	if (d)
	{
		send_to_char(d->character.get(), "��� ����� ��� %s ������.\r\n", action ? "�������" : "��������");
	}
	return d;
}

/**
* �������� ������ �� ������ ������� � ����������/��������.
* ���������� ������ �� ���������, ���� �� ������. ������ � ����, ���� �������.
* \param action - 0 ������ ������, 1 ���������
*/
bool TitleSystem::manage_title_list(std::string &name, bool action, CHAR_DATA *ch)
{
	name_convert(name);
	TitleListType::iterator it = title_list.find(name);
	if (it != title_list.end())
	{
		if (action)
		{
			DESCRIPTOR_DATA* d = send_result_message(it->second->unique, action);
			// ��� ����� �� ����� ?
			if (d)
			{
				set_player_title(d->character.get(), it->second->pre_title, it->second->title, GET_NAME(ch));
				sprintf(buf, "&c%s ������� ����� ������ %s!&n\r\n", GET_NAME(ch), GET_NAME(d->character));
				send_to_gods(buf, true);
			}
			else
			{
				Player victim;
				if (load_char(it->first.c_str(), &victim) < 0)
				{
					send_to_char("�������� ��� ������ ��� �������� �����-�� �����.\r\n", ch);
					title_list.erase(it);
					return TITLE_FIND_CHAR;
				}
				set_player_title(&victim, it->second->pre_title, it->second->title, GET_NAME(ch));
				sprintf(buf, "&c%s ������� ����� ������ %s[�������].&n\r\n", GET_NAME(ch), GET_NAME(&victim));
				send_to_gods(buf, true);
				victim.save_char();
			}
		}
		else
		{
			send_result_message(it->second->unique, action);

			DESCRIPTOR_DATA* d = send_result_message(it->second->unique, action);
			if (d)
			{			
				sprintf(buf, "&c%s �������� ����� ������ %s.&n\r\n", GET_NAME(ch), GET_NAME(d->character));
				send_to_gods(buf, true);
			}
			else
			{
				Player victim;
				if (load_char(it->first.c_str(), &victim) < 0)
				{
				    send_to_char("�������� ��� ������ ��� �������� �����-�� �����.\r\n", ch);
				    title_list.erase(it);
				    return TITLE_FIND_CHAR;
				}
				sprintf(buf, "&c%s �������� ����� ������ %s[�������].&n\r\n", GET_NAME(ch), GET_NAME(&victim));
				send_to_gods(buf, true);
				victim.save_char();
			}
		}
		title_list.erase(it);
		return TITLE_FIND_CHAR;
	}
	return TITLE_CANT_FIND_CHAR;
}

// * ����� ����� �������, ������ ���������
void TitleSystem::show_title_list(CHAR_DATA* ch)
{
	if (title_list.empty()) return;

	std::stringstream out;
	out << "\r\n������ ��������� ���� ��������� ������ (����� <�����> ��������/���������):\r\n" << CCWHT(ch, C_NRM);
	for (TitleListType::const_iterator it = title_list.begin(); it != title_list.end(); ++it)
		out << print_title_string(it->first, it->second->pre_title, it->second->title);
	out << CCNRM(ch, C_NRM);
	send_to_char(out.str(), ch);
}

// * ���������� ������ ������ ���� ��� ��������� (��� �������� ���������)
std::string TitleSystem::print_title_string(const std::string& name, const std::string& pre_title, const std::string& title)
{
	std::stringstream out;
	if (!pre_title.empty())
		out << pre_title << " ";
	out << name;
	if (!title.empty())
		out << ", " << title;
	out << "\r\n";
	return out.str();
}

// * ���������� ������ ������, ��� ��� ����� ����� � ���� � ������ ����� ��
std::string TitleSystem::print_title_string(CHAR_DATA* ch, const std::string& pre_title, const std::string& title)
{
	std::stringstream out;
	out << CCPK(ch, C_NRM, ch);
	if (!pre_title.empty())
		out << pre_title << " ";
	out << GET_NAME(ch);
	if (!title.empty())
		out << ", " << title;
	out << CCNRM(ch, C_NRM) << "\r\n";
	return out.str();
}

/**
* ��������� ���� ������ (GET_TITLE(ch)) ������
* \param ch - ���� ������
* \param god - ��� ����������� ����� ����
*/
void TitleSystem::set_player_title(CHAR_DATA* ch, const std::string& pre_title, const std::string& title, const char* god)
{
	std::stringstream out;
	out << title;
	if (!pre_title.empty())
		out << ";" << pre_title;
	out << "/������� �����: " << god << "/";
	GET_TITLE(ch) = out.str();
}

// * ��� ���������� ������ ��������� ���� � ������
const char* TitleSystem::print_help_string(CHAR_DATA* ch)
{
	if (IS_GOD(ch) || Privilege::check_flag(ch, Privilege::TITLE))
		return GOD_DO_TITLE_FORMAT;

	return MORTAL_DO_TITLE_FORMAT;
}

// * ���������� ������ ������� �� ���������
void TitleSystem::save_title_list()
{
	std::ofstream file(TITLE_FILE);
	if (!file.is_open())
	{
		log("Error open file: %s! (%s %s %d)", TITLE_FILE, __FILE__, __func__, __LINE__);
		return;
	}
	for (TitleListType::const_iterator it = title_list.begin(); it != title_list.end(); ++it)
		file << it->first << " " <<  it->second->unique << "\n" << it->second->pre_title << "\n" << it->second->title << "\n";
	file.close();
}

// * �������� ������ ������� �� ���������, ������ ������������ ������ ����� reload title
void TitleSystem::load_title_list()
{
	title_list.clear();

	std::ifstream file(TITLE_FILE);
	if (!file.is_open())
	{
		log("Error open file: %s! (%s %s %d)", TITLE_FILE, __FILE__, __func__, __LINE__);
		return;
	}
	std::string name, pre_title, title;
	long unique;
	while (file >> name >> unique)
	{
		ReadEndString(file);
		std::getline(file, pre_title);
		std::getline(file, title);
		WaitingTitlePtr temp(new waiting_title);
		temp->title = title;
		temp->pre_title = pre_title;
		temp->unique = unique;
		title_list[name] = temp;
	}
	file.close();
}

/**
* ��� ������� ������� ����� ��� ��� ����� �� ������ (�� �����, ���� ��� ���� �������������� ������)
* \return 0 �� ������� ������ � ��������� ��� ���� ������, 1 ����� ������
*/
bool TitleSystem::is_new_petition(CHAR_DATA* ch)
{
	TitleListType::iterator it = temp_title_list.find(GET_NAME(ch));
	if (it != temp_title_list.end())
		return false;
	return true;
}

/**
* ��� ����������� ���������� ��� ��� � ������ ���� �� ������
* \param new_petition - 0 ���������� ������ ������, 1 ����� ������
*/
std::string TitleSystem::print_agree_string(bool new_petition)
{
	std::stringstream out;
	out << "��� ������������� ������ �������� '����� ��������'";
	if (new_petition)
		out << ", ��������� ������ " << SET_TITLE_COST << " ���.";
	out << "\r\n";
	return out.str();
}

// * ��������� ������� ������ ������� �����
void TitleSystem::do_title_empty(CHAR_DATA* ch)
{
	if ((IS_GOD(ch) || Privilege::check_flag(ch, Privilege::TITLE)) && !title_list.empty())
		show_title_list(ch);
	else
	{
		std::stringstream out;
		TitleListType::iterator it = title_list.find(GET_NAME(ch));
		if (it != title_list.end())
		{
			out << "������ ������ ��������� � ������������ � �����:\r\n"
			<< print_title_string(ch, it->second->pre_title, it->second->title)
			<< TITLE_SEND_FORMAT << "\r\n";
		}
		it = temp_title_list.find(GET_NAME(ch));
		if (it != temp_title_list.end())
		{
			out << "������ ����� ���� ������ ������������� ��� �������� ������ �����:\r\n"
			<< print_title_string(ch, it->second->pre_title, it->second->title)
			<< print_agree_string(is_new_petition(ch)) << "\r\n";
		}
		out << print_help_string(ch);
		send_to_char(out.str(), ch);
	}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
