////////////////////////////////////////////////////////////////////////////////
// Main File:        cache1D.c
// This File:        cache1D.c
// Semester:         CS 354 Fall 2016
//
// Author:           William Shu
// Email:            fshu2@wisc.edu
// CS Login:         wshu
//////////////////////////// 80 columns wide ///////////////////////////////////

int arr[100000]; // The array size of 100 thousand

/* This program set its element of an array of
 * 100 thousand integers to the value of the index
 *
 */
int main() {
  int a; // current pointed location
  for (a = 0; a < 100000; a++) {
    arr[a] = a;
  }
  return 0;
}
