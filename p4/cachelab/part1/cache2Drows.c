////////////////////////////////////////////////////////////////////////////////
// Main File:        cache2Drows.c
// This File:        cache2Drows.c
// Semester:         CS 354 Fall 2016
//
// Author:           William Shu
// Email:            fshu2@wisc.edu
// CS Login:         wshu
//////////////////////////// 80 columns wide ///////////////////////////////////

int twoDArray[3000][500]; // The 3000 by 500 2-dimensional array

/* This program set its element of an 2D array of
 * 3000 by 500 2-dimensional array
 * treaverse first by rows
 */
int main() {

  int row, column; // The Row and Column indicies.
  for (row = 0; row < 3000; row++) {
    for (column = 0; column < 500; column++) {
      twoDArray[row][column] = row + column;
    }
  }
  return 0;
}
