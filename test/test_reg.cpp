#include <string>
#include <regex>
#include <iostream>
int main()
{
    std::string tar = "POST /audiolibrary/music?ar=1595301089068&n=1p1 HTTP/1.1\\r\\n";
    std::regex r{"(T.*n)"};
    std::smatch res;
    std::regex_match(tar, res, r);
    // std::regex_match("<header>value</header>", res, std::regex({"<(.*)>value</\\1>"}));
    // if (ret.empty())
    // {
    //     std::cout << "Error" << std::endl;
    // }
    for (std::string str : res)
    {
        std::cout << str << std::endl;
    }
    return 0;
}