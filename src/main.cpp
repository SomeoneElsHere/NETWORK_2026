#include <iostream>
#include <bitset>
#include "util.h"

using namespace std;

int main() {
  const char* file = "./build/main";
  string res = md5(file);
  cout << res << endl;
  return 0;
}
