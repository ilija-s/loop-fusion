// Loops will be fused, since both of them iterate from [0, n) and they
// do not modify the same array.
int main() {
  int A[100] = {0};
  int B[100] = {0};
  int n = 100;

  for (int i = 0; i < n; i++) {
    if (i < n / 2) {
      A[i] = i;
    } else {
      A[i] = i * 2;
    }
  }

  for (int i = 0; i < n; i++) {
    if (i < n / 2) {
      B[i] = i * 2;
    } else {
      B[i] = i + 1;
    }
  }

  return B[99];
}
