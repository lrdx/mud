#ifndef __STIGMAS_HPP__
#define __STIGMAS_HPP__

#include <boost/algorithm/string.hpp>

#include <string>
#include "char.hpp"

#define STIGMA_FIRE_DRAGON 1

struct Stigma
{
		// id татуировки
		unsigned int id;
		// имя 
		std::string name;
		// функция активации татуировки
		void(*activation_stigma)(CHAR_DATA*);
		// время релоада
		unsigned reload;
	};

struct StigmaWear
{
		Stigma stigma;
		// время до релоада
		unsigned int reload;
		// получить имя
		std::string get_name() const;
};

#endif	//__STIGMAS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
