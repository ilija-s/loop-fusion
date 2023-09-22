// Loops won't be fused, since the second loop uses value from array A that is
// not yet computed.
// If we were to merge them, `B[j] = A[j] + A[j+1] * p;` would try to access
// `A[j+1]` which is not computed.
int main() {
  int p = 2;
  int n = 5;
  int A[6] = {1, 2, 3, 4, 5, 6};
  int B[5] = {0};

  // A = {2, 4, 6, 8, 10, 6}
  for (int i = 0; i < n; i++) {
    A[i] = A[i] * 2;
  }

  // B = { 10, 16, 22, 28, 22}
  for (int j = 0; j < n; j++) {
    B[j] = A[j] + A[j + 1] * p;
  }

  // Correct output is 28, with loop fusion the output is 18.
  return B[3];
}