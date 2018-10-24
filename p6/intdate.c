////////////////////////////////////////////////////////////////////////////////
// Main File:        intdate.c
// This File:        intdate.c
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

int count1 = 5; // The variable used to count down 5 times

/* This is the method used to display the current time and rest for 3 seconds
 *
 * @int k
 *
 * @return void
 */
void alarm_handler(int k) {
  time_t now = time(0);
  printf("current time is %s", ctime(&now));
  alarm(3);
}

/* This is the method used to print the specific messages which is based on
 * the value of count1
 *
 * @int k
 *
 * @return void
 */
void count_handler(int k) {
  count1--;
  if (count1 > 0) {
    printf("\nControl -c caught. %d more before program ends.\n", count1);
  } else {
    printf("\nFinal control-c caught. Exiting.\n");
    exit(0);
  }
}

/* This is the main method which has infinite loop to check whether the user
 * used ^c command and call the two methods within the infinite loop.
 *
 * @return int
 */
int main() {
  alarm(3);
  struct sigaction count; // the count
  struct sigaction act;   // the action
  memset(&act, 0, sizeof(act));
  memset(&count, 0, sizeof(count));
  act.sa_handler = alarm_handler;
  count.sa_handler = count_handler;
  sigaction(SIGINT, &count, NULL);
  sigaction(SIGALRM, &act, NULL);
  printf("Date will be printed every 3 seconds\n");
  printf("Enter ^C 5 times to end the program:\n");
  while (1) {
  }
}
