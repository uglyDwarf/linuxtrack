#include <windows.h>
#include "rest.h"

int main()
{
  //signatures
  char verse1[200], verse2[200];
  game_desc_t gd;
  if(game_data_get_desc(1001, &gd) && getSomeSeriousPoetry(verse1, verse2)){
    //data available, all is OK
  }else{
    MessageBox(NULL,
    "To fully utilize linuxtrack-wine,\ninstall the support data in ltr_gui (Misc.pane)!",
    "Linuxtrack-wine check", 
    MB_OK);
  }
  return 0;
}

