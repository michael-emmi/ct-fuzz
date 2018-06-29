void public_in(int);

int bar(void);

int foo(int x) {
  int y = x + bar();
  public_in(y);
  return y + 1;
}
