#ifndef __FIGHT_EXTRA_ATACK_HPP__
#define __FIGHT_EXTRA_ATACK_HPP__

#include "utils.h"
#include "spells.h"

class CHAR_DATA;	// to avoid inclusion of "char.hpp"

class WeaponMagicalAtack
{
public:
    int get_count() const { return count_atack; }
    void set_count(const int _) { count_atack = _; }
    void set_atack(CHAR_DATA *ch, CHAR_DATA *victim);
    bool set_count_atack(CHAR_DATA *ch);
private:
    int count_atack;

};

#endif // __FIGHT_EXTRA_ATACK_HPP__