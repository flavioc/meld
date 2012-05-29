
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>

#include "utils/fs.hpp"

using namespace std;

namespace utils
{

void
file_print_and_remove(const string& filename)
{
   ifstream fp(filename.c_str());
   char buf[256];
   while(!fp.eof()) {
      fp.getline(buf, 256);
      if(!fp.eof())
         cout << buf << endl;
   }
   remove(filename.c_str());
}

bool
file_exists(const string& filename)
{
	if(access(filename.c_str(), R_OK) < 0)
		return false;
	
	struct stat info;

	if(stat(filename.c_str(), &info) < 0)
		return false;
		
	return S_ISREG(info.st_mode);
}

}