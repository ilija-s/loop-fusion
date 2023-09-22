// Loops will not be fused, since the second loop uses variable whose value
// is modified in the first loop.
int main() {
  int result1 = 0;
  int result2 = 0;

  // result1 has value 0 + 1 + 2 + 3 + 4 = 10
  for (int i = 0; i < 5; i++) {
    result1 += i;
  }

  // result2 has value 10 + 11 + 12 + 13 + 14 = 60
  for (int j = 0; j < 5; j++) {
    result2 += result1 + j;
  }

  return 0;
}
