#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include "ptree.h"

// Defining the constants described in ptree.h
const unsigned int MAX_PATH_LENGTH = 1024;

// If TEST is defined (see the Makefile), will look in the tests
// directory for PIDs, instead of /proc.
#ifdef TEST
     const char *PROC_ROOT = "tests";
#else
     const char *PROC_ROOT = "/proc";
#endif

int check_exe_filepath(pid_t pid) {
	char procfile[MAX_PATH_LENGTH + 1];
    if (sprintf(procfile, "%s/%d/exe", PROC_ROOT, pid) < 0) {
        //Error: cannot copy or find exe filepath
        fprintf(stderr, "sprintf failed to produce a filename\n");
        return 1;
    }
    struct stat stat_buf;
    if (lstat(procfile, &stat_buf) != 0) {
        //Error: something wrong with symbolic link
        return 1;
	}
	return 0;
}

int assign_name(struct TreeNode **root, pid_t pid) {
	char procfile[MAX_PATH_LENGTH + 1];
	if (sprintf(procfile, "%s/%d/cmdline", PROC_ROOT, pid) < 0) {
	    fprintf(stderr, "sprintf failed to produce a filename\n");
	    return 1;
	} else {
	    char name_of_file[MAX_PATH_LENGTH];
	    FILE *name_file;
	    name_file = fopen(procfile, "r");
	    if (name_file == NULL) {
			(*root) -> name = NULL;
	        return 1;
	    } else {
	        if (fgets(name_of_file, MAX_PATH_LENGTH - 1, name_file) != NULL) {
	            int length = strlen(name_of_file) + 1;
	            char *file_name = malloc(sizeof(char)* length);
	            strcpy(file_name, name_of_file);
	            (*root) -> name = file_name;
	        } else {
	            (*root) -> name = NULL;
	        }
	        if (fclose(name_file) != 0) {
	    	    return 1;
	        }
	    }
		return 0;
	}
}

int validate_child_or_sibling(int *previous_pid, FILE *child_file) {
    int validate_link_success = check_exe_filepath(*previous_pid);
    int has_child_or_sibling = 1;
    if (validate_link_success == 0) {
       //To use it on the child: first pid is not corrupted and therefore is the child
       //To use it on the sibling: the next sibling is not corrupted and therefore is the rightful sibilng
       has_child_or_sibling = 0;
       return 0;
    }
    while(fscanf(child_file, "%d", previous_pid) == 1) {
        validate_link_success = check_exe_filepath(*previous_pid);
        if (validate_link_success == 0) {
            has_child_or_sibling = 0;
            break;
        } 
    }
    if (has_child_or_sibling == 0) {
        //To use it on the child: has child but not the first pid, the first one might be corrupted
        //To use it on the sibling: the next sibling is corrupted and there exists a successor
        return 1;
    } else {
        //To use it on the child: all pids are corrupted
        //To use it on the sibling: the next sibling is corrupted and there is no successor
        return 2;
    }
}

/*
 * Creates a PTree rooted at the process pid.
 * The function returns 0 if the tree was created successfully
 * and 1 if the tree could not be created or if at least
 * one PID was encountered that could not be found or was not an
 * executing process.
 */
int generate_ptree(struct TreeNode **root, pid_t pid) {
    // Here's a way to generate a string representing the name of
    // a file to open. Note that it uses the PROC_ROOT variable.
    char procfile[MAX_PATH_LENGTH + 1];
    int success_check_exe_filepath = check_exe_filepath(pid);
    if (success_check_exe_filepath != 0) {
    	return success_check_exe_filepath;
    }
	*root = malloc(sizeof(struct TreeNode));
	(*root) -> pid = pid;
	(*root) -> sibling = NULL;
	int success_name_assignment = assign_name(root, pid);
    if (success_name_assignment != 0) {
    	return success_check_exe_filepath;
    }
	if (sprintf(procfile, "%s/%d/task/%d/children", PROC_ROOT, pid, pid) < 0) {
	    return 1;
	} else {
	    int pid_of_prev_child = 0;
	    FILE* child_file;
	    child_file = fopen(procfile, "r");
	    if (child_file == NULL) {
	        (*root) -> child = NULL;
	        return 1;
	    } else {
	        if (fscanf(child_file, "%d ", &pid_of_prev_child) == EOF) {
	            //The case where root has no child;
	            (*root) -> child = NULL;
	            if (fclose(child_file) == 0) {
	                return 0;
	            } else {
	                return 1;
	            }
	        } else {
                struct TreeNode **previous_node = &((*root) -> child);
                int returned_validate_child = validate_child_or_sibling(&pid_of_prev_child, child_file);
                if (returned_validate_child == 1) {
                    generate_ptree(previous_node, pid_of_prev_child);
                    while(fscanf(child_file, "%d", &pid_of_prev_child) == 1) {
                        int returned_validate_sibling = validate_child_or_sibling(&pid_of_prev_child, child_file);
                        if (returned_validate_sibling == 2) {
                            break;
                        }
                        previous_node = &((*previous_node) -> sibling);
	                	generate_ptree(previous_node, pid_of_prev_child);
	                }
	                fclose(child_file);
                    return 1;
                } else if (returned_validate_child == 2) {
                    (*root) -> child = NULL;
                    return 1;
                } else {
                    int success = generate_ptree(previous_node, pid_of_prev_child);
                    if (success != 0) {
                        return success;
                    }
			        int overall_success = 0;
                    while(fscanf(child_file, "%d", &pid_of_prev_child) == 1) {
                        int returned_validate_sibling = validate_child_or_sibling(&pid_of_prev_child, child_file);
                        if (returned_validate_sibling == 1) {
                            overall_success = 1;
                        } else if (returned_validate_sibling == 2) {
                            overall_success = 1;
                            break;
                        }
                        previous_node = &((*previous_node) -> sibling);
	                	int success = generate_ptree(previous_node, pid_of_prev_child);
	                	if (success != 0) {
						    return success;
	                	}
	                }
                    if (fclose(child_file) == 0 && overall_success == 0) {
	                    return 0;
	                } else {
	                    return 1;
	                }
                }
            }
	    }
    }
}

void print_ptree_at_depth(struct TreeNode *root, int current_depth, int required_depth) {
    if(root != NULL) {
    	printf("%*s", current_depth * 2, "");
        printf("%d: %s\n", root -> pid, root -> name);
        if (current_depth < required_depth) {
            print_ptree_at_depth(root -> child, current_depth + 1, required_depth);
        }
        print_ptree_at_depth(root -> sibling, current_depth, required_depth);
    }
}

void print_ptree_without_requirement(struct TreeNode *root, int current_depth) {
    if(root != NULL) {
    	printf("%*s", current_depth * 2, "");
        printf("%d: %s\n", root -> pid, root -> name);
        print_ptree_without_requirement(root -> child, current_depth + 1);
        print_ptree_without_requirement(root -> sibling, current_depth);
    }
}

/*
 * Prints the TreeNodes encountered on a preorder traversal of an PTree
 * to a specified maximum depth. If the maximum depth is 0, then the
 * entire tree is printed.
 */
void print_ptree(struct TreeNode *root, int max_depth) {
    // Here's a way to keep track of the depth (in the tree) you're at
    // and print 2 * that many spaces at the beginning of the line.
    if (max_depth == 0) {
      print_ptree_without_requirement(root, 0);
    } else {
      print_ptree_at_depth(root, 0, max_depth);
    }
}
