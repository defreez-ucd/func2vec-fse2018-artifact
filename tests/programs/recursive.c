#include <stdio.h>

void foo(int count) {
  printf("%d", count);
  if (count > 0) {
    foo(--count);
  }
}

int main(int argc, char **argv) {
  foo(argc);
  return 0;
}
