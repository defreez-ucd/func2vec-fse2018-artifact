void interesting() {}
void foo_a() {}
void foo_b() {}
void bar_a() {}
void bar_b() {}

struct st {
  void (*foo)();
  void (*bar)();
};

const struct st a_ptrs = {
  .foo = foo_a,
  .bar = bar_a,
};

const struct st b_ptrs = {
  .foo = foo_b,
  .bar = bar_b,
};

// Path from foo_a to interesting
// foo_b and bar_b should automatically be added to the
// list of interesting functions
void func2() {
  foo_b();
  bar_b();
  interesting();
}

void func1() {
  struct st st1 = a_ptrs;
  st1.foo();
  func2();
  st1.bar();
}

