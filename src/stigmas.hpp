#ifndef __STIGMAS_HPP__
#define __STIGMAS_HPP__

#include <boost/algorithm/string.hpp>

#include <string>
#include "char.hpp"

#define STIGMA_FIRE_DRAGON 1

struct Stigma
{
		// id ����������
		unsigned int id;
		// ��� 
		std::string name;
		// ������� ��������� ����������
		void(*activation_stigma)(CHAR_DATA*);
		// ����� �������
		unsigned reload;
	};

struct StigmaWear
{
		Stigma stigma;
		// ����� �� �������
		unsigned int reload;
		// �������� ���
		std::string get_name() const;
};

#endif	//__STIGMAS_HPP__

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
