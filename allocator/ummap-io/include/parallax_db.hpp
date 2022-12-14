  //extern "C" is mandatory here
extern "C"{
  #include <parallax.h>
}
#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <stdlib.h>
#ifndef PARALLAX_DB
#define PARALLAX_DB  
  #define DB_NUM  1
  #define PATH  "/mnt/datasets/parallax.txt"

  extern std::vector<par_handle> dbs;

  extern int Parallax_init();

 // extern int Parallax_put();

  extern int Parallax_get(const std::string &key);

  //extern int Parallax_close();
#endif

