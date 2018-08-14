#include "fight_hit.hpp"

#include "logger.hpp"
#include "handler.h"
#include "screen.h"
#include "dg_scripts.h"
#include "pk.h"
#include "dps.hpp"
#include "fight.h"
#include "house_exp.hpp"
#include "poison.hpp"
#include "bonus.h"
#include "features.hpp"

// extern
int extra_aco(int class_num, int level);
int thaco(int class_num, int level);
void npc_groupbattle(CHAR_DATA * ch);
void set_wait(CHAR_DATA * ch, int waittime, int victim_in_room);
void go_autoassist(CHAR_DATA * ch);

int armor_class_limit(CHAR_DATA * ch)
{
	if (IS_CHARMICE(ch))
	{
		return -200;
	};
	if (IS_NPC(ch))
	{
		return -300;
	};
	switch (GET_CLASS(ch))
	{
	case CLASS_ASSASINE:
	case CLASS_THIEF:
	case CLASS_GUARD:
		return -250;
		break;
	case CLASS_MERCHANT:
	case CLASS_WARRIOR:
	case CLASS_PALADINE:
	case CLASS_RANGER:
	case CLASS_SMITH:
		return -200;
		break;
	case CLASS_CLERIC:
	case CLASS_DRUID:
		return -170;
		break;
	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
	case CLASS_NECROMANCER:
		return -150;
		break;
	}
	return -300;
}

int compute_armor_class(CHAR_DATA * ch)
{
	int armorclass = GET_REAL_AC(ch);

	if (AWAKE(ch))
	{
		int high_stat = GET_REAL_DEX(ch);
		// � ������� ��� �� ������� ���� ����: �������� ��� 3/4 ����
		if (!IS_NPC(ch))
		{
			high_stat = std::max(high_stat, GET_REAL_INT(ch) * 3 / 4);
		}
		armorclass -= dex_ac_bonus(high_stat) * 10;
		armorclass += extra_aco((int) GET_CLASS(ch), (int) GET_LEVEL(ch));
	};

	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BERSERK))
	{
		armorclass -= (240 * ((GET_REAL_MAX_HIT(ch) / 2) - GET_HIT(ch)) / GET_REAL_MAX_HIT(ch));
	}

	// Gorrah: ����� � �� �� ���������� ������ "�������� �����"
	if (PRF_FLAGS(ch).get(PRF_IRON_WIND))
	{
		armorclass += ch->get_skill(SKILL_IRON_WIND) / 2;
	}

	armorclass += (size_app[GET_POS_SIZE(ch)].ac * 10);

	if (GET_AF_BATTLE(ch, EAF_PUNCTUAL))
	{
		if (GET_EQ(ch, WEAR_WIELD))
		{
			if (GET_EQ(ch, WEAR_HOLD))
				armorclass +=
					10 * MAX(-1,
							 (GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD)) +
							  GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HOLD))) / 5 - 6);
			else
				armorclass += 10 * MAX(-1, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD)) / 5 - 6);
		}
		if (GET_EQ(ch, WEAR_BOTHS))
			armorclass += 10 * MAX(-1, GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_BOTHS)) / 5 - 6);
	}

	armorclass = MIN(100, armorclass);
	return (MAX(armor_class_limit(ch), armorclass));
}

void haemorragia(CHAR_DATA * ch, int percent)
{
	AFFECT_DATA<EApplyLocation> af[3];

	af[0].type = SPELL_HAEMORRAGIA;
	af[0].location = APPLY_HITREG;
	af[0].modifier = -percent;
	//TODO: ������������� �����, ���� ���� ������ 31?
	af[0].duration = pc_duration(ch, number(1, 31 - GET_REAL_CON(ch)), 0, 0, 0, 0);
	af[0].bitvector = 0;
	af[0].battleflag = 0;
	af[1].type = SPELL_HAEMORRAGIA;
	af[1].location = APPLY_MOVEREG;
	af[1].modifier = -percent;
	af[1].duration = af[0].duration;
	af[1].bitvector = 0;
	af[1].battleflag = 0;
	af[2].type = SPELL_HAEMORRAGIA;
	af[2].location = APPLY_MANAREG;
	af[2].modifier = -percent;
	af[2].duration = af[0].duration;
	af[2].bitvector = 0;
	af[2].battleflag = 0;

	for (int i = 0; i < 3; i++)
	{
		affect_join(ch, af[i], TRUE, FALSE, TRUE, FALSE);
	}
}

