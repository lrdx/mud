/* ****************************************************************************
* File: top.h                                                  Part of Bylins *
* Usage: ��� ������� ���������                                                *
* (c) 2006 Krodo                                                              *
******************************************************************************/

#ifndef _TOP_H_
#define _TOP_H_

#include "conf.h"
#include <string>
#include <list>
#include <vector>

// ���-�� ������������ � ���� ������� �� ���������
#define MAX_TOP_CLASS 10

class TopPlayer;
typedef std::vector< std::list<TopPlayer> > TopListType;

class TopPlayer
{
public:
	TopPlayer(long _unique, const char * _name, long _exp, int _remort)
			: unique(_unique), name(_name), exp(_exp), remort(_remort) {};
	~TopPlayer() {};

	static const char * TopFormat[];

	static void Remove(CHAR_DATA * ch);
	static void Refresh(CHAR_DATA * ch, bool reboot = 0);

private:
	long unique;      // ���
	std::string name; // ���
	long exp;         // �����
	int remort;       // ��������

	static TopListType TopList; // ������ ���

	friend void DoBest(CHAR_DATA *ch, char *argument, int cmd, int subcmd);
};

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
