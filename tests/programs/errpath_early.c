// This is for testing ERR_ and NO_ERR_ path annotations
// foo6 should be annotated with both ERR_ and NO_ERR

int interesting();
int foo2();
int foo3();
int foo4();
int foo6();

int main() {
  interesting();
  foo2();
  return 0;
}
