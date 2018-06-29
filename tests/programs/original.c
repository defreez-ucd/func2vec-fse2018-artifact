void func2() {}
void func3() {}
void func4() {}
void func5() {}
void func6() {}
void func7() {}

void foo();
void function1();
void interesting();

void foo() {
  func2();
  func3();
}

void function1() {
  int x;

  foo();
  if (x) {
    func5();
  } else {
    func6();
  }
  interesting();
  func7();  
}

void zoo() {
  func4();
  function1();
}
