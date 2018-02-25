#include <stdio.h>
#include <stdlib.h>

int populate_array(int, int *);
int check_sin(int *);


int main(int argc, char **argv) {
    if ((argc != 2) || (argv[1][0] == '0')) {
      printf("Invalid SIN\n");
      return 1;
    }
    int sin_number_array[9];
    int sin_number = strtol(argv[1], NULL, 10);
    int success = populate_array(sin_number, sin_number_array);
    int success_second = check_sin(sin_number_array);
    if (success_second == 0 && success == 0) {
      printf("Valid SIN\n");
      return 0;
    } else {
      printf("Invalid SIN\n");
      return 1;
    }
}
