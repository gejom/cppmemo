#include "cppmemo.hpp"

#include <utility>
#include <iostream>

using namespace cppmemo;

void declarePrerequisites(int i, typename CppMemo<int, int>::PrerequisitesDeclarer declare) {
    if (i != 0) {
        // circular dependency (intentionally added)
        if (i == 8) declare(13);
        else declare(i-1);
    }
}

int calculate(int i, typename CppMemo<int, int>::PrerequisitesProvider prereqs) {
    if (i == 0) return 0;
    else return 1 + prereqs(i-1);
}

int ELEM_NO = 20;

int main(void) {

    try {
        CppMemo<int, int> cppMemo(1, 0, true);
        cppMemo.getValue(ELEM_NO, calculate, declarePrerequisites);
    } catch (CircularDependencyException<int>& e) {
        std::cout << e.what() << std::endl;
        std::cout << "Keys stack:";
        for (auto it = e.getKeysStack().rbegin(); it != e.getKeysStack().rend(); ++it) {
            std::cout << " " << *it;
        }
        std::cout << std::endl;
        std::cout << "TEST SUCCEEDED" << std::endl;
        return EXIT_SUCCESS;
    }

    std::cout << "You shouldn't read this message." << std::endl;

    return EXIT_FAILURE;

}
