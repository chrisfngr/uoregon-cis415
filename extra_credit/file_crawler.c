#define _DEFAULT_SOURCE 1	/* enables macros to test type of directory entry */

#include <sys/types.h>
#include <dirent.h>
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "linkedlist.h"
#include "treeset.h"
#include "re.h"

LinkedList *dir_queue = NULL;
TreeSet    *to_print_ts = NULL;

/*
 * routine to convert bash pattern to regex pattern
 * 
 * e.g. if bashpat is "*.c", pattern generated is "^.*\.c$"
 *      if bashpat is "a.*", pattern generated is "^a\..*$"
 *
 * i.e. '*' is converted to ".*"
 *      '.' is converted to "\."
 *      '?' is converted to "."
 *      '^' is put at the beginning of the regex pattern
 *      '$' is put at the end of the regex pattern
 *
 * assumes 'pattern' is large enough to hold the regular expression
 */
static void cvtPattern(char pattern[], const char *bashpat) {
   char *p = pattern;

   *p++ = '^';
   while (*bashpat != '\0') {
      switch (*bashpat) {
      case '*':
         *p++ = '.';
	 *p++ = '*';
	 break;
      case '.':
         *p++ = '\\';
	 *p++ = '.';
	 break;
      case '?':
         *p++ = '.';
	 break;
      default:
         *p++ = *bashpat;
      }
      bashpat++;
   }
   *p++ = '$';
   *p = '\0';
}












// take directory name, process it, add any additional directories to queue and files to print

