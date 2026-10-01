#include <iostream>
#include "threadtest/threadtest.h"
using namespace std;

const chrono::milliseconds WAIT_LONG = chrono::milliseconds(20000);
const chrono::milliseconds WAIT_SHORT = chrono::milliseconds(4000);
const chrono::milliseconds WAIT_BRIEF = chrono::milliseconds(2000);

int main() {
  cout << "hello, world\n" << endl;

  ThreadTest* test = new ThreadTest();

  /* First-run of each thread */
  test -> runClientThread();
  test -> runServerThread();

  this_thread::sleep_for(WAIT_LONG);

  cout << "Is client still running? " << (test->isClientRunning() ? "Yes" : "No") << ".\n";
  cout << "Is server still running? " << (test->isServerRunning() ? "Yes" : "No") << ".\n";

  /* Stop running each thread */
  this_thread::sleep_for(WAIT_SHORT);

  test -> stopClientThread();
  test -> stopServerThread();

  this_thread::sleep_for(WAIT_SHORT);

  cout << "Is client still running? " << (test->isClientRunning() ? "Yes" : "No") << ".\n";
  cout << "Is server still running? " << (test->isServerRunning() ? "Yes" : "No") << ".\n";

  /* Test destructor */
  this_thread::sleep_for(WAIT_SHORT);

  test -> runClientThread();
  test -> runServerThread();

  this_thread::sleep_for(WAIT_BRIEF);

  delete test;

  return 0;
}
