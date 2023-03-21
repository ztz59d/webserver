#include "log.hpp"
#include <iostream>
using namespace std;
int main()
{
    log log("log-230226");
    string a("abc");
    cout << a.size() << endl;
    string b("testing\n");
    cout << b.size() << endl;
    log.LOG(std::move(a));
    log.LOG(std::move(b));
    log.flush();
}