#include <stdio.h>
#include <string.h>

void sensitiveFunction(char *Input) {
  char Buffer[50];
  strcpy(Buffer, Input);
  printf("Buffer content: %s\n", Buffer);
}

void processInput(char *Input) {
  char Temp[100];
  strcpy(Temp, Input);
  sensitiveFunction(Temp);
}

int main() {
  char UserInput[100];
  printf("Enter some input: ");
  fgets(UserInput, sizeof(UserInput), stdin); // Source of taint
  processInput(UserInput);
  return 0;
}
