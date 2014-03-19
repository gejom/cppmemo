#include "cppmemo.hpp"
#include "common.hpp"

#include <iostream>
#include <iomanip> // std::setw

using namespace cppmemo;

static const std::vector<int> WEIGHTS = {0, // 0-th element is never used
        3851, 29521, 18550, 2453, 18807, 20622, 17505, 18855, 75601, 8657,
        9411, 15447, 20454, 96502, 56825, 15199, 25559, 56504, 95545, 8580,
        8441, 48557, 41552, 10441, 15485, 35246, 4561, 5451, 8759, 4771,
        5647, 1834, 5537, 15234, 19375, 74982, 3452, 3314, 35453, 15583,
        9853, 11252, 2123, 5324, 7572, 3142, 6733, 25051, 26523, 15642};

static const std::vector<int> VALUES = {0, // 0-th element is never used
        124, 32, 15, 23, 8, 12, 34, 11, 23, 4,
        41, 45, 87, 41, 52, 65, 71, 101, 25, 254,
        415, 24, 142, 98, 42, 46, 41, 99, 101, 52,
        372, 34, 23, 102, 324, 31, 87, 23, 12, 87,
        12, 54, 123, 45, 12, 78, 231, 32, 12, 99};

struct Key {
    int items;
    int weight;
    bool operator==(const Key& other) const {
        return other.items == items && other.weight == weight;
    }
};

struct KeyHash1 {
    std::size_t operator()(const Key& key) const {
        // FNV hash
        std::size_t hash = 2166136261;
        hash = (hash * 16777619) ^ key.items;
        hash = (hash * 16777619) ^ key.weight;
        return hash;
    }
};

struct KeyHash2 {
    std::size_t operator()(const Key& key) const {
        return key.items ^ key.weight;
    }
};

typedef CppMemo<Key, int, KeyHash1, KeyHash2> CppMemoType;

int knapsack(const Key& key, CppMemoType::PrerequisitesProvider prereqs) {
    if (key.items == 0) return 0;
    if (WEIGHTS[key.items] > key.weight) {
        return prereqs({ key.items - 1, key.weight });
    } else {
        int val1 = prereqs({ key.items - 1, key.weight });
        int val2 = prereqs({ key.items - 1, key.weight - WEIGHTS[key.items] }) + VALUES[key.items];
        return std::max(val1, val2);
    }
}

int main(int argc, char** argv) {

    if (argc != 3) {
        std::cerr << "usage: knapsack NUMBER_OF_THREADS KNAPSACK_CAPACITY" << std::endl;
        return -1;
    }

    const int numItems = WEIGHTS.size() - 1;
    const int numThreads = std::stoi(argv[1]);
    const int knapsackCapacity = std::stoi(argv[2]);

    const bool printAsRow = getenv("CPPMEMO_PRINT_AS_ROW") != nullptr;

    CppMemoType cppMemo(numThreads, numItems * knapsackCapacity);
    int maxValue;

    const Timestamp start = now();
    // find prerequisites by dry-running the compute function (knapsack)
    maxValue = cppMemo.getValue({ numItems, knapsackCapacity }, knapsack);
    const Timestamp end = now();
    const double timeElapsed = elapsedSeconds(start, end);

    std::vector<int> selectedItems;
    int currentWeight = knapsackCapacity;
    for (int itemNo = numItems; itemNo >= 1; itemNo--) {
        int val1 = cppMemo.getValue({ itemNo, currentWeight });
        int val2 = cppMemo.getValue({ itemNo - 1, currentWeight });
        if (val1 != val2) {
            selectedItems.push_back(itemNo);
            currentWeight -= WEIGHTS[itemNo];
        }
    }

    if (!printAsRow) {

        std::cout << "Max value: " << maxValue << std::endl;

        std::cout << "Selected items: ";
        for (int itemNo : selectedItems) {
            std::cout << itemNo << " ";
        }

        std::cout << std::endl;

        std::cout << std::endl;
        std::cout << "Elapsed time (sec.): " << timeElapsed << std::endl;

    } else {

        std::cout << std::left << std::fixed << std::setprecision(3)
                  << std::setw(18) << numItems
                  << std::setw(20) << knapsackCapacity
                  << std::setw(20) << numThreads
                  << std::setw(19) << timeElapsed
                  << std::endl;

    }

    return EXIT_SUCCESS;

}
