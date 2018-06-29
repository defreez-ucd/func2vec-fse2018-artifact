// This is for testing ERR_ and NO_ERR_ path annotations

int foo1();
int interesting();
int foo3();
int foo4();
int foo5();
int foo6();

int main() {
  foo1();
  // We need to put an interesting function somewhere to
  // seed path generation. This isn't a represenative
  // for the interesting function, but foo4() would
  // not work because all paths through foo4 return early.
  interesting();
  int err;
  if ((err = foo3() < 0)) {
    foo4(); // snd_trident_free
    return 1;
  }
  if ((err = foo5() < 0)) { // snd_trident_mixer
    return 1;
  }
  foo6();
  return 0;
}