void HitData::compute_critical(CHAR_DATA * ch, CHAR_DATA * victim)
{
	const char *to_char = NULL, *to_vict = NULL;
	AFFECT_DATA<EApplyLocation> af[4];
	OBJ_DATA *obj;
	int unequip_pos = 0;

	for (int i = 0; i < 4; i++)
	{
		af[i].type = 0;
		af[i].location = APPLY_NONE;
		af[i].bitvector = 0;
		af[i].modifier = 0;
		af[i].battleflag = 0;
		af[i].duration = pc_duration(victim, 2, 0, 0, 0, 0);
	}

	switch (number(1, 10))
	{
	case 1:
	case 2:
	case 3:
	case 4:		// FEETS
		switch (dam_critic)
		{
		case 1:
		case 2:
		case 3:
			// Nothing
			return;
		case 5:	// Hit genus, victim bashed, speed/2
			SET_AF_BATTLE(victim, EAF_SLOW);
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 10);
		case 4:	// victim bashed
			if (GET_POS(victim) > POS_SITTING)
				GET_POS(victim) = POS_SITTING;
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
			to_char = "�������� $N3 �� �����";
			to_vict = "��������� ��� ������, ������� �� �����";
			break;
		case 6:	// foot damaged, speed/2
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 9);
			to_char = "��������� �������� $N1";
			to_vict = "������� ��� �������";
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 7:
		case 9:	// armor damaged else foot damaged, speed/4
			if (GET_EQ(victim, WEAR_LEGS))
				alt_equip(victim, WEAR_LEGS, 100, 100);
			else
			{
				dam *= (ch->get_skill(SKILL_PUNCTUAL) / 8);
				to_char = "��������� �������� $N1";
				to_vict = "������� ��� ����";
				af[0].type = SPELL_BATTLE;
				af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
				SET_AF_BATTLE(victim, EAF_SLOW);
			}
			break;
		case 8:	// femor damaged, no speed
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 7);
			to_char = "������ ��������� �������� $N1";
			to_vict = "������� ��� �����";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			haemorragia(victim, 20);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 10:	// genus damaged, no speed, -2HR
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 7);
			to_char = "������ ��������� �������� $N1";
			to_vict = "���������� ��� ������";
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -2;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 11:	// femor damaged, no speed, no attack
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 7);
			to_char = "������ $N3 �� �����";
			to_vict = "���������� ��� �����";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
			af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			af[1].type = SPELL_BATTLE;
			af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			haemorragia(victim, 20);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		default:	// femor damaged, no speed, no attack
			if (dam_critic > 12)
				dam *= (ch->get_skill(SKILL_PUNCTUAL) / 5);
			else
				dam *= (ch->get_skill(SKILL_PUNCTUAL) / 6);
			to_char = "������ $N3 �� �����";
			to_vict = "����������� ��� ����";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
			af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			af[1].type = SPELL_BATTLE;
			af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			haemorragia(victim, 50);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		}
		break;
	case 5:		//  ABDOMINAL
		switch (dam_critic)
		{
		case 1:
		case 2:
		case 3:
			// nothing
			return;
		case 4:	// waits 1d6
			WAIT_STATE(victim, number(2, 6) * PULSE_VIOLENCE);
			to_char = "����� $N2 �������";
			to_vict = "����� ��� �������";
			break;

		case 5:	// abdomin damaged, waits 1, speed/2
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 8);
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
			to_char = "������ $N3 � �����";
			to_vict = "������ ��� � �����";
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 6:	// armor damaged else dam*3, waits 1d6
			WAIT_STATE(victim, number(2, 6) * PULSE_VIOLENCE);
			if (GET_EQ(victim, WEAR_WAIST))
				alt_equip(victim, WEAR_WAIST, 100, 100);
			else
				dam *= (ch->get_skill(SKILL_PUNCTUAL) / 7);
			to_char = "��������� $N2 �����";
			to_vict = "��������� ��� �����";
			break;
		case 7:
		case 8:	// abdomin damage, speed/2, HR-2
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 6);
			to_char = "������ $N3 � �����";
			to_vict = "������ ��� � �����";
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -2;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 9:	// armor damaged, abdomin damaged, speed/2, HR-2
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 5);
			alt_equip(victim, WEAR_BODY, 100, 100);
			to_char = "������ $N3 � �����";
			to_vict = "������ ��� � �����";
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -2;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			haemorragia(victim, 20);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 10:	// abdomin damaged, no speed, no attack
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 4);
			to_char = "��������� $N2 �����";
			to_vict = "��������� ��� �����";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
			af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			af[1].type = SPELL_BATTLE;
			af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			haemorragia(victim, 20);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 11:	// abdomin damaged, no speed, no attack
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 3);
			to_char = "��������� $N2 �����";
			to_vict = "��������� ��� �����";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
			af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			af[1].type = SPELL_BATTLE;
			af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			haemorragia(victim, 40);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		default:	// abdomin damaged, hits = 0
			dam *= ch->get_skill(SKILL_PUNCTUAL) / 2;
			to_char = "���������� $N2 �����";
			to_vict = "���������� ��� �����";
			haemorragia(victim, 60);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		}
		break;
	case 6:
	case 7:		// CHEST
		switch (dam_critic)
		{
		case 1:
		case 2:
		case 3:
			// nothing
			return;
		case 4:	// waits 1d4, bashed
			WAIT_STATE(victim, number(2, 5) * PULSE_VIOLENCE);
			if (GET_POS(victim) > POS_SITTING)
				GET_POS(victim) = POS_SITTING;
			to_char = "��������� $N2 �����, ������ $S � ���";
			to_vict = "��������� ��� �����, ������ ��� � ���";
			break;
		case 5:	// chest damaged, waits 1, speed/2
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 6);
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
			to_char = "��������� $N2 ��������";
			to_vict = "��������� ��� ��������";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 6:	// shield damaged, chest damaged, speed/2
			alt_equip(victim, WEAR_SHIELD, 100, 100);
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 6);
			to_char = "��������� $N2 ��������";
			to_vict = "��������� ��� ��������";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 7:	// srmor damaged, chest damaged, speed/2, HR-2
			alt_equip(victim, WEAR_BODY, 100, 100);
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 5);
			to_char = "��������� $N2 ��������";
			to_vict = "��������� ��� ��������";
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -2;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 8:	// chest damaged, no speed, no attack
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 5);
			to_char = "������ $N3 �� �����";
			to_vict = "��������� ��� ��������";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
			af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			af[1].type = SPELL_BATTLE;
			af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			haemorragia(victim, 20);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 9:	// chest damaged, speed/2, HR-2
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 4);
			to_char = "��������� $N3 �������� ������";
			to_vict = "������� ��� �����";
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -2;
			af[1].type = SPELL_BATTLE;
			af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			haemorragia(victim, 20);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 10:	// chest damaged, no speed, no attack
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 4);
			to_char = "������ $N3 �� �����";
			to_vict = "������� ��� �����";
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
			af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			af[1].type = SPELL_BATTLE;
			af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			haemorragia(victim, 40);
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		case 11:	// chest crushed, hits 0
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
			af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			dam *= ch->get_skill(SKILL_PUNCTUAL) / 2;
			haemorragia(victim, 50);
			to_char = "������ $N3 �� �����";
			to_vict = "��������� ��� �����";
			break;
		default:	// chest crushed, killing
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
			af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			dam *= ch->get_skill(SKILL_PUNCTUAL) / 2;
			haemorragia(victim, 60);
			to_char = "������ $N3 �� �����";
			to_vict = "���������� ��� �����";
			break;
		}
		break;
	case 8:
	case 9:		// HANDS
		switch (dam_critic)
		{
		case 1:
		case 2:
		case 3:
			return;
		case 4:	// hands damaged, weapon/shield putdown
			to_char = "�������� ������ $N1";
			to_vict = "������ ��� ����";
			if (GET_EQ(victim, WEAR_BOTHS))
				unequip_pos = WEAR_BOTHS;
			else if (GET_EQ(victim, WEAR_WIELD))
				unequip_pos = WEAR_WIELD;
			else if (GET_EQ(victim, WEAR_HOLD))
				unequip_pos = WEAR_HOLD;
			else if (GET_EQ(victim, WEAR_SHIELD))
				unequip_pos = WEAR_SHIELD;
			break;
		case 5:	// hands damaged, shield damaged/weapon putdown
			to_char = "�������� ������ $N1";
			to_vict = "������ ��� � ����";
			if (GET_EQ(victim, WEAR_SHIELD))
				alt_equip(victim, WEAR_SHIELD, 100, 100);
			else if (GET_EQ(victim, WEAR_BOTHS))
				unequip_pos = WEAR_BOTHS;
			else if (GET_EQ(victim, WEAR_WIELD))
				unequip_pos = WEAR_WIELD;
			else if (GET_EQ(victim, WEAR_HOLD))
				unequip_pos = WEAR_HOLD;
			break;

		case 6:	// hands damaged, HR-2, shield putdown
			to_char = "�������� ������ $N1";
			to_vict = "������� ��� ����";
			if (GET_EQ(victim, WEAR_SHIELD))
				unequip_pos = WEAR_SHIELD;
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -2;
			break;
		case 7:	// armor damaged, hand damaged if no armour
			if (GET_EQ(victim, WEAR_ARMS))
				alt_equip(victim, WEAR_ARMS, 100, 100);
			else
				alt_equip(victim, WEAR_HANDS, 100, 100);
			if (!GET_EQ(victim, WEAR_ARMS) && !GET_EQ(victim, WEAR_HANDS))
				dam *= (ch->get_skill(SKILL_PUNCTUAL) / 7);
			to_char = "�������� ����� $N1";
			to_vict = "��������� ��� ����";
			break;
		case 8:	// shield damaged, hands damaged, waits 1
			alt_equip(victim, WEAR_SHIELD, 100, 100);
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 7);
			to_char = "���������� $N3";
			to_vict = "��������� ��� ����";
			break;
		case 9:	// weapon putdown, hands damaged, waits 1d4
			WAIT_STATE(victim, number(2, 4) * PULSE_VIOLENCE);
			if (GET_EQ(victim, WEAR_BOTHS))
				unequip_pos = WEAR_BOTHS;
			else if (GET_EQ(victim, WEAR_WIELD))
				unequip_pos = WEAR_WIELD;
			else if (GET_EQ(victim, WEAR_HOLD))
				unequip_pos = WEAR_HOLD;
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 6);
			to_char = "���������� $N3";
			to_vict = "��������� ��� ����";
			break;
		case 10:	// hand damaged, no attack this
			if (!AFF_FLAGGED(victim, EAffectFlag::AFF_STOPRIGHT))
			{
				to_char = "�������� ����� $N1";
				to_vict = "����������� ��� ������ ����";
				af[0].type = SPELL_BATTLE;
				af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPRIGHT);
				af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
				af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			}
			else if (!AFF_FLAGGED(victim, EAffectFlag::AFF_STOPLEFT))
			{
				to_char = "�������� ����� $N1";
				to_vict = "����������� ��� ����� ����";
				af[0].type = SPELL_BATTLE;
				af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPLEFT);
				af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
				af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			}
			else
			{
				to_char = "������ $N3 �� �����";
				to_vict = "������ ��� �� �����";
				af[0].type = SPELL_BATTLE;
				af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
				af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
				af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			}
			haemorragia(victim, 20);
			break;
		default:	// no hand attack, no speed, dam*2 if >= 13
			if (!AFF_FLAGGED(victim, EAffectFlag::AFF_STOPRIGHT))
			{
				to_char = "�������� ������ $N1";
				to_vict = "����������� ��� ������ ����";
				af[0].type = SPELL_BATTLE;
				af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPRIGHT);
				af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
				af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			}
			else if (!AFF_FLAGGED(victim, EAffectFlag::AFF_STOPLEFT))
			{
				to_char = "�������� ������ $N1";
				to_vict = "����������� ��� ����� ����";
				af[0].type = SPELL_BATTLE;
				af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPLEFT);
				af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
				af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			}
			else
			{
				to_char = "������ $N3 �� �����";
				to_vict = "������ ��� �� �����";
				af[0].type = SPELL_BATTLE;
				af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
				af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
				af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			}
			af[1].type = SPELL_BATTLE;
			af[1].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			haemorragia(victim, 30);
			if (dam_critic >= 13)
				dam *= ch->get_skill(SKILL_PUNCTUAL) / 5;
			SET_AF_BATTLE(victim, EAF_SLOW);
			break;
		}
		break;
	default:		// HEAD
		switch (dam_critic)
		{
		case 1:
		case 2:
		case 3:
			// nothing
			return;
		case 4:	// waits 1d6
			WAIT_STATE(victim, number(2, 6) * PULSE_VIOLENCE);
			to_char = "�������� $N2 ��������";
			to_vict = "�������� ���� ��������";
			break;

		case 5:	// head damaged, cap putdown, waits 1, HR-2 if no cap
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
			if (GET_EQ(victim, WEAR_HEAD))
				unequip_pos = WEAR_HEAD;
			else
			{
				af[0].type = SPELL_BATTLE;
				af[0].location = APPLY_HITROLL;
				af[0].modifier = -2;
			}
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 4);
			to_char = "��������� $N2 ������";
			to_vict = "��������� ��� ������";
			break;
		case 6:	// head damaged
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -2;
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 4);
			to_char = "��������� $N2 ������";
			to_vict = "��������� ��� ������";
			break;
		case 7:	// cap damaged, waits 1d6, speed/2, HR-4
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
			alt_equip(victim, WEAR_HEAD, 100, 100);
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_HITROLL;
			af[0].modifier = -4;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_NOFLEE);
			to_char = "������ $N3 � ������";
			to_vict = "������ ��� � ������";
			break;
		case 8:	// cap damaged, hits 0
			WAIT_STATE(victim, 4 * PULSE_VIOLENCE);
			alt_equip(victim, WEAR_HEAD, 100, 100);
			//dam = GET_HIT(victim);
			dam *= ch->get_skill(SKILL_PUNCTUAL) / 2;
			to_char = "������ � $N1 ��������";
			to_vict = "������ � ��� ��������";
			haemorragia(victim, 20);
			break;
		case 9:	// head damaged, no speed, no attack
			af[0].type = SPELL_BATTLE;
			af[0].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af[0].duration = pc_duration(victim, 30, 0, 0, 0, 0);
			af[0].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			haemorragia(victim, 30);
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 3);
			to_char = "�������� $N3 � ����������";
			to_vict = "�������� ��� � ����������";
			break;
		case 10:	// head damaged, -1 INT/WIS/CHA
			dam *= (ch->get_skill(SKILL_PUNCTUAL) / 2);
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_INT;
			af[0].modifier = -1;
			af[0].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[0].battleflag = AF_DEADKEEP;
			af[1].type = SPELL_BATTLE;
			af[1].location = APPLY_WIS;
			af[1].modifier = -1;
			af[1].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[1].battleflag = AF_DEADKEEP;
			af[2].type = SPELL_BATTLE;
			af[2].location = APPLY_CHA;
			af[2].modifier = -1;
			af[2].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[2].battleflag = AF_DEADKEEP;
			af[3].type = SPELL_BATTLE;
			af[3].bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af[3].duration = pc_duration(victim, 30, 0, 0, 0, 0);
			af[3].battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			haemorragia(victim, 50);
			to_char = "������� � $N1 �����";
			to_vict = "������� � ��� �����";
			break;
		case 11:	// hits 0, WIS/2, INT/2, CHA/2
			dam *= ch->get_skill(SKILL_PUNCTUAL) / 2;
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_INT;
			af[0].modifier = -victim->get_int() / 2;
			af[0].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[0].battleflag = AF_DEADKEEP;
			af[1].type = SPELL_BATTLE;
			af[1].location = APPLY_WIS;
			af[1].modifier = -victim->get_wis() / 2;
			af[1].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[1].battleflag = AF_DEADKEEP;
			af[2].type = SPELL_BATTLE;
			af[2].location = APPLY_CHA;
			af[2].modifier = -victim->get_cha() / 2;
			af[2].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[2].battleflag = AF_DEADKEEP;
			haemorragia(victim, 60);
			to_char = "������� � $N1 �����";
			to_vict = "������� � ��� �����";
			break;
		default:	// killed
			af[0].type = SPELL_BATTLE;
			af[0].location = APPLY_INT;
			af[0].modifier = -victim->get_int() / 2;
			af[0].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[0].battleflag = AF_DEADKEEP;
			af[1].type = SPELL_BATTLE;
			af[1].location = APPLY_WIS;
			af[1].modifier = -victim->get_wis() / 2;
			af[1].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[1].battleflag = AF_DEADKEEP;
			af[2].type = SPELL_BATTLE;
			af[2].location = APPLY_CHA;
			af[2].modifier = -victim->get_cha() / 2;
			af[2].duration = pc_duration(victim, number(1, 6) * 24, 0, 0, 0, 0);
			af[2].battleflag = AF_DEADKEEP;
			dam *= ch->get_skill(SKILL_PUNCTUAL) / 2;
			to_char = "���������� $N2 ������";
			to_vict = "���������� ��� ������";
			haemorragia(victim, 90);
			break;
		}
		break;
	}

	for (int i = 0; i < 4; i++)
	{
		if (af[i].type)
		{
			if (victim->get_role(MOB_ROLE_BOSS)
				&& (af[i].bitvector == to_underlying(EAffectFlag::AFF_STOPFIGHT)
					|| af[i].bitvector == to_underlying(EAffectFlag::AFF_STOPRIGHT)
					|| af[i].bitvector == to_underlying(EAffectFlag::AFF_STOPLEFT)))
			{
				af[i].duration /= 5;
				// ��� ������ ���� ������ �� ���� �����, ��� �������� ����, ����� ��� ������.
				sh_int extra_duration = 0;
				OBJ_DATA* both = GET_EQ(ch, WEAR_BOTHS);
				OBJ_DATA* wield = GET_EQ(ch, WEAR_WIELD);
				if (both) {
					extra_duration = GET_OBJ_WEIGHT(both) / 5;
				} else if (wield) {
					extra_duration = GET_OBJ_WEIGHT(wield) / 5;
				}
				af[i].duration += pc_duration(victim, GET_REMORT(ch)/2 + extra_duration, 0, 0, 0, 0);
			}
			affect_join(victim, af[i], TRUE, FALSE, TRUE, FALSE);
		}
	}

	if (to_char)
	{
		sprintf(buf, "&G&q���� ������ ��������� %s.&Q&n", to_char);
		act(buf, FALSE, ch, 0, victim, TO_CHAR);
		sprintf(buf, "������ ��������� $n1 %s.", to_char);
		act(buf, TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
	}

	if (to_vict)
	{
		sprintf(buf, "&R&q������ ��������� $n1 %s.&Q&n", to_vict);
		act(buf, FALSE, ch, 0, victim, TO_VICT);
	}
	if (unequip_pos && GET_EQ(victim, unequip_pos))
	{
		obj = unequip_char(victim, unequip_pos);
		switch (unequip_pos)
		{
			case 6:		//WEAR_HEAD
				sprintf(buf, "%s ������%s � ����� ������.", obj->get_PName(0).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, FALSE, ch, 0, victim, TO_VICT);
				sprintf(buf, "%s ������%s � ������ $N1.", obj->get_PName(0).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, FALSE, ch, 0, victim, TO_CHAR);
				act(buf, TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
				break;

			case 11:	//WEAR_SHIELD
				sprintf(buf, "%s ������%s � ����� ����.", obj->get_PName(0).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, FALSE, ch, 0, victim, TO_VICT);
				sprintf(buf, "%s ������%s � ���� $N1.", obj->get_PName(0).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, FALSE, ch, 0, victim, TO_CHAR);
				act(buf, TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
				break;

			case 16:	//WEAR_WIELD
			case 17:	//WEAR_HOLD
				sprintf(buf, "%s �����%s �� ����� ����.", obj->get_PName(0).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, FALSE, ch, 0, victim, TO_VICT);
				sprintf(buf, "%s �����%s �� ���� $N1.", obj->get_PName(0).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, FALSE, ch, 0, victim, TO_CHAR);
				act(buf, TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
				break;

			case 18:	//WEAR_BOTHS
				sprintf(buf, "%s �����%s �� ����� ���.", obj->get_PName(0).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, FALSE, ch, 0, victim, TO_VICT);
				sprintf(buf, "%s �����%s �� ��� $N1.", obj->get_PName(0).c_str(), GET_OBJ_SUF_1(obj));
				act(buf, FALSE, ch, 0, victim, TO_CHAR);
				act(buf, TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
				break;
		}
		if (!IS_NPC(victim) && ROOM_FLAGGED(IN_ROOM(victim), ROOM_ARENA))
			obj_to_char(obj, victim);
		else
			obj_to_room(obj, IN_ROOM(victim));
		obj_decay(obj);
	}
	if (!IS_NPC(victim))
	{
		dam /= 5;
	}
	dam = calculate_resistance_coeff(victim, VITALITY_RESISTANCE, dam);
}

/**
* ������ ��������� ������ ����� � ������������� ����.
* �������: 1 + ((����-25)*0.4 + �����*0.2)/10 * ����/5,
* � ������ ��������� �������� �� 1 �� 2.6 � �����������
* �������������� 62.5% �� ���� � 37.5% �� ������ + ������ �� 5 �����.
* ����������� �� ��������� ��� �������� ����� � ���������.
*/
int calculate_strconc_damage(CHAR_DATA * ch, OBJ_DATA* /*wielded*/, int damage)
{
	if (IS_NPC(ch)
		|| GET_REAL_STR(ch) <= 25
		|| !can_use_feat(ch, STRENGTH_CONCETRATION_FEAT)
		|| GET_AF_BATTLE(ch, EAF_IRON_WIND)
		|| GET_AF_BATTLE(ch, EAF_STUPOR))
	{
		return damage;
	}
	const float str_mod = (GET_REAL_STR(ch) - 25) * 0.4;
	const float lvl_mod = GET_LEVEL(ch) * 0.2;
	const float rmt_mod = MIN(5, GET_REMORT(ch)) / 5.0;
	const float res_mod = 1 + (str_mod + lvl_mod) / 10.0 * rmt_mod;

	return static_cast<int>(damage * res_mod);
}

/**
* ������ �������� ������ �� �������� �����.
* (�����/5 + �������*3) * (�������/(10 + �������/2)) * (�����/30)
*/
int calculate_noparryhit_dmg(CHAR_DATA * ch, OBJ_DATA * wielded)
{
	if (!ch->get_skill(SKILL_NOPARRYHIT)) return 0;

	const float weap_dmg = (((GET_OBJ_VAL(wielded, 2) + 1) / 2.0) * GET_OBJ_VAL(wielded, 1));
	const float weap_mod = weap_dmg / (10 + weap_dmg / 2);
	const float level_mod = static_cast<float>(GET_LEVEL(ch)) / 30;
	const float skill_mod = static_cast<float>(ch->get_skill(SKILL_NOPARRYHIT)) / 5;

	return static_cast<int>((skill_mod + GET_REMORT(ch) * 3) * weap_mod * level_mod);
}

void might_hit_bash(CHAR_DATA *ch, CHAR_DATA *victim)
{
	if (MOB_FLAGGED(victim, MOB_NOBASH) || !GET_MOB_HOLD(victim))
	{
		return;
	}

	act("$n ��������� �������$u �� �����.", TRUE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
	WAIT_STATE(victim, 3 * PULSE_VIOLENCE);

	if (GET_POS(victim) > POS_SITTING)
	{
		GET_POS(victim) = POS_SITTING;
		send_to_char(victim, "&R&q����������� ���� %s ���� ��� � ���.&Q&n\r\n", PERS(ch, victim, 1));
	}
}

bool check_mighthit_weapon(CHAR_DATA *ch)
{
	if (!GET_EQ(ch, WEAR_BOTHS)
		&& !GET_EQ(ch, WEAR_WIELD)
		&& !GET_EQ(ch, WEAR_HOLD)
		&& !GET_EQ(ch, WEAR_LIGHT)
		&& !GET_EQ(ch, WEAR_SHIELD))
	{
			return true;
	}
	return false;
}

// * ��� ������ ���� � 1.5 � �� ���� 1% ����, ��� ���� ����� ������ ����� ������.
void try_remove_extrahits(CHAR_DATA *ch, CHAR_DATA *victim)
{
	if (((!IS_NPC(ch) && ch != victim)
			|| (ch->has_master()
				&& !IS_NPC(ch->get_master())
				&& ch->get_master() != victim))
		&& !IS_NPC(victim)
		&& GET_POS(victim) != POS_DEAD
		&& GET_HIT(victim) > GET_REAL_MAX_HIT(victim) * 1.5
		&& number(1, 100) == 5)// ����� ����� 5, � �� 1 �� ������������ ��������� �������� ���-�� �����
	{
		GET_HIT(victim) = GET_REAL_MAX_HIT(victim);
		send_to_char(victim, "%s'����%s ���%s ��� ������' - ��������� ����� ����� � ����� ������.%s\r\n",
				CCWHT(victim, C_NRM), GET_CH_POLY_1(victim), GET_CH_EXSUF_1(victim), CCNRM(victim, C_NRM));
		act("�� �������� ���������� ����, �������� $N3 ������.", FALSE, ch, 0, victim, TO_CHAR);
		act("$n �������$g ���������� ����, �������� $N3 ������.", FALSE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
	}
}

/**
* ������� �������������� ���������.
* �������� ��� 100 ������ � 21 �����: 100  60 30 10
* �������� ��� 200 ������ � 50 �����: 100 100 70 30
* �� ������� 70% �� �������� ���������.
* � �������� 100% � 60% ��� ��������� �����.
*/
void addshot_damage(CHAR_DATA * ch, int type, int weapon)
{
	int prob = train_skill(ch, SKILL_ADDSHOT, skill_info[SKILL_ADDSHOT].max_percent, ch->get_fighting());

	// ����� ������ ������ ���� 21 (��������� �������� �����) � �� 50
	float dex_mod = static_cast<float>(MAX(MIN(GET_REAL_DEX(ch), 50) - 21, 0)) / 29 * 2;  //�������� ��� � 50 ����� ��� �����������
	// ����� �� 4� � 5� �������� ��� ����� ���� 5 ������, �� 20% �� ����
	float remort_mod = static_cast<float>(GET_REMORT(ch)) / 5;
	if (remort_mod > 1) remort_mod = 1;
	// �� ���� ����������� ��������� �� 70% �� �������� ��������� ������� ����
	const float sit_mod = (GET_POS(ch) >= POS_FIGHTING) ? 1 : 0.7;

	// � �������� ������� ������ �� 100+ ������ � �������� 2 ��� �����
	if (IS_CHARMICE(ch))
	{
		prob = MIN(100, prob);
		dex_mod = 0;
		remort_mod = 0;
	}

	// ���� 100% ���� ������ �������
	// ����� ���� 100 ��������� ������ ����� ������� � ���������
	// � ��� ������ �� ����� ��� �������� ��� ����
	prob = MIN(100, prob);

	int percent = number(1, skill_info[SKILL_ADDSHOT].max_percent);
	// 1� ��� - �� ����� 100% ��� ������ 100+
	if (prob * sit_mod >= percent / 2)
		hit(ch, ch->get_fighting(), type, weapon);

	percent = number(1, skill_info[SKILL_ADDSHOT].max_percent);
	// 2� ��� - 60% ��� ������ 100, �� 100% ��� ��������� ������ � �����
	if ((prob * 3 + dex_mod * 100) * sit_mod > percent * 5 / 2 && ch->get_fighting())
		hit(ch, ch->get_fighting(), type, weapon);

	percent = number(1, skill_info[SKILL_ADDSHOT].max_percent);
	// 3� ��� - 30% ��� ������ 100, �� 70% ��� ��������� ������ � ����� (��� 5+ ������)
	if ((prob * 3 + dex_mod * 200) * remort_mod * sit_mod > percent * 5 && ch->get_fighting())
		hit(ch, ch->get_fighting(), type, weapon);

	percent = number(1, skill_info[SKILL_ADDSHOT].max_percent);
	// 4� ��� - 20% ��� ������ 100, �� 60% ��� ��������� ������ � ����� (��� 5+ ������)
	if ((prob + dex_mod * 100) * remort_mod * sit_mod > percent * 5 / 2 && ch->get_fighting())
		hit(ch, ch->get_fighting(), type, weapon);
}

// ������/������ ������� �� ������ ������������ ����� ������
void apply_weapon_bonus(int ch_class, const ESkill skill, int *damroll, int *hitroll)
{
	int dam = *damroll;
	int calc_thaco = *hitroll;

	switch (ch_class)
	{
	case CLASS_CLERIC:
		switch (skill)
		{
		case SKILL_CLUBS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_AXES:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_LONGS:
			calc_thaco += 2;
			dam -= 1;
			break;
		case SKILL_SHORTS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_NONSTANDART:
			calc_thaco += 1;
			dam -= 2;
			break;
		case SKILL_BOTHHANDS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_PICK:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_SPADES:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_BOWS:
			calc_thaco -= 0;
			dam += 0;
			break;

		default:
			break;
		}
		break;

	case CLASS_BATTLEMAGE:
	case CLASS_DEFENDERMAGE:
	case CLASS_CHARMMAGE:
	case CLASS_NECROMANCER:
		switch (skill)
		{
		case SKILL_CLUBS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_AXES:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_LONGS:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_SHORTS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_NONSTANDART:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_BOTHHANDS:
			calc_thaco += 1;
			dam -= 3;
			break;
		case SKILL_PICK:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_SPADES:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_BOWS:
			calc_thaco -= 0;
			dam += 0;
			break;
		default:
			break;
		}
		break;

	case CLASS_WARRIOR:
		switch (skill)
		{
		case SKILL_CLUBS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_AXES:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_LONGS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_SHORTS:
			calc_thaco += 2;
			dam += 0;
			break;
		case SKILL_NONSTANDART:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_BOTHHANDS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_PICK:
			calc_thaco += 2;
			dam += 0;
			break;
		case SKILL_SPADES:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_BOWS:
			calc_thaco -= 0;
			dam += 0;
			break;
		default:
			break;
		}
		break;

	case CLASS_RANGER:
		switch (skill)
		{
		case SKILL_CLUBS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_AXES:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_LONGS:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_SHORTS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_NONSTANDART:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_BOTHHANDS:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_PICK:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_SPADES:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_BOWS:
			calc_thaco -= 0;
			dam += 0;
			break;
		default:
			break;
		}
		break;

	case CLASS_GUARD:
	case CLASS_THIEF:
		switch (skill)
		{
		case SKILL_CLUBS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_AXES:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_LONGS:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_SHORTS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_NONSTANDART:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_BOTHHANDS:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_PICK:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_SPADES:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_BOWS:
			calc_thaco -= 0;
			dam += 0;
			break;
		default:
			break;
		}
		break;

	case CLASS_ASSASINE:
		switch (skill)
		{
		case SKILL_CLUBS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_AXES:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_LONGS:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_SHORTS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_NONSTANDART:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_BOTHHANDS:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_PICK:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_SPADES:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_BOWS:
			calc_thaco -= 0;
			dam += 0;
			break;
		default:
			break;
		}
		break;
		/*	case CLASS_PALADINE:
			case CLASS_SMITH:
				switch (skill) {
					case SKILL_CLUBS:	calc_thaco -= 0; dam += 0; break;
					case SKILL_AXES:	calc_thaco -= 0; dam += 0; break;
					case SKILL_LONGS:	calc_thaco -= 0; dam += 0; break;
					case SKILL_SHORTS:	calc_thaco -= 0; dam += 0; break;
					case SKILL_NONSTANDART:	calc_thaco -= 0; dam += 0; break;
					case SKILL_BOTHHANDS:	calc_thaco -= 0; dam += 0; break;
					case SKILL_PICK:	calc_thaco -= 0; dam += 0; break;
					case SKILL_SPADES:	calc_thaco -= 0; dam += 0; break;
					case SKILL_BOWS:	calc_thaco -= 0; dam += 0; break;
				}
				break; */
	case CLASS_MERCHANT:
		switch (skill)
		{
		case SKILL_CLUBS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_AXES:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_LONGS:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_SHORTS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_NONSTANDART:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_BOTHHANDS:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_PICK:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_SPADES:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_BOWS:
			calc_thaco -= 0;
			dam += 0;
			break;
		default:
			break;
		}
		break;

	case CLASS_DRUID:
		switch (skill)
		{
		case SKILL_CLUBS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_AXES:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_LONGS:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_SHORTS:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_NONSTANDART:
			calc_thaco -= 0;
			dam += 0;
			break;
		case SKILL_BOTHHANDS:
			calc_thaco += 1;
			dam += 0;
			break;
		case SKILL_PICK:
			calc_thaco += 0;
			dam += 0;
			break;
		case SKILL_SPADES:
			calc_thaco += 0;
			dam += 0;
			break;
		case SKILL_BOWS:
			calc_thaco += 1;
			dam += 0;
			break;
		default:
			break;
		}
		break;
	}

	*damroll = dam;
	*hitroll = calc_thaco;
}

int do_punctual(CHAR_DATA *ch, CHAR_DATA* /*victim*/, OBJ_DATA *wielded)
{
	int dam_critic = 0, wapp = 0;

	if (wielded)
	{
		wapp = (int)(GET_OBJ_SKILL(wielded) == SKILL_BOWS) ?
		   GET_OBJ_WEIGHT(wielded) * 1 / 3 : GET_OBJ_WEIGHT(wielded);
	}

	if (wapp < 10)
		dam_critic = dice(1, 6);
	else if (wapp < 19)
		dam_critic = dice(2, 5);
	else if (wapp < 27)
		dam_critic = dice(3, 4);
	else if (wapp < 36)
		dam_critic = dice(3, 5);
	else if (wapp < 44)
		dam_critic = dice(3, 6);
	else
		dam_critic = dice(4, 5);

	const int skill = 1 + ch->get_skill(SKILL_PUNCTUAL) / 6;
	dam_critic = MIN(number(4, skill), dam_critic);

	return dam_critic;
}

// * ��������� ������ ��� �����.
int backstab_mult(int level)
{
	if (level <= 0)
		return 1;	// level 0 //
	else if (level <= 5)
		return 2;	// level 1 - 5 //
	else if (level <= 10)
		return 3;	// level 6 - 10 //
	else if (level <= 15)
		return 4;	// level 11 - 15 //
	else if (level <= 20)
		return 5;	// level 16 - 20 //
	else if (level <= 25)
		return 6;	// level 21 - 25 //
	else if (level <= 30)
		return 7;	// level 26 - 30 //
	else
		return 10;
}

/**
* ������� ����������� ����.����� = �����/11 + (�����-20)/(�����/30)
* �������� �� 50% �� ������ � �����, �������� 36,18%.
*/
int calculate_crit_backstab_percent(CHAR_DATA *ch)
{
	return static_cast<int>(ch->get_skill(SKILL_BACKSTAB)/11.0 + (GET_REAL_DEX(ch) - 20) / (GET_REAL_DEX(ch) / 30.0));
}

// * ������ ��������� ����.����� (�� ������� ������ ��� �����).
double HitData::crit_backstab_multiplier(CHAR_DATA *ch, CHAR_DATA *victim)
{
	double bs_coeff = 1.0;
	if (IS_NPC(victim))
	{
		if (can_use_feat(ch, THIEVES_STRIKE_FEAT))
		{
			bs_coeff *= ch->get_skill(SKILL_BACKSTAB) / 20.0;
		}
		else
		{
			bs_coeff *= ch->get_skill(SKILL_BACKSTAB) / 25.0;
		}

		// ������ ������� �� ��������: ���� ������� ����� ���� � ����� ���������� (��� ����), �� �����
		// ������ ��� ������ ����� ���� ����� �����������. ������, � ���� ������� ����� �� ����
		// ������� ��� �����. ����� ��� ���������
		// ���������, ���� �� ��� �����
		if (can_use_feat(ch, SHADOW_STRIKE_FEAT)
			&& (ch->get_skill(SKILL_NOPARRYHIT)))
		{
			bs_coeff *= (1 + (ch->get_skill(SKILL_NOPARRYHIT) * 0.00125));
		}
	}
	else if (can_use_feat(ch, THIEVES_STRIKE_FEAT))
	{
		if (victim->get_fighting()) //���� ���� � ��� ����� 1.125
		{
			bs_coeff *= (1.0 + (ch->get_skill(SKILL_BACKSTAB) * 0.00125));
			// ���� ����� 1.25
		}
		else
		{
			bs_coeff *= (1.0 + (ch->get_skill(SKILL_BACKSTAB) * 0.00250));
		}
		// ����� � ������ ��� ����� �������,
		// ����� ����� ��� �����-����� �������������
		set_flag(FightSystem::IGNORE_SANCT);
		set_flag(FightSystem::IGNORE_PRISM);
	}

	if (bs_coeff < 1.0)
	{
		bs_coeff = 1.0;
	}

	return bs_coeff;
}

// * ����� �� �������� ����������� ����� ��������� (������ � ������ ������, ��� ����� �����).
bool can_auto_block(CHAR_DATA *ch)
{
	if (GET_EQ(ch, WEAR_SHIELD) && GET_AF_BATTLE(ch, EAF_AWAKE) && GET_AF_BATTLE(ch, EAF_AUTOBLOCK))
		return true;
	else
		return false;
}

// * �������� �� ��� "������� ������".
void HitData::check_weap_feats(CHAR_DATA *ch)
{
	switch (weap_skill)
	{
	case SKILL_PUNCH:
		if (HAVE_FEAT(ch, PUNCH_FOCUS_FEAT))
		{
			calc_thaco -= 2;
			dam += 2;
		}
		break;
	case SKILL_CLUBS:
		if (HAVE_FEAT(ch, CLUB_FOCUS_FEAT))
		{
			calc_thaco -= 2;
			dam += 2;
		}
		break;
	case SKILL_AXES:
		if (HAVE_FEAT(ch, AXES_FOCUS_FEAT))
		{
			calc_thaco -= 1;
			dam += 2;
		}
		break;
	case SKILL_LONGS:
		if (HAVE_FEAT(ch, LONGS_FOCUS_FEAT))
		{
			calc_thaco -= 1;
			dam += 2;
		}
		break;
	case SKILL_SHORTS:
		if (HAVE_FEAT(ch, SHORTS_FOCUS_FEAT))
		{
			calc_thaco -= 2;
			dam += 3;
		}
		break;
	case SKILL_NONSTANDART:
		if (HAVE_FEAT(ch, NONSTANDART_FOCUS_FEAT))
		{
			calc_thaco -= 1;
			dam += 3;
		}
		break;
	case SKILL_BOTHHANDS:
		if (HAVE_FEAT(ch, BOTHHANDS_FOCUS_FEAT))
		{
			calc_thaco -= 1;
			dam += 3;
		}
		break;
	case SKILL_PICK:
		if (HAVE_FEAT(ch, PICK_FOCUS_FEAT))
		{
			calc_thaco -= 2;
			dam += 3;
		}
		break;
	case SKILL_SPADES:
		if (HAVE_FEAT(ch, SPADES_FOCUS_FEAT))
		{
			calc_thaco -= 1;
			dam += 2;
		}
		break;
	case SKILL_BOWS:
		if (HAVE_FEAT(ch, BOWS_FOCUS_FEAT))
		{
			calc_thaco -= 7;
			dam += 4;
		}
		break;
	default:
		break;
	}
}

// * ������.
void hit_touching(CHAR_DATA *ch, CHAR_DATA *vict, int *dam)
{
	if (vict->get_touching() == ch
		&& !AFF_FLAGGED(vict, EAffectFlag::AFF_STOPFIGHT)
		&& !AFF_FLAGGED(vict, EAffectFlag::AFF_MAGICSTOPFIGHT)
		&& !AFF_FLAGGED(vict, EAffectFlag::AFF_STOPRIGHT)
		&& GET_WAIT(vict) <= 0
		&& !GET_MOB_HOLD(vict)
		&& (IS_IMMORTAL(vict) || IS_NPC(vict)
			|| !(GET_EQ(vict, WEAR_WIELD) || GET_EQ(vict, WEAR_BOTHS)))
		&& GET_POS(vict) > POS_SLEEPING)
	{
		int percent = number(1, skill_info[SKILL_TOUCH].max_percent);
		int prob = train_skill(vict, SKILL_TOUCH, skill_info[SKILL_TOUCH].max_percent, ch);
		if (IS_IMMORTAL(vict) || GET_GOD_FLAG(vict, GF_GODSLIKE))
		{
			percent = prob;
		}
		if (GET_GOD_FLAG(vict, GF_GODSCURSE))
		{
			percent = 0;
		}
		CLR_AF_BATTLE(vict, EAF_TOUCH);
		SET_AF_BATTLE(vict, EAF_USEDRIGHT);
		vict->set_touching(0);
		if (prob < percent)
		{
			act("�� �� ������ ����������� ����� $N1.", FALSE, vict, 0, ch, TO_CHAR);
			act("$N �� ����$Q ����������� ���� �����.", FALSE, ch, 0, vict, TO_CHAR);
			act("$n �� ����$q ����������� ����� $N1.", TRUE, vict, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			prob = 2;
		}
		else
		{
			act("�� ����������� ����� $N1.", FALSE, vict, 0, ch, TO_CHAR);
			act("$N ����������$G ���� �����.", FALSE, ch, 0, vict, TO_CHAR);
			act("$n ����������$g ����� $N1.", TRUE, vict, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			*dam = -1;
			prob = 1;
		}
		if (!WAITLESS(vict))
		{
			WAIT_STATE(vict, prob * PULSE_VIOLENCE);
		}
	}
}

void hit_deviate(CHAR_DATA *ch, CHAR_DATA *victim, int *dam)
{
	const int range = number(1, skill_info[SKILL_DEVIATE].max_percent);
	int prob = train_skill(victim, SKILL_DEVIATE, skill_info[SKILL_DEVIATE].max_percent, ch);
	if (GET_GOD_FLAG(victim, GF_GODSCURSE))
	{
		prob = 0;
	}
	prob = prob * 100 / range;
	if (check_spell_on_player(victim, SPELL_WEB))
	{
		prob /= 3;
	}
	if (prob < 60)
	{
		act("�� �� ������ ���������� �� ����� $N1", FALSE, victim, 0, ch, TO_CHAR);
		act("$N �� �����$G ���������� �� ����� �����", FALSE, ch, 0, victim, TO_CHAR);
		act("$n �� �����$g ���������� �� ����� $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
                SET_AF_BATTLE(victim, EAF_DEVIATE);
}
	else if (prob < 100)
	{
		act("�� ������� ���������� �� ����� $N1", FALSE, victim, 0, ch, TO_CHAR);
		act("$N ������� �������$U �� ����� �����", FALSE, ch, 0, victim, TO_CHAR);
		act("$n ������� �������$u �� ����� $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
		*dam = *dam * 10 / 15;
                SET_AF_BATTLE(victim, EAF_DEVIATE);
	}
	else if (prob < 200)
	{
		act("�� �������� ���������� �� ����� $N1", FALSE, victim, 0, ch, TO_CHAR);
		act("$N �������� �������$U �� ����� �����", FALSE, ch, 0, victim, TO_CHAR);
		act("$n �������� �������$u �� ����� $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
		*dam = *dam / 2;
                SET_AF_BATTLE(victim, EAF_DEVIATE);
	}
	else
	{
		act("�� ���������� �� ����� $N1", FALSE, victim, 0, ch, TO_CHAR);
		act("$N �������$U �� ����� �����", FALSE, ch, 0, victim, TO_CHAR);
		act("$n �������$u �� ����� $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
		*dam = -1;
                SET_AF_BATTLE(victim, EAF_DEVIATE);
	}
	BATTLECNTR(victim)++;
}

void hit_parry(CHAR_DATA *ch, CHAR_DATA *victim, int skill, int hit_type, int *dam)
{
	if (!((GET_EQ(victim, WEAR_WIELD)
			&& GET_OBJ_TYPE(GET_EQ(victim, WEAR_WIELD)) == OBJ_DATA::ITEM_WEAPON
			&& GET_EQ(victim, WEAR_HOLD)
			&& GET_OBJ_TYPE(GET_EQ(victim, WEAR_HOLD)) == OBJ_DATA::ITEM_WEAPON)
		|| IS_NPC(victim)
		|| IS_IMMORTAL(victim)))
	{
		send_to_char("� ��� ����� ��������� ����� ����������\r\n", victim);
		CLR_AF_BATTLE(victim, EAF_PARRY);
	}
	else
	{
		const int range = number(1, skill_info[SKILL_PARRY].max_percent);
		int prob = train_skill(victim, SKILL_PARRY,
				skill_info[SKILL_PARRY].max_percent, ch);
		prob = prob * 100 / range;

		if (prob < 70
			|| ((skill == SKILL_BOWS || hit_type == FightSystem::type_maul)
				&& !IS_IMMORTAL(victim)
				&& (!can_use_feat(victim, PARRY_ARROW_FEAT)
				|| number(1, 1000) >= 20 * MIN(GET_REAL_DEX(victim), 35))))
		{
			act("�� �� ������ ������ ����� $N1", FALSE, victim, 0, ch, TO_CHAR);
			act("$N �� �����$G ������ ���� �����", FALSE, ch, 0, victim, TO_CHAR);
			act("$n �� �����$g ������ ����� $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			prob = 2;
			SET_AF_BATTLE(victim, EAF_USEDLEFT);
		}
		else if (prob < 100)
		{
			act("�� ������� ��������� ����� $N1", FALSE, victim, 0, ch, TO_CHAR);
			act("$N ������� ��������$G ���� �����", FALSE, ch, 0, victim, TO_CHAR);
			act("$n ������� ��������$g ����� $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			alt_equip(victim, number(0, 2) ? WEAR_WIELD : WEAR_HOLD, *dam, 10);
			prob = 1;
			*dam = *dam * 10 / 15;
			SET_AF_BATTLE(victim, EAF_USEDLEFT);
		}
		else if (prob < 170)
		{
			act("�� �������� ��������� ����� $N1", FALSE, victim, 0, ch, TO_CHAR);
			act("$N �������� ��������$G ���� �����", FALSE, ch, 0, victim, TO_CHAR);
			act("$n �������� ��������$g ����� $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			alt_equip(victim, number(0, 2) ? WEAR_WIELD : WEAR_HOLD, *dam, 15);
			prob = 0;
			*dam = *dam / 2;
			SET_AF_BATTLE(victim, EAF_USEDLEFT);
		}
		else
		{
			act("�� ��������� ��������� ����� $N1", FALSE, victim, 0, ch, TO_CHAR);
			act("$N ��������� ��������$G ���� �����", FALSE, ch, 0, victim, TO_CHAR);
			act("$n ��������� ��������$g ����� $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			alt_equip(victim, number(0, 2) ? WEAR_WIELD : WEAR_HOLD, *dam, 25);
			prob = 0;
			*dam = -1;
		}
		if (!WAITLESS(ch) && prob)
		{
			WAIT_STATE(victim, PULSE_VIOLENCE * prob);
		}
		CLR_AF_BATTLE(victim, EAF_PARRY);
	}
}

void hit_multyparry(CHAR_DATA *ch, CHAR_DATA *victim, int skill, int hit_type, int *dam)
{
	if (!((GET_EQ(victim, WEAR_WIELD)
			&& GET_OBJ_TYPE(GET_EQ(victim, WEAR_WIELD)) == OBJ_DATA::ITEM_WEAPON
			&& GET_EQ(victim, WEAR_HOLD)
			&& GET_OBJ_TYPE(GET_EQ(victim, WEAR_HOLD)) == OBJ_DATA::ITEM_WEAPON)
		|| IS_NPC(victim)
		|| IS_IMMORTAL(victim)))
	{
		send_to_char("� ��� ����� ��������� ����� �����������\r\n", victim);
	}
	else
	{
		const int range = number(1,
				skill_info[SKILL_MULTYPARRY].max_percent) + 15 * BATTLECNTR(victim);
		int prob = train_skill(victim, SKILL_MULTYPARRY,
				skill_info[SKILL_MULTYPARRY].max_percent + BATTLECNTR(ch) * 15, ch);
		prob = prob * 100 / range;

		if ((skill == SKILL_BOWS || hit_type == FightSystem::type_maul)
			&& !IS_IMMORTAL(victim)
			&& (!can_use_feat(victim, PARRY_ARROW_FEAT)
				|| number(1, 1000) >= 20 * MIN(GET_REAL_DEX(victim), 35)))
		{
			prob = 0;
		}
		else
		{
			BATTLECNTR(victim)++;
		}

		if (prob < 50)
		{
			act("�� �� ������ ������ ����� $N1", FALSE, victim, 0, ch, TO_CHAR);
			act("$N �� �����$G ������ ���� �����", FALSE, ch, 0, victim, TO_CHAR);
			act("$n �� �����$g ������ ����� $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
		}
		else if (prob < 90)
		{
			act("�� ������� ��������� ����� $N1", FALSE, victim, 0, ch, TO_CHAR);
			act("$N ������� ��������$G ���� �����", FALSE, ch, 0, victim, TO_CHAR);
			act("$n ������� ��������$g ����� $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			alt_equip(victim, number(0, 2) ? WEAR_WIELD : WEAR_HOLD, *dam, 10);
			*dam = *dam * 10 / 15;
		}
		else if (prob < 180)
		{
			act("�� �������� ��������� ����� $N1", FALSE, victim, 0, ch, TO_CHAR);
			act("$N �������� ��������$G ���� �����", FALSE, ch, 0, victim, TO_CHAR);
			act("$n �������� ��������$g ����� $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			alt_equip(victim, number(0, 2) ? WEAR_WIELD : WEAR_HOLD, *dam, 15);
			*dam = *dam / 2;
		}
		else
		{
			act("�� ��������� ��������� ����� $N1", FALSE, victim, 0, ch, TO_CHAR);
			act("$N ��������� ��������$G ���� �����", FALSE, ch, 0, victim, TO_CHAR);
			act("$n ��������� ��������$g ����� $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			alt_equip(victim, number(0, 2) ? WEAR_WIELD : WEAR_HOLD, *dam, 25);
			*dam = -1;
		}
	}
}

void hit_block(CHAR_DATA *ch, CHAR_DATA *victim, int *dam)
{
	if (!(GET_EQ(victim, WEAR_SHIELD)
		|| IS_NPC(victim)
		|| IS_IMMORTAL(victim)))
	{
		send_to_char("� ��� ����� �������� ����� ����������\r\n", victim);
	}
	else
	{
		const int range = number(1, skill_info[SKILL_BLOCK].max_percent);
		int prob =
			train_skill(victim, SKILL_BLOCK, skill_info[SKILL_BLOCK].max_percent, ch);
		prob = prob * 100 / range;
		BATTLECNTR(victim)++;
		if (prob < 100)
		{
			act("�� �� ������ �������� ����� $N1", FALSE, victim, 0, ch, TO_CHAR);
			act("$N �� �����$G �������� ���� �����", FALSE, ch, 0, victim, TO_CHAR);
			act("$n �� �����$g �������� ����� $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
		}
		else if (prob < 150)
		{
			act("�� ������� �������� ����� $N1", FALSE, victim, 0, ch, TO_CHAR);
			act("$N ������� �������$G ���� �����", FALSE, ch, 0, victim, TO_CHAR);
			act("$n ������� �������$g ����� $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			alt_equip(victim, WEAR_SHIELD, *dam, 10);
			*dam = *dam * 10 / 15;
		}
		else if (prob < 250)
		{
			act("�� �������� �������� ����� $N1", FALSE, victim, 0, ch, TO_CHAR);
			act("$N �������� �������$G ���� �����", FALSE, ch, 0, victim, TO_CHAR);
			act("$n �������� �������$g ����� $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			alt_equip(victim, WEAR_SHIELD, *dam, 15);
			*dam = *dam / 2;
		}
		else
		{
			act("�� ��������� �������� ����� $N1", FALSE, victim, 0, ch, TO_CHAR);
			act("$N ��������� �������$G ���� �����", FALSE, ch, 0, victim, TO_CHAR);
			act("$n ��������� �������$g ����� $N1", TRUE, victim, 0, ch, TO_NOTVICT | TO_ARENA_LISTEN);
			alt_equip(victim, WEAR_SHIELD, *dam, 25);
			*dam = -1;
		}
	}
}

void appear(CHAR_DATA * ch)
{
	const bool appear_msg = AFF_FLAGGED(ch, EAffectFlag::AFF_INVISIBLE)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_CAMOUFLAGE)
		|| AFF_FLAGGED(ch, EAffectFlag::AFF_HIDE);

	if (affected_by_spell(ch, SPELL_INVISIBLE))
		affect_from_char(ch, SPELL_INVISIBLE);
	if (affected_by_spell(ch, SPELL_HIDE))
		affect_from_char(ch, SPELL_HIDE);
	if (affected_by_spell(ch, SPELL_SNEAK))
		affect_from_char(ch, SPELL_SNEAK);
	if (affected_by_spell(ch, SPELL_CAMOUFLAGE))
		affect_from_char(ch, SPELL_CAMOUFLAGE);

	AFF_FLAGS(ch).unset(EAffectFlag::AFF_INVISIBLE);
	AFF_FLAGS(ch).unset(EAffectFlag::AFF_HIDE);
	AFF_FLAGS(ch).unset(EAffectFlag::AFF_SNEAK);
	AFF_FLAGS(ch).unset(EAffectFlag::AFF_CAMOUFLAGE);

	if (appear_msg)
	{
		if (IS_NPC(ch)
			|| GET_LEVEL(ch) < LVL_IMMORT)
		{
			act("$n �������� ������$u �� �������.", FALSE, ch, 0, 0, TO_ROOM);
		}
		else
		{
			act("�� ������������� �������� ����������� $n1.", FALSE, ch, 0, 0, TO_ROOM);
		}
	}
}

// message for doing damage with a weapon
void Damage::dam_message(CHAR_DATA* ch, CHAR_DATA* victim) const
{
	int dam_msgnum;
	int w_type = msg_num;

	static const struct dam_weapon_type
	{
		const char *to_room;
		const char *to_char;
		const char *to_victim;
	} dam_weapons[] =
	{

		// use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes")

		{
			"$n �������$u #W $N3, �� ���������$u.",	// 0: 0      0 //
			"�� ���������� #W $N3, �� ������������.",
			"$n �������$u #W ���, �� ���������$u."
		}, {
			"$n �������� #w$g $N3.",	//  1..5 1 //
			"�� �������� #w� $N3.",
			"$n �������� #w$g ���."
		}, {
			"$n ������ #w$g $N3.",	//  6..11  2 //
			"�� ������ #w� $N3.",
			"$n ������ #w$g ���."
		}, {
			"$n #w$g $N3.",	//  12..18   3 //
			"�� #w� $N3.",
			"$n #w$g ���."
		}, {
			"$n #w$g $N3.",	// 19..26  4 //
			"�� #w� $N3.",
			"$n #w$g ���."
		}, {
			"$n ������ #w$g $N3.",	// 27..35  5 //
			"�� ������ #w� $N3.",
			"$n ������ #w$g ���."
		}, {
			"$n ����� ������ #w$g $N3.",	//  36..45 6  //
			"�� ����� ������ #w� $N3.",
			"$n ����� ������ #w$g ���."
		}, {
			"$n ����������� ������ #w$g $N3.",	//  46..55  7 //
			"�� ����������� ������ #w� $N3.",
			"$n ����������� ������ #w$g ���."
		}, {
			"$n ������ #w$g $N3.",	//  56..96   8 //
			"�� ������ #w� $N3.",
			"$n ������ #w$g ���."
		}, {
			"$n ����� ������ #w$g $N3.",	//    97..136  9  //
			"�� ����� ������ #w� $N3.",
			"$n ����� ������ #w$g ���."
		}, {
			"$n ����������� ������ #w$g $N3.",	//   137..176  10 //
			"�� ����������� ������ #w� $N3.",
			"$n ����������� ������ #w$g ���."
		}, {
			"$n ���������� ������ #w$g $N3.",	//    177..216  11 //
			"�� ���������� ������ #w� $N3.",
			"$n ���������� ������ #w$g ���."
		}, {
			"$n ������� #w$g $N3.",	//    217..256  13 //
			"�� ������� #w� $N3.",
			"$n ������� #w$g ���."
		}, {
			"$n ������ #w$g $N3.",	//    257..296  13 //
			"�� ������ #w� $N3.",
			"$n ������ #w$g ���."
		}, {
			"$n ����������� #w$g $N3.",	//    297..400  15 //
			"�� ����������� #w� $N3.",
			"$n ����������� #w$g ���."
		}, {
			"$n ���������� #w$g $N3.",	// 400+  16 //
			"�� ���������� #w� $N3.",
			"$n ���������� #w$g ���."
		}
	};

	if (w_type >= TYPE_HIT && w_type < TYPE_MAGIC)
		w_type -= TYPE_HIT;	// Change to base of table with text //
	else
		w_type = TYPE_HIT;

	if (dam == 0)
		dam_msgnum = 0;
	else if (dam <= 5)
		dam_msgnum = 1;
	else if (dam <= 11)
		dam_msgnum = 2;
	else if (dam <= 18)
		dam_msgnum = 3;
	else if (dam <= 26)
		dam_msgnum = 4;
	else if (dam <= 35)
		dam_msgnum = 5;
	else if (dam <= 45)
		dam_msgnum = 6;
	else if (dam <= 56)
		dam_msgnum = 7;
	else if (dam <= 96)
		dam_msgnum = 8;
	else if (dam <= 136)
		dam_msgnum = 9;
	else if (dam <= 176)
		dam_msgnum = 10;
	else if (dam <= 216)
		dam_msgnum = 11;
	else if (dam <= 256)
		dam_msgnum = 12;
	else if (dam <= 296)
		dam_msgnum = 13;
	else if (dam <= 400)
		dam_msgnum = 14;
	else
		dam_msgnum = 15;
	// damage message to onlookers
	char *buf_ptr = replace_string(dam_weapons[dam_msgnum].to_room,
		attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
	if (brief_shields_.empty())
	{
		act(buf_ptr, FALSE, ch, NULL, victim, TO_NOTVICT | TO_ARENA_LISTEN);
	}
	else
	{
		char buf_[MAX_INPUT_LENGTH];
		snprintf(buf_, sizeof(buf_), "%s%s", buf_ptr, brief_shields_.c_str());
		act(buf_, FALSE, ch, NULL, victim, TO_NOTVICT | TO_ARENA_LISTEN | TO_BRIEF_SHIELDS);
		act(buf_ptr, FALSE, ch, NULL, victim, TO_NOTVICT | TO_ARENA_LISTEN | TO_NO_BRIEF_SHIELDS);
	}

	// damage message to damager
	send_to_char(ch, "%s", dam ? "&Y&q" : "&y&q");
	if (!brief_shields_.empty() && PRF_FLAGGED(ch, PRF_BRIEF_SHIELDS))
	{
		char buf_[MAX_INPUT_LENGTH];
		snprintf(buf_, sizeof(buf_), "%s%s",
			replace_string(dam_weapons[dam_msgnum].to_char,
				attack_hit_text[w_type].singular, attack_hit_text[w_type].plural),
			brief_shields_.c_str());
		act(buf_, FALSE, ch, NULL, victim, TO_CHAR);
	}
	else
	{
		buf_ptr = replace_string(dam_weapons[dam_msgnum].to_char,
			attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
		act(buf_ptr, FALSE, ch, NULL, victim, TO_CHAR);
	}
	send_to_char("&Q&n", ch);

	// damage message to damagee
	send_to_char("&R&q", victim);
	if (!brief_shields_.empty() && PRF_FLAGGED(victim, PRF_BRIEF_SHIELDS))
	{
		char buf_[MAX_INPUT_LENGTH];
		snprintf(buf_, sizeof(buf_), "%s%s",
			replace_string(dam_weapons[dam_msgnum].to_victim,
				attack_hit_text[w_type].singular, attack_hit_text[w_type].plural),
			brief_shields_.c_str());
		act(buf_, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
	}
	else
	{
		buf_ptr = replace_string(dam_weapons[dam_msgnum].to_victim,
			attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
		act(buf_ptr, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
	}
	send_to_char("&Q&n", victim);
}

// �����������.
// ����-�������� ��� ������ ������, ���� �� ��� ���� ���������������.
// ���� ���� ����� (�������� �� MOB_CLONE), ���� ������ ������.
// ���� ���� ����� ��� ������ (��� ��������� ��� ������), �� ������ ����
// ��� ����� ������ �� �������.
void update_mob_memory(CHAR_DATA *ch, CHAR_DATA *victim)
{
	// ������ -- ���� ����, �� ���������� ��������
	if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_MEMORY))
	{
		if (!IS_NPC(ch))
		{
			remember(victim, ch);
		}
		else if (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
			&& ch->has_master()
			&& !IS_NPC(ch->get_master()))
		{
			if (MOB_FLAGGED(ch, MOB_CLONE))
			{
				remember(victim, ch->get_master());
			}
			else if (IN_ROOM(ch->get_master()) == IN_ROOM(victim)
				&& CAN_SEE(victim, ch->get_master()))
			{
				remember(victim, ch->get_master());
			}
		}
	}

	// ������ -- ���� ��� ��� � ����������, ���� ����� �������� :)
	if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_MEMORY))
	{
		if (!IS_NPC(victim))
		{
			remember(ch, victim);
		}
		else if (AFF_FLAGGED(victim, EAffectFlag::AFF_CHARM)
			&& victim->has_master()
			&& !IS_NPC(victim->get_master()))
		{
			if (MOB_FLAGGED(victim, MOB_CLONE))
			{
				remember(ch, victim->get_master());
			}
			else if (IN_ROOM(victim->get_master()) == ch->in_room
				&& CAN_SEE(ch, victim->get_master()))
			{
				remember(ch, victim->get_master());
			}
		}
	}
}

bool Damage::magic_shields_dam(CHAR_DATA *ch, CHAR_DATA *victim)
{
	// ������ �����
	if (dam && AFF_FLAGGED(victim, EAffectFlag::AFF_SHIELD))
	{
		if (skill_num == SKILL_BASH)
		{
			skill_message(dam, ch, victim, msg_num);
		}
		act("���������� ����� ��������� �������� ���� $N1.",
			FALSE, victim, 0, ch, TO_CHAR);
		act("���������� ����� ������ $N1 ��������� �������� ��� ����.",
			FALSE, ch, 0, victim, TO_CHAR);
		act("���������� ����� ������ $N1 ��������� �������� ���� $n1.",
			TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
		return true;
	}

	if (dam <= 0)
	{
		return false;
	}

	// ��������� ����� ��� ������ �� �������
	if (AFF_FLAGGED(victim, EAffectFlag::AFF_MAGICGLASS)
		&& dmg_type == FightSystem::MAGE_DMG)
	{
		int pct = 6;
		if (IS_NPC(victim) && !IS_CHARMICE(victim))
		{
			pct += 2;
			if (victim->get_role(MOB_ROLE_BOSS))
			{
				pct += 2;
			}
		}
		// ����� �������
		const int mg_damage = dam * pct / 100;
		if (mg_damage > 0
			&& victim->get_fighting()
			&& GET_POS(victim) > POS_STUNNED
			&& IN_ROOM(victim) != NOWHERE)
		{
			flags.set(FightSystem::DRAW_BRIEF_MAG_MIRROR);
			Damage dmg(SpellDmg(SPELL_MAGICGLASS), mg_damage, FightSystem::UNDEF_DMG);
			dmg.flags.set(FightSystem::NO_FLEE);
			dmg.flags.set(FightSystem::MAGIC_REFLECT);
			dmg.process(victim, ch);
		}
	}

	// ��������� �����, �� Damage::post_init_shields()
	if (flags[FightSystem::VICTIM_FIRE_SHIELD]
		&& !flags[FightSystem::CRIT_HIT])
	{
		if (dmg_type == FightSystem::PHYS_DMG
			&& !flags[FightSystem::IGNORE_FSHIELD])
		{
			int pct = 15;
			if (IS_NPC(victim) && !IS_CHARMICE(victim))
			{
				pct += 5;
				if (victim->get_role(MOB_ROLE_BOSS))
				{
					pct += 5;
				}
			}
			fs_damage = dam * pct / 100;
		}
		else
		{
			act("�������� ��� ������ $N1 ������� ���� �����.",
				FALSE, ch, 0, victim, TO_CHAR | TO_NO_BRIEF_SHIELDS);
			act("�������� ��� ������ ����� ����������� �� ����.",
				FALSE, ch, 0, victim, TO_VICT | TO_NO_BRIEF_SHIELDS);
			act("�������� ��� ������ $N1 ������� ����� $n1.",
				TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN | TO_NO_BRIEF_SHIELDS);
		}
		flags.set(FightSystem::DRAW_BRIEF_FIRE_SHIELD);
		dam -= (dam * number(30, 50) / 100);
	}

        // ���� ����������� ���� (�� ����� � ����) � ���� ��� - 95% ���� � ������
        // ����������� ��������� ����� ���� ������� ������� � ������������ �������
	if (dam
		&& flags[FightSystem::CRIT_HIT] && flags[FightSystem::VICTIM_ICE_SHIELD]
		&& !dam_critic
		&& spell_num != SPELL_POISON 
                && number(0, 100)<94)
	{
            act("���� ������ ��������� �������� ������� � ������� ������ ������ $N1.",
			FALSE, ch, 0, victim, TO_CHAR | TO_NO_BRIEF_SHIELDS);
            act("������ ��������� �������� ������� � ������� ������ ����.",
			FALSE, ch, 0, victim, TO_VICT | TO_NO_BRIEF_SHIELDS);
            act("������� ��� ������ $N1 �������� �������� ������ ��������� $n1.",
			TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN | TO_NO_BRIEF_SHIELDS);
			
                   flags[FightSystem::CRIT_HIT] = false; //��� ��� ����� ����� ��������� ���������
		   if (dam > 0) dam -= (dam * number(30, 50) / 100);
        }
    //��� ������ ����� �������������� �������
	else if (dam > 0
		&& flags[FightSystem::VICTIM_ICE_SHIELD]
		&& !flags[FightSystem::CRIT_HIT])
	{
		flags.set(FightSystem::DRAW_BRIEF_ICE_SHIELD);
		act("������� ��� ������ $N1 ������� ��� ����.",
			FALSE, ch, 0, victim, TO_CHAR | TO_NO_BRIEF_SHIELDS);
		act("������� ��� ������ ����� ����� �� ����.",
			FALSE, ch, 0, victim, TO_VICT | TO_NO_BRIEF_SHIELDS);
		act("������� ��� ������ $N1 ������� ���� $n1.",
			TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN | TO_NO_BRIEF_SHIELDS);
		dam -= (dam * number(30, 50) / 100);
	}
	
	if (dam > 0
		&& flags[FightSystem::VICTIM_AIR_SHIELD]
		&& !flags[FightSystem::CRIT_HIT])
	{
		flags.set(FightSystem::DRAW_BRIEF_AIR_SHIELD);
		act("��������� ��� ������ $N1 ������� ��� ����.",
			FALSE, ch, 0, victim, TO_CHAR | TO_NO_BRIEF_SHIELDS);
		act("��������� ��� ������� ���� $n1.",
			FALSE, ch, 0, victim, TO_VICT | TO_NO_BRIEF_SHIELDS);
		act("��������� ��� ������ $N1 ������� ���� $n1.",
			TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN | TO_NO_BRIEF_SHIELDS);
		dam -= (dam * number(30, 50) / 100);
	}

	return false;
}

void Damage::armor_dam_reduce(CHAR_DATA *ch, CHAR_DATA *victim)
{
	// ����� �� ��� �����
	if (dam > 0
		&& dmg_type == FightSystem::PHYS_DMG)
	{
		alt_equip(victim, NOWHERE, dam, 50);
		if (!flags[FightSystem::CRIT_HIT]
			&& !flags[FightSystem::IGNORE_ARMOR])
		{
			// 50 ����� = 50% �������� ������
			int max_armour = 50;
			if (can_use_feat(victim, IMPREGNABLE_FEAT)
				&& PRF_FLAGS(victim).get(PRF_AWAKE))
			{
				// ������������� � ��������� - �� 75 �����
				max_armour = 75;
			}
			int tmp_dam = dam * MAX(0, MIN(max_armour, GET_ARMOUR(victim))) / 100;
			// �������������� ����� �� ����� �����
			if (tmp_dam >= 2
				&& flags[FightSystem::HALF_IGNORE_ARMOR])
			{
				tmp_dam /= 2;
			}
			dam -= tmp_dam;
			// ���� ���� �������� �����, ���� ������ ��� ������ � ��� ���.����
		}
		else if (flags[FightSystem::CRIT_HIT]
			&& (GET_LEVEL(victim) >= 5
				|| !IS_NPC(ch))
			&& !AFF_FLAGGED(victim, EAffectFlag::AFF_PRISMATICAURA)
			&& !flags[FightSystem::VICTIM_ICE_SHIELD])
		{
			dam = MAX(dam, MIN(GET_REAL_MAX_HIT(victim) / 8, dam * 2));
		}
	}
}

/**
 * ��������� ���������� ��� � ��� �����.
 * \return true - ������ ����������
 */
bool Damage::dam_absorb(CHAR_DATA *ch, CHAR_DATA *victim)
{
	if (dmg_type == FightSystem::PHYS_DMG
		&& skill_num < 0
		&& spell_num < 0
		&& dam > 0
		&& GET_ABSORBE(victim) > 0)
	{
		// ����� ����������: ������������� � ��������� 15%, ��������� 10%
		int chance = 10;
		if (can_use_feat(victim, IMPREGNABLE_FEAT)
			&& PRF_FLAGS(victim).get(PRF_AWAKE))
		{
			chance = 15;
		}
		// ��� ���� - ������ ��������� �� ������
		if (number(1, 100) <= chance)
		{
			dam -= GET_ABSORBE(victim) / 2;
			if (dam <= 0)
			{
				act("���� ������� ��������� ��������� ���� $n1.",
					FALSE, ch, 0, victim, TO_VICT);
				act("������� $N1 ��������� ��������� ��� ����.",
					FALSE, ch, 0, victim, TO_CHAR);
				act("������� $N1 ��������� ��������� ���� $n1.",
					TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
				return true;
			}
		}
	}
	else if (dmg_type == FightSystem::MAGE_DMG
		&& dam > 0
		&& GET_ABSORBE(victim) > 0
		&& !flags[FightSystem::IGNORE_ABSORBE])
	{
		// ��� ���� - �� 1% �� ������ 2 �������, �������� 25% (����� �� mag_damage)
		const int absorb = MIN(GET_ABSORBE(victim) / 2, 25);
		dam -= dam * absorb / 100;
	}
	return false;
}

void Damage::send_critical_message(CHAR_DATA *ch, CHAR_DATA *victim)
{
	// ������� ������� ����� ��� ������� ���� ����� ���������,
	// ��� ��� ������� ��������� ��������� ��� �������� ���� (������)
	if (!flags[FightSystem::VICTIM_ICE_SHIELD])
	{
		sprintf(buf, "&B&q���� ������ ��������� ������ ������ %s.&Q&n\r\n",
			PERS(victim, ch, 3));
	}
	else
	{
		sprintf(buf, "&B&q���� ������ ��������� ������� � ������� ������ ���� %s.&Q&n\r\n",
			PERS(victim, ch, 1));
	}

	send_to_char(buf, ch);

	if (!flags[FightSystem::VICTIM_ICE_SHIELD])
	{
		sprintf(buf, "&r&q������ ��������� %s ������ ������ ���.&Q&n\r\n",
			PERS(ch, victim, 1));
	}
	else
	{
		sprintf(buf, "&r&q������ ��������� %s ������� � ������� ������ ������ ����.&Q&n\r\n",
			PERS(ch, victim, 1));
	}

	send_to_char(buf, victim);
	// ����������� ����� �� �������, ������� ����� � ���� ������
	//act("������ ��������� $N1 ��������� $n3 �����������.", TRUE, victim, 0, ch, TO_NOTVICT);
}

void update_dps_stats(CHAR_DATA *ch, int real_dam, int over_dam)
{
	if (!IS_NPC(ch))
	{
		ch->dps_add_dmg(DpsSystem::PERS_DPS, real_dam, over_dam);
		log("DmetrLog. Name(player): %s, class: %d, remort:%d, level:%d, dmg: %d, over_dmg:%d", GET_NAME(ch), GET_CLASS(ch), GET_REMORT(ch), GET_LEVEL(ch), real_dam, over_dam);
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_GROUP))
		{
			CHAR_DATA *leader = ch->has_master() ? ch->get_master() : ch;
			leader->dps_add_dmg(DpsSystem::GROUP_DPS, real_dam, over_dam, ch);
		}
	}
	else if (IS_CHARMICE(ch)
		&& ch->has_master())
	{
		ch->get_master()->dps_add_dmg(DpsSystem::PERS_CHARM_DPS, real_dam, over_dam, ch);
		if (!IS_NPC(ch->get_master()))
		{
			log("DmetrLog. Name(charmice): %s, name(master): %s, class: %d, remort: %d, level: %d, dmg: %d, over_dmg:%d",
				GET_NAME(ch), GET_NAME(ch->get_master()), GET_CLASS(ch->get_master()), GET_REMORT(ch->get_master()),
				GET_LEVEL(ch->get_master()), real_dam, over_dam);
		}

		if (AFF_FLAGGED(ch->get_master(), EAffectFlag::AFF_GROUP))
		{
			CHAR_DATA *leader = ch->get_master()->has_master() ? ch->get_master()->get_master() : ch->get_master();
			leader->dps_add_dmg(DpsSystem::GROUP_CHARM_DPS, real_dam, over_dam, ch);
		}
	}
}

void try_angel_sacrifice(CHAR_DATA *ch, CHAR_DATA *victim)
{
	// ���� ������ � ������ � ���-�� � ������� - ������ ������ ������� ������� �����
	if (GET_HIT(victim) <= 0
		&& !IS_NPC(victim)
		&& AFF_FLAGGED(victim, EAffectFlag::AFF_GROUP))
	{
		const auto people = world[IN_ROOM(victim)]->people;	// make copy of people because keeper might be removed from this list inside the loop
		for (const auto keeper : people)
		{
			if (IS_NPC(keeper)
				&& MOB_FLAGGED(keeper, MOB_ANGEL)
				&& keeper->has_master()
				&& AFF_FLAGGED(keeper->get_master(), EAffectFlag::AFF_GROUP))
			{
				CHAR_DATA *keeper_leader = keeper->get_master()->has_master()
					? keeper->get_master()->get_master()
					: keeper->get_master();
				CHAR_DATA *victim_leader = victim->has_master()
					? victim->get_master()
					: victim;

				if ((keeper_leader == victim_leader)
					&& (may_kill_here(keeper->get_master(), ch)))
				{
					if (!pk_agro_action(keeper->get_master(), ch))
					{
						return;
					}
					send_to_char(victim, "%s �����������%s ����� ������, ���������� ��� � ���� �����!\r\n",
						GET_PAD(keeper, 0), GET_CH_SUF_1(keeper));
					snprintf(buf, MAX_STRING_LENGTH, "%s �����������%s ����� ������, ���������� %s � ���� �����!",
						GET_PAD(keeper, 0), GET_CH_SUF_1(keeper), GET_PAD(victim, 3));
					act(buf, FALSE, victim, 0, 0, TO_ROOM | TO_ARENA_LISTEN);

					extract_char(keeper, 0);
					GET_HIT(victim) = MIN(300, GET_MAX_HIT(victim) / 2);
				}
			}
		}
	}
}

void update_pk_logs(CHAR_DATA *ch, CHAR_DATA *victim)
{
	ClanPkLog::check(ch, victim);
	sprintf(buf2, "%s killed by %s at %s [%d] ", GET_NAME(victim), GET_NAME(ch),
		IN_ROOM(victim) != NOWHERE ? world[IN_ROOM(victim)]->name : "NOWHERE", GET_ROOM_VNUM(IN_ROOM(victim)));
	log("%s",buf2);

	if ((!IS_NPC(ch)
			|| (ch->has_master()
				&& !IS_NPC(ch->get_master())))
		&& RENTABLE(victim)
		&& !ROOM_FLAGGED(IN_ROOM(victim), ROOM_ARENA))
	{
		mudlog(buf2, BRF, LVL_IMPL, SYSLOG, 0);
		if (IS_NPC(ch)
			&& (AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) || IS_HORSE(ch))
			&& ch->has_master()
			&& !IS_NPC(ch->get_master()))
		{
			sprintf(buf2, "%s is following %s.", GET_NAME(ch), GET_PAD(ch->get_master(), 2));
			mudlog(buf2, BRF, LVL_IMPL, SYSLOG, TRUE);
		}
	}
}

void Damage::process_death(CHAR_DATA *ch, CHAR_DATA *victim)
{
	CHAR_DATA *killer = NULL;

	if (IS_NPC(victim) || victim->desc)
	{
		if (victim == ch && IN_ROOM(victim) != NOWHERE)
		{
			if (spell_num == SPELL_POISON)
			{
				for (const auto poisoner : world[IN_ROOM(victim)]->people)
				{
					if (poisoner != victim
						&& GET_ID(poisoner) == victim->Poisoner)
					{
						killer = poisoner;
					}
				}
			}
			else if (msg_num == TYPE_SUFFERING)
			{
				for (const auto attacker : world[IN_ROOM(victim)]->people)
				{
					if (attacker->get_fighting() == victim)
					{
						killer = attacker;
					}
				}
			}
		}

		if (ch != victim)
		{
			killer = ch;
		}
	}

	if (killer)
	{
		if (AFF_FLAGGED(killer, EAffectFlag::AFF_GROUP))
		{
			// �.�. ������� ������ AFF_GROUP - ����� PC
			group_gain(killer, victim);
		}
		else if ((AFF_FLAGGED(killer, EAffectFlag::AFF_CHARM)
				|| MOB_FLAGGED(killer, MOB_ANGEL)
				|| MOB_FLAGGED(killer, MOB_GHOST))
			&& killer->has_master())
			// killer - ������������ NPC � ��������
		{
			// �� ������ ���� �� �������, ��� ���� ������� ��� � ������, ��
			// ���-�� �� ������ ������� � ������, �� ���� �������� ���������,
			// ������� ����� � ������� ���� ��������.
			if (AFF_FLAGGED(killer->get_master(), EAffectFlag::AFF_GROUP)
				&& IN_ROOM(killer) == IN_ROOM(killer->get_master()))
			{
				// ������ - PC � ������ => ���� ������
				group_gain(killer->get_master(), victim);
			}
			else if (IN_ROOM(killer) == IN_ROOM(killer->get_master()))
				// ������ � ������ � ����� �������
				// ���� �������
			{
				perform_group_gain(killer->get_master(), victim, 1, 100);
			}
			// else
			// � ������� �� ����� �� ���������, ��� ������� - ������
			// ����� �������� ����  perform_group_gain( killer, victim, 1, 100 );
		}
		else
		{
			// ������ NPC ��� PC ��� �� ����
			perform_group_gain(killer, victim, 1, 100);
		}
	}

	// � ������ ����� ���� ������ ������ � �� (��� ����)
	// � ���� ������� ��� ������ �����
	// ���� ��� ���� ������� �� ���� �� ������

	if (!IS_NPC(victim) && !(killer && PRF_FLAGGED(killer, PRF_EXECUTOR)))
	{
		update_pk_logs(ch, victim);

	for (const auto& ch_vict : world[ch->in_room]->people)
	{
		//���� ��� ��� ������������� ��� ������ ������ ��������
		if (IS_IMMORTAL(ch_vict))
			continue;
		if (!HERE(ch_vict))
			continue;
		if (!IS_NPC(ch_vict))
			continue;
		if (MOB_FLAGGED(ch_vict, MOB_MEMORY))
		{
			forget(ch_vict, victim);
		}
	}
                
	}

	if (killer)
	{
		ch = killer;
	}

	die(victim, ch);
}

// ��������� �����, ��, ����������, ��������� ��� ���. ���� �� �����
// ���������� ��������� �����
int Damage::process(CHAR_DATA *ch, CHAR_DATA *victim)
{
	post_init(ch, victim);

	if (!check_valid_chars(ch, victim, __FILE__, __LINE__))
	{
		return 0;
	}

	if (IN_ROOM(victim) == NOWHERE
		|| ch->in_room == NOWHERE
		|| ch->in_room != IN_ROOM(victim))
	{
		log("SYSERR: Attempt to damage '%s' in room NOWHERE by '%s'.",
			GET_NAME(victim), GET_NAME(ch));
		return 0;
	}

	if (GET_POS(victim) <= POS_DEAD)
	{
		log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
			GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(victim)), GET_NAME(ch));
		die(victim, NULL);
		return 0;	// -je, 7/7/92
	}

	// ������ ����� �������� ���� � hit ����� �������� �����
	if (dam >= 0 && damage_mtrigger(ch, victim))
		return 0;

	// No fight mobiles
	if ((IS_NPC(ch) && MOB_FLAGGED(ch, MOB_NOFIGHT))
		|| (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_NOFIGHT)))
	{
		return 0;
	}

	if (dam > 0)
	{
		// You can't damage an immortal!
		if (IS_GOD(victim))
			dam = 0;
		else if (IS_IMMORTAL(victim) || GET_GOD_FLAG(victim, GF_GODSLIKE))
			dam /= 4;
		else if (GET_GOD_FLAG(victim, GF_GODSCURSE))
			dam *= 2;
	}

	// ����������� ������ ��������� � �����
	update_mob_memory(ch, victim);

	// ������ � ������ ���������� ��������
	appear(ch);
	appear(victim);

	// If you attack a pet, it hates your guts
	if (!same_group(ch, victim))
		check_agro_follower(ch, victim);

	if (victim != ch)  	// Start the attacker fighting the victim
	{
		if (GET_POS(ch) > POS_STUNNED && (ch->get_fighting() == NULL))
		{
			if (!pk_agro_action(ch, victim))
				return (0);
			set_fighting(ch, victim);
			npc_groupbattle(ch);
		}
		// Start the victim fighting the attacker
		if (GET_POS(victim) > POS_STUNNED && (victim->get_fighting() == NULL))
		{
			set_fighting(victim, ch);
			npc_groupbattle(victim);
		}

		//check self horse attack
		if (on_horse(ch) && get_horse(ch) == victim)
		{
			horse_drop(victim);
		}
		else if (on_horse(victim) && get_horse(victim) == ch)
		{
			horse_drop(ch);
		}
	}

	// If negative damage - return
	if (dam < 0
		|| ch->in_room == NOWHERE
		|| IN_ROOM(victim) == NOWHERE
		|| ch->in_room != IN_ROOM(victim))
	{
		return (0);
	}

	// �����/������ ��� ��� � ��� �����
	if (dam >= 2)
	{
		if (AFF_FLAGGED(victim, EAffectFlag::AFF_PRISMATICAURA) &&  !(skill_num == SKILL_BACKSTAB && can_use_feat(ch, THIEVES_STRIKE_FEAT)))
		{
			if (dmg_type == FightSystem::PHYS_DMG)
			{
				dam *= 2;
			}
			else if (dmg_type == FightSystem::MAGE_DMG)
			{
				dam /= 2;
			}
		}
		if (AFF_FLAGGED(victim, EAffectFlag::AFF_SANCTUARY) &&  !(skill_num == SKILL_BACKSTAB && can_use_feat(ch, THIEVES_STRIKE_FEAT)))
		{
			if (dmg_type == FightSystem::PHYS_DMG)
			{
				dam /= 2;
			}
			else if (dmg_type == FightSystem::MAGE_DMG)
			{
				dam *= 2;
			}
		}

		if (IS_NPC(victim) && Bonus::is_bonus(Bonus::BONUS_DAMAGE))
		{
			dam *= Bonus::get_mult_bonus();
		}
	}

	// ���� ��������� ���������� � ������
	// Include a damage multiplier if victim isn't ready to fight:
	// Position sitting  1.5 x normal
	// Position resting  2.0 x normal
	// Position sleeping 2.5 x normal
	// Position stunned  3.0 x normal
	// Position incap    3.5 x normal
	// Position mortally 4.0 x normal
	// Note, this is a hack because it depends on the particular
	// values of the POSITION_XXX constants.

	// ��� ������ ������� �� �������� ���������
	if (ch_start_pos < POS_FIGHTING
		&& dmg_type == FightSystem::PHYS_DMG)
	{
		dam -= dam * (POS_FIGHTING - ch_start_pos) / 4;
	}

	// ����� �� ������������� ����:
	// �� ������ ���� ��������� ���
	// ����� - ���� ���� (� mage_damage ���������� ������ �� ������� ���� ������ � ��������)
	if (victim_start_pos < POS_FIGHTING
		&& !flags[FightSystem::VICTIM_AIR_SHIELD]
		&& !(dmg_type == FightSystem::MAGE_DMG
			&& IS_NPC(ch)))
	{
		dam += dam * (POS_FIGHTING - victim_start_pos) / 4;
	}

	// ������ ���������

	// ��������� ��� ����� �� �����
	if (GET_MOB_HOLD(victim)
		&& dmg_type == FightSystem::PHYS_DMG)
	{
		if (IS_NPC(ch)
			&& !IS_CHARMICE(ch))
		{
			dam = dam * 15 / 10;
		}
		else
		{
			dam = dam * 125 / 100;
		}
	}

	// ������ ������ �������� �� �����
	if (!IS_NPC(victim)
		&& IS_CHARMICE(ch))
	{
		dam = dam * 8 / 10;
	}

	// �� ������ ��� ��� �����
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BELENA_POISON)
		&& dmg_type == FightSystem::PHYS_DMG)
	{
		dam -= dam * GET_POISON(ch) / 100;
	}

	// added by WorM(�������) ���������� ���.����� � %
	//if(GET_PR(victim) && IS_NPC(victim)
	if (GET_PR(victim) && dmg_type == FightSystem::PHYS_DMG)
	{
		if (PRF_FLAGGED(victim, PRF_TESTER))
		{
			const int ResultDam = dam - (dam * GET_PR(victim) / 100);
			sprintf(buf, "&C���� ���������� �����: %d ���������, %d ���������.&n\r\n", dam, ResultDam);
			send_to_char(buf, victim);
			dam = ResultDam;
		}
		else
		{
			dam = dam - (dam * GET_PR(victim) / 100);
		}
	}

	// ��, ����, �����, ����������
	if (victim != ch)
	{
		const bool shield_full_absorb = magic_shields_dam(ch, victim);
		// ������� �����
		armor_dam_reduce(ch, victim);
		// ����� ������
		const bool armor_full_absorb = dam_absorb(ch, victim);
		// ������ ����������
		if (shield_full_absorb
			|| armor_full_absorb)
		{
			return 0;
		}
	}

	// ������ magic_shields_dam ���������� dmg::proccess, ���� ��� ��� �����, �� ����� ����
	if (!(ch && victim) || (ch->purged() || victim->purged()))
		return 0;

	// �� �� ����
	if (MOB_FLAGGED(victim, MOB_PROTECT))
	{
		if (victim != ch)
		{
			act("$n ��������� ��� ������� �����.", FALSE, victim, 0, 0, TO_ROOM);
		}
		return 0;
	}

	// �� ��������
	if (skill_num != SKILL_BACKSTAB
		&& AFF_FLAGGED(victim, EAffectFlag::AFF_SCOPOLIA_POISON))
	{
		dam += dam * GET_POISON(victim) / 100;
	}

	// ������ ���� !������ �������!, ��� ������ ���� ������ - �� ����
	DamageActorParameters params(ch, victim, dam);
	handle_affects(params);
	dam = params.damage;
	DamageVictimParameters params1(ch, victim, dam);
	handle_affects(params1);
	dam = params1.damage;

	// ������� ��� ������� �������
	if (dmg_type == FightSystem::PHYS_DMG)
	{
		dam += ch->obj_bonus().calc_phys_dmg(dam);
	}
	else if (dmg_type == FightSystem::MAGE_DMG)
	{
		dam += ch->obj_bonus().calc_mage_dmg(dam);
	}

	// ������� �� ������/��������� ����
	if (flags[FightSystem::MAGIC_REFLECT])
	{
		// ����������� ��� ������ �� 40% �� ���� �� �������
		dam = MIN(dam, GET_MAX_HIT(victim) * 4 / 10);
		// ����� �� ������� ��������
		dam = MIN(dam, GET_HIT(victim) - 1);
	}

	dam = MAX(0, MIN(dam, MAX_HITS));

	// ������ ����-����� ��� �����
	gain_battle_exp(ch, victim, dam);

	// real_dam ��� �� ���� � ������� �� ���.����
	int real_dam = dam;
	int over_dam = 0;

	// ���������� ��������� ������
	if (dam > GET_HIT(victim) + 11)
	{
		real_dam = GET_HIT(victim) + 11;
		over_dam = dam - real_dam;
	}
	GET_HIT(victim) -= dam;
	// ���� �� ������� ������
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_VAMPIRE))
	{
		GET_HIT(ch) = MAX(GET_HIT(ch), MIN(GET_HIT(ch) + MAX(1, dam * 0.1), GET_REAL_MAX_HIT(ch)
			+ GET_REAL_MAX_HIT(ch) * GET_LEVEL(ch) / 10));
		// ���� ���� ������� ���, �� ���� ������� �� 5% �� ������ � ��
		if (ch->has_master())
		{
			if (can_use_feat(ch->get_master(), SOULLINK_FEAT))
			{
				GET_HIT(ch->get_master()) = MAX(GET_HIT(ch->get_master()),
					MIN(GET_HIT(ch->get_master()) + MAX(1, dam * 0.05),
						GET_REAL_MAX_HIT(ch->get_master())
						+ GET_REAL_MAX_HIT(ch->get_master()) * GET_LEVEL(ch->get_master()) / 10));
			}
		}
	}

	// ������ � ����� ������������ � ���� ������
	update_dps_stats(ch, real_dam, over_dam);
	// ������ ������ � ������ ��������
	if (IS_NPC(victim))
	{
		victim->add_attacker(ch, ATTACKER_DAMAGE, real_dam);
	}

	// ������� ������ ������ ����� ������
	try_angel_sacrifice(ch, victim);

	// ���������� ������� ����� ����� � ������
	update_pos(victim);
	// ���� ����� ������ ���� ����� �����, �� ���������� ����, ������� ������� ��� ����������
	if (!(ch && victim) || (ch->purged() || victim->purged()))
	{
		log("Error in fight_hit, function process()\r\n");
		return 0;
	}
	// ���� � ���� ���� ����� �����
	if (can_use_feat(victim, HARVESTLIFE_FEAT))
	{
		if (GET_POS(victim) == POS_DEAD)
		{
			const int souls = victim->get_souls();
			if (souls >= 10)
			{
				GET_HIT(victim) = 0 + souls * 10;
				update_pos(victim);
				send_to_char("&G���� ������ ��� �� ������!&n\r\n", victim);
				victim->set_souls(0);
			}
		}
	}
	// �������� ������ �������� //
	if (spell_num != SPELL_POISON
		&& dam > 0
		&& !flags[FightSystem::MAGIC_REFLECT])
	{
		try_remove_extrahits(ch, victim);
	}

	// ��������� � ���� ������ //
	if (dam
		&& flags[FightSystem::CRIT_HIT]
		&& !dam_critic
		&& spell_num != SPELL_POISON)
	{
		send_critical_message(ch, victim);
	}

	//  skill_message sends a message from the messages file in lib/misc.
	//  dam_message just sends a generic "You hit $n extremely hard.".
	//  skill_message is preferable to dam_message because it is more
	//  descriptive.
	//
	//  If we are _not_ attacking with a weapon (i.e. a spell), always use
	//  skill_message. If we are attacking with a weapon: If this is a miss or a
	//  death blow, send a skill_message if one exists; if not, default to a
	//  dam_message. Otherwise, always send a dam_message.
	// log("[DAMAGE] Attack message...");

	// ���� Damage::brief_shields_
	if (flags.test(FightSystem::DRAW_BRIEF_FIRE_SHIELD)
		|| flags.test(FightSystem::DRAW_BRIEF_AIR_SHIELD)
		|| flags.test(FightSystem::DRAW_BRIEF_ICE_SHIELD)
		|| flags.test(FightSystem::DRAW_BRIEF_MAG_MIRROR))
	{
		char buf_[MAX_INPUT_LENGTH];
		snprintf(buf_, sizeof(buf_), "&n (%s%s%s%s)",
			flags.test(FightSystem::DRAW_BRIEF_FIRE_SHIELD) ? "&R*&n" : "",
			flags.test(FightSystem::DRAW_BRIEF_AIR_SHIELD) ? "&C*&n" : "",
			flags.test(FightSystem::DRAW_BRIEF_ICE_SHIELD) ? "&W*&n" : "",
			flags.test(FightSystem::DRAW_BRIEF_MAG_MIRROR) ? "&M*&n" : "");
		brief_shields_ = buf_;
	}

	// ��������� �� ������ //
	if (skill_num >= 0 || spell_num >= 0 || hit_type < 0)
	{
		// �����, �����, ��������� �����
		skill_message(dam, ch, victim, msg_num, brief_shields_);
	}
	else
	{
		// ������� ���� �����/�������
		if (GET_POS(victim) == POS_DEAD || dam == 0)
		{
			if (!skill_message(dam, ch, victim, msg_num,  brief_shields_))
			{
				dam_message(ch, victim);
			}
		}
		else
		{
			dam_message(ch, victim);
		}
	}

	/// Use send_to_char -- act() doesn't send message if you are DEAD.
	char_dam_message(dam, ch, victim, flags[FightSystem::NO_FLEE]);

	if (PRF_FLAGGED(ch, PRF_CODERINFO) || PRF_FLAGGED(ch, PRF_TESTER))
	{
		sprintf(buf, "&M�������� ���� = %d&n\r\n", dam);
		send_to_char(buf,ch);
	}

	// ���������, ��� ������ ��� ��� ���. ����� ��� ������� �� ��������.
	// �����, ������� �������� ����������.
	// ����������, ���� ������ � FIRESHIELD,
	// �� ��������� ����������� �� ���������� �� �����
	if (ch->in_room != IN_ROOM(victim))
	{
		return dam;
	}

	// Stop someone from fighting if they're stunned or worse
	if ((GET_POS(victim) <= POS_STUNNED)
		&& (victim->get_fighting() != NULL))
	{
		stop_fighting(victim, GET_POS(victim) <= POS_DEAD);
	}

	// ������ ������� //
	if (GET_POS(victim) == POS_DEAD)
	{
		process_death(ch, victim);
		return -1;
	}

	// ������� �� ��������� ����
	if (fs_damage > 0
		&& victim->get_fighting()
		&& GET_POS(victim) > POS_STUNNED
		&& IN_ROOM(victim) != NOWHERE)
	{
		Damage dmg(SpellDmg(SPELL_FIRE_SHIELD), fs_damage, FightSystem::UNDEF_DMG);
		dmg.flags.set(FightSystem::NO_FLEE);
		dmg.flags.set(FightSystem::MAGIC_REFLECT);
		dmg.process(victim, ch);
	}

	return dam;
}

void HitData::try_mighthit_dam(CHAR_DATA *ch, CHAR_DATA *victim)
{
	int percent = number(1, skill_info[SKILL_MIGHTHIT].max_percent);
	int prob = train_skill(ch, SKILL_MIGHTHIT, skill_info[SKILL_MIGHTHIT].max_percent, victim);
	int lag = 0, might = 0;

	if (GET_MOB_HOLD(victim))
	{
		percent = number(1, 25);
	}

	if (IS_IMMORTAL(victim))
	{
		prob = 0;
	}

/*  ����������� ����� ������.
send_to_char(ch, "���������� ������: Prob == %d, Percent == %d, Might == %d, Stab == %d\r\n", prob, percent, might, stab);
 sprintf(buf, "%s ������� : Percent == %d,Prob == %d, Might == %d, Stability == %d\r\n",GET_NAME(ch), percent, prob, might, stab);
                mudlog(buf, LGH, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), SYSLOG, TRUE);
*/
	if (percent > prob || dam == 0)
	{
		sprintf(buf, "&c&q��� ����������� ���� ������ �������.&Q&n\r\n");
		send_to_char(buf, ch);
		lag = 3;
		dam = 0;
	}
	else if (MOB_FLAGGED(victim, MOB_NOHAMER))
	{
		sprintf(buf, "&c&q�� ������ ���� ���� ���������!&Q&n\r\n");
		send_to_char(buf, ch);
		lag = 1;
		dam = 0;
	}
	else
	{
		might = prob * 100 / percent;
		if (might < 180)
		{
			sprintf(buf, "&b&q��� ����������� ���� ����� %s.&Q&n\r\n",
				PERS(victim, ch, 3));
			send_to_char(buf, ch);
			lag = 1;
			WAIT_STATE(victim, PULSE_VIOLENCE);
			AFFECT_DATA<EApplyLocation> af;
			af.type = SPELL_BATTLE;
			af.bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af.location = APPLY_NONE;
			af.modifier = 0;
			af.duration = pc_duration(victim, 1, 0, 0, 0, 0);
			af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			affect_join(victim, af, TRUE, FALSE, TRUE, FALSE);
			sprintf(buf, "&R&q���� �������� ������������ ����� ����� %s.&Q&n\r\n", PERS(ch, victim, 1));
			send_to_char(buf, victim);
			act("$N ���������$U �� ������������ ����� $n1.", TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
			if (!number(0, 2))
			{
				might_hit_bash(ch, victim);
			}
		}
		else if (might < 800)
		{
			sprintf(buf, "&g&q��� ����������� ���� �������� %s.&Q&n\r\n", PERS(victim, ch, 3));
			send_to_char(buf, ch);
			lag = 2;
			dam += (dam / 1);
			WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
			AFFECT_DATA<EApplyLocation> af;
			af.type = SPELL_BATTLE;
			af.bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af.location = APPLY_NONE;
			af.modifier = 0;
			af.duration = pc_duration(victim, 2, 0, 0, 0, 0);
			af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			affect_join(victim, af, TRUE, FALSE, TRUE, FALSE);
			sprintf(buf, "&R&q���� �������� ���������� ����� ����� %s.&Q&n\r\n", PERS(ch, victim, 1));
			send_to_char(buf, victim);
			act("$N ��������$U �� ������������ ����� $n1.", TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
			if (!number(0, 1))
			{
				might_hit_bash(ch, victim);
			}
		}
		else
		{
			sprintf(buf, "&G&q��� ����������� ���� ������ %s.&Q&n\r\n", PERS(victim, ch, 3));
			send_to_char(buf, ch);
			lag = 2;
			dam *= 4;
			WAIT_STATE(victim, 3 * PULSE_VIOLENCE);
			AFFECT_DATA<EApplyLocation> af;
			af.type = SPELL_BATTLE;
			af.bitvector = to_underlying(EAffectFlag::AFF_STOPFIGHT);
			af.location = APPLY_NONE;
			af.modifier = 0;
			af.duration = pc_duration(victim, 3, 0, 0, 0, 0);
			af.battleflag = AF_BATTLEDEC | AF_PULSEDEC;
			affect_join(victim, af, TRUE, FALSE, TRUE, FALSE);
			sprintf(buf, "&R&q���� �������� �������� ����� ����� %s.&Q&n\r\n", PERS(ch, victim, 1));
			send_to_char(buf, victim);
			act("$N �������$U �� ������������ ����� $n1.", TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
			might_hit_bash(ch, victim);
		}
	}
	set_wait(ch, lag, TRUE);
}

void HitData::try_stupor_dam(CHAR_DATA *ch, CHAR_DATA *victim)
{
	const int percent = number(1, skill_info[SKILL_STUPOR].max_percent);
	int prob = train_skill(ch, SKILL_STUPOR, skill_info[SKILL_STUPOR].max_percent, victim);
	int lag = 0;

	if (GET_MOB_HOLD(victim))
	{
		prob = MAX(prob, percent * 150 / 100 + 1);
	}

	if (IS_IMMORTAL(victim))
	{
		prob = 0;
	}

	if (prob * 100 / percent < 117
		|| dam == 0
		|| MOB_FLAGGED(victim, MOB_NOSTUPOR))
	{
		sprintf(buf, "&c&q�� ���������� �������� %s, �� �� ������.&Q&n\r\n", PERS(victim, ch, 3));
		send_to_char(buf, ch);
		lag = 3;
		dam = 0;
	}
	else if (prob * 100 / percent < 300)
	{
		sprintf(buf, "&g&q���� ������ ����� �������� %s.&Q&n\r\n", PERS(victim, ch, 3));
		send_to_char(buf, ch);
		lag = 2;
		int k = ch->get_skill(SKILL_STUPOR) / 30;
		if (!IS_NPC(victim))
		{
			k = MIN(2, k);
		}
		dam *= MAX(2, number(1, k));
		WAIT_STATE(victim, 3 * PULSE_VIOLENCE);
		sprintf(buf, "&R&q���� �������� ������ ���������� ����� ����� %s.&Q&n\r\n", PERS(ch, victim, 1));
		send_to_char(buf, victim);
		act("$n �������$a $N3.", TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
	}
	else
	{
		if (MOB_FLAGGED(victim, MOB_NOBASH))
		{
			sprintf(buf, "&G&q��� ��������� ���� ������� %s.&Q&n\r\n", PERS(victim, ch, 3));
		}
		else
		{
			sprintf(buf, "&G&q��� ��������� ���� ���� %s � ���.&Q&n\r\n", PERS(victim, ch, 3));
		}
		send_to_char(buf, ch);
		if (MOB_FLAGGED(victim, MOB_NOBASH))
		{
			act("$n ������ ������ �������$a $N3.", TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
		}
		else
		{
			act("$n ����� ���������� ������ ����$a $N3 � ���.", TRUE, ch, 0, victim, TO_NOTVICT | TO_ARENA_LISTEN);
		}
		lag = 2;
		int k = ch->get_skill(SKILL_STUPOR) / 20;
		if (!IS_NPC(victim))
		{
			k = MIN(4, k);
		}
		dam *= MAX(3, number(1, k));
		WAIT_STATE(victim, 3 * PULSE_VIOLENCE);
		if (GET_POS(victim) > POS_SITTING && !MOB_FLAGGED(victim, MOB_NOBASH))
		{
			GET_POS(victim) = POS_SITTING;
			sprintf(buf, "&R&q���������� ���� %s ���� ��� � ���.&Q&n\r\n", PERS(ch, victim, 1));
			send_to_char(buf, victim);
		}
		else
		{
			sprintf(buf, "&R&q���� �������� ������ ���������� ����� ����� %s.&Q&n\r\n", PERS(ch, victim, 1));
			send_to_char(buf, victim);
		}
	}
	set_wait(ch, lag, TRUE);
}

int HitData::extdamage(CHAR_DATA *ch, CHAR_DATA *victim)
{
	if (!check_valid_chars(ch, victim, __FILE__, __LINE__))
	{
		return 0;
	}

	const int mem_dam = dam;

	if (dam < 0)
	{
		dam = 0;
	}

	//* ����������� ����� //
	// � ��� ������� ������ ��������� �� ����, ����� EAF_MIGHTHIT �� ��������
	// � ���� �� ���� ���, ���� �� �� ����� �� �����-�� �������� ���������
	if (GET_AF_BATTLE(ch, EAF_MIGHTHIT)
		&& GET_WAIT(ch) <= 0)
	{
		CLR_AF_BATTLE(ch, EAF_MIGHTHIT);
		if (check_mighthit_weapon(ch) && !GET_AF_BATTLE(ch, EAF_TOUCH))
		{
			try_mighthit_dam(ch, victim);
		}
	}
	//* �������� //
	// ���������� ������, ��� ��� ������� ����������� ������
	else if (GET_AF_BATTLE(ch, EAF_STUPOR) && GET_WAIT(ch) <= 0)
	{
		CLR_AF_BATTLE(ch, EAF_STUPOR);
		if (IS_NPC(ch) || IS_IMMORTAL(ch))
		{
			try_stupor_dam(ch, victim);
		}
		else if (wielded)
		{
			if (GET_OBJ_SKILL(wielded) == SKILL_BOWS)
			{
				send_to_char("����� �������� ������.\r\n", ch);
			}
			else if (!GET_AF_BATTLE(ch, EAF_PARRY) && !GET_AF_BATTLE(ch, EAF_MULTYPARRY))
			{
				if (GET_OBJ_WEIGHT(wielded) > 18)
				{
					try_stupor_dam(ch, victim);
				}
				else
				{
					send_to_char("&W���� ������ ������� ������, ����� �� ����� ���� ��������!&Q&n\r\n", ch);
				}
			}
		}
		else
		{
			sprintf(buf,"&c&q�� ��������� ��� ������, � ������� �������� ������.&Q&n\r\n");
			send_to_char(buf, ch);
			sprintf(buf,"&c&q%s �������� ��� ������ � �� ���� ��� ��������.&Q&n\r\n", GET_NAME(ch));
			send_to_char(buf, victim);
		}
	}
	//* ��� �� ����� �������� //
	else if (!MOB_FLAGGED(victim, MOB_PROTECT)
		&& dam
		&& wielded
		&& wielded->has_timed_spell()
		&& ch->get_skill(SKILL_POISONED))
	{
		try_weap_poison(ch, victim, wielded->timed_spell().is_spell_poisoned());
	}
	//* �������� ���� ��� //
	else if (dam
		&& IS_NPC(ch)
		&& NPC_FLAGGED(ch, NPC_POISON)
		&& !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
		&& GET_WAIT(ch) <= 0
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_POISON)
		&& number(0, 100) < GET_LIKES(ch) + GET_LEVEL(ch) - GET_LEVEL(victim)
		&& !general_savingthrow(ch, victim, SAVING_CRITICAL, - GET_REAL_CON(victim)))
	{
		poison_victim(ch, victim, MAX(1, GET_LEVEL(ch) - GET_LEVEL(victim)) * 10);
	}
	//* ������ ����� //
	else if (dam && get_flags()[FightSystem::CRIT_HIT] && dam_critic)
	{
		compute_critical(ch, victim);
	}

	// ���� ���� ���������, ���������� ��� ����� ��������� � �����.
	// ���������� damage � ������������� ������
	dam = mem_dam >= 0 ? dam : -1;

	Damage dmg(SkillDmg(skill_num), dam, FightSystem::PHYS_DMG);
	dmg.hit_type = hit_type;
	dmg.dam_critic = dam_critic;
	dmg.flags = get_flags();
	dmg.ch_start_pos = ch_start_pos;
	dmg.victim_start_pos = victim_start_pos;

	return dmg.process(ch, victim);
}

/**
 * ������������� ���� ������ ��������� ����� (�������� � ������), �����
 * ���� ������� ��� ���������� �������� ������ ��������/�������� � �������.
 */
void HitData::init(CHAR_DATA *ch, CHAR_DATA *victim)
{
	// Find weapon for attack number weapon //

	if (weapon == 1)
	{
		if (!(wielded = GET_EQ(ch, WEAR_WIELD)))
		{
			wielded = GET_EQ(ch, WEAR_BOTHS);
			weapon_pos = WEAR_BOTHS;
		}
	}
	else if (weapon == 2)
	{
		wielded = GET_EQ(ch, WEAR_HOLD);
		weapon_pos = WEAR_HOLD;
	}

	if (wielded
		&& GET_OBJ_TYPE(wielded) == OBJ_DATA::ITEM_WEAPON)
	{
		// ��� ���� ����� ���� ����� ������� �� �����, ���� ��� ����
		weap_skill = static_cast<ESkill>(GET_OBJ_SKILL(wielded));
	}
	else
	{
		// ���� ������ ������
		weap_skill = SKILL_PUNCH;
	}
	weap_skill_is = train_skill(ch, weap_skill, skill_info[weap_skill].max_percent, victim);

	//* ��������� SKILL_NOPARRYHIT //
	if (skill_num == TYPE_UNDEFINED && ch->get_skill(SKILL_NOPARRYHIT))
	{
		const int tmp_skill = train_skill(ch, SKILL_NOPARRYHIT, skill_info[SKILL_NOPARRYHIT].max_percent, victim);
		// TODO: max_percent � ������ ������ 100 (������ �� �� ���� 200, � � % �����)
		if (tmp_skill >= number(1, skill_info[SKILL_NOPARRYHIT].max_percent))
		{
			hit_no_parry = true;
		}
	}

	if (GET_AF_BATTLE(ch, EAF_STUPOR) || GET_AF_BATTLE(ch, EAF_MIGHTHIT))
	{
		hit_no_parry = true;
	}

	if (wielded && GET_OBJ_TYPE(wielded) == OBJ_DATA::ITEM_WEAPON)
	{
		hit_type = GET_OBJ_VAL(wielded, 3);
	}
	else
	{
		weapon_pos = 0;
		if (IS_NPC(ch))
		{
			hit_type = ch->mob_specials.attack_type;
		}
	}

	// ������� ����������� �� ���������� ������ � �������, ��� ����� �� ��������
	ch_start_pos = GET_POS(ch);
	victim_start_pos = GET_POS(victim);
}

/**
 * ������� ��������� �������� � ����, �� ���������� �� ������� ���� train_skill
 * (� ��� ����� weap_skill_is) ��� ���������� ����������.
 * ��������������, ��� � ����� ��� ������ � '���� ���' ����� ���-�� �����
 * test_self_hitroll() � ������ ������.
 */
void HitData::calc_base_hr(CHAR_DATA *ch)
{
	if (skill_num != SKILL_THROW && skill_num != SKILL_BACKSTAB)
	{
		if (wielded
			&& GET_OBJ_TYPE(wielded) == OBJ_DATA::ITEM_WEAPON
			&& !IS_NPC(ch))
		{
			// Apply HR for light weapon
			int percent = 0;
			switch (weapon_pos)
			{
			case WEAR_WIELD:
				percent = (str_bonus(GET_REAL_STR(ch), STR_WIELD_W) - GET_OBJ_WEIGHT(wielded) + 1) / 2;
				break;
			case WEAR_HOLD:
				percent = (str_bonus(GET_REAL_STR(ch), STR_HOLD_W) - GET_OBJ_WEIGHT(wielded) + 1) / 2;
				break;
			case WEAR_BOTHS:
				percent = (str_bonus(GET_REAL_STR(ch), STR_WIELD_W) +
				   str_bonus(GET_REAL_STR(ch), STR_HOLD_W) - GET_OBJ_WEIGHT(wielded) + 1) / 2;
				break;
			}
			calc_thaco -= MIN(3, MAX(percent, 0));

			// Penalty for unknown weapon type
			// shapirus: ������ ����� ������ �� ��������, ��� �����, ��� unknown_weapon_fault
			// ����� �� ������������. ������ ����� �� ���� ���� ����. ���� ������ ����� �� �������.
			// ���� ����� ����, �� ����� �� ����, � ��������� ������/������ �� ������
			if (ch->get_skill(weap_skill) == 0)
			{
				calc_thaco += (50 - MIN(50, GET_REAL_INT(ch))) / 3;
				dam -= (50 - MIN(50, GET_REAL_INT(ch))) / 6;
			}
			else
			{
				apply_weapon_bonus(GET_CLASS(ch), weap_skill, &dam, &calc_thaco);
			}
		}
		else if (!IS_NPC(ch))
		{
			// �������� � ��� ���������� ���� ������ ��������� :)
			if (!can_use_feat(ch, BULLY_FEAT))
				calc_thaco += 4;
			else	// � ��������� ������� ����� �� ���������� ������
				calc_thaco -= 3;
		}
		// Bonus for leadership
		if (calc_leadership(ch))
			calc_thaco -= 2;
	}

	check_weap_feats(ch);

	if (GET_AF_BATTLE(ch, EAF_STUPOR) || GET_AF_BATTLE(ch, EAF_MIGHTHIT))
	{
		calc_thaco -= MAX(0, (ch->get_skill(weap_skill) - 70) / 8);
	}

	//    AWAKE style - decrease hitroll
	if (GET_AF_BATTLE(ch, EAF_AWAKE)
		&& !can_use_feat(ch, SHADOW_STRIKE_FEAT)
		&& skill_num != SKILL_THROW
		&& skill_num != SKILL_BACKSTAB)
	{
		if (can_auto_block(ch))
		{
			// ��������� �� ����� � ����� � ����� � ������ - ������ �� ������� (�� 0 �� 10)
			calc_thaco += ch->get_skill(SKILL_AWAKE) * 5 / 100;
		}
		else
		{
			// ����� ��� ���� ������ �� ����� ����� �������, �� �������������� ������
			// �� ���� ����� ��� ���, ��� ��� ������ �� ���� ����
			calc_thaco += ((ch->get_skill(SKILL_AWAKE) + 9) / 10) + 2;
		}
	}

	if (!IS_NPC(ch) && skill_num != SKILL_THROW && skill_num != SKILL_BACKSTAB)
	{
		// Casters use weather, int and wisdom
		if (IS_CASTER(ch))
		{
			/*	  calc_thaco +=
				    (10 -
				     complex_skill_modifier (ch, SKILL_THAC0, GAPPLY_SKILL_SUCCESS,
							     10));
			*/
			calc_thaco -= (int)((GET_REAL_INT(ch) - 13) / GET_LEVEL(ch));
			calc_thaco -= (int)((GET_REAL_WIS(ch) - 13) / GET_LEVEL(ch));
		}
		// Skill level increase damage
		if (ch->get_skill(weap_skill) >= 60)
			dam += ((ch->get_skill(weap_skill) - 50) / 10);
	}

	// bless
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_BLESS))
	{
		calc_thaco -= 4;
	}
	// curse
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CURSE))
	{
		calc_thaco += 6;
		dam -= 5;
	}

	// ���� ������ � ���������� �����
	if (PRF_FLAGGED(ch, PRF_POWERATTACK) && can_use_feat(ch, POWER_ATTACK_FEAT))
	{
		calc_thaco += 2;
		dam += 5;
	}
	else if (PRF_FLAGGED(ch, PRF_GREATPOWERATTACK) && can_use_feat(ch, GREAT_POWER_ATTACK_FEAT))
	{
		calc_thaco += 4;
		dam += 10;
	}
	else if (PRF_FLAGGED(ch, PRF_AIMINGATTACK) && can_use_feat(ch, AIMING_ATTACK_FEAT))
	{
		calc_thaco -= 2;
		dam -= 5;
	}
	else if (PRF_FLAGGED(ch, PRF_GREATAIMINGATTACK) && can_use_feat(ch, GREAT_AIMING_ATTACK_FEAT))
	{
		calc_thaco -= 4;
		dam -= 10;
	}

	// Calculate the THAC0 of the attacker
	if (!IS_NPC(ch))
	{
		calc_thaco += thaco((int) GET_CLASS(ch), (int) GET_LEVEL(ch));
	}
	else
	{
		// ����� ����� �� ������������ ��������
		calc_thaco += (25 - GET_LEVEL(ch) / 3);
	}

	//��������� ����� �� �����
	const float p_hitroll = ch->get_cond_penalty(P_HITROLL);

	calc_thaco -= GET_REAL_HR(ch) ? GET_REAL_HR(ch) * p_hitroll : GET_REAL_HR(ch);
	
	// ������������� �������� ������ ���� ��� ���������
	if (can_use_feat(ch, WEAPON_FINESSE_FEAT))
	{
		if (wielded && GET_OBJ_WEIGHT(wielded) > 20)
			calc_thaco -= str_bonus(GET_REAL_STR(ch), STR_TO_HIT) * p_hitroll;
		else
			calc_thaco -= str_bonus(GET_REAL_DEX(ch), STR_TO_HIT) * p_hitroll;
	}
	else
	{
		calc_thaco -= str_bonus(GET_REAL_STR(ch), STR_TO_HIT)?str_bonus(GET_REAL_STR(ch), STR_TO_HIT) * p_hitroll:str_bonus(GET_REAL_STR(ch), STR_TO_HIT);
	}

	if ((skill_num == SKILL_THROW
			|| skill_num == SKILL_BACKSTAB)
		&& wielded
		&& GET_OBJ_TYPE(wielded) == OBJ_DATA::ITEM_WEAPON)
	{
		if (skill_num == SKILL_BACKSTAB)
		{
			calc_thaco -= MAX(0, (ch->get_skill(SKILL_SNEAK) + ch->get_skill(SKILL_HIDE) - 100) / 30);
		}
	}
	else
	{
		// ������ ��������� �������� ��� :)
		calc_thaco += 4;
	}

	//dzMUDiST ��������� !�����������! +Gorrah
	if (affected_by_spell(ch, SPELL_BERSERK))
	{
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_BERSERK))
		{
			calc_thaco -= (12 * ((GET_REAL_MAX_HIT(ch) / 2) - GET_HIT(ch)) / GET_REAL_MAX_HIT(ch));
		}
	}
	
}

/**
 * �������������� ������� ������������ ��������, �� �������� � calc_base_hr()
 * ���, ��� �������� �� ���� � ���� ��� ��� ������ �����������
 * ��� ����� ��������� ����������� ����� ������������ ������������ calc_stat_hr()
 */
void HitData::calc_rand_hr(CHAR_DATA *ch, CHAR_DATA *victim)
{
	//������� ����� �� �����
	const float p_hitroll = ch->get_cond_penalty(P_HITROLL);
	// ����� � ������� 1 �������� �� ������
	// ������������ 10% ������ "���� ����� �����"
	
	if (weapon == LEFT_WEAPON
		&& skill_num != SKILL_THROW
		&& skill_num != SKILL_BACKSTAB
		&& !(wielded && GET_OBJ_TYPE(wielded) == OBJ_DATA::ITEM_WEAPON)
		&& !IS_NPC(ch))
	{
		calc_thaco += (skill_info[SKILL_SHIT].max_percent -
		   train_skill(ch, SKILL_SHIT, skill_info[SKILL_SHIT].max_percent, victim)) / 10;
	}

	// courage
	if (affected_by_spell(ch, SPELL_COURAGE))
	{
		const int range = number(1, skill_info[SKILL_COURAGE].max_percent + GET_REAL_MAX_HIT(ch) - GET_HIT(ch));
		const int prob = train_skill(ch, SKILL_COURAGE, skill_info[SKILL_COURAGE].max_percent, victim);
		if (prob > range)
		{
			dam += ((ch->get_skill(SKILL_COURAGE) + 19) / 20);
			calc_thaco -= ((ch->get_skill(SKILL_COURAGE) + 9) / 20) * p_hitroll;
		}
	}

	// Horse modifier for attacker
	if (!IS_NPC(ch) && skill_num != SKILL_THROW && skill_num != SKILL_BACKSTAB && on_horse(ch))
	{
		train_skill(ch, SKILL_HORSE, skill_info[SKILL_HORSE].max_percent, victim);
//		dam += ((prob + 19) / 10);
//		int range = number(1, skill_info[SKILL_HORSE].max_percent);
//		if (range > prob)
//			calc_thaco += ((range - prob) + 19 / 20);
//		else
//			calc_thaco -= ((prob - range) + 19 / 20);
		calc_thaco += 10 - GET_SKILL(ch, SKILL_HORSE) / 20;
	}

	// not can see (blind, dark, etc)
	if (!CAN_SEE(ch, victim))
		calc_thaco += (can_use_feat(ch, BLIND_FIGHT_FEAT) ? 2 : IS_NPC(ch) ? 6 : 10);
	if (!CAN_SEE(victim, ch))
		calc_thaco -= (can_use_feat(victim, BLIND_FIGHT_FEAT) ? 2 : 8);

	// some protects
	if (AFF_FLAGGED(victim, EAffectFlag::AFF_PROTECT_EVIL) && IS_EVIL(ch))
		calc_thaco += 2;
	if (AFF_FLAGGED(victim, EAffectFlag::AFF_PROTECT_GOOD) && IS_GOOD(ch))
		calc_thaco += 2;

	// "Dirty" methods for battle
	if (skill_num != SKILL_THROW && skill_num != SKILL_BACKSTAB)
	{
		const int prob = (ch->get_skill(weap_skill) + cha_app[GET_REAL_CHA(ch)].illusive) -
			   (victim->get_skill(weap_skill) + int_app[GET_REAL_INT(victim)].observation);
		if (prob >= 30 && !GET_AF_BATTLE(victim, EAF_AWAKE)
				&& (IS_NPC(ch) || !GET_AF_BATTLE(ch, EAF_PUNCTUAL)))
		{
			calc_thaco -= (ch->get_skill(weap_skill) - victim->get_skill(weap_skill) > 60 ? 2 : 1) * p_hitroll;
			if (!IS_NPC(victim))
				dam += (prob >= 70 ? 3 : (prob >= 50 ? 2 : 1));
		}
	}

	// AWAKE style for victim
	if (GET_AF_BATTLE(victim, EAF_AWAKE)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPFIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_MAGICSTOPFIGHT)
		&& !GET_MOB_HOLD(victim)
		&& train_skill(victim, SKILL_AWAKE, skill_info[SKILL_AWAKE].max_percent,
			ch) >= number(1, skill_info[SKILL_AWAKE].max_percent))
	{
		// > � ����� ���? ��� �������� ���������� ��������.
		// ������� ���. �������, ����� ����������� �� ��������
		dam -= IS_NPC(ch) ? 5 : 4;
		calc_thaco += IS_NPC(ch) ? 4 : 2;
	}

	// ����� �������� ������ ��� ������ ������
	if (weap_skill_is <= 80)
		calc_thaco -= (weap_skill_is / 20) * p_hitroll;
	else if (weap_skill_is <= 160)
		calc_thaco -= (4 + (weap_skill_is - 80) / 10) * p_hitroll;
	else
		calc_thaco -= (4 + 8 + (weap_skill_is - 160) / 5) * p_hitroll;
}

// * ������ calc_rand_hr ��� ������ �� '����', ��� �������� � ������ ������.
void HitData::calc_stat_hr(CHAR_DATA *ch)
{
	//������� ����� �� �����
	const float p_hitroll = ch->get_cond_penalty(P_HITROLL);
	// ����� � ������� 1 �������� �� ������
	// ������������ 10% ������ "���� ����� �����"
	if (weapon == LEFT_WEAPON
		&& skill_num != SKILL_THROW
		&& skill_num != SKILL_BACKSTAB
		&& !(wielded && GET_OBJ_TYPE(wielded) == OBJ_DATA::ITEM_WEAPON)
		&& !IS_NPC(ch))
	{
		calc_thaco += (skill_info[SKILL_SHIT].max_percent - ch->get_skill(SKILL_SHIT)) / 10;
	}

	// courage
	if (affected_by_spell(ch, SPELL_COURAGE))
	{
		dam += ((ch->get_skill(SKILL_COURAGE) + 19) / 20);
		calc_thaco -= ((ch->get_skill(SKILL_COURAGE) + 9) / 20) * p_hitroll;
	}

	// Horse modifier for attacker
	if (!IS_NPC(ch) && skill_num != SKILL_THROW && skill_num != SKILL_BACKSTAB && on_horse(ch))
	{
		const int prob = ch->get_skill(SKILL_HORSE);
		const int range = skill_info[SKILL_HORSE].max_percent / 2;

		dam += ((prob + 19) / 10);

		if (range > prob)
			calc_thaco += ((range - prob) + 19 / 20);
		else
			calc_thaco -= ((prob - range) + 19 / 20);
	}

	// ����� �������� ������ ��� ������ ������
	if (ch->get_skill(weap_skill) <= 80)
		calc_thaco -= (ch->get_skill(weap_skill) / 20)*p_hitroll;
	else if (ch->get_skill(weap_skill) <= 160)
		calc_thaco -= (4 + (ch->get_skill(weap_skill) - 80) / 10)*p_hitroll;
	else
		calc_thaco -= (4 + 8 + (ch->get_skill(weap_skill) - 160) / 5)*p_hitroll;
}

// * ������� ����� ������ ������.
void HitData::calc_ac(CHAR_DATA *victim)
{
	
	
	// Calculate the raw armor including magic armor.  Lower AC is better.
	victim_ac += compute_armor_class(victim);
	victim_ac /= 10;
	//������� ����� �� �����
	if (!IS_NPC(victim) && victim_ac<5) { //��� ��������
		const int p_ac = (1 - victim->get_cond_penalty(P_AC))*40;
		if (p_ac)  {
			if (victim_ac+p_ac>5) {
				victim_ac = 5;
			} else {
				victim_ac+=p_ac;
			}
		}
	}
	
	if (GET_POS(victim) < POS_FIGHTING)
		victim_ac += 4;
	if (GET_POS(victim) < POS_RESTING)
		victim_ac += 3;
	if (AFF_FLAGGED(victim, EAffectFlag::AFF_HOLD))
		victim_ac += 4;
	if (AFF_FLAGGED(victim, EAffectFlag::AFF_CRYING))
		victim_ac += 4;
}

// * ��������� �������� �������: ������, �����, ����, ����.
void HitData::check_defense_skills(CHAR_DATA *ch, CHAR_DATA *victim)
{
	if (!hit_no_parry)
	{
		// ���������� �������� ������
		const auto& people = world[ch->in_room]->people;
		for (const auto vict : people)
		{
			if (dam < 0)
			{
				break;
			}

			hit_touching(ch, vict, &dam);
		}
	}

	if (dam > 0
		&& !hit_no_parry
		&& GET_AF_BATTLE(victim, EAF_DEVIATE)
		&& GET_WAIT(victim) <= 0
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPFIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_MAGICSTOPFIGHT)
		&& GET_MOB_HOLD(victim) == 0
		&& BATTLECNTR(victim) < (GET_LEVEL(victim) + 7) / 8)
	{
		// ���������� �������   ����������
		hit_deviate(ch, victim, &dam);
	}
	else
	if (dam > 0
		&& !hit_no_parry
		&& GET_AF_BATTLE(victim, EAF_PARRY)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPFIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_MAGICSTOPFIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPRIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPLEFT)
		&& GET_WAIT(victim) <= 0
		&& GET_MOB_HOLD(victim) == 0)
	{
		// ���������� �������  ����������
		hit_parry(ch, victim, weap_skill, hit_type, &dam);
	}
	else
	if (dam > 0
		&& !hit_no_parry
		&& GET_AF_BATTLE(victim, EAF_MULTYPARRY)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPFIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_MAGICSTOPFIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPRIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPLEFT)
		&& BATTLECNTR(victim) < (GET_LEVEL(victim) + 4) / 5
		&& GET_WAIT(victim) <= 0
		&& GET_MOB_HOLD(victim) == 0)
	{
		// ���������� �������  ������� ������
		hit_multyparry(ch, victim, weap_skill, hit_type, &dam);
	}
	else
	if (dam > 0
		&& !hit_no_parry
		&& ((GET_AF_BATTLE(victim, EAF_BLOCK) || can_auto_block(victim)) && GET_POS(victim) > POS_SITTING)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPFIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_MAGICSTOPFIGHT)
		&& !AFF_FLAGGED(victim, EAffectFlag::AFF_STOPLEFT)
		&& GET_WAIT(victim) <= 0
		&& GET_MOB_HOLD(victim) == 0
		&& BATTLECNTR(victim) < (GET_LEVEL(victim) + 8) / 9)
	{
		// ���������� �������   �����������
		hit_block(ch, victim, &dam);
	}
}

/**
 * � ������ ������:
 * ���������� �������� � �����
 * ���������� ������ �� ������������ ����
 */
void HitData::add_weapon_damage(CHAR_DATA *ch)
{
	int damroll = dice(GET_OBJ_VAL(wielded, 1),
		GET_OBJ_VAL(wielded, 2));

	if (IS_NPC(ch)
		&& !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM)
		&& !(MOB_FLAGGED(ch, MOB_ANGEL)|| MOB_FLAGGED(ch, MOB_GHOST)))
	{
		damroll *= MOB_DAMAGE_MULT;
	}
	else
	{
		damroll = MIN(damroll,
			damroll * GET_OBJ_CUR(wielded) / MAX(1, GET_OBJ_MAX(wielded)));
	}

	damroll = calculate_strconc_damage(ch, wielded, damroll);
	dam += MAX(1, damroll);
}

// * ���������� ������ �� ����� ��� � ������.
void HitData::add_hand_damage(CHAR_DATA *ch)
{
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_STONEHAND))
		dam += dice(2, 3);
	else
		dam += number(1, 3);

	if (can_use_feat(ch, BULLY_FEAT))
	{
		dam += GET_LEVEL(ch) / 5;
		dam += MAX(0, GET_REAL_STR(ch) - 25);
	}

	// �������������� ����������� ��� ������ � � ��������� (�������� ������������)
	// <��� ��������> <����������>
	// 0  50%
	// 5 100%
	// 10 150%
	// 15 200%
	// �� ����� �� ������
	if (!GET_AF_BATTLE(ch, EAF_MIGHTHIT)
		|| get_flags()[FightSystem::CRIT_HIT]) //� ������ ������ ���� ���� ��������
	{
		int modi = 10 * (5 + (GET_EQ(ch, WEAR_HANDS) ? MIN(GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_HANDS)), 18) : 0)); //��� �������� ������ 18 �� �����������
		if (IS_NPC(ch) || can_use_feat(ch, BULLY_FEAT))
		{
			modi = MAX(100, modi);
		}
		dam *= modi / 100;
	}
}

// * ������ ����� �� ����������� ���� (�� ������).
void HitData::calc_crit_chance(CHAR_DATA *ch)
{
	dam_critic = 0;
	int calc_critic = 0;

	// ����, ������ � ��-���������� ������� �� ����� ������� //
	if ((!IS_NPC(ch) && !IS_MAGIC_USER(ch) && !IS_DRUID(ch))
		|| (IS_NPC(ch) && (!AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM) && !AFF_FLAGGED(ch, EAffectFlag::AFF_HELPER))))
	{
		calc_critic = MIN(ch->get_skill(weap_skill), 70);
		// ���������� ���� �� ������ ��������� ���� ������������ ��������� //
		for (int i = PUNCH_MASTER_FEAT; i <= BOWS_MASTER_FEAT; i++)
		{
			if ((ubyte) feat_info[i].affected[0].location == weap_skill && can_use_feat(ch, i))
			{
				calc_critic += MAX(0, ch->get_skill(weap_skill) -  70);
				break;
			}
		}
		if (can_use_feat(ch, THIEVES_STRIKE_FEAT))
		{
			calc_critic += ch->get_skill(SKILL_BACKSTAB);
		}
		//������ ��� ��������� �����?
		//������ ����������, ������ ������� ��� ����� ����������.
		//� ���� ���� �� �������� -- ���� �������� �� ������.
		if (!IS_NPC(ch))
		{
			calc_critic += (int)(ch->get_skill(SKILL_PUNCTUAL) / 2);
			calc_critic += (int)(ch->get_skill(SKILL_NOPARRYHIT) / 3);
		}
		if (IS_NPC(ch) && !AFF_FLAGGED(ch, EAffectFlag::AFF_CHARM))
		{
			calc_critic += GET_LEVEL(ch);
		}
	}
	else
	{
		//Polud �� ������ - ��� ����� � �� �������
		reset_flag(FightSystem::CRIT_HIT);
	}

	//critical hit ignore magic_shields and armour
	if (number(0, 2000) < calc_critic)
	{
		set_flag(FightSystem::CRIT_HIT);
	}
	else
	{
		reset_flag(FightSystem::CRIT_HIT);
	}
}

/* �������� ������ �� "������" � ������ �������, ���� ������� ��������� */
double expedient_bonus_damage(CHAR_DATA *ch, CHAR_DATA *victim)
{
	double factor = 1.00;

	switch (ch->get_extra_attack_mode())
	{
	case EXTRA_ATTACK_CUT_SHORTS:
        if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_AWARE))
        {
		factor = 1 + ch->get_skill(SKILL_SHORTS)/35.0;
        } else
        {
		factor = 1 + ch->get_skill(SKILL_SHORTS)/30.0;
        }
        break;
	case EXTRA_ATTACK_CUT_PICK:
		factor = 1 + ch->get_skill(SKILL_PICK)/70.0;
        break;
	default:
		factor = 1.00;
        break;
	}
	if (PRF_FLAGGED(ch, PRF_CODERINFO) || PRF_FLAGGED(ch, PRF_TESTER))
	{
        sprintf(buf, "&C��������� ����� �� ������ = %f&n\r\n", factor);
        send_to_char(buf,ch);
    }

    return factor;
};

