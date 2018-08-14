#include "stigmas.hpp"

#include "char.hpp"
#include "interpreter.h"
#include "handler.h"

void do_touch_stigma(CHAR_DATA *ch, char*, int, int);

std::vector<Stigma> stigmas;

void do_touch_stigma(CHAR_DATA *ch, char* argument, int, int)
{
	CHAR_DATA *vict = NULL;

	one_argument(argument, buf);

	if (!*buf)
	{
		send_to_char(ch, "�� ������������ � ����. �������!\r\n");
		return;
	}

	if (!(vict = get_player_vis(ch, buf, FIND_CHAR_WORLD)))
	{
		ch->touch_stigma(buf);
	}
	else
	{
		sprintf(buf, "�� ������������ � %s. ������ �� ���������.\r\n", vict->get_name().c_str());
		send_to_char(buf, ch);
	}
}

std::string StigmaWear::get_name() const
{
	return this->stigma.name;
}

// ������ �������� ������
void stigma_fire_dragon(CHAR_DATA *ch)
{
	send_to_char(ch, "�� ������������ � ������ � ������������ ��������� �������.\r\n");
	send_to_char(ch, "������� �������� � �� ������������ ��������� ����.");
}

// ������������� �����
void init_stigmas()
{
	Stigma tmp;
	tmp.id = STIGMA_FIRE_DRAGON;
	tmp.name = "�������� ������";
	tmp.activation_stigma = &stigma_fire_dragon;
	tmp.reload = 10;
	stigmas.push_back(tmp);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
