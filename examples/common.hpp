#ifndef CPPMEMO_EXAMPLES_COMMON_H_
#define CPPMEMO_EXAMPLES_COMMON_H_

typedef std::chrono::time_point<std::chrono::system_clock> Timestamp;

Timestamp now() {
    return std::chrono::system_clock::now();
}

double elapsedSeconds(Timestamp start, Timestamp end) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() / 1000.0;
}

#endif // CPPMEMO_EXAMPLES_COMMON_H_
