int interesting();
int foo3();
int foo4();
int foo5();
int foo6();

int main() {
  interesting();
  int err;
  if ((err = foo3() < 0)) {
    foo4();
    // Not RETURN_ERR_NO_ERR because indistinguishable from
    // return 0;
  }

  return 0;
}