static int processDirectory(char *dirname) {
   DIR *dd;
   struct dirent *dent;
   char *sp;
   int len, status = 1;
   char d[4096];

   /*
    * eliminate trailing slash, if there
    */
   strcpy(d, dirname);
   len = strlen(d);
   if (len > 1 && d[len-1] == '/')
      d[len-1] = '\0';
   /*
    * open the directory
    */
   if ((dd = opendir(d)) == NULL) {
      if (verbose)
         fprintf(stderr, "Error opening directory `%s'\n", d);
      return 1;
   }
   /*
    * duplicate directory name to insert into linked list
    */
   sp = strdup(d);
   if (sp == NULL) {
      fprintf(stderr, "Error adding `%s' to linked list\n", d);
      status = 0;
      goto cleanup;
   }
   if (!ll_add(ll, sp)) {
      fprintf(stderr, "Error adding `%s' to linked list\n", sp);
      free(sp);
      status = 0;
      goto cleanup;
   }
   if (len == 1 && d[0] == '/')
      d[0] = '\0';

   /*
    * read entries from the directory
    */

   while (status && (dent = readdir(dd)) != NULL) {
      if (strcmp(".", dent->d_name) == 0 || strcmp("..", dent->d_name) == 0)
         continue;
      if (dent->d_type & DT_DIR) {
         char b[4096];
         sprintf(b, "%s/%s", d, dent->d_name);
         // add dir to dir_queue instead of recursion
         if(!ll_add(dir_queue, b) {
           fprintf(stderr, "Error adding `&s' to directory queue\n", b);
           free(b);
           status = 0;
           goto cleanup;
         }
      } else {
    	 /*
	  * see if filename matches regular expression
	  */
	 if (! re_match(reg, dent->d_name))
            continue;
         sprintf(b, "%s/%s", dir, dent->d_name);
	 /*
	  * duplicate fully qualified pathname for insertion into treeset
	  */
	 if ((sp = strdup(b)) != NULL) {
            if (!ts_add(to_print_queue, sp)) {
               fprintf(stderr, "Error adding `%s' to tree set\n", sp);
	       free(sp);
	       status = 0;
	       goto cleanup;
	    }
	 } else {
            fprintf(stderr, "Error adding `%s' to tree set\n", b);
	    status = 0;
	    goto cleanup;
	 }
      }
   }
cleanup:
   (void) closedir(dd);
   return status;
}



void crawler_thread()
{

  /*

   get lock

   while dir_queue empty
      sleeping_threads++
      if sleeping_threads == # total threads (CRAWLER_THREADS) then
         broadcast that we all need to die (handled by main thread)
      (otherwise)
      wait for signal that queue has  element

   processDir(dir name from queue) <-- signal that queue is non-empty when element added

   release lock

   */

}







/*
 * comparison function between strings
 *
 * need this shim function to match the signature in treeset.h
 */
static int scmp(void *a, void *b) {
   return strcmp((char *)a, (char *)b);
}

/*
 * applies regular expression pattern to contents of the directory
 *
 * for entries that match, the fully qualified pathname is inserted into
 * the treeset
 */


/*********************************************** NOT USING THIS **************************************/
static int applyRe(char *dir, RegExp *reg, TreeSet *ts) {
   DIR *dd;
   struct dirent *dent;
   int status = 1;

   /*
    * open the directory
    */
   if ((dd = opendir(dir)) == NULL) {
      fprintf(stderr, "Error opening directory `%s'\n", dir);
      return 0;
   }
   /*
    * for each entry in the directory
    */
   while (status && (dent = readdir(dd)) != NULL) {
      if (strcmp(".", dent->d_name) == 0 || strcmp("..", dent->d_name) == 0)
         continue;
      if (!(dent->d_type & DT_DIR)) {
         char b[4096], *sp;
	 /*
	  * see if filename matches regular expression
	  */
	 if (! re_match(reg, dent->d_name))
            continue;
         sprintf(b, "%s/%s", dir, dent->d_name);
	 /*
	  * duplicate fully qualified pathname for insertion into treeset
	  */
	 if ((sp = strdup(b)) != NULL) {
            if (!ts_add(ts, sp)) {
               fprintf(stderr, "Error adding `%s' to tree set\n", sp);
	       free(sp);
	       status = 0;
	       break;
	    }
	 } else {
            fprintf(stderr, "Error adding `%s' to tree set\n", b);
	    status = 0;
	    break;
	 }
      }
   }
   (void) closedir(dd);
   return status;
}

/************************************* END NOT USING SPACE ***************************************/

int main(int argc, char *argv[]) {
   char *sp;
   char pattern[4096];
   RegExp *reg;
   Iterator *it;

   if (argc < 2) {
      fprintf(stderr, "Usage: ./fileCrawler pattern [dir] ...\n");
      return -1;
   }
   /*
    * convert bash expression to regular expression and compile
    */
   cvtPattern(pattern, argv[1]);
   if ((reg = re_create()) == NULL) {
      fprintf(stderr, "Error creating Regular Expression Instance\n");
      return -1;
   }
   if (! re_compile(reg, pattern)) {
      char eb[4096];
      re_status(reg, eb, sizeof eb);
      fprintf(stderr, "Compile error - pattern: `%s', error message: `%s'\n",
              pattern, eb);
      re_destroy(reg);
      return -1;
   }
   /*
    * create linked list (dir_queue) and treeset (to print)
    */
   if ((ll = ll_create()) == NULL) {
      fprintf(stderr, "Unable to create linked list\n");
      goto done;
   }
   if ((ts = ts_create(scmp)) == NULL) {
      fprintf(stderr, "Unable to create tree set\n");
      goto done;
   }
   /*
    * populate linked list
    */
   if (argc == 2) {
      if (! processDirectory(".", ll, 1))
         goto done;
   } else {
      int i;
      for (i = 2; i < argc; i++) {
         // add directory to queue (instead of this other stuff)
         if (! ll_add(dir_queue, argv[i]))
            goto done;
      }
   }
   /*
    * for each directory in the linked list, apply regular expression
    */

   // instead, do all this with pthreads and also take care of the rest of the directories 

   /*

   create/initialize pthreads ( ? and locks and conditions ? )

   start running crawler_thread # of them (either 2 or specified by env var

      (having each thread run an instance of cralwer_thread function thing above)

   wait until kill us all is signalled, then kill them all

   ... go on to iterator stage

    */

   while (ll_removeFirst(ll, (void **)&sp)) {
      int stat = applyRe(sp, reg, ts);
      free(sp);
      if (! stat)
         break;
   }
   /*
    * create iterator to traverse files matching pattern in sorted order
    */
   if ((it = ts_it_create(ts)) == NULL) {
      fprintf(stderr, "Unable to create iterator over tree set\n");
      goto done;
   }
   while (it_hasNext(it)) {
      char *s;
      (void) it_next(it, (void **)&s);
      printf("%s\n", s);
   }
   it_destroy(it);
/*
 * cleanup after ourselves so there are no memory leaks
 */
done:
   if (ll != NULL)
      ll_destroy(ll, free);
   if (ts != NULL)
      ts_destroy(ts, free);
   re_destroy(reg);
   return 0;
}
