#include "log.hpp"
#include <iostream>
using namespace std;
int main()
{
    log &logger = log::get_instance("log-230226");
    // log log("log-230226");
    string a("abc0423");
    cout << a.size() << endl;
    string b("testing\n");
    cout << b.size() << endl;
    logger.LOG(std::move(a));
    logger.LOG(std::move(b));
    logger.flush();
}