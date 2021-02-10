// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <iostream>


struct test_struct {
    int x;
    int y;
};

test_struct& pass(test_struct& ts) {
    return ts;
}

int main() {
    std::cout << "there" << std::endl;
    test_struct ts{1, 2};
    std::cout << "before" << std::endl;
    std::cout << ts.x << " " << ts.y << std::endl;

    test_struct& ts_ref = pass(ts);
    ts_ref.x = 99;
    std::cout << ts.x << " " << ts.y << std::endl;
    std::cout << ts_ref.x << " " << ts_ref.y << std::endl;

    return 0;
}

