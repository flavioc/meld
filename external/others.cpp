
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "external/others.hpp"
#include "runtime/string.hpp"

using namespace runtime;
using namespace std;
using namespace vm;

namespace vm
{

namespace external
{

static inline int_val
count_number_words(ifstream& fp)
{
   string word;
   int_val total(0);

   while(fp >> word) {
      ++total;
   }

   return total;
}

argument
filecountwords(EXTERNAL_ARG(dirname), EXTERNAL_ARG(filenum))
{
   DECLARE_STRING(dirname);
   DECLARE_INT(filenum);

   struct dirent *dp;
   DIR *dfd;
   string dir(dirname->get_content());

   dfd = opendir(dir.c_str());
   if(dfd == NULL) {
      cerr << "Can't open directory " << dir << endl;
      RETURN_INT(0);
   }
#define BUF_SIZE 1024

   char filename_buffer[BUF_SIZE];
   int index = 0;

   while((dp = readdir(dfd)) != NULL) {
      struct stat stbuf;
      snprintf(filename_buffer, BUF_SIZE, "%s/%s", dir.c_str(), dp->d_name);
      if(stat(filename_buffer, &stbuf) == -1) {
         continue;
      }

      if((stbuf.st_mode & S_IFMT) == S_IFREG) {
         if(index == filenum) {
            closedir(dfd);

            ifstream fp(filename_buffer, ifstream::in);

            const int_val total(count_number_words(fp));

            fp.close();
   
            RETURN_INT(total);
         }
         ++index;
      } else {
         continue;
      }
   }

   closedir(dfd);

   RETURN_INT(0);
}

}

}
