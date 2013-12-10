#include "cppmemo.hpp"

#include <utility>
#include <iostream>

void declarePrerequisites(int i, std::function<void(int)> declare) {
    if (i != 0) {
        // circular dependency (intentionally added)
        if (i == 8) declare(24);
        else declare(i-1);
    }
}

int calculate(int i, std::function<int(int)> prereqs) {
    if (i == 0) return 0;
    else return 1 + prereqs(i-1);
}

int ELEM_NO = 200;

int main(void) {

    try {
        cppmemo::CppMemo<int, int> cppMemo(1, 0, true);
        cppMemo.getValue(ELEM_NO, calculate, declarePrerequisites);
    } catch (cppmemo::CircularDependencyException<int>& e) {
        std::cout << e.what() << std::endl;
        std::cout << "Offending key: " << e.key() << std::endl;
        std::cout << "TEST SUCCEEDED" << std::endl;
        return EXIT_SUCCESS;
    }

    std::cout << "You shouldn't read this message." << std::endl;

    return EXIT_FAILURE;

}
