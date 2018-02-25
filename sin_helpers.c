/*
 * Convert a 9 digit int to a 9 element int array.
 */
int populate_array(int sin, int *sin_array) {
    int size = 0;
    int number = sin;
    while (sin) {
        sin = sin / 10;
	      size++;
    }
    if (size != 9) {
        return 1;
    }
    for (int i = 0; i < 9; i++) {
    	int digit = number % 10;
    	sin_array[8 - i] = digit;
	    number = number / 10;
    }

    if (sin_array[0] == 0) {
      return 1;
    }
    return 0;
}

/*
 * Return 0 (true) iff the given sin_array is a valid SIN.
 */
int check_sin(int *sin_array) {
    int test[9] = {1, 2, 1, 2, 1, 2, 1, 2, 1};
    int result[9];
    int sum = 0;
    for (int i = 0; i < 9; i++) {
        int product = test[i] * sin_array[i];
	      int copy = product;
	      int size = 0;
        if (copy != 0) {
      	  while (copy) {
            copy = copy / 10;
            size++;
          }
        } else {
          size = 1;
        }
      	if (size == 1) {
      	    result[i] = product;
      	} else {
            int products[2];
            for (int a = 0; a < 2; a++) {
            	int digit = product % 10;
            	products[a] = digit;
        	    product = product / 10;
            }
            result[i] = products[0] + products[1];
      	}
        sum += result[i];
   }
   if (sum % 10 == 0) {
     return 0;
   } else {
     return 1;
   }
 }
