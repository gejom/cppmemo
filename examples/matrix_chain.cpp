#include "cppmemo.hpp"
#include "common.hpp"

#include <iostream>
#include <iomanip> // std::setw

using namespace cppmemo;

class Range : public std::pair<int, int> { 
public:
    int from() const {
        return first;
    }
    int to() const {
        return second;
    }
    Range(int from, int to) : std::pair<int, int>(from, to) {
    }
    Range() {
    }
};

struct Matrix {
    int p;
    int q;
};

struct Result {
    int lowestCost;
    int bestSplit;
};

typedef CppMemo<Range, Result, cppmemo::PairHash1<Range>, cppmemo::PairHash2<Range> > CppMemoType;

void declarePrerequisites(Range range, typename CppMemoType::PrerequisitesGatherer declare) {
    const int size = range.to() - range.from() + 1;
    for (int i = 0; i < size - 1; i++) {
        const int split = range.from() + i;
        declare({ range.from(), split });
        declare({ split + 1, range.to() });
    }
}

std::vector<Matrix> matrices;

Result calculate(Range range, typename CppMemoType::PrerequisitesProvider prereqs) {

    const int size = range.to() - range.from() + 1;

    if (size == 1) return { 0, range.from() };

    int lowestCost = std::numeric_limits<int>::max();
    int bestSplit = 0;

    for (int i = 0; i < size - 1; i++) {
        const int split = range.from() + i;
        const Range subrange1 { range.from(), split };
        const Range subrange2 { split + 1, range.to() };
        const Matrix& first = matrices[subrange1.from()];
        const Matrix& middle = matrices[subrange1.to()];
        const Matrix& last = matrices[subrange2.to()];
        const int cost = prereqs(subrange1).lowestCost + prereqs(subrange2).lowestCost +
                (first.p * middle.q * last.q);
        if (cost < lowestCost) {
            lowestCost = cost;
            bestSplit = split;
        }
    }

    return { lowestCost, bestSplit };

}

std::string parenthesize(const Range& range, const CppMemoType& cppMemo) {

    const int size = range.to() - range.from() + 1;

    if (size == 1) return "A" + std::to_string(range.from()) + " ";

    const int bestSplit = cppMemo(range).bestSplit;

    const Range left { range.from(), bestSplit };
    const Range right { bestSplit + 1, range.to() };

    return "( " + parenthesize(left, cppMemo) + parenthesize(right, cppMemo) + ") ";

}

static const int MATRIX_MIN_DIM = 3;
static const int MATRIX_MAX_DIM = 10;

int main(int argc, char** argv) {

    if (argc != 3) {
        std::cerr << "usage: matrix_chain NUMBER_OF_THREADS NUMBER_OF_MATRICES" << std::endl;
        return -1;
    }

    const int numThreads = std::stoi(argv[1]);
    const int numMatrices = std::stoi(argv[2]);

    const bool printAsRow = getenv("CPPMEMO_PRINT_AS_ROW") != nullptr;

    std::minstd_rand randGen;
    std::uniform_int_distribution<int> randNum(MATRIX_MIN_DIM, MATRIX_MAX_DIM);

    std::vector<int> p(numMatrices + 1);
    for (std::size_t i = 0; i < (std::size_t) numMatrices + 1; i++) {
        p[i] = randNum(randGen);
        if (i > 0) matrices.push_back({ p[i - 1], p[i] });
    }

    if (!printAsRow) {
        std::cout << "p : {";
        bool first = true;
        for (int i : p) {
            if (!first) std::cout << ", ";
            else std::cout << " ";
            first = false;
            std::cout << i;
        }
        std::cout << " }" << std::endl;
        std::cout << std::endl;
    }

    CppMemoType cppMemo(numThreads, numMatrices * numMatrices);

    const Range fullRange { 0, (int) matrices.size() - 1 };
    Result result;

    const Timestamp start = now();
    result = cppMemo.getValue(fullRange, calculate, declarePrerequisites);
    const Timestamp end = now();
    const double timeElapsed = elapsedSeconds(start, end);

    if (!printAsRow) {

        std::cout << "Best parenthesization: " << parenthesize(fullRange, cppMemo) << std::endl;
        std::cout << "Cost: " << result.lowestCost << std::endl;

        std::cout << std::endl;
        std::cout << "Elapsed time (sec.): " << timeElapsed << std::endl;

    } else {

        std::cout << std::left << std::fixed << std::setprecision(3)
                  << std::setw(21) << numMatrices
                  << std::setw(20) << numThreads
                  << std::setw(19) << timeElapsed
                  << std::endl;

    }

    return EXIT_SUCCESS;

}
