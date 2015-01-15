
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "external/others.hpp"
#include "runtime/objs.hpp"
#include "vm/types.hpp"

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
   if(dfd == nullptr) {
      cerr << "Can't open directory " << dir << endl;
      RETURN_INT(0);
   }
#define BUF_SIZE 1024

   char filename_buffer[BUF_SIZE];
   int index = 0;

   while((dp = readdir(dfd)) != nullptr) {
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

argument
queens_violation(EXTERNAL_ARG(y), EXTERNAL_ARG(cols))
{
   DECLARE_INT(y);
   DECLARE_LIST(cols);

   int_val dly(y - 1);
   int_val dry(y + 1);
   runtime::cons *l((runtime::cons *)cols);
   while(!runtime::cons::is_null(l)) {
      const vm::int_val p(FIELD_INT(l->get_head()));
      if(p == y || p == dly || p == dry)
         RETURN_BOOL(true);
      dly--;
      dry++;
      l = l->get_tail();
   }
   RETURN_BOOL(false);
}

}

}
