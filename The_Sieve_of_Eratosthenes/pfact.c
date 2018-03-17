#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    char *remainding_char = NULL;
    int target = strtol(argv[1], &remainding_char, 10);
    if (target <= 1 || argc != 2 || strlen(remainding_char) != 0) {
        fprintf(stderr, "Usage:\n\tpfact n\n");
        return 1;
    }
    
    //attributes as parent
    int parent_signal = 0;
    int possible_parent_factor = 0;
    int keep_forking = 1;
    int number_of_ints = target - 1;
    int number_of_filtered_ints = number_of_ints;

    //attributes that parent and child share
    int status;
    int current_num;

    int fd_1[2];
    int fd_2[2];
    if (pipe(fd_1) == -1) {
        perror("pipe fd_1\n");
        exit(-1);
    }
    //attributes as child
    int factor;
    int child_signal;
    int result[2];

    int n = fork();
    if (n < 0) {
        perror("fork");
        exit(-1);
    } else if (n == 0) {
        while (keep_forking) {
            //update number_of_ints to read in children
            number_of_ints = number_of_filtered_ints;
            if (n < 0) {
                perror("fork");
                exit(-1);
            } else if (n == 0) {
                fd_2[0] = fd_1[0];
                fd_2[1] = fd_1[1];
                if (close(fd_2[1]) == -1 ) {
                    perror("close write fd_2 in children\n");
                    exit(-1);
                }
                if (read(fd_2[0], &factor, sizeof(int)) == -1) {
                    perror("write factor in parent\n");
                    exit(-1);
                }
                if (factor * factor < target) {
                    // the case where factor is smaller than square root of target
                    if (target % factor == 0) {
                        //case example: 12
                        if (parent_signal == 1) {
                            for(int i = 1; i <= number_of_ints - 1; i++) {
                                if (read(fd_2[0], &current_num, sizeof(int)) == -1) {
                                    perror("write list of number to be filtered in parent\n");
                                    exit(-1);
                                }
                            }
                            if (close(fd_2[0]) == -1) {
                                perror("close fd_2 read in children\n");
                                exit(-1);
                            }
                            child_signal = 3;
                            keep_forking = 0;
                        } else {
                            //keep forking while passing in the parent_signal of 1
                            parent_signal = 1;
                            possible_parent_factor = factor;
                            if (pipe(fd_1) == -1) {
                                perror("pipe fd_1\n");
                                exit(-1);
                            }
                            n = fork();
                        }
                    } else {
                        if (pipe(fd_1) == -1) {
                            perror("pipe fd_1\n");
                            exit(-1);
                        }
                        n = fork();
                    }
                } else if (factor * factor == target){
                    //the case where factor is equal to the square root of target
                    //read the rest of the numbers from parent pipe(fd_2)
                    for(int i = 1; i <= number_of_ints - 1; i++) {
                        if (read(fd_2[0], &current_num, sizeof(int)) == -1) {
                            perror("write list of number to be filtered in parent\n");
                            exit(-1);
                        }
                    }
                    if (close(fd_2[0]) == -1) {
                        perror("close fd_2 read in children\n");
                        exit(-1);
                    }
                    keep_forking = 0;
                    child_signal = 1;
                    result[0] = factor;
                    result[1] = factor;
                } else {
                    //the case where factor is larger than the square root of target
                    int array[number_of_ints];
                    array[0] = factor;
                    //read the rest of the numbers from parent pipe(fd_2)
                    for(int i = 1; i <= number_of_ints - 1; i++) {
                        if (read(fd_2[0], &current_num, sizeof(int)) == -1) {
                            perror("write list of number to be filtered in parent\n");
                            exit(-1);
                        }
                        array[i] = current_num;
                    }
                    if (close(fd_2[0]) == -1) {
                        perror("close fd_2 read in children\n");
                        exit(-1);
                    }
                    keep_forking = 0;
                    if (parent_signal == 1) {
                        //the other factor to look for
                        int new_target = target / possible_parent_factor;
                        int find = 0;
                        for (int i = 0; i < number_of_ints; i++) {
                            if (array[i] == new_target) {
                                find = 1;
                            }
                        }
                        if (find == 1) {
                            //case example: 14
                            child_signal = 2;
                            result[0] = possible_parent_factor;
                            result[1] = new_target;
                        } else {
                            //case expample: 16
                            child_signal = 3;
                            result[0] = 0;
                            result[1] = 0;
                        }
                    } else {
                        //case expample: 3
                        child_signal = 0;
                        result[0] = 0;
                        result[1] = 0;
                    }
                }
                if (keep_forking == 0) {
                    if (child_signal == 3) {
                        //signal is 3 means the target is made of more than two prime number factors
                        printf("%d is not the product of two primes\n", target);
                    } else if (child_signal == 0){
                        //signal is 0 means the target is a prime
                        printf("%d is prime\n", target);
                    } else {
                        //signal is 1 means the target has only one prime number as the factor
                        //signal is 2 means the target has two prime number factors and this is only of them
                        printf("%d %d %d\n", target, result[0], result[1]);
                    }
                    exit(0);
                }
            } else {
                if (close(fd_1[0]) == -1 ) {
                    perror("close fd_1 read in parent\n");
                    return 1;
                }
                //reset the count of numbers to be written to child to 0
                number_of_filtered_ints = 0;
                for(int i = 1; i <= number_of_ints - 1; i++) {
                    //read the rest of the numbers from parent pipe(fd_2)
                    if (read(fd_2[0], &current_num, sizeof(int)) == -1) {
                        perror("write list of number to be filtered in parent\n");
                        exit(-1);
                    }
                    //begin filter process while writing new list to the child_pipe(fd_1)
                    if (current_num % factor != 0) {
                        //keep track of numbers that are written to child
                        number_of_filtered_ints++;
                        if (write(fd_1[1], &current_num, sizeof(int)) == -1) {
                            perror("write list of number to be filtered in parent\n");
                            exit(-1);
                        }
                    }
                }

                if (close(fd_2[0]) == -1) {
                    perror("close fd_2 read in children\n");
                    exit(-1);
                }
                if (close(fd_1[1]) == -1) {
                    perror("close fd_1 write in parent\n");
                    exit(-1);
                }
                if (wait(&status) == -1) {
                    perror("wait in parent\n");
                    exit(-1);
                }
                if (WIFEXITED(status)) {
                    if (WEXITSTATUS(status) != -1) {
                        exit(WEXITSTATUS(status) + 1);
                    } else {
                        exit(-1);
                    }
                } else {
                    exit(-1);
                }
            }
        }
    } else {
        if (close(fd_1[0]) == -1 ) {
            perror("close read in main\n");
            return 1;
        }
        for (int i = 2; i <= target; i++) {
            if (write(fd_1[1], &i, sizeof(int)) == -1) {
                perror("write a list of number to be filtered in main parent\n");
                return 1;
            }
        }
        if (close(fd_1[1]) == -1) {
            perror("close write fd_1 in main parent\n");
            return 1;
        }
        int main_status;
        if (wait(&main_status) == -1) {
            perror("wait in main parent\n");
            return 1;
        }
        if (WIFEXITED(main_status)) {
            if (WEXITSTATUS(main_status) != -1) {
                printf("Number of filters = %d\n", WEXITSTATUS(main_status));
            } else {
                return 1;
            }
        } else {
            return 1;
        }
    }
}
