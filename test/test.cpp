#include <list>
#include <string>
#include <memory>
#include <iostream>
#include "timer.h"
using namespace std;
int main(int argc, char *argv[])
{
    list<list<shared_ptr<int>>> lo(243, list<shared_ptr<int>>());
    cout << lo.size() << endl;

    return 0;
}