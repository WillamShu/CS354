////////////////////////////////////////////////////////////////////////////////
// Main File:        division.c
// This File:        division.c
// Semester:         CS 354 Fall 2016
//
// Author:           William Shu
// Email:            fshu2@wisc.edu
// CS Login:         wshu
//
//////////////////////////// 80 columns wide ///////////////////////////////////

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
int count1 = 0; // The number of operation

/* This is the method used to check whether the divisor is 0
 *
 * @int k
 *
 * @return void
 */
void div_handler(int k) {
  printf("Error: a division by 0 operation was attempted.\n");
  printf("Total number of operations completed: %d\n", count1);
  printf("The program will be stopped.\n");
  exit(0);
}

/* This is the method used to check the total number of operations
 *
 * @int k
 *
 * @return void
 */
void count_handler(int k) {
  printf("\nTotal number of operation successfully completed: %d\n", count1);
  printf("The program will be halted.\n");
  exit(0);
}

/* This is the main method which has infinite loop to let user input variables
 * used to do the operations
 *
 * @return int
 */
int main() {
  struct sigaction act;   // the action
  struct sigaction count; // the count
  memset(&act, 0, sizeof(act));
  memset(&count, 0, sizeof(count));
  act.sa_handler = div_handler;
  count.sa_handler = count_handler;
  sigaction(SIGINT, &count, NULL);
  sigaction(SIGFPE, &act, NULL);
  char buf[100];
  int a, b; // Two variables used to do the division.
  while (1) {
    printf("Enter first integer: ");
    fgets(buf, 100, stdin);
    a = atoi(buf);
    printf("Enter second integer: ");
    fgets(buf, 100, stdin);
    b = atoi(buf);
    printf("%d / %d is %d with a remainder of %d\n", a, b, a / b, a % b);
    count1++; // Increase the number of operations
  }
}
