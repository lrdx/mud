// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include "poison.hpp"

#include "logger.hpp"
#include "utils.h"
#include "obj.hpp"
#include "char.hpp"
#include "spells.h"
#include "liquid.hpp"
#include "screen.h"
#include "handler.h"
#include "db.h"
#include "comm.h"
#include "skills.h"
#include "room.hpp"
#include "fight.h"

extern void drop_from_horse(CHAR_DATA *victim);
extern void set_wait(CHAR_DATA * ch, int waittime, int victim_in_room);
extern int interpolate(int min_value, int pulse);

namespace
{
// * ��������� ���� � �����, ������ ��������� �� ���� ���.
bool poison_affect_join(CHAR_DATA *ch, AFFECT_DATA<EApplyLocation>& af)
{
	bool found = false;

	for (auto affect_i = ch->affected.begin(); affect_i != ch->affected.end() && af.location; ++affect_i)
	{
		const auto affect = *affect_i;
		if ((affect->type == SPELL_ACONITUM_POISON
				|| affect->type == SPELL_SCOPOLIA_POISON
				|| affect->type == SPELL_BELENA_POISON
				|| affect->type == SPELL_DATURA_POISON)
			&& af.type != affect->type)
		{
			// ���� ��� ���� ������ �� - ������
			return false;
		}

		if ((affect->type == af.type) && (affect->location == af.location))
		{
			if (abs(affect->modifier/3) < abs(af.modifier))
			{
				af.modifier += affect->modifier;
			}
			else
			{
				af.modifier = affect->modifier;
			}

			ch->affect_remove(affect_i);
			affect_to_char(ch, *affect);
			found = true;
			break;
		}
	}

	if (!found)
	{
		affect_to_char(ch, af);
	}

	return true;
}

// * ���������� � �����.
bool weap_poison_vict(CHAR_DATA *ch, CHAR_DATA *vict, int spell_num)
{
	if (GET_AF_BATTLE(ch, EAF_POISONED))
		return false;

	if (spell_num == SPELL_ACONITUM_POISON)
	{
		// ���� 5 + �����/2, �� 5 �� 20 �� ����
		AFFECT_DATA<EApplyLocation> af;
		af.type = SPELL_ACONITUM_POISON;
		af.location = APPLY_ACONITUM_POISON;
		af.duration = 7;
		af.modifier = GET_LEVEL(ch)/2 + 5;
		af.bitvector = to_underlying(EAffectFlag::AFF_POISON);
		af.battleflag = AF_SAME_TIME;
		if (poison_affect_join(vict, af))
		{
			vict->Poisoner = GET_ID(ch);
			SET_AF_BATTLE(ch, EAF_POISONED);
			return true;
		}
	}
	else if (spell_num == SPELL_SCOPOLIA_POISON)
	{
		// �� +5% ������ �� ���� �� ���� (����� �����)
		AFFECT_DATA<EApplyLocation> af;
		af.type = SPELL_SCOPOLIA_POISON;
		af.location = APPLY_SCOPOLIA_POISON;
		af.duration = 7;
		af.modifier = 5;
		af.bitvector = to_underlying(EAffectFlag::AFF_POISON) | to_underlying(EAffectFlag::AFF_SCOPOLIA_POISON);
		af.battleflag = AF_SAME_TIME;
		if (poison_affect_join(vict, af))
		{
			vict->Poisoner = GET_ID(ch);
			SET_AF_BATTLE(ch, EAF_POISONED);
			return true;
		}
	}
	else if (spell_num == SPELL_BELENA_POISON)
	{
		// �� ������������� (���)
		// -�������/��-���/�����-���.����/�����

		AFFECT_DATA<EApplyLocation> af[3];
		// ����� * 0.05 �� ����� � + 5 �� �����. 4-10% � 9-15% (80-200 �����)
		int percent = 0;
		if (ch->get_skill(SKILL_POISONED) >= 80)
		{
			percent = (ch->get_skill(SKILL_POISONED) * 5 / 100) + (IS_NPC(vict) ? 5 : 0);
		}
		// -����� ���.���� � ������
		af[0].location = APPLY_BELENA_POISON;
		af[0].modifier = percent;

		// ����� * 0.05 + 5 �� ����� � + 10 �� �����. 5.5-15% � 10.5-20% (10-200 �����)
		percent = (ch->get_skill(SKILL_POISONED) * 5 / 100) + (IS_NPC(vict) ? 10 : 5);
		// -�������
		int remove_hit = GET_REAL_HR(vict) * (percent/100);
		af[1].location = APPLY_HITROLL;
		af[1].modifier = -remove_hit;
		// -��-���
		int remove_hp = GET_HITREG(vict) * (percent/100);
		af[2].location = APPLY_HITREG;
		af[2].modifier = -remove_hp;

		bool was_poisoned = true;
		for (int i = 0; i < 3; i++)
		{
			af[i].type = SPELL_BELENA_POISON;
			af[i].duration = 7;
			af[i].bitvector = to_underlying(EAffectFlag::AFF_POISON)
				| to_underlying(EAffectFlag::AFF_BELENA_POISON)
				| to_underlying(EAffectFlag::AFF_SKILLS_REDUCE)
				| to_underlying(EAffectFlag::AFF_NOT_SWITCH);
			af[i].battleflag = AF_SAME_TIME;

			if (!poison_affect_join(vict, af[i]))
			{
				was_poisoned = false;
			}
		}

		if (was_poisoned)
		{
			vict->Poisoner = GET_ID(ch);
			SET_AF_BATTLE(ch, EAF_POISONED);
			return true;
		}
	}
	else if (spell_num == SPELL_DATURA_POISON)
	{
		// �� ������������� (���)
		// -����/���-���/�����-������/�����
		// AFF_DATURA_POISON - ���� �� �������� ������ � ������

		AFFECT_DATA<EApplyLocation> af[3];
		// ����� * 0.05 �� ����� � + 5 �� �����. 4-10% � 9-15% (80-200 �����)
		int percent = 0;
		if (ch->get_skill(SKILL_POISONED) >= 80)
			percent = (ch->get_skill(SKILL_POISONED) * 5 / 100) + (IS_NPC(vict) ? 5 : 0);
		// -����� ������ � ������
		af[0].location = APPLY_DATURA_POISON;
		af[0].modifier = percent;

		// ����� * 0.05 + 5 �� ����� � + 10 �� �����. 5.5-15% � 10.5-20% (10-200 �����)
		percent = (ch->get_skill(SKILL_POISONED) * 5 / 100) + (IS_NPC(vict) ? 10 : 5);
		// -����
		int remove_cast = GET_CAST_SUCCESS(vict) * (percent/100);
		af[1].location = APPLY_CAST_SUCCESS;
		af[1].modifier = -remove_cast;
		// -���
		int remove_mem = GET_MANAREG(vict) * (percent/100);
		af[2].location = APPLY_MANAREG;
		af[2].modifier = -remove_mem;

		bool was_poisoned = true;
		for (int i = 0; i < 3; i++)
		{
			af[i].type = SPELL_DATURA_POISON;
			af[i].duration = 7;
			af[i].bitvector = to_underlying(EAffectFlag::AFF_POISON)
				| to_underlying(EAffectFlag::AFF_DATURA_POISON)
				| to_underlying(EAffectFlag::AFF_SKILLS_REDUCE)
				| to_underlying(EAffectFlag::AFF_NOT_SWITCH);
			af[i].battleflag = AF_SAME_TIME;

			if (!poison_affect_join(vict, af[i]))
			{
				was_poisoned = false;
			}
		}

		if (was_poisoned)
		{
			vict->Poisoner = GET_ID(ch);
			SET_AF_BATTLE(ch, EAF_POISONED);
			return true;
		}
	}
	return false;
}

// * ���� ��� ���������� � �����.
void weap_crit_poison(CHAR_DATA *ch, CHAR_DATA *vict, int/* spell_num*/)
{
	int percent = number(1, skill_info[SKILL_POISONED].max_percent * 3);
	int prob = calculate_skill(ch, SKILL_POISONED, vict);
	if (prob >= percent)
	{
		switch (number(1, 5))
		{
		case 1:
		{
			// ������ ���� � �����
			if (GET_POS(vict) >= POS_FIGHTING)
			{
				if (on_horse(vict))
				{
					send_to_char(ch, "%s�� �������� ������ ��� � %s ����������� ������!%s\r\n",
							CCGRN(ch, C_NRM), PERS(vict, ch, 1), CCNRM(ch, C_NRM));
					send_to_char(vict, "�� ������������� ������� �������������� � �� ������ ������� �� %s!\r\n",
							GET_PAD(get_horse(vict), 5));
					act("$n0 �������$u � �� ����$q ������� �� $N5.", true, vict, 0, get_horse(vict), TO_NOTVICT);
				}
				else
				{
					send_to_char(ch, "%s�� �������� ������ ��� � %s ����������� ����!%s\r\n",
							CCGRN(ch, C_NRM), PERS(vict, ch, 1), CCNRM(ch, C_NRM));
					send_to_char(vict, "�� ������������� ������� �������������� � �� ������ ������� �� �����!\r\n");
					act("$N0 �������$U � �� ����$Q ������� �� �����.", true, ch, 0, vict, TO_NOTVICT);
				}
				GET_POS(vict) = POS_SITTING;
				drop_from_horse(vict);
				set_wait(vict, 3, false);
				break;
			}
			// ���� ���� ������ ����� - ���� ������ �� ������
		}
		case 2:
		{
			// ����� ����� (1..5)
			AFFECT_DATA<EApplyLocation> af;
			af.type = SPELL_POISON;
			af.duration = 30;
			af.modifier = -GET_LEVEL(ch)/6;
			af.bitvector = to_underlying(EAffectFlag::AFF_POISON);
			af.battleflag = AF_SAME_TIME;

			for (int i = APPLY_STR; i <= APPLY_CHA; i++)
			{
				af.location = static_cast<EApplyLocation>(i);
				affect_join(vict, af, false, false, false, false);
			}

			send_to_char(ch, "%s�� �������� ������ ��� %s ���������%s!%s\r\n",
					CCGRN(ch, C_NRM), PERS(vict, ch, 0), GET_CH_VIS_SUF_1(vict, ch), CCNRM(ch, C_NRM));
			send_to_char(vict, "�� ������������� �������� �� ���� ����!\r\n");
			act("$N0 ���������$G �� ����� ������.", true, ch, 0, vict, TO_NOTVICT);
			break;
		}
		case 3:
		{
			// ����� ������� (1..5)
			AFFECT_DATA<EApplyLocation> af;
			af.type = SPELL_POISON;
			af.duration = 30;
			af.location = APPLY_SAVING_REFLEX;
			af.modifier = GET_LEVEL(ch)/6; //Polud � ������, ��������� ����� ��� ������ - ��� ����
			af.bitvector = to_underlying(EAffectFlag::AFF_POISON);
			af.battleflag = AF_SAME_TIME;
			affect_join(vict, af, false, false, false, false);
			send_to_char(ch, "%s�� �������� ������ ��� %s ����%s ���� ����������� �� �������� �����������!%s\r\n",
					CCGRN(ch, C_NRM), PERS(vict, ch, 0), GET_CH_VIS_SUF_1(vict, ch), CCNRM(ch, C_NRM));
			send_to_char(vict, "��� ����� ������� ����������� �� �������� �����������!\r\n");
			act("$N0 ����$G ���� ����������� �� ���� ��������!", true, ch, 0, vict, TO_NOTVICT);
			break;
		}
		case 4:
		{
			// ����� ���������� (1..5)
			AFFECT_DATA<EApplyLocation> af;
			af.type = SPELL_POISON;
			af.duration = 30;
			af.location = APPLY_INITIATIVE;
			af.modifier = -GET_LEVEL(ch)/6;
			af.bitvector = to_underlying(EAffectFlag::AFF_POISON);
			af.battleflag = AF_SAME_TIME;
			affect_join(vict, af, false, false, false, false);
			send_to_char(ch, "%s�� �������� ������ ��� %s ����%s ������� ��������� ���������!%s\r\n",
					CCGRN(ch, C_NRM), PERS(vict, ch, 0), GET_CH_VIS_SUF_1(vict, ch), CCNRM(ch, C_NRM));
			send_to_char(vict, "�� ����� ������� ��������� ���������!\r\n");
			act("$N0 ����$G ������� ��������� ���������!", true, ch, 0, vict, TO_NOTVICT);
			break;
		}
		case 5:
		{
			// ����� ��������� (1..5)
			AFFECT_DATA<EApplyLocation> af;
			af.type = SPELL_POISON;
			af.duration = 30;
			af.location = APPLY_RESIST_VITALITY;
			af.modifier = -GET_LEVEL(ch)/6;
			af.bitvector = to_underlying(EAffectFlag::AFF_POISON);
			af.battleflag = AF_SAME_TIME;
			affect_join(vict, af, false, false, false, false);
			send_to_char(ch, "%s�� �������� ������ ��� %s ����%s ���� ���������� �����������!%s\r\n",
					CCGRN(ch, C_NRM), PERS(vict, ch, 0), GET_CH_VIS_SUF_1(vict, ch), CCNRM(ch, C_NRM));
			send_to_char(vict, "�� ����� ���� ���������� �����������!\r\n");
			act("$N0 ����$G ���� ���������� �����������!", true, ch, 0, vict, TO_NOTVICT);
			break;
		}
		} // switch
	}
	return;
}

} // namespace

