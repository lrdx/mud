/*************************************************************************
*   File: features.hpp                                 Part of Bylins    *
*   Features code header                                                 *
*                                                                        *
*  $Author$                                                     *
*  $Date$                                          *
*  $Revision$                                                  *
**************************************************************************/
#ifndef __FEATURES_HPP__
#define __FEATURES_HPP__

#include "structs.h"

#include <array>
#include <bitset>

using std::bitset;

#define THAC0_FEAT				0   //DO NOT USED
#define BERSERK_FEAT			1   //������������ ������
#define PARRY_ARROW_FEAT		2   //������ ������
#define BLIND_FIGHT_FEAT		3   //������ ���
#define IMPREGNABLE_FEAT		4   //�������������
#define APPROACHING_ATTACK_FEAT	5   //��������� �����
#define DEFENDER_FEAT			6   //���������
#define DODGER_FEAT				7   //��������������
#define LIGHT_WALK_FEAT			8   //������ �������
#define WRIGGLER_FEAT			9   //�������
#define SPELL_SUBSTITUTE_FEAT	10  //������� ����������
#define POWER_ATTACK_FEAT		11  //������ �����
#define WOODEN_SKIN_FEAT		12  //���������� ����
#define IRON_SKIN_FEAT			13  //�������� ����
#define CONNOISEUR_FEAT			14  //������
#define EXORCIST_FEAT			15  //���������� ������
#define HEALER_FEAT				16  //��������
#define LIGHTING_REFLEX_FEAT	17  //���������� �������
#define DRUNKARD_FEAT			18  //�������
#define POWER_MAGIC_FEAT		19  //���� ����������
#define ENDURANCE_FEAT			20  //������������
#define GREAT_FORTITUDE_FEAT	21  //���� ����
#define FAST_REGENERATION_FEAT	22  //������� ����������
#define STEALTHY_FEAT			23  //������������
#define RELATED_TO_MAGIC_FEAT	24  //���������� �������
#define SPLENDID_HEALTH_FEAT	25  //����������� ��������
#define TRACKER_FEAT			26  //��������
#define WEAPON_FINESSE_FEAT		27  //������ ����
#define COMBAT_CASTING_FEAT		28  //������ �����������
#define PUNCH_MASTER_FEAT		29  //�������� ����
#define CLUBS_MASTER_FEAT		30  //������ ������
#define AXES_MASTER_FEAT		31  //������ ������
#define LONGS_MASTER_FEAT		32  //������ ����
#define SHORTS_MASTER_FEAT		33  //������ �������
#define NONSTANDART_MASTER_FEAT		34  //��������� ������
#define BOTHHANDS_MASTER_FEAT		35  //������ ����������
#define PICK_MASTER_FEAT		36  //������ ����
#define SPADES_MASTER_FEAT		37  //������ �����
#define BOWS_MASTER_FEAT		38  //������ ��������
#define FOREST_PATHS_FEAT		39  //������ �����
#define MOUNTAIN_PATHS_FEAT		40  //������ �����
#define LUCKY_FEAT			41  //�����������
#define SPIRIT_WARRIOR_FEAT		42  //������ ���
#define RELIABLE_HEALTH_FEAT		43  //������� ��������
#define EXCELLENT_MEMORY_FEAT		44  //������������ ������
#define ANIMAL_DEXTERY_FEAT		45  //�������� �����
#define LEGIBLE_WRITTING_FEAT		46  //������ ������
#define IRON_MUSCLES_FEAT		47  //�������� �����
#define MAGIC_SIGN_FEAT			48  //���� �������
#define GREAT_ENDURANCE_FEAT		49  //������������
#define BEST_DESTINY_FEAT		50  //������ ����
#define BREW_POTION_FEAT		51  //�������
#define JUGGLER_FEAT			52  //�������
#define NIMBLE_FINGERS_FEAT		53  //������
#define GREAT_POWER_ATTACK_FEAT		54  //���������� ������ �����
#define IMMUNITY_FEAT			55  //�������� � ���
#define MOBILITY_FEAT			56  //�����������
#define NATURAL_STRENGTH_FEAT		57  //�����
#define NATURAL_DEXTERY_FEAT		58  //����������
#define NATURAL_INTELLECT_FEAT		59  //��������� ��
#define NATURAL_WISDOM_FEAT		60  //������
#define NATURAL_CONSTITUTION_FEAT	61  //��������
#define NATURAL_CHARISMA_FEAT		62  //��������� �������
#define MNEMONIC_ENHANCER_FEAT		63  //�������� ������
#define MAGNETIC_PERSONALITY_FEAT	64  //������������
#define DAMROLL_BONUS_FEAT		65  //����� �� ����
#define HITROLL_BONUS_FEAT		66  //������� ����
#define MAGICAL_INSTINCT_FEAT		67  //���������� �����
#define PUNCH_FOCUS_FEAT		68  //������� ������: ����� ����
#define CLUB_FOCUS_FEAT			69  //������� ������: ������
#define AXES_FOCUS_FEAT			70  //������� ������: ������
#define LONGS_FOCUS_FEAT		71  //������� ������: ���
#define SHORTS_FOCUS_FEAT		72  //������� ������: ���
#define NONSTANDART_FOCUS_FEAT		73  //������� ������: ���������
#define BOTHHANDS_FOCUS_FEAT		74  //������� ������: ���������
#define PICK_FOCUS_FEAT			75  //������� ������: ������
#define SPADES_FOCUS_FEAT		76  //������� ������: �����
#define BOWS_FOCUS_FEAT			77  //������� ������: ���
#define AIMING_ATTACK_FEAT		78  //���������� �����
#define GREAT_AIMING_ATTACK_FEAT	79  //���������� ���������� �����
#define DOUBLESHOT_FEAT			80  //������� �������
#define PORTER_FEAT			81  //���������
#define RUNE_NEWBIE_FEAT		82  //����������� ���
#define RUNE_USER_FEAT			83  //������ ����
#define RUNE_MASTER_FEAT		84  //�������� ����
#define RUNE_ULTIMATE_FEAT		85  //���� �����
#define TO_FIT_ITEM_FEAT		86  //��������
#define TO_FIT_CLOTHCES_FEAT		87  //��������
#define STRENGTH_CONCETRATION_FEAT	88 // ������������ ����
#define DARK_READING_FEAT		89 // ������� ����
#define SPELL_CAPABLE_FEAT		90 // ����������
#define ARMOR_LIGHT_FEAT		91 // ������� ������� ���� ��������
#define ARMOR_MEDIAN_FEAT		92 // ������� �������� ���� ��������
#define ARMOR_HEAVY_FEAT		93 // ������� �������� ���� ��������
#define GEMS_INLAY_FEAT			94 // ����������� ������� (TODO: �� ������������)
#define WARRIOR_STR_FEAT		95 // ����������� ����
#define RELOCATE_FEAT			96 // �������������
#define SILVER_TONGUED_FEAT		97 //�����������
#define BULLY_FEAT				98 //�������
#define THIEVES_STRIKE_FEAT		99 //��������� ����
#define MASTER_JEWELER_FEAT		100 //�������� ������
#define SKILLED_TRADER_FEAT		101 //�������� ������
#define ZOMBIE_DROVER_FEAT		102 //�������� ��������
#define EMPLOYER_FEAT			103 //����� �����
#define MAGIC_USER_FEAT			104 //������������� ��������
#define GOLD_TONGUE_FEAT		105 //��������
#define CALMNESS_FEAT			106 //������������
#define RETREAT_FEAT			107 //�����������
#define SHADOW_STRIKE_FEAT		108 //��������� ����
#define	THRIFTY_FEAT			109 //������������
#define CYNIC_FEAT			    110 // ����������
#define PARTNER_FEAT			111 // ��������
#define HELPDARK_FEAT			112 // ������ ����
#define FURYDARK_FEAT           113 // ������ ����
#define DARKREGEN_FEAT			114 // ������ ��������������
#define SOULLINK_FEAT			115 // ������� ���
#define STRONGCLUTCH_FEAT		116 // ������� ������
#define MAGICARROWS_FEAT		117 // ���������� ������
#define COLLECTORSOULS_FEAT		118 // ������������ ���
#define DARKDEAL_FEAT			119 // ������ ������
#define DECLINE_FEAT			120 // �����
#define HARVESTLIFE_FEAT		121 // ����� �����
#define LOYALASSIST_FEAT		122 // ������ ��������
#define HAUNTINGSPIRIT_FEAT		123 // ���������� ���
#define SNEAKRAGE_FEAT			124 // ������ ����
#define HORSE_STUN			125 // ��������� �� ������
//������������ ��� �������

#define TEAMSTER_UNDEAD_FEAT	125 // �������� ������
#define ELDER_TASKMASTER_FEAT	126 // ������� �����������
#define LORD_UNDEAD_FEAT		127 // ���������� ������
#define DARK_WIZARD_FEAT		128 // ������ ���
#define ELDER_PRIEST_FEAT		129 // ������� ����
#define HIGH_LICH_FEAT			130 // ��������� ���
#define BLACK_RITUAL_FEAT		131 // ������ ������

/*
//������������ ��� ����

#define RESERVED_FEAT_1			132 //
#define RESERVED_FEAT_2			133 //
#define RESERVED_FEAT_3			134 //
#define RESERVED_FEAT_4			135 //
#define RESERVED_FEAT_5			136 //
#define RESERVED_FEAT_6			137 //
*/

#define EVASION_FEAT            138 //���������� ���
#define EXPEDIENT_CUT_FEAT      139 //�����
#define SHOT_FINESSE_FEAT		140  //������ �������

/*
#define AIR_MAGIC_FOCUS_FEAT		  //�������_�����: ������
#define FIRE_MAGIC_FOCUS_FEAT		  //�������_�����: �����
#define WATER_MAGIC_FOCUS_FEAT		  //�������_�����: ����
#define EARTH_MAGIC_FOCUS_FEAT		  //�������_�����: �����
#define LIGHT_MAGIC_FOCUS_FEAT		  //�������_�����: ����
#define DARK_MAGIC_FOCUS_FEAT		  //�������_�����: ����
#define MIND_MAGIC_FOCUS_FEAT		  //�������_�����: �����
#define LIFE_MAGIC_FOCUS_FEAT		  //�������_�����: �����
*/