/**
* ��������� ������ �������, �����, ������, �����, ���.
* \param weapon = 1 - ����� ������ ��� ����� ������
*               = 2 - ����� ����� �����
*/
void hit(CHAR_DATA *ch, CHAR_DATA *victim, int type, int weapon)
{
	if (!victim)
	{
		return;
	}
	if (!ch || ch->purged() || victim->purged())
	{
		log("SYSERROR: ch = %s, victim = %s (%s:%d)",
			ch ? (ch->purged() ? "purged" : "true") : "false",
			victim->purged() ? "purged" : "true", __FILE__, __LINE__);
		return;
	}
	// Do some sanity checking, in case someone flees, etc.
	if (ch->in_room != IN_ROOM(victim) || ch->in_room == NOWHERE)
	{
		if (ch->get_fighting() && ch->get_fighting() == victim)
		{
			stop_fighting(ch, TRUE);
		}
		return;
	}
	// Stand awarness mobs
	if (CAN_SEE(victim, ch)
		&& !victim->get_fighting()
		&& ((IS_NPC(victim) && (GET_HIT(victim) < GET_MAX_HIT(victim)
			|| MOB_FLAGGED(victim, MOB_AWARE)))
			|| AFF_FLAGGED(victim, EAffectFlag::AFF_AWARNESS))
		&& !GET_MOB_HOLD(victim) && GET_WAIT(victim) <= 0)
	{
		set_battle_pos(victim);
	}

	//go_autoassist(ch);

	// ����� ������������� ����� ��� �����
	HitData hit_params;
	//����������� �������, ������� ������������ ����� hit()
	//c ���_�����, � ���_�����.
	hit_params.skill_num = type != SKILL_STUPOR && type != SKILL_MIGHTHIT ? type : TYPE_UNDEFINED;
	hit_params.weapon = weapon;
	hit_params.init(ch, victim);

	//  �������������� ���. ����� ���������� �� ��������� ���. �����
	if (AFF_FLAGGED(ch, EAffectFlag::AFF_CLOUD_OF_ARROWS) && hit_params.skill_num < 0
		&& (ch->get_fighting()
		|| (!GET_AF_BATTLE(ch, EAF_MIGHTHIT) && !GET_AF_BATTLE(ch, EAF_STUPOR))))
	{
		// ����� ����� �������� ����������� victim, �� ch �� ����� �� �������
		mag_damage(1, ch, victim, SPELL_MAGIC_MISSILE, SAVING_REFLEX);
		if (ch->purged() || victim->purged())
		{
			return;
		}
	}

	// ���������� ��������/��
	hit_params.calc_base_hr(ch);
	hit_params.calc_rand_hr(ch, victim);
	hit_params.calc_ac(victim);

	const int victim_lvl_miss = victim->get_level() + victim->get_remort();
	const int ch_lvl_miss = ch->get_level() + ch->get_remort();

	// ������ ��������� ������ ��� ���
	if (victim_lvl_miss - ch_lvl_miss <= 5
		|| (!IS_NPC(ch) && !IS_NPC(victim)))
	{
		// 5% ���� ���������, ���� ���� � �������� 5 ������� ��� ��� ������
		if ((number(1, 100) <= 5))
		{
			hit_params.dam = 0;
			hit_params.extdamage(ch, victim);
			hitprcnt_mtrigger(victim);
			return;
		}
	}
	else
	{
		// ���� ��������� = ������� ������� � ������
		const int diff = victim_lvl_miss - ch_lvl_miss;
		if (number(1, 100) <= diff)
		{
			hit_params.dam = 0;
			hit_params.extdamage(ch, victim);
			hitprcnt_mtrigger(victim);
			return;
		}
	}
	// ������ ���� 5% ����������� ������� (diceroll == 20)
	if ((hit_params.diceroll < 20 && AWAKE(victim))
		&& hit_params.calc_thaco - hit_params.diceroll > hit_params.victim_ac)
	{
		hit_params.dam = 0;
		hit_params.extdamage(ch, victim);
		hitprcnt_mtrigger(victim);
		return;
	}
	// ���� � ������ ��������� ����� ���������� ��������
	if (AFF_FLAGGED(victim, EAffectFlag::AFF_BLINK)
		&& !GET_AF_BATTLE(ch, EAF_MIGHTHIT)
		&& !GET_AF_BATTLE(ch, EAF_STUPOR)
		&& (!(hit_params.skill_num == SKILL_BACKSTAB && can_use_feat(ch, THIEVES_STRIKE_FEAT)))
		&& number(1, 100) <= 20)
	{
		sprintf(buf,
			"%s�� ��������� �� ������� �� ���� ������ ����������.%s\r\n",
			CCINRM(victim, C_NRM), CCNRM(victim, C_NRM));
		send_to_char(buf, victim);
		hit_params.dam = 0;
		hit_params.extdamage(ch, victim);
		return;
	}

	// ��������� �� ����� ���������
	hit_params.dam += get_real_dr(ch);
	if (can_use_feat(ch, SHOT_FINESSE_FEAT))
		hit_params.dam += str_bonus(GET_REAL_DEX(ch), STR_TO_DAM);
	else
		hit_params.dam += str_bonus(GET_REAL_STR(ch), STR_TO_DAM);

	// ������ ������� �������� ������
	if (hit_params.dam > 0)
	{
		const int min_rnd = hit_params.dam - hit_params.dam / 4;
		const int max_rnd = hit_params.dam + hit_params.dam / 4;
		hit_params.dam = MAX(1, number(min_rnd, max_rnd));
	}

	if (GET_EQ(ch, WEAR_BOTHS) && hit_params.weap_skill != SKILL_BOWS)
		hit_params.dam *= 2;

	if (IS_NPC(ch))
	{
		hit_params.dam += dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
	}

	// ������ ����������� ������
	hit_params.calc_crit_chance(ch);

	// ������/���� � ������������ ����� ������, � ���� ���������
	if (hit_params.wielded
		&& GET_OBJ_TYPE(hit_params.wielded) == OBJ_DATA::ITEM_WEAPON)
	{
		hit_params.add_weapon_damage(ch);
		// ������� ����
		const int tmp_dam = calculate_noparryhit_dmg(ch, hit_params.wielded);
		if (tmp_dam > 0)
		{
			// 0 ����� � ���� = 70% ��������, ������ ����� * 0.4 (�� 5 ������)
			const int round_dam = tmp_dam * 7 / 10;
			if (can_use_feat(ch, SNEAKRAGE_FEAT))
			{
				if (ROUND_COUNTER(ch) >= 1 && ROUND_COUNTER(ch) <= 3)
				{
					hit_params.dam *= ROUND_COUNTER(ch);
				}
			}
			if (hit_params.skill_num == SKILL_BACKSTAB || ROUND_COUNTER(ch) <= 0)
			{
				hit_params.dam += round_dam;
			}
			else
			{
				hit_params.dam += round_dam * MIN(3, ROUND_COUNTER(ch));
			}
		}
	}
	else
	{
		hit_params.add_hand_damage(ch);
	}

	// Gorrah: ����� � ������������ �� ������ "�������� �����"
	// x5 �� ���� ������ ����
	if (GET_AF_BATTLE(ch, EAF_IRON_WIND))
		hit_params.dam += ch->get_skill(SKILL_IRON_WIND) / 2;
	//dzMUDiST ��������� !�����������! +Gorrah
	if (affected_by_spell(ch, SPELL_BERSERK))
	{
		if (AFF_FLAGGED(ch, EAffectFlag::AFF_BERSERK))
		{
			hit_params.dam = (hit_params.dam * MAX(150, 150 + GET_LEVEL(ch) + dice(0, GET_REMORT(ch)) * 2)) / 100;
		}
	}

	// at least 1 hp damage min per hit
	hit_params.dam = MAX(1, hit_params.dam);
	if (GET_SKILL(ch, SKILL_HORSE) > 100 && on_horse(ch))
	{
		hit_params.dam *= 1 + (GET_SKILL(ch, SKILL_HORSE) - 100) / 500.0; // �� ������� �� +20%
	}

	// ������� �� alt_equip, ����� �� ������� ����������� �����
	if (damage_mtrigger(ch, victim))
	{
		return;
	}

	if (hit_params.weapon_pos)
	{
		alt_equip(ch, hit_params.weapon_pos, hit_params.dam, 10);
	}

	if (hit_params.skill_num == SKILL_BACKSTAB)
	{
		hit_params.reset_flag(FightSystem::CRIT_HIT);
		hit_params.set_flag(FightSystem::IGNORE_FSHIELD);
		if (can_use_feat(ch, THIEVES_STRIKE_FEAT) || can_use_feat(ch, SHADOW_STRIKE_FEAT))
		{
			// ���� ������� ����� ���������
			// � ����� ����!
			hit_params.set_flag(FightSystem::IGNORE_ARMOR);
		}
		else
		{
			//���� ������� ���������
			hit_params.set_flag(FightSystem::HALF_IGNORE_ARMOR);
		}
		// ����� ������� ������
		if (can_use_feat(ch, SHADOW_STRIKE_FEAT) && IS_NPC(victim))
		{
			hit_params.dam *= backstab_mult(GET_LEVEL(ch)) * (1.0 + ch->get_skill(SKILL_NOPARRYHIT) / 200.0);
		}
		else if (can_use_feat(ch, THIEVES_STRIKE_FEAT))
		{
			if (victim->get_fighting())
			{
				hit_params.dam *= backstab_mult(GET_LEVEL(ch));
			}
			else
			{
				hit_params.dam *= backstab_mult(GET_LEVEL(ch)) * 1.3;
			}
		}
		else
		{
			hit_params.dam *= backstab_mult(GET_LEVEL(ch));
		}
		
		if (can_use_feat(ch, SHADOW_STRIKE_FEAT)
			&& IS_NPC(victim)
			&& !(AFF_FLAGGED(victim, EAffectFlag::AFF_SHIELD)
				&& !(MOB_FLAGGED(victim, MOB_PROTECT)))
			&& (number(1,100) <= 6 * ch->get_cond_penalty(P_HITROLL)) //�������� ���� ������� ������� ����
			&& IS_NPC(victim)
			&& !victim->get_role(MOB_ROLE_BOSS))
		{
			    GET_HIT(victim) = 1;
			    hit_params.dam = 2000; // ��� ����������
			    send_to_char(ch, "&G����� � ������, ��������!&n\r\n");
			    hit_params.extdamage(ch, victim);
			    return;
		}
		
		if (number(1, 100) < calculate_crit_backstab_percent(ch) * ch->get_cond_penalty(P_HITROLL)
			&& !general_savingthrow(ch, victim, SAVING_REFLEX, dex_bonus(GET_REAL_DEX(ch))))
		{
			hit_params.dam = static_cast<int>(hit_params.dam * hit_params.crit_backstab_multiplier(ch, victim));
			if ((hit_params.dam > 0)
				&& !AFF_FLAGGED(victim, EAffectFlag::AFF_SHIELD)
				&& !(MOB_FLAGGED(victim, MOB_PROTECT)))
			{
				send_to_char("&G����� � ������!&n\r\n", ch);
			}
		}

		const int initial_dam = hit_params.dam;
		//Adept: ��������� ������� �� ����. �����������
		hit_params.dam = calculate_resistance_coeff(victim, VITALITY_RESISTANCE, hit_params.dam);
		// ������� �������� ������� ���������
		if (PRF_FLAGGED(ch, PRF_CODERINFO) || PRF_FLAGGED(ch, PRF_TESTER))
		{
			sprintf(buf, "&C������ ����� �� ����� ��������� = %d, ��������� = %d, ���������� ������� = %g&n\r\n", initial_dam, GET_RESIST(victim, VITALITY_RESISTANCE), float(hit_params.dam) / initial_dam);
			send_to_char(buf,ch);
		}
		// ����� ����
		if (can_use_feat(ch, SHADOW_STRIKE_FEAT) && !IS_NPC(ch))
		{
			hit_params.dam = MIN(8000 + GET_REMORT(ch) * 20 * GET_LEVEL(ch), hit_params.dam);
		}

		if (IS_IMPL(ch) || IS_IMPL(victim) || PRF_FLAGGED(ch, PRF_TESTER))
		{
			sprintf(buf, "&C������ ����� ����� = %d&n\r\n", hit_params.dam);
			send_to_char(buf,ch);
			sprintf(buf, "&R������ �����  ����� = %d&n\r\n", hit_params.dam);
			send_to_char(buf,victim);
		}
		hit_params.extdamage(ch, victim);
		return;
	}

	if (hit_params.skill_num == SKILL_THROW)
	{
		hit_params.set_flag(FightSystem::IGNORE_FSHIELD);
		hit_params.dam *= (calculate_skill(ch, SKILL_THROW, victim) + 10) / 10;
		if (IS_NPC(ch))
		{
			hit_params.dam = MIN(300, hit_params.dam);
		}
		hit_params.dam = calculate_resistance_coeff(victim, VITALITY_RESISTANCE, hit_params.dam);
		hit_params.extdamage(ch, victim);
		if (IS_IMPL(ch) || IS_IMPL(victim))
		{
			sprintf(buf, "&C������ ������� ����� = %d&n\r\n", hit_params.dam);
			send_to_char(buf,ch);
			sprintf(buf, "&R������ ������� ����� = %d&n\r\n", hit_params.dam);
			send_to_char(buf,victim);
		}
		return;
	}

	if (GET_AF_BATTLE(ch, EAF_PUNCTUAL)
		&& GET_PUNCTUAL_WAIT(ch) <= 0
		&& GET_WAIT(ch) <= 0
		&& (hit_params.diceroll >= 18 - GET_MOB_HOLD(victim))
		&& !MOB_FLAGGED(victim, MOB_NOTKILLPUNCTUAL))
	{
		const int percent = train_skill(ch, SKILL_PUNCTUAL, skill_info[SKILL_PUNCTUAL].max_percent, victim);
		if (!PUNCTUAL_WAITLESS(ch))
		{
			PUNCTUAL_WAIT_STATE(ch, 1 * PULSE_VIOLENCE);
		}

		if (percent >= number(1, skill_info[SKILL_PUNCTUAL].max_percent)
			&& (hit_params.calc_thaco - hit_params.diceroll < hit_params.victim_ac - 5
				|| percent >= skill_info[SKILL_PUNCTUAL].max_percent))
		{
			hit_params.set_flag(FightSystem::CRIT_HIT);
			// CRIT_HIT � ��� ���� �������, �� ��� �������
			hit_params.set_flag(FightSystem::IGNORE_FSHIELD);
			hit_params.dam_critic = do_punctual(ch, victim, hit_params.wielded);
			if (IS_IMPL(ch) || IS_IMPL(victim))
			{
				sprintf(buf, "&C������ ����� ����� = %d&n\r\n", hit_params.dam_critic);
				send_to_char(buf,ch);
			}

			if (!PUNCTUAL_WAITLESS(ch))
			{
				PUNCTUAL_WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
			}
		}
	}

	if (ch->get_extra_attack_mode() != EXTRA_ATTACK_UNUSED)
        hit_params.dam *= expedient_bonus_damage(ch, victim);

	if (PRF_FLAGGED(ch, PRF_CODERINFO) || PRF_FLAGGED(ch, PRF_TESTER))
	{
		sprintf(buf, "&C������� ����� = %d&n\r\n", hit_params.dam);
		send_to_char(buf,ch);
	}

	// �������� �����, ���� � ����������� ���� ���
	if ((GET_AF_BATTLE(ch, EAF_STUPOR) || GET_AF_BATTLE(ch, EAF_MIGHTHIT))
		&& GET_WAIT(ch) > 0)
	{
		CLR_AF_BATTLE(ch, EAF_STUPOR);
		CLR_AF_BATTLE(ch, EAF_MIGHTHIT);
	}

	// ��������� �������� ������ (������, �����, �����, ����, ����)
	hit_params.check_defense_skills(ch, victim);
	
	//����� ����� �� ������
	if (hit_params.dam)
		hit_params.dam *= ch->get_cond_penalty(P_DAMROLL);
	// �������� �����
	const int made_dam = hit_params.extdamage(ch, victim);

	//��������� ����, ����� ������ ���� � �����������
	//�������� ��� ��������. ����� ��� ��� ���� ������ ��
	//�������� ������� ����������(�����, ����� � �.�.)
	if (CHECK_WAIT(ch)
		&& made_dam == -1
		&& (type == SKILL_STUPOR
			|| type == SKILL_MIGHTHIT))
	{
		ch->set_wait(0u);
	}

	// check if the victim has a hitprcnt trigger
	if (made_dam != -1)
	{
		// victim is not dead after hit
		hitprcnt_mtrigger(victim);
	}
}