// * ���������� � ���������� '��'.
void poison_victim(CHAR_DATA * ch, CHAR_DATA * vict, int modifier)
{
	AFFECT_DATA<EApplyLocation> af[4];

	// change strength
	af[0].type = SPELL_POISON;
	af[0].location = APPLY_STR;
	af[0].duration = pc_duration(vict, 0, MAX(2, GET_LEVEL(ch) - GET_LEVEL(vict)), 2, 0, 1);
	af[0].modifier = -MIN(2, (modifier + 29) / 40);
	af[0].bitvector = to_underlying(EAffectFlag::AFF_POISON);
	af[0].battleflag = AF_SAME_TIME;
	// change damroll
	af[1].type = SPELL_POISON;
	af[1].location = APPLY_DAMROLL;
	af[1].duration = af[0].duration;
	af[1].modifier = -MIN(2, (modifier + 29) / 30);
	af[1].bitvector = to_underlying(EAffectFlag::AFF_POISON);
	af[1].battleflag = AF_SAME_TIME;
	// change hitroll
	af[2].type = SPELL_POISON;
	af[2].location = APPLY_HITROLL;
	af[2].duration = af[0].duration;
	af[2].modifier = -MIN(2, (modifier + 19) / 20);
	af[2].bitvector = to_underlying(EAffectFlag::AFF_POISON);
	af[2].battleflag = AF_SAME_TIME;
	// change poison level
	af[3].type = SPELL_POISON;
	af[3].location = APPLY_POISON;
	af[3].duration = af[0].duration;
	af[3].modifier = GET_LEVEL(ch);
	af[3].bitvector = to_underlying(EAffectFlag::AFF_POISON);
	af[3].battleflag = AF_SAME_TIME;

	for (int i = 0; i < 4; i++)
	{
		affect_join(vict, af[i], false, false, false, false);
	}
	vict->Poisoner = GET_ID(ch);

	snprintf(buf, sizeof(buf), "%s�� �������� $N3.%s", CCIGRN(ch, C_NRM), CCCYN(ch, C_NRM));
	act(buf, false, ch, 0, vict, TO_CHAR);
	snprintf(buf, sizeof(buf), "%s$n �������$g ���.%s", CCIRED(ch, C_NRM), CCCYN(ch, C_NRM));
	act(buf, false, ch, 0, vict, TO_VICT);
}

