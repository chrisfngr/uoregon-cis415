/*

Joseph Yaconelli
josephy
CIS 415 Extra Credit

This is my own work except that I discussed handling the final thread case with Liz Olson.


*/


#define _DEFAULT_SOURCE 1	/* enables macros to test type of directory entry */

#include <unistd.h>
#include <pthread.h>
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

pthread_t crawler_thread;

pthread_mutex_t dir_queue_lock;
pthread_cond_t  queue_nonempty_cond,
                kill_us;

RegExp *reg;

int sleeping_thread_count = 0;
int total_thread_count    = 1; /* for now */
int dont_stop             = 1; /* mainly testing */

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





/* take directory name, process it, add any additional directories to queue and files to print
*/

static int processDirectory(char *dirname) {
   DIR *dd;
   struct dirent *dent;
   char *dir_str, *file_str;
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
      fprintf(stderr, "Error opening directory `%s'\n", d);
      return 1;
   }
   /*
    * duplicate directory name to insert into linked list
    */
   /*sp = strdup(d);
   if (sp == NULL) {
      fprintf(stderr, "Error adding `%s' to linked list\n", d);
      status = 0;
      goto cleanup;
   }
   */
   if (len == 1 && d[0] == '/')
      d[0] = '\0';

   /*
    * read entries from the directory
    */

   char b[4096];

   while (status && (dent = readdir(dd)) != NULL) {
      if (strcmp(".", dent->d_name) == 0 || strcmp("..", dent->d_name) == 0)
         continue;
      if (dent->d_type & DT_DIR) {
         sprintf(b, "%s/%s", d, dent->d_name);
         /* add dir to dir_queue instead of recursion */
         dir_str = strdup(b);
         if(!ll_add(dir_queue, dir_str)) {
           fprintf(stderr, "Error adding `%s' to directory queue\n", dir_str);
           /* free(b); */
           status = 0;
           goto cleanup;
         }
         printf("added `%s' to directory queue\n", dir_str);
      } else {
    	 /*
	  * see if filename matches regular expression
	  */
	 if (! re_match(reg, dent->d_name))
            continue;
         sprintf(b, "%s/%s", d, dent->d_name);
	 /*
	  * duplicate fully qualified pathname for insertion into treeset
	  */
	 if ((file_str = strdup(b)) != NULL) {
            if (!ts_add(to_print_ts, file_str)) {
               fprintf(stderr, "Error adding `%s' to tree set\n", file_str);
	       free(file_str);
	       status = 0;
	       goto cleanup;
	    }
	 } else {
            fprintf(stderr, "Error adding `%s' to tree set\n", b);
	    status = 0;
	    goto cleanup;
	 }

         printf("added `%s' to tree set\n", file_str);
      }
   }
cleanup:
   (void) closedir(dd);
   return status;
}



void * crawler_worker(void * unused)
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




while(dont_stop)
{

   char** d;
   //pthread_mutex_lock(&dir_queue_lock);
/*
   while(ll_size(dir_queue) == 0){
      sleeping_thread_count++;
      if(sleeping_thread_count == total_thread_count){
         pthread_cond_broadcast(&kill_us);
         goto unlock; 
         pthread_mutex_unlock(&dir_queue_lock);
         sleeping_thread_count = 1;
      }else{
         pthread_cond_wait(&queue_nonempty_cond, &dir_queue_lock);
         sleeping_thread_count--;
      }
   }

*/
   if(!ll_removeFirst(dir_queue, (void **) d)){
      fprintf(stderr, "Unable to remove element from dir_queue\n");
      fprintf(stderr, "while testing means time to end\n");
      sleeping_thread_count = 1;
   }


   fprintf(stderr, "about to call procssDir on `%s'\n", *d);   
   processDirectory(*d);
   fprintf(stderr, "just returned from processDir\n");



   //pthread_mutex_unlock(&dir_queue_lock);

}
   return NULL;

}


/*
 * comparison function between strings
 *
 * need this shim function to match the signature in treeset.h
 */
static int scmp(void *a, void *b) {
   return strcmp((char *)a, (char *)b);
}


int main(int argc, char *argv[]) {
   char *sp;
   char pattern[4096];
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
   if ((dir_queue = ll_create()) == NULL) {
      fprintf(stderr, "Unable to create linked list\n");
      goto done;
   }
   if ((to_print_ts = ts_create(scmp)) == NULL) {
      fprintf(stderr, "Unable to create tree set\n");
      goto done;
   }
   /*
    * populate linked list
    */
   if (argc == 2) {
      if (! processDirectory("."))
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


   //clean up routine
   /*
   pthread_cleanup_push(cleanup_func(), arg);

   */

   /* initialize lock(s) and condition(s) */
   pthread_mutex_init(&dir_queue_lock, NULL);
   pthread_cond_init(&queue_nonempty_cond, NULL);
   pthread_cond_init(&kill_us, NULL);

   /* initialize thread(s) */
   if(pthread_create(&crawler_thread, NULL, &crawler_worker, NULL) != 0)
      fprintf(stderr, "Unable to create pthread\n");


   printf("thread started\n");

   
   /*
   the cleanup_func will get called first
   pthread_cond_wait(&kill_us, &dir_queue_lock);
   pthread_cancel(crawler_thread);
   */

  /* bad busy wait for testing */
   int i = 0;

   /*pthread_mutex_lock(&dir_queue_lock);*/
   while(sleeping_thread_count != total_thread_count){
    printf("\t\tMAIN THREAD: still waiting... %d\n", i);
    i += 1;
    sleep(1);
    //pthread_cond_wait(&kill_us, &dir_queue_lock);
   }

   //pthread_mutex_unlock(&dir_queue_lock);

   printf("\t\tMAIN THREAD: done waiting!\n");
   //pthread_cancel(crawler_thread);


   dont_stop = 0;
   //pthread_join(crawler_thread, NULL);

   printf("\t\tMAIN THREAD: killed threads!\n");
   /*
    * create iterator to traverse files matching pattern in sorted order
    */
   if ((it = ts_it_create(to_print_ts)) == NULL) {
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
   if (dir_queue != NULL)
      ll_destroy(dir_queue, free);
   if (to_print_ts != NULL)
      ts_destroy(to_print_ts, free);
   re_destroy(reg);
   return 0;
}
