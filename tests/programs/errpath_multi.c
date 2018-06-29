// This is for testing ERR_ and NO_ERR_ path annotations

int interesting();
int foo2();
int foo3();
int foo4();

int main() {
  interesting();
  int err;
  if ((err = foo3() < 0)) {
    err = foo2();
    if (err) {
      foo4();
    }
  }
}
