// This is for testing ERR_ and NO_ERR_ path annotations

int interesting();
int foo3();
int foo4();

int main() {
  interesting();
  int err;
  if ((err = foo3() < 0)) {
    foo4();
    return 1;
  }
}
