// Loops won't be fused, since the loops are not adjacent.
int main() {
  int A[100] = {0};
  int B[100] = {0};
  int n = 100;
  int d = 0;

  for (int i = 0; i < n; i++) {
    A[i] = i;
  }

  if (n < 200) {
    d = 1;
  } else {
    d++;
  }

  for (int i = 0; i < n; i++) {
    B[i] = i * 2;
  }

  return 0;
}