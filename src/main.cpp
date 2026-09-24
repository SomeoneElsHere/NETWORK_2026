#include <iostream>
#include "util.h"

using namespace std;

int main() {
  string s = "hello, world";
  if(!string_pad(s, '0', 16)) cout << "err" << endl;
  cout << s << endl;
  return 0;
}
