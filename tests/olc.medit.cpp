#include <gtest/gtest.h>

#include <boost\dynamic_bitset.hpp>

#include "olc.utilities.hpp"

#include <utils.h>
#include <olc.h>
#include <dg_olc.h>

// Extern functions from medit.cpp
void medit_setup(DESCRIPTOR_DATA * d, int real_num);
void medit_parse(DESCRIPTOR_DATA * d, char *arg);

TEST(Olc_Medit, ChangeRace_CorrectRace)
{
	test_utils::OlcBuilder builder;
	builder.create_new_descriptor();
	auto desc = builder.get_descriptor().get();
	medit_setup(desc, -1);
	auto mob = builder.get_mob();

	builder.set_olc_mode(MEDIT_RACE);
	medit_parse(desc, const_cast<char*>(std::to_string(NPC_RACE_BASIC - NPC_RACE_BASIC).c_str()));
	EXPECT_EQ(NPC_RACE_BASIC, mob->get_race());
	builder.set_olc_mode(MEDIT_RACE);
	medit_parse(desc, const_cast<char*>(std::to_string(NPC_RACE_HUMAN_ANIMAL - NPC_RACE_BASIC).c_str()));
	EXPECT_EQ(NPC_RACE_HUMAN_ANIMAL, mob->get_race());
	builder.set_olc_mode(MEDIT_RACE);
	medit_parse(desc, const_cast<char*>(std::to_string(NPC_RACE_MAGIC_CREATURE - NPC_RACE_BASIC).c_str()));
	EXPECT_EQ(NPC_RACE_MAGIC_CREATURE, mob->get_race());
}

TEST(Olc_Medit, ChangeRace_IncorrectRace)
{
	test_utils::OlcBuilder builder;
	builder.create_new_descriptor();
	auto desc = builder.get_descriptor().get();
	medit_setup(desc, -1);
	auto mob = builder.get_mob();

	builder.set_olc_mode(MEDIT_RACE);
	medit_parse(desc, "-1");
	EXPECT_EQ(NPC_RACE_BASIC, mob->get_race());
	builder.set_olc_mode(MEDIT_RACE);
	medit_parse(desc, const_cast<char*>(std::to_string(NPC_RACE_NEXT).c_str()));
	EXPECT_EQ(NPC_RACE_NEXT - 1, mob->get_race());
}

TEST(Olc_Medit, ChangeRole_Add)
{
	test_utils::OlcBuilder builder;
	builder.create_new_descriptor();
	auto desc = builder.get_descriptor().get();
	medit_setup(desc, -1);
	auto mob = builder.get_mob();

	//TODO: Why role_t is boost::dynamic_bitset?
	for (size_t i = 0; i < MOB_ROLE_TOTAL_NUM; ++i)
	{
		mob->set_role(i, false);
	}

	for (size_t i = 0; i < MOB_ROLE_TOTAL_NUM; ++i)
	{
		builder.set_olc_mode(MEDIT_ROLE);
		medit_parse(desc, const_cast<char*>(std::to_string(i + 1).c_str()));
		EXPECT_EQ(true, mob->get_role(i));
	}

	for (size_t i = 0; i < MOB_ROLE_TOTAL_NUM; ++i)
	{
		EXPECT_EQ(true, mob->get_role(i));
	}
}

TEST(Olc_Medit, ChangeRole_Remove)
{
	test_utils::OlcBuilder builder;
	builder.create_new_descriptor();
	auto desc = builder.get_descriptor().get();
	medit_setup(desc, -1);
	auto mob = builder.get_mob();

	for (auto i = 0; i < MOB_ROLE_TOTAL_NUM; ++i)
	{
		mob->set_role(i, true);
	}

	for (auto i = 0; i < MOB_ROLE_TOTAL_NUM; ++i)
	{
		builder.set_olc_mode(MEDIT_ROLE);
		medit_parse(desc, const_cast<char*>(std::to_string(i + 1).c_str()));
		EXPECT_EQ(false, mob->get_role(i));
	}

	for (size_t i = 0; i < MOB_ROLE_TOTAL_NUM; ++i)
	{
		EXPECT_EQ(false, mob->get_role(i));
	}
}

TEST(Olc_Medit, ChangeFeat_Add)
{
	test_utils::OlcBuilder builder;
	builder.create_new_descriptor();
	auto desc = builder.get_descriptor().get();
	medit_setup(desc, -1);
	auto mob = builder.get_mob();
	
	mob->real_abils.Feats.reset();

	for (auto i = 0; i < MAX_FEATS; ++i)
	{
		builder.set_olc_mode(MEDIT_FEATURES);
		medit_parse(desc, const_cast<char*>(std::to_string(i + 1).c_str()));
		EXPECT_EQ(false, mob->real_abils.Feats.test(i));
	}
}

TEST(Olc_Medit, ChangeFeat_Remove)
{
	test_utils::OlcBuilder builder;
	builder.create_new_descriptor();
	auto desc = builder.get_descriptor().get();
	medit_setup(desc, -1);
	auto mob = builder.get_mob();

	mob->real_abils.Feats.set();

	for (auto i = 0; i < MAX_FEATS; ++i)
	{
		builder.set_olc_mode(MEDIT_FEATURES);
		medit_parse(desc, const_cast<char*>(std::to_string(i + 1).c_str()));
		EXPECT_EQ(false, mob->real_abils.Feats.test(i));
	}

	EXPECT_EQ(true, mob->real_abils.Feats.none());
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
