void public_in(int);

int foo(int x) {
  public_in(x);
  return x + 1;
}
