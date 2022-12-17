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
  #define PATH  "/mnt/fmap/parallax.txt"

  extern std::vector<par_handle> dbs;

  extern void Parallax_init();

  extern void Parallax_insert(const std::string &key, const std::string &value);

  extern void Parallax_read(const std::string &key);

  extern void Parallax_close();
#endif

