// Loops won't be fused, since the second loop uses value from array A that is
// not yet computed.
// If we were to merge them, `B[j] = A[j] + A[j+1] * p;` would try to access
// `A[j+1]` which is not computed.
int main() {
  int p = 2;
  int n = 5;
  int A[6] = {1, 2, 3, 4, 5, 6};
  int B[5] = {0};

  for (int i = 0; i < n; i++) {
    A[i] = A[i] * 2;
  }

  for (int j = 0; j < n; j++) {
    B[j] = A[j] + A[j + 1] * p;
  }

  return 0;
}