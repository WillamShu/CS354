////////////////////////////////////////////////////////////////////////////////
// Main File:        cache2Dcols.c
// This File:        cache2Dcols.c
// Semester:         CS 354 Fall 2016
//
// Author:           William Shu
// Email:            fshu2@wisc.edu
// CS Login:         wshu
//////////////////////////// 80 columns wide ///////////////////////////////////

int twoDArray[3000][500]; // The 3000 by 500 2-dimensional array

/* This program set its element of an 2D array of
 * 3000 by 500 2-dimensional array
 * treaverse first by columns
 */
int main() {
  int row, column; // The Row and Column indicies.
  for (column = 0; column < 500; column++) {
    for (row = 0; row < 3000; row++) {
      twoDArray[row][column] = row + column;
    }
  }
  return 0;
}
