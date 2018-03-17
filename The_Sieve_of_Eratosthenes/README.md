This is a program that determines whether a number N can be factored into exactly two primes, and if it can be factored, identifies the values of the two factors. 
The algorithm implemented is based on the Sieve of Eratosthenes, which itself is an algorithm to identify prime numbers. 
The Sieve of Eratosthenes
The idea behind the Sieve of Eratosthenes is to use a set of consecutive filters. Each filter takes a list of numbers as input and removes (or filters out) some of them, so that the output is a smaller list. Each filter is related to a prime number, which is a natural number greater than 1 that can be divided without remainder only by itself and 1. The job of the filter related to some prime m, is to remove all multiples of m. For example, the filter for m=3 would remove any of 3,6,9,12,15, ... that appear in the input. The filters in the sieve are related to the prime numbers in increasing order. So they are m=2, m=3, m=5 etc. If the sieve's input is the list of integers from 2 to n, the basic sieve will have filters for all the prime numbers between 2 and n. See below for an example sieve for n = 14. 
2 3 4 5 6    m=2            m=3              m=5            m=7         m=11       m=13
7 8 9 10     ==>   3 5 7 9  ==>  5 7 11 13   ==>  7 11 13   ==>   11 13  ==>   13  ==>
11 12 13 14        11 13
The program creates filters starting with m=2 and use the list of integers from 2 to n as the initial input. 
Each time before it creates a filter for value m, check to see if m is a factor of n and keeps track of the factors found. 
It stops creating filters when m would be greater than or equal to the square root of n. It does not create a filter for the square root if n is a perfect square.
If the program finds no factors, then n must be prime.
As soon as the program discover two primes that are factors, and both are less than the square root of n, it concludes that n is not the product of two primes. In this case, it stops creating filters immediately. 
If the program finds exactly one factor between 2 and square root of n (inclusive), then n may be the product of two primes.
The program takes in a number as an argument and prints the corresponding message. Please do not provide n greater than 273210, there may be some cases where the program does not stop until it recieves a stop signal or as such. You must not leave processes lying around on any of the machines you work on, or the machines will eventually become unusable. Please use ps aux | grep <username> or top to check if you have any stray processes still running. Use kill to kill them. You may find killall pfact on Linux to be useful.
