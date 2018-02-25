#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ptree.h"


int main(int argc, char **argv) {
    // TODO: Update error checking and add support for the optional -d flag
    // printf("Usage:\n\tptree [-d N] PID\n");
    // NOTE: This only works if no -d option is provided and does not
    // error check the provided argument or generate_ptree. Fix this!
	if (argc == 2) {
	    struct TreeNode *root = NULL;
	    int success = generate_ptree(&root, strtol(argv[1], NULL, 10));
	    print_ptree(root, 0);
	    if (success != 0) {
	    	return 2;
	    } else {
	    	return 0;
	    }
	} else if (argc == 4) {
	    if (strcmp(argv[1], "-d") != 0) {
	        fprintf(stderr, "Usage:\n\tptree [-d N] PID\n");
	        return 1;
	    } else {
			struct TreeNode *root = NULL;
	        int max_depth = strtol(argv[2], NULL, 10);
	        int success = generate_ptree(&root, strtol(argv[3], NULL, 10));
	        print_ptree(root, max_depth);
		    if (success != 0) {
	            return 2;
		    } else {
		        return 0;
		    }
	    }
	} else {
	    fprintf(stderr, "Usage:\n\tptree [-d N] PID\n");
	    return 1;
	}
}
