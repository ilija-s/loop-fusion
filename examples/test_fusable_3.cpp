const int N = 100;
const int M = 100;

int main() {
  int p, j, k, l, s, g, r, a, b, c, d, e, f;
  float q;
  int A[N], B[M], C[N], D[M];

  j = k = l = r = a = b = c = d = e = f = 0;
  p = 4;
  q = 0.0;

  // Initialization
  for (int i = 0; i < N; i++) {
    A[i] = (i * 2) + (i - 6);
    B[i] = (i * 3) + (6 - i);
    C[i] = (i * 4) + (i + 2);
    D[i] = (i * 5) + (i + 4);
  }

  for (int i = 0; i < N; i++) {
    j += i + 2;
    l += i * 3;
    a += i + 2;
    b += i - 2;
    q = q + i;
    p = p - i;
    for (g = 0; g < M; g++) {
      k += g - 4;
      r += g * 2;
      c += g + 6;
    }
    s = 0;
    p += 5;
    q -= 4;
  }

  return 0;
}