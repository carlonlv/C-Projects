This is a C program that could be used to check whether a Canadian Social Insurance Number (SIN) is valid.
This program reads a single command line argument representing a candidate SIN number. The program then prints one of two messages (followed by a single newline \n character) to standard output : "Valid SIN", if the value given is a valid Canadian SIN number, or "Invalid SIN", if it is invalid.
The function implements the Luhn algorithm (a process for validating Canadian SIN numbers) to perform this task. The Luhn algorithm is outlined in the Wikipedia page. Here is an example of how to validate a candidate SIN number: 
810620716 <--- A candidate SIN number
121212121 <--- Multiply each digit in the top number by the digit below it.


If the product is a one-digit number, put it in the corresponding position of the result.
If the product is a two-digit number, add the digits together and put the sum in the
corresponding position of the result. For example, in the fourth column, the product of 6
multiplied by 2 is 12. Add the digits together (1 + 2 = 3) and put 3 in the fourth column of result.

The result is:
820320726

Next, sum the digits of the result to produce a total:
8+2+0+3+2+0+7+2+6=30

If the SIN is valid, the total will be divisible by 10.
Note: as a simplification, we will only consider numbers that begin with a non-zero digit to be valid. Therefore, numbers such as 046454286 and 000000026 are considered invalid. 
