#include <stdio.h>
#include <stdlib.h>

double data[5];

int main() {

  int i;
  for(i = 0; i < 5; i++) {
    data[i] = i;
  }

  double value = data[3];

  return 0;
}