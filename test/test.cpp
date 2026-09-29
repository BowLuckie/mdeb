#include "../vendor/linenoise/linenoise.h"
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <iostream>

using std::cout;
int add(int a, int b) { return a + b; }

int main() {
  cout << "Hello World!\n";
  cout << "1 + 3 = " << add(1, 3) << '\n';

  linenoise("Can i get your take on this? ");

  std::srand(std::time(nullptr));
  int random = (std::rand() % 100) + 1;
  if (random % 2 == 0) {
    cout << "Great take!\n";
  } else {
    cout << "bad take :(\n";
  }

  return 1;
}
