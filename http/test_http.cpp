#include "http_conn.hpp"

int main()
{
    Http_connection http(1, 4);
    // http.m_read_buffer.Append(std::string("GET / HTTP/1.0\r\nReferer: http://www.google.cn\r\nAccept-Language: zh-cn\r\nAccept-Encoding: gzip, deflate\r\n\r\n"));
    http.process();

    return 0;
}

// #include <iostream>
// #include <list>
// #include <memory>
// using namespace std;
// std::ostream &operator<<(std::ostream &ostr, const std::list<int> &list)
// {
//     for (auto &i : list)
//     {
//         ostr << " " << i;
//     }
//     return ostr;
// }
// class Ent;
// class Target
// {
// public:
//     weak_ptr<Ent> entry_ptr;
//     inline void close()
//     {
//         if (entry_ptr.lock())
//         {
//             cout << " lock valided" << endl;
//         }
//         else
//         {
//             cout << " closed as expected" << endl;
//         }
//     }
// };
// class Ent
// {
// public:
//     Target *m_tar;
//     Ent(Target *tar)
//     {
//         m_tar = tar;
//     }
//     ~Ent()
//     {
//         m_tar->close();
//     }
// };

// int main()
// {
//     Target tar;
//     shared_ptr<Ent> ptr = make_shared<Ent>(&tar);
//     move(ptr);
// };

while (x != T.root && x.color == RED)
{
    if（x.p == x.p.p.left）
    {
        // x的爸爸是爷爷的左孩子，categoryA

        y == x.p.p.right; // y是爷爷的右孩子，也就是x的叔叔
        if（y.color == red）
        {
            // case1的情况

            x.p.color = black;
            y.color = black;
            x.p.p.color = red;
            x = x.p.p;
        }
        else
        {
            // x的叔叔不是红色，分成case2和case3

            if (x == x.p.right)
            {
                // case2——冲突成z型

                x = x.p;
                LEFT - ROTATE(T, x); // 左旋转操作

                // 然后就变成case3
            }

            // case3 选择加变色
            x.p.color = black;
            x.p.p.color = red;
            RIGHT - ROTATE(T, x.p.p);
        }
    }
    else(x.p为右子树，也就是x的爸爸是爷爷的右孩子，和爸爸是左孩子的操作相反即可);
}
T.root.color = BLACK; // 如果一查入就是根节点，就直接到这里但根节点还是红色，所以要变成黑色。