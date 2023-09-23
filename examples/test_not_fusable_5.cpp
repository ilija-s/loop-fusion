// Loops won't be fused, since they use a shared iterator declared outside-of
// the loops, thus merging them would result in incrementing the `i` two times
// and breaking the program.
int main() {
  int A[100] = {0};
  int B[100] = {0};
  int n = 100;
  int i;

  for (i = 0; i < n; i++) {
    A[i] = i;
  }

  for (i = 0; i < n; i++) {
    B[i] = i * 2;
  }

  return 0;
}
