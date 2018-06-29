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
  }

  return err;
}
