/*
 * Obviously, this is a crazy way to implement a simple fibonacci calculation!
 * This example is provided just to show how to implement a custom container for C++Memo.
 */

#include "cppmemo.hpp"

#include <iostream>
#include <vector>

using namespace cppmemo;

class CustomContainer {
    
private:
    
    std::vector<bool> memoized;
    std::vector<int> entries;
    
    void ensureStorage(int key) {
        if (key > (int) entries.size() - 1) {
            memoized.resize(key + 1, false);
            entries.resize(key + 1);
        }
    }
    
public:
    
    CustomContainer(std::size_t initialCapacity) : memoized(initialCapacity, false), entries(initialCapacity) {
    }
    
    bool isMemoized(int key) const {
        if (key > (int) memoized.size() - 1) return false;
        return memoized[key];
    }
    
    const int& retrieve(int key) const {
        return entries[key];
    }
    
    void memoize(int key, int value) {
        ensureStorage(key);
        entries[key] = value;
    }
    
    template<typename ComputeValueFunction>
    void memoize(int key, ComputeValueFunction computeValueFunction) {
        ensureStorage(key);
        entries[key] = computeValueFunction(key);
    }
    
};

typedef CppMemo<int, int, std::hash<int>, DefaultKeyHash2<int>, std::equal_to<int>, CustomContainer> CppMemoType;

int fibonacci(int i, typename CppMemoType::PrerequisitesProvider prereqs) {
    if (i == 0) return 0;
    if (i <= 2) return 1;
    return prereqs(i-1) + prereqs(i-2);
}

static const int ELEM_NO = 30;

int main(void) {

    // find prerequisites by dry-running of the compute function (fibonacci)
    CppMemoType cppMemo;
    const int result = cppMemo.getValue(ELEM_NO, fibonacci);

    std::cout << "Fibonacci #" << ELEM_NO << ": " << result << std::endl;

    return EXIT_SUCCESS;

}
