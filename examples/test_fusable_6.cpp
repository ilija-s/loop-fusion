// Loops 2 and 3 will be fused.
int main() {
  int A[100] = {0};
  int B[100] = {0};
  int p = 0;
  const int N = 100;
  const int M = 50;

  // Loop 1
  for (int i = 0; i < N; i++) {
    B[i] = i;
  }

  // Loop 2
  for (int i = 0; i < N; i++) {
    B[i] = i * 2;
  }

  // Loop 3
  for (int i = 0; i < N; i++) {
    A[i] = i;
    for (int j = 0; j < M; j++) {
      p += A[i];
    }
  }

  return p;
}
