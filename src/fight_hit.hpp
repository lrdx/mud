// Part of Bylins http://www.mud.ru

#ifndef _FIGHT_HIT_HPP_
#define _FIGHT_HIT_HPP_

#include "fight_constants.hpp"
#include "AffectHandler.hpp"
#include "utils.h"
#include "conf.h"
#include "sysdep.h"
#include "structs.h"

#define RIGHT_WEAPON 1
#define LEFT_WEAPON  2

struct HitData
{
	HitData() : weapon(0), wielded(0), weapon_pos(WEAR_WIELD), weap_skill(SKILL_INVALID),
		weap_skill_is(0), skill_num(-1), hit_type(0), hit_no_parry(false),
		ch_start_pos(-1), victim_start_pos(-1), victim_ac(0), calc_thaco(0),
		dam(0), dam_critic(0)
	{
		diceroll = number(100, 2099) / 100;
	};

	// hit
	void init(CHAR_DATA *ch, CHAR_DATA *victim);
	void calc_base_hr(CHAR_DATA *ch);
	void calc_rand_hr(CHAR_DATA *ch, CHAR_DATA *victim);
	void calc_stat_hr(CHAR_DATA *ch);
	void calc_ac(CHAR_DATA *victim);
	void add_weapon_damage(CHAR_DATA *ch);
	void add_hand_damage(CHAR_DATA *ch);
	void check_defense_skills(CHAR_DATA *ch, CHAR_DATA *victim);
	void calc_crit_chance(CHAR_DATA *ch);
	void check_weap_feats(CHAR_DATA *ch);
	double crit_backstab_multiplier(CHAR_DATA *ch, CHAR_DATA *victim);

	// extdamage
	int extdamage(CHAR_DATA *ch, CHAR_DATA *victim);
	void try_mighthit_dam(CHAR_DATA *ch, CHAR_DATA *victim);
	void try_stupor_dam(CHAR_DATA *ch, CHAR_DATA *victim);
	void compute_critical(CHAR_DATA *ch, CHAR_DATA *victim);

	// init()
	// 1 - ����� ������ ��� ����� ������ (RIGHT_WEAPON),
	// 2 - ����� ����� ����� (LEFT_WEAPON)
	int weapon;
	// �����, ������� � ������ ������ ������������ ����
	OBJ_DATA *wielded;
	// ����� ������� (NUM_WEARS) �����
	int weapon_pos;
	// ����� �����, ������ �� ����� ��� ����� ���
	ESkill weap_skill;
	// ���� ����� weap_skill � ����, ������ ����� train_skill (����� ���� ��������)
	int weap_skill_is;
	// ��������� ������ �� ������ ������� ���������
	int diceroll;
	// ����� �����, ��������� �� ������ hit(), ����� ���� TYPE_UNDEFINED
	// � ����� ���� < 0 - ���������, ��� ���� ������� ������ hit_type
	// ���� >= 0 - ���������, ��� ���� ������
	int skill_num;
	// ��� ����� ������ ��� ������ (attack_hit_text[])
	// ������ � ����� ������ ���������� �� skill_num
	int hit_type;
	// true - ���� �� ����������/�� ��������/�� �������� � �.�.
	bool hit_no_parry;
	// ������� ���������� �� ������ �����
	int ch_start_pos;
	// ������� ������ �� ������ �����
	int victim_start_pos;

	// ������������� �� ���� ���
	// �� ������ ��� ������� ���������
	int victim_ac;
	// ������� ���������� ��� ������� ���������
	int calc_thaco;
	// ����� ����������
	int dam;
	// flags[CRIT_HIT] = true, dam_critic = 0 - ����������� ����
	// flags[CRIT_HIT] = true, dam_critic > 0 - ���� ������ ������
	int dam_critic;

public:
	using flags_t = std::bitset<FightSystem::HIT_TYPE_FLAGS_NUM>;

	const flags_t& get_flags() const { return m_flags; }
	void set_flag(const size_t flag) { m_flags.set(flag); }
	void reset_flag(const size_t flag) { m_flags.reset(flag); }

private:
	// �����-������� ����� ������, ��� �� ���������� � damage()
	flags_t m_flags;
};

int check_agro_follower(CHAR_DATA * ch, CHAR_DATA * victim);
void set_battle_pos(CHAR_DATA * ch);

void gain_battle_exp(CHAR_DATA *ch, CHAR_DATA *victim, int dam);
void perform_group_gain(CHAR_DATA * ch, CHAR_DATA * victim, int members, int koef);
void group_gain(CHAR_DATA * ch, CHAR_DATA * victim);

char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural);
bool check_valid_chars(CHAR_DATA *ch, CHAR_DATA *victim, const char *fname, int line);

void exthit(CHAR_DATA * ch, int type, int weapon);
void hit(CHAR_DATA *ch, CHAR_DATA *victim, int type, int weapon);

#endif // _FIGHT_LOCAL_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
