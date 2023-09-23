// Loops won't be fused, since the first loop contains volatile memory access.
int main() {
  int result1 = 0;
  int result2 = 0;
  volatile int value;

  for (int i = 0; i < 5; i++) {
    value++;
    result1 += i;
  }

  for (int j = 0; j < 5; j++) {
    result2 += result1 + j;
  }

  return result2;
}