// MAX_FEATS ������������ � structs.h

#define UNUSED_FTYPE	-1
#define NORMAL_FTYPE	0
#define AFFECT_FTYPE	1
#define SKILL_MOD_FTYPE	2

/* ��������� � �������, ������������ ����� ������������ � ���������
   �������� ��������� ����� ������ ����� �������� ��� ������� ������
      �������������, �� ��������� ���� �������� ������ �������� �� 28 ������ */

//��� � ������� �������� ���������� ����� ���� ��� �����������

const int feat_slot_for_remort[NUM_PLAYER_CLASSES] = { 5,6,4,4,4,4,6,6,6,4,4,4,4,5 };
// ���������� ��� "��������-��������" � �����������
#define MAX_FEAT_AFFECT	5
// ����������� ��������� �� ����� ���������� ��-���������� ������������
#define MAX_ACC_FEAT(ch)	((int) 1+(LVL_IMMORT-1)*(5+GET_REMORT(ch)/feat_slot_for_remort[(int) GET_CLASS(ch)])/28)

// ���� ��������� ��� ������������ (����� AFFECT_FTYPE, ��� ��� ������������ ����������� ���� APPLY)
#define FEAT_TIMER 1
#define FEAT_SKILL 2

extern struct SFeatInfo feat_info[MAX_FEATS];

int find_feat_num(const char *name, bool alias = false);
void assign_feats(void);
bool can_use_feat(const CHAR_DATA *ch, int feat);
bool can_get_feat(CHAR_DATA *ch, int feat);
bool have_feat_slot(CHAR_DATA *ch, int feat);
int feature_mod(int feat, int location);
void check_berserk(CHAR_DATA * ch);
void set_race_feats(CHAR_DATA *ch, bool flag = true);
void set_natural_feats(CHAR_DATA *ch);

/*
��������� ����� ��� �������� �������� �������� � ������ affected ��������� �����������
���� � ����-�� ���� �������, ����� ������ ������� ������������ ��� ���� �����, ����������
������ ������� � ���������, ������ ������� ���� � ���������� �����������
������ ����� �������� ������� ��������� � ���������� feat_info �� ����
*/
class CFeatArray
{
public:
	explicit CFeatArray() : _pos(0), i(MAX_FEAT_AFFECT) {}

	int pos(int pos = -1);
	void insert(const int location, sbyte modifier);
	void clear();

	struct CFeatAffect
	{
		int location;
		int modifier;

		CFeatAffect() : location(APPLY_NONE), modifier(0) {}

		CFeatAffect(int __location, int __modifier)
			: location(__location), modifier(__modifier) {}

		bool operator!=(const CFeatAffect &r) const
		{
			return (location != r.location || modifier != r.modifier);
		}
		bool operator==(const CFeatAffect &r) const
		{
			return !(*this != r);
		}
	};

	struct CFeatAffect affected[MAX_FEAT_AFFECT];

private:
	int _pos, i;
};

struct SFeatInfo
{
	int min_remort[NUM_PLAYER_CLASSES][NUM_KIN];
	int slot[NUM_PLAYER_CLASSES][NUM_KIN];
	bool classknow[NUM_PLAYER_CLASSES][NUM_KIN];
	bool natural_classfeat[NUM_PLAYER_CLASSES][NUM_KIN];
	std::array<CFeatArray::CFeatAffect, MAX_FEAT_AFFECT> affected;
	int type;
	bool up_slot;
	const char *name;
	std::string alias;
};

#endif // __FEATURES_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