// * ������� ��������� � ����� ��� �����.
void try_weap_poison(CHAR_DATA *ch, CHAR_DATA *vict, int spell_num)
{
	if (spell_num < 0)
	{
		return;
	}

	if (number(1, 200) <= 25
		|| (!GET_AF_BATTLE(vict, EAF_FIRST_POISON) && !AFF_FLAGGED(vict, EAffectFlag::AFF_POISON)))
	{
		improove_skill(ch, SKILL_POISONED, TRUE, vict);
		if (weap_poison_vict(ch, vict, spell_num))
		{
			if (spell_num == SPELL_ACONITUM_POISON)
			{
				send_to_char(ch, "������������ ���� ������� ���� %s.\r\n",
					PERS(vict, ch, 1));
			}
			else if (spell_num == SPELL_SCOPOLIA_POISON)
			{
				strcpy(buf1, PERS(vict, ch, 0));
				CAP(buf1);
				send_to_char(ch, "%s �������%s �� ����������� ����.\r\n",
					buf1, GET_CH_VIS_SUF_2(vict, ch));
				SET_AF_BATTLE(vict, EAF_FIRST_POISON);
			}
			else if (spell_num == SPELL_BELENA_POISON)
			{
				strcpy(buf1, PERS(vict, ch, 3));
				CAP(buf1);
				send_to_char(ch, "%s ��������� ��������� ����.\r\n", buf1);
				SET_AF_BATTLE(vict, EAF_FIRST_POISON);
			}
			else if (spell_num == SPELL_DATURA_POISON)
			{
				strcpy(buf1, PERS(vict, ch, 2));
				CAP(buf1);
				send_to_char(ch, "%s ����� ������� ������ ����������.\r\n", buf1);
				SET_AF_BATTLE(vict, EAF_FIRST_POISON);
			}
			else
			{
				send_to_char(ch, "�� �������� %s.%s\r\n", PERS(ch, vict, 3));
			}
			send_to_char(vict, "%s%s �������%s ���.%s\r\n",
					CCIRED(ch, C_NRM), PERS(ch, vict, 0),
					GET_CH_VIS_SUF_1(ch, vict), CCNRM(ch, C_NRM));
			weap_crit_poison(ch, vict, spell_num);
		}
	}
}

