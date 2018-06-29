// At some point Aditya said that paths "should go around the loop once"
// Does this mean that the body of the loop should be printed twice?

void interesting() {}
void a() {}

int main() {
  for (int i = 0; i < 10; ++i) {
    a();
  }
  interesting();
  return 0;
}
