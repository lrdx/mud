/* ************************************************************************
*   File: im.cpp                                        Part of Bylins    *
*  Usage: Ingradient handling function                                    *
*                                                                         *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#ifndef _IM_H_
#define _IM_H_

#include "structs.h"

class OBJ_DATA;	// forward declaration to avoid inclusion of obj.hpp and any dependencies of that header.
struct ROOM_DATA;	//

// ����������� �������� ������� ������������: �����, ����, ������
#define		IM_CLASS_ROSL		0
#define		IM_CLASS_JIV		1
#define		IM_CLASS_TVERD		2

#define 	IM_POWER_SLOT		1
#define		IM_TYPE_SLOT		2
#define     IM_INDEX_SLOT       3

#define		IM_NPARAM			3

struct _im_tlist_tag
{
	long size;		// ���������� �����
	long *types;		// ������� �����
};
typedef struct _im_tlist_tag im_tlist;

struct _im_memb_tag
{
	int power;		// ���� �����������
	ESex sex;		// ��� ��������� (0-����,1-���,2-���,3-��.�)
	char **aliases;		// ������ ��� �������
	struct _im_memb_tag *next;	// ������ �� ���������
};
typedef struct _im_memb_tag im_memb;

// �������� ������������ ���� �����������
struct _im_type_tag
{
	int id;			// ����� �� im.lst
	char *name;		// �������� ���� �����������
	int proto_vnum;		// vnum �������-���������
	im_tlist tlst;		// ������ ID �����/���������, �������
	// ����������� ������ ���
	im_memb *head;		// ������ ���������� �����
	// �� ������������ ��� ������������ ������ ����
};
typedef struct _im_type_tag im_type;

// �������� �������� �� ���������� �� �������� ������������ ����
// ���� cls � �������� �������� �� ������������
// ���� members � �������� �������� �� ������������

// �������� ��������������� ����������
struct _im_addon_tag
{
	int id;			// ��� �����������, ������ �������
	int k0, k1, k2;		// ������������� �������
	OBJ_DATA *obj;		// ������������� ������
	struct _im_addon_tag *link;	// ������
};
typedef struct _im_addon_tag im_addon;

#define IM_MSG_OK		0
#define IM_MSG_FAIL		1
#define IM_MSG_DAM		2

// +newbook.patch (Alisher)
#define KNOW_RECIPE  1
// -newbook.patch (Alisher)

// �������� �������
struct _im_recipe_tag
{
	int id;			// ����� �� im.lst
	char *name;		// �������� �������
	int k_improove;		// ��������� ��������
	int result;		// VNUM ��������� ����������
	float k[IM_NPARAM], kp;	// ����� ��������
	int *require;		// ������ ������������ �����������
	int nAddon;		// ���������� ���������� �����������
	im_addon *addon;	// ������ ���������� �����������
	std::array<char *, 3> msg_char;	// ��������� OK,FAIL,DAM
	std::array<char *, 3> msg_room;	// ��������� OK,FAIL,DAM
	int x, y;		// XdY - �����������
// +newbook.patch (Alisher)
	std::array<int, NUM_PLAYER_CLASSES> classknow; // ������� �� ����� ������ ��������
	int level; // �� ����� ������ ����� ������� ������
	int remort; // ������� �������� ���������� ��� �������
// -newbook.patch (Alisher)
};
typedef struct _im_recipe_tag im_recipe;

// �������� �������-������
struct im_rskill
{
	int rid;		// ������ � ������� ������� ��������
	int perc;		// ������� �������� �������
	im_rskill *link;	// ��������� �� ��������� ������ � �������
};

extern im_recipe *imrecipes;
extern im_type *imtypes;
extern int top_imtypes;

void im_parse(int **ing_list, char *line);
//MZ.load
void im_reset_room(ROOM_DATA * room, int level, int type);
//-MZ.load
OBJ_DATA* try_make_ingr(CHAR_DATA* mob, int prob_default, int prob_special);
int im_assign_power(OBJ_DATA * obj);
int im_get_recipe(int id);
int im_get_type_by_name(char *name, int mode);
OBJ_DATA *load_ingredient(int index, int power, int rnum);
int im_ing_dump(int *ping, char *s);
void im_inglist_copy(int **pdst, int *src);
void im_inglist_save_to_disk(FILE * f, int *ping);
void im_extract_ing(int **pdst, int num);
int im_get_char_rskill_count(CHAR_DATA * ch);
void trg_recipeturn(CHAR_DATA * ch, int rid, int recipediff);
void trg_recipeadd(CHAR_DATA * ch, int rid, int recipediff);
int im_get_recipe_by_name(char *name);
im_rskill *im_get_char_rskill(CHAR_DATA * ch, int rid);
void compose_recipe(CHAR_DATA * ch, char *argument, int subcmd);
void forget_recipe(CHAR_DATA * ch, char *argument, int subcmd);
int im_get_idx_by_type(int type);

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
