#ifndef __OLC__UTILITIES_HPP__
#define __OLC__UTILITIES_HPP__

#include "char.utilities.hpp"

#include <structs.h>
#include <olc.h>

namespace test_utils
{
	class OlcBuilder
	{
	public:
		using descriptor_t = std::shared_ptr<DESCRIPTOR_DATA>;
		using character_t = CharacterBuilder::character_t;

		void create_new_descriptor();

		void create_new_mob();
		void create_new_obj();
		void create_new_room();
		void create_new_zone();

		void set_olc_item_type(int type);
		void set_olc_mode(int mode);
		void set_olc_script_mode(int mode);

		descriptor_t get_descriptor() { return m_result; }

		olc_data* get_olc() { return m_result->olc; }
		character_t* get_mob() { return m_result->olc->mob; }

	private:

		static void check_descriptor_existance(descriptor_t character);
		void check_descriptor_existance() const;

		descriptor_t m_result;
	};
}	// namespace test_utils

#endif //__OLC__UTILITIES_HPP__

	// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
