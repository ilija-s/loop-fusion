// Loops won't be fused, since the first loop iterates `m` times, and the second
// loop iterates `n` times.
int main() {
  int p = 4;
  int n = 5;
  int m = 5;
  int B[5] = {0};

  double q = 0.0;

  for (int k = 0; k < m; k++) {
    q = q + B[k];
  }

  for (int l = 0; l < n; l++) {
    B[l] = B[l] * p;
  }

  return 0;
}