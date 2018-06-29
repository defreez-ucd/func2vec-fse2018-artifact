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


// Path from foo_a to {interesting, bar_a} should return two paths
void func1() {
  struct st st1 = a_ptrs;
  st1.foo();
  interesting();
  st1.bar();
}

// Note that the control flow pass cannot distinguish between
// st1 and st2. This is because we approximate function pointers in structs
// by the struct type, which is the same in each case.
//
// So, func2 would look identical to func1. This is actually OK, because
// we are interested in all possible assignments to the struct, and individual
// file systems do not call their functions through the interface.

void func2() { 
   struct st st2 = b_ptrs;
   st2.foo();
   interesting();
   st2.bar();
}
