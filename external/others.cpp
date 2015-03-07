
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

argument
minimax_score(EXTERNAL_ARG(game), EXTERNAL_ARG(player), EXTERNAL_ARG(rootplayer))
{
   DECLARE_LIST(game);
   DECLARE_INT(player);
   DECLARE_INT(rootplayer);

   vector<int, mem::allocator<int>> board;
   board.reserve(10);

   runtime::cons *p((runtime::cons*)game);
   bool all_non_zero(true);
   int count(0);
   while(!runtime::cons::is_null(p)) {
      vm::int_val v(FIELD_INT(p->get_head()));
      if(all_non_zero) {
         if(v != 0)
            all_non_zero = false;
         if(v == rootplayer)
            count++;
      }
      board.push_back(v);
      p = p->get_tail();
   }
   int other_player(player == 1 ? 2 : 1);

   static vector<vector<int>> valid = {{0, 3, 6}, {1, 4, 7}, {2, 5, 8}, {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 4, 8}, {2, 4, 6}};

   for(vector<int> v : valid) {
      bool found(true);
      for(int i : v) {
         if(board[i] != player) {
            found = false;
            break;
         }
      }
      if(found) {
         if(player == rootplayer) {
            RETURN_INT(200);
         } else {
            RETURN_INT(-100);
         }
      }
      found = true;
      for(int i : v) {
         if(board[i] != other_player) {
            found = false;
            break;
         }
      }
      if(found) {
         if(other_player == rootplayer) {
            RETURN_INT(200);
         } else {
            RETURN_INT(-100);
         }
      }
   }
   if(all_non_zero) {
      RETURN_INT(count);
   } else {
      RETURN_INT(0);
   }
}

int_val
minmax_points_factor(const runtime::array *a)
{
   int_val num_zeros(0);
   for(size_t i(0); i < a->get_size(); ++i) {
      if(FIELD_INT(a->get_item(i)) == 0)
         num_zeros++;
   }

   return std::max((int_val)1, (int_val)(a->get_size() - num_zeros));
}

static vector<vector<int>> valid = {{0, 3, 6}, {1, 4, 7}, {2, 5, 8}, {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 4, 8}, {2, 4, 6}};

argument
minimax_score2(EXTERNAL_ARG(game), EXTERNAL_ARG(player), EXTERNAL_ARG(rootplayer))
{
   DECLARE_ARRAY(game);
   DECLARE_INT(player);
   DECLARE_INT(rootplayer);

   int other_player(player == 1 ? 2 : 1);

   for(vector<int>& v : valid) {
      bool found(true);
      for(int i : v) {
         if(FIELD_INT(game->get_item(i)) != player) {
            found = false;
            break;
         }
      }
      if(found) {
         if(player == rootplayer) {
            RETURN_INT(minmax_points_factor(game) * 200);
         } else {
            RETURN_INT(minmax_points_factor(game) * -100);
         }
      }
      found = true;
      for(int i : v) {
         if(FIELD_INT(game->get_item(i)) != other_player) {
            found = false;
            break;
         }
      }
      if(found) {
         if(other_player == rootplayer) {
            RETURN_INT(minmax_points_factor(game) * 200);
         } else {
            RETURN_INT(minmax_points_factor(game) * -100);
         }
      }
   }
   int count(0);
   for(size_t i(0); i < game->get_size(); ++i) {
      const vm::int_val v(FIELD_INT(game->get_item(i)));
      if(v == 0) {
         RETURN_INT(0);
      }
      if(v == rootplayer)
         count++;
   }
   RETURN_INT(count);
}

argument
minimax_points(EXTERNAL_ARG(game), EXTERNAL_ARG(rootplayer))
{
   DECLARE_LIST(game);
   DECLARE_INT(rootplayer);

   runtime::cons *p((runtime::cons*)game);
   int count(0);
   while(!runtime::cons::is_null(p)) {
      vm::int_val v(FIELD_INT(p->get_head()));
      if(v == rootplayer)
         count++;
      p = p->get_tail();
   }

   RETURN_INT(count);
}

argument
minimax_points2(EXTERNAL_ARG(game), EXTERNAL_ARG(rootplayer))
{
   DECLARE_ARRAY(game);
   DECLARE_INT(rootplayer);

   int count(0);
   for(size_t i(0); i < game->get_size(); ++i) {
      vm::int_val v(FIELD_INT(game->get_item(i)));
      if(v == rootplayer)
         count++;
   }

   RETURN_INT(count);
}

}
}
