#include "cppmemo.hpp"

#include <iostream>

using namespace cppmemo;

int fibonacci(int i, CppMemo<int, int>::PrerequisitesProvider prereqs) {
    if (i == 0) return 0;
    if (i <= 2) return 1;
    return prereqs(i-1) + prereqs(i-2);
}

static const int ELEM_NO = 30;

int main(void) {

    // find prerequisites by dry-running of the compute function (fibonacci)
    CppMemo<int, int> cppMemo;
    const int result = cppMemo.getValue(ELEM_NO, fibonacci);

    std::cout << "Fibonacci #" << ELEM_NO << ": " << result << std::endl;

    return EXIT_SUCCESS;

}