// * �������� ���� �������� �� �� ��� ��������� �� �����.
bool poison_in_vessel(int liquid_num)
{
	if (liquid_num == LIQ_POISON_ACONITUM
		|| liquid_num == LIQ_POISON_SCOPOLIA
		|| liquid_num == LIQ_POISON_BELENA
		|| liquid_num == LIQ_POISON_DATURA)
	{
		return true;
	}
	return false;
}

// * ��� ��� �� ����� � ����������� �� ���� ��������.
void set_weap_poison(OBJ_DATA *weapon, int liquid_num)
{
	const int poison_timer = 30;
	if (liquid_num == LIQ_POISON_ACONITUM)
		weapon->add_timed_spell(SPELL_ACONITUM_POISON, poison_timer);
	else if (liquid_num == LIQ_POISON_SCOPOLIA)
		weapon->add_timed_spell(SPELL_SCOPOLIA_POISON, poison_timer);
	else if (liquid_num == LIQ_POISON_BELENA)
		weapon->add_timed_spell(SPELL_BELENA_POISON, poison_timer);
	else if (liquid_num == LIQ_POISON_DATURA)
		weapon->add_timed_spell(SPELL_DATURA_POISON, poison_timer);
	else
		log("SYSERROR: liquid_num == %d (%s %s %d)", liquid_num, __FILE__, __func__, __LINE__);
}