OBJ_DATA *GetUsedWeapon(CHAR_DATA *ch, int SelectedWeapon)
{
	OBJ_DATA *UsedWeapon = nullptr;

	if (SelectedWeapon == RIGHT_WEAPON)
	{
		if (!(UsedWeapon = GET_EQ(ch, WEAR_WIELD)))
			UsedWeapon = GET_EQ(ch, WEAR_BOTHS);
	}
	else if (SelectedWeapon == LEFT_WEAPON)
		UsedWeapon = GET_EQ(ch, WEAR_HOLD);

    return UsedWeapon;
}

//**** This function realize second shot for bows ******
void exthit(CHAR_DATA * ch, int type, int weapon)
{
	if (!ch || ch->purged())
	{
		log("SYSERROR: ch = %s (%s:%d)",
				ch ? (ch->purged() ? "purged" : "true") : "false",
				__FILE__, __LINE__);
		return;
	}

	OBJ_DATA *wielded = NULL;
	int percent = 0, prob = 0, div = 0, moves = 0;

	if (IS_NPC(ch))
	{
		if (MOB_FLAGGED(ch, MOB_EADECREASE) && weapon > 1)
		{
			if (ch->mob_specials.ExtraAttack * GET_HIT(ch) * 2 < weapon * GET_REAL_MAX_HIT(ch))
				return;
		}
		if (MOB_FLAGGED(ch, (MOB_FIREBREATH | MOB_GASBREATH | MOB_FROSTBREATH |
							 MOB_ACIDBREATH | MOB_LIGHTBREATH)))
		{
			for (prob = percent = 0; prob <= 4; prob++)
				if (MOB_FLAGGED(ch, (INT_TWO | (1 << prob))))
					percent++;
			percent = weapon % percent;
			for (prob = 0; prob <= 4; prob++)
			{
				if (MOB_FLAGGED(ch, (INT_TWO | (1 << prob))))
				{
					if (0 == percent)
					{
						break;
					}

					--percent;
				}
			}

			if (MOB_FLAGGED(ch, MOB_AREA_ATTACK))
			{
				const auto people = world[ch->in_room]->people;	// make copy because inside loop this list might be changed.
				for (const auto& tch : people)
				{
					if (IS_IMMORTAL(tch)
						|| ch->in_room == NOWHERE
						|| IN_ROOM(tch) == NOWHERE)
					{
						continue;
					}

					if (tch != ch
						&& !same_group(ch, tch))
					{
						mag_damage(GET_LEVEL(ch), ch, tch, SPELL_FIRE_BREATH + MIN(prob, 4), SAVING_CRITICAL);
					}
				}
			}
			else
			{
				mag_damage(GET_LEVEL(ch), ch, ch->get_fighting(), SPELL_FIRE_BREATH + MIN(prob, 4), SAVING_CRITICAL);
			}

			return;
		}
	}

	wielded = GetUsedWeapon(ch, weapon);

	if (wielded
			&& !GET_EQ(ch, WEAR_SHIELD)
			&& GET_OBJ_SKILL(wielded) == SKILL_BOWS
			&& GET_EQ(ch, WEAR_BOTHS))
	{
		// ��� � ����� ����� - ����� ���. ��� ������� �������
		if (can_use_feat(ch, DOUBLESHOT_FEAT) && !ch->get_skill(SKILL_ADDSHOT)
				&& MIN(850, 200 + ch->get_skill(SKILL_BOWS) * 4 + GET_REAL_DEX(ch) * 5) >= number(1, 1000))
		{
			hit(ch, ch->get_fighting(), type, weapon);
			prob = 0;
		}
		else if (ch->get_skill(SKILL_ADDSHOT) > 0)
		{
			addshot_damage(ch, type, weapon);
		}
	}

	/*
	������ ������ ������ "�������� �����"
	������ �������������� ����� ������ ��������� 100%
	������ �������������� ����� ������ �������� ���������� � 80%+ ������, �� �� ����� ��� � 80% ������������
	������ �������������� ����� ����� �������� ���������� �����, �� �� ����� ��� � 80% ������������
	������ �������������� ����� ����� �������� ���������� � 170%+ ������, �� �� ����� ��� � 30% �����������
	*/
	if (PRF_FLAGS(ch).get(PRF_IRON_WIND))
	{
		percent = ch->get_skill(SKILL_IRON_WIND);
		moves = GET_MAX_MOVE(ch) / (6 + MAX(10, percent) / 10);
		prob = GET_AF_BATTLE(ch, EAF_IRON_WIND);
		if (prob && !check_moves(ch, moves))
		{
			CLR_AF_BATTLE(ch, EAF_IRON_WIND);
		}
		else if (!prob && (GET_MOVE(ch) > moves))
		{
			SET_AF_BATTLE(ch, EAF_IRON_WIND);
		};
	};
	if (GET_AF_BATTLE(ch, EAF_IRON_WIND))
	{
		(void) train_skill(ch, SKILL_IRON_WIND, skill_info[SKILL_IRON_WIND].max_percent, ch->get_fighting());
		if (weapon == RIGHT_WEAPON)
		{
			div = 100 + MIN(80, MAX(1, percent - 80));
			prob = 100;
		}
		else
		{
			div = MIN(80, percent + 10);
			prob = 80 - MIN(30, MAX(0, percent - 170));
		};
		while (div > 0)
		{
			if (number(1, 100) < div)
				hit(ch, ch->get_fighting(), type, weapon);
			div -= prob;
		};
	};

	hit(ch, ch->get_fighting(), type, weapon);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
