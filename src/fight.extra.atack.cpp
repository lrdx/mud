#include "fight.extra.atack.hpp"

#include "char.hpp"
#include "spells.h"
#include "utils.h"

#include <boost/algorithm/string.hpp>

void WeaponMagicalAtack::set_atack(CHAR_DATA *ch, CHAR_DATA *victim) 
{
    OBJ_DATA *mag_cont;

    mag_cont = GET_EQ(ch, WEAR_QUIVER);
    if (GET_SHOOT_VAL(GET_OBJ_VAL(mag_cont, 1), 100) == 0) 
    {
            send_to_char("Эх какой выстрел мог бы быть, а так колчан пуст.\r\n", ch);
            return;
    }
    
    mag_single_target(GET_LEVEL(ch), ch, victim, NULL, GET_OBJ_VAL(mag_cont, 0), SAVING_REFLEX);
    
}
bool WeaponMagicalAtack::set_count_atack(CHAR_DATA *ch)
{
    //выстрел из колчана
    if (((GET_OBJ_TYPE(GET_EQ(ch, WEAR_BOTHS))) == OBJ_DATA::ITEM_WEAPON) 
            && (GET_OBJ_SKILL(GET_EQ(ch, WEAR_BOTHS)) == SKILL_BOWS )
            && (GET_EQ(ch, WEAR_QUIVER)))
        {
            //если у нас в руках лук и носим колчан
            set_count(get_count()+1);
            
            //по договоренности кадый 4 выстрел магический
            if (get_count() == 4)
            {
                set_count(0);
                return true;
                
            }
            else if ((can_use_feat(ch, DEFT_SHOOTER_FEAT))&&(get_count() == 3))
            {
                set_count(0);
                return true;
            }
            return false;
        }
    set_count(0);
    return false;
}