// * ����� ����� ��� �� ������ ��� ���������� (��� ������� �����).
std::string get_poison_by_spell(int spell)
{
	switch (spell)
	{
	case SPELL_ACONITUM_POISON:
		return drinknames[LIQ_POISON_ACONITUM];
	case SPELL_SCOPOLIA_POISON:
		return drinknames[LIQ_POISON_SCOPOLIA];
	case SPELL_BELENA_POISON:
		return drinknames[LIQ_POISON_BELENA];
	case SPELL_DATURA_POISON:
		return drinknames[LIQ_POISON_DATURA];
	}
	return "";
}

// * ��������, �������� �� ���������� ����.
bool check_poison(int spell)
{
	switch (spell)
	{
	case SPELL_ACONITUM_POISON:
	case SPELL_SCOPOLIA_POISON:
	case SPELL_BELENA_POISON:
	case SPELL_DATURA_POISON:
		return true;
	}
	return false;
}

/**
* ���� � AF_SAME_TIME ���������� ��������� (� ���/��� ���):
* APPLY_POISON - � ������ ��� � 2 ������� �����, � ���� ��� � ������ �����.
* ��������� ������� - � ������ ��� � 2 ������� �����, � ���� � ��� ��� � 2 �������, ��� ��� - ��� � ������.
*/
int same_time_update(CHAR_DATA* ch, const AFFECT_DATA<EApplyLocation>::shared_ptr& af)
{
	int result = 0;
	if (af->location == APPLY_POISON)
	{
		int poison_dmg = GET_POISON(ch) * (IS_NPC(ch) ? 4 : 5);
		// ����� �� ���� �� ����, � ����� ������ ���� ��� ������������� ���� ��� �� ��� ���� ������ �� 30 ��� ��� ����
		if(!IS_NPC(ch))
			poison_dmg = poison_dmg/30;
		//poison_dmg = interpolate(poison_dmg, 2); // � ��� ��� ������ �������� ���� ������ �� �����, ����� ������ ��� ��� �� ��������
		Damage dmg(SpellDmg(SPELL_POISON), poison_dmg, FightSystem::UNDEF_DMG);
		dmg.flags.set(FightSystem::NO_FLEE);
		result = dmg.process(ch, ch);
	}
	else if (af->location == APPLY_ACONITUM_POISON)
	{
		Damage dmg(SpellDmg(SPELL_POISON), GET_POISON(ch), FightSystem::UNDEF_DMG);
		dmg.flags.set(FightSystem::NO_FLEE);
		result = dmg.process(ch, ch);
	}
	return result;
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
