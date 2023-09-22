// Loops will be fused, since both of them iterate from [0, n) and they
// do not modify the same array.
int main() {
  int A[100] = {0};
  int B[100] = {0};
  int n = 100;

  for (int i = 0; i < n; i++) {
    A[i] = i;
  }

  for (int i = 0; i < n; i++) {
    B[i] = i * 2;
  }

  return B[99];
}
