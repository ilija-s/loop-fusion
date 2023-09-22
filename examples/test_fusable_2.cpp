// Loops will be fused, because they do not modify the same data.
int main() {
  int result1 = 0;
  int result2 = 0;
  int arr[5] = {0, 1, 2, 3, 4};

  for (int i = 0; i < 5; i++) {
    result1 += i;
  }

  for (int j = 0; j < 5; j++) {
    result2 = arr[j] * j;
  }

  return result2;
}
