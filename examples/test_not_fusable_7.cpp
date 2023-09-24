// Loops will not be fused because the second loop modifies the increment.
int main() {
  int A[100] = {0};
  int B[100] = {0};
  int n = 100;
  int m = 100;

  for (int i = 0; i < n; i++) {
    A[1] = 1;
  }

  for (int i = 0; i < n; i++) {
    i = i + 1;
  }

  return B[99];
}
