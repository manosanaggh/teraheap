#include "/home1/public/manosanag/parallax/lib/include/parallax/parallax.h"
#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <stdlib.h>

#ifndef PARALLAX_DB
#define PARALLAX_DB
  
  #define DB_NUM  1
  #define PATH  "/mnt/fmap"

  std::vector<par_handle> dbs;

  int Parallax_init();

  int Parallax_put();

  int Parallax_get(const std::string &key);

  int Parallax_close();
#endif
