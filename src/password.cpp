// password.cpp
// Copyright (c) 2007 Krodo
// Part of Bylins http://www.mud.ru

#if defined(__APPLE__) || defined(__MACH__)
#include <unistd.h>
#else
#	ifndef _MSC_VER
#	include <crypt.h>
#	endif
#endif
#include "password.hpp"
#include "utils.h"
#include "interpreter.h"
#include "char.hpp"
#include "char_player.hpp"

// ��� ������� ���������� ���������� (�� ������� ����� ��������� ����� make test � �� ��������)
//#define NOCRYPT
// � ������ ������ ��� ����������� ������ ����� ������ � �������� ����
#if defined(NOCRYPT)
#define CRYPT(a,b) ((char *) (a))
#else
#define CRYPT(a,b) ((char *) crypt((a),(b)))
#endif

namespace Password
{

const char *BAD_PASSWORD = "������ ������ ���� �� 8 �� 50 �������� � �� ������ ���� ������ ���������.";
const unsigned int MIN_PWD_LENGTH = 8;
const unsigned int MAX_PWD_LENGTH = 50;

// * ��������� ���� � �����-����� ��������� �������
std::string generate_md5_hash(const std::string &pwd)
{
#ifdef NOCRYPT
	return pwd;
#else
	char key[14];
	key[0] = '$';
	key[1] = '1';
	key[2] = '$';
	for (int i = 3; i < 12; i ++)
	{
		int c = number(0, 63);
		if (c < 26)
			key[i] = c + 'a';
		else if (c < 52)
			key[i] = c - 26 + 'A';
		else if (c < 62)
			key[i] = c - 52 + '0';
		else
			key[i] = '/';
	}
	key[12] = '$';
	key[13] = '\0';
	return CRYPT(pwd.c_str(), key);
#endif
}

/**
* ���������� ����� ��� � ����� ��� ����
* TODO: � �������� ����� � ���������� � ������� ������.
*/
void set_password(CHAR_DATA *ch, const std::string &pwd)
{
	ch->set_passwd(generate_md5_hash(pwd));
}

// ���������� ������ �� ���� ����� ������� ������
// �����, ������� ��, ���������� ������ ����� ���� openssl ����� � ������
// �� ��� ������ �����
void send_password(const std::string& email, const std::string& password, const std::string& name)
{
	std::string cmd_line = "python3 change_pass.py " + email + " " + password + " " + name + " &";
	auto result = system(cmd_line.c_str());
	UNUSED_ARG(result);
}


void send_password(const std::string& email, const std::string& password)
{
	std::string cmd_line = "python3 change_pass.py " + email + " " + password + " &";
	auto result = system(cmd_line.c_str());
	UNUSED_ARG(result);
}

// �������� set_password, ������� ���������� ������ �� ����
void set_password_to_email(CHAR_DATA *ch, const std::string &pwd)
{
	ch->set_passwd(generate_md5_hash(pwd));
	send_password(std::string(GET_EMAIL(ch)), pwd, std::string(GET_NAME(ch)));
}

// �������� set_password, ������� ���������� ������ �� ����
// � �������, ��� ���� ��� ���������� �������� ������
 void set_all_password_to_email(const char* email, const std::string &pwd, const std::string &name)
 {
	send_password(std::string(email), pwd, name);
 }

/**
* ��� ���� � ������
* \return  0 - des, 1 - md5
*/
bool get_password_type(const CHAR_DATA *ch)
{
	return CompareParam("$1$", ch->get_passwd());
}

/**
* ��������� ����� � ������� ��� ������������� � ��5
* \return 0 - �� �������, 1 - �������
*/
bool compare_password(CHAR_DATA *ch, const std::string &pwd)
{
	bool result = false;
	if (get_password_type(ch))
		result = CompareParam(ch->get_passwd(), CRYPT(pwd.c_str(), ch->get_passwd().c_str()), true);
	else
	{
		// ���� ������ des ������� - ��������� ����� � md5 (10 - ������ MAX_PWD_LENGTH)
		char* s = (char*) CRYPT(pwd.c_str(), ch->get_passwd().c_str());
		if (s && !strncmp(s, ch->get_passwd().c_str(), 10))
		{
			set_password(ch, pwd);
			result = true;
		}
		else if (s == NULL)
		{
			send_to_char("�������� �������� ��� �������� ������ ������. ���������� � ������� ����� ��� ��� ������.\r\n", ch);
			result = false;
		}
	}
	return result;
}

/**
* �������� ������ �� ����� � �������
* \return 0 - ������������ ������, 1 - ����������
*/
bool check_password(const CHAR_DATA *ch, const char *pwd)
{
// ��� ����������� ����������� �� ������� ������ ����� ������� �����
#ifndef NOCRYPT
	if (!pwd || !str_cmp(pwd, GET_PC_NAME(ch)) || strlen(pwd) > MAX_PWD_LENGTH || strlen(pwd) < MIN_PWD_LENGTH)
		return 0;
#else
	UNUSED_ARG(ch);
	UNUSED_ARG(pwd);
#endif
	return true;
}

/**
* ����� ������������� ������ compare_password.
* \return 0 - �� �������, 1 - �������
*/
bool compare_password(std::string const &hash, std::string const &pass)
{
	return CompareParam(hash, CRYPT(pass.c_str(), hash.c_str()), true);
}

} // namespace Password

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
