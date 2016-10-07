#include <stdio.h>
#include "mentry.h"
#include "mlist.h"

static void usage(char *program) {
	fprintf(stderr, "usage: %s [file]\n", program);
}

int main(int argc, char *argv[]) {
	MEntry *mep, *meq;
	MList *ml;
	FILE *fd;

	if (argc > 2) {
		usage(argv[0]); return -1;
	}
	if (argc > 1) {
		fd = fopen(argv[1], "r");
		if (fd == NULL) {
			fprintf(stderr, "Error opening %s\n", argv[1]);
			return -1;
		}
	} else
		fd = stdin;
		
	ml = ml_create();
	while ((mep = me_get(fd)) != NULL) {
	
		if (meq == NULL)
			return 0;
		else {
	

			me_print(mep, stdout);



			me_destroy(mep);
		}
	}
	ml_destroy(ml);
	if (fd != stdin)
		fclose(fd);
	return 0;
}
