void interesting() {}
void a() {}
void b() {}

// a interesting b
// interesting b

int main() {
  a();
  for (int i = 0; i < 10; ++i) {
    interesting();    
  }
  b();
  return 0;
}
