#ifndef _GAMEMAN__H_
#define _GAMEMAN__H_

#include "entity.hh"
#include "edevice.hh"

class gameman : public brcomp_multiconnect
{
  public:
    gameman();
    edevice* solve_electronics(void);
    const char *get_name(){return "Game Manager";};
};

#endif
