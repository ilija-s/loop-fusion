// Loops 2 and 3 will be fused.
int main() {
  int A[100] = {0};
  int B[100] = {0};
  int n = 100;

  // Loop 1
  for (int i = 0; i < n; i++) {
    B[i] = i;
  }

  // Loop 2
  for (int i = 0; i < n; i++) {
    B[i] = i * 2;
  }

  // Loop 3
  for (int i = 0; i < n; i++) {
    A[i] = i;
  }

  return B[99];
}
