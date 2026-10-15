#include "includes.hpp"

int main() {
  Tracker tracker("9999");
  tracker.print_status();
  //tracker.run();
  sleep(3);
  printf("\n");
  tracker.print_status();
}
