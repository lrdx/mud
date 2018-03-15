#include "olc.utilities.hpp"

#include <olc.h>

namespace test_utils
{
	void OlcBuilder::create_new_descriptor()
	{
		descriptor_t result = std::make_shared<DESCRIPTOR_DATA>();
		
		CharacterBuilder builder;
		builder.create_new();		
				
		result->character = builder.get();
		result->olc = new olc_data;
		m_result = result;
	}

	void OlcBuilder::create_new_mob()
	{
		if (!m_result)
		{
			create_new_descriptor();
			check_descriptor_existance();
		}

		CharacterBuilder builder;
		builder.create_new();
		m_result->olc->mob = builder.get().get();
	}

	void OlcBuilder::create_new_obj()
	{
	}

	void OlcBuilder::create_new_room()
	{
	}

	void OlcBuilder::create_new_zone()
	{
	}

	void OlcBuilder::set_olc_item_type(int type)
	{
		check_descriptor_existance();
		m_result->olc->item_type = type;
	}

	void OlcBuilder::set_olc_mode(int mode)
	{
		check_descriptor_existance();
		m_result->olc->mode = mode;
	}

	void OlcBuilder::set_olc_script_mode(int mode)
	{
		check_descriptor_existance();
		m_result->olc->script_mode = mode;
	}

	void OlcBuilder::check_descriptor_existance() const
	{
		OlcBuilder::check_descriptor_existance(m_result);
	}

	void OlcBuilder::check_descriptor_existance(descriptor_t desc)
	{
		if (!desc)
		{
			throw std::runtime_error("Descriptor wasn't created.");
		}
	}
}	// namespace test_utils

	// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
