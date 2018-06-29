void interesting() {}
void A() {}
void B() {}
void C() {}
void D() {}
void U() {}
void V() {}
void W() {}
void X() {}

void func1() {
  interesting();
}

void func2() {
  A();
  //B();
  func1();
  //  C();
  D();
}


void func3() {
  U();
  //  V();
  func1();
  //  W();
  X();
}

// The backward paths from interesting_func are { <A,B>, <U,V>}.
// And the forward paths from interesting_func are { <C,D>, <W,X>}.
// Performing a cartesian product you will end up with the invalid paths <A,B,W,X> and <U,V,C,D>.
