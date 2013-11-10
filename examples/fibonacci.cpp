#include "cppmemo.hpp"

#include <iostream>

int fibonacci(int i, std::function<int(int)> prereqs) {
    if (i <= 2) return 1;
    return prereqs(i-1) + prereqs(i-2);
}

static const int ELEM_NO = 30;

int main(void) {

    // find prerequisites by dry-running of the compute function (fibonacci)
    cppmemo::CppMemo<int, int> cppMemo;
    const int result = cppMemo.getValue(ELEM_NO, fibonacci);

    std::cout << "Fibonacci #" << ELEM_NO << ": " << result << std::endl;

    return EXIT_SUCCESS;

}
