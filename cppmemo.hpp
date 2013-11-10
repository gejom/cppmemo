/**
 * @file
 * @author  Giacomo Drago <giacomo@giacomodrago.com>
 * @version 0.9 beta
 *
 *
 * @section LICENSE
 *
 * Copyright (c) 2013, Giacomo Drago <giacomo@giacomodrago.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes C++Memo, a software developed by Giacomo Drago.
 *      Website: http://projects.giacomodrago.com/c++memo
 * 4. Neither the name of Giacomo Drago nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GIACOMO DRAGO "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL GIACOMO DRAGO BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @section DESCRIPTION
 *
 * This header file contains a generic framework for memoization (see
 * @link http://en.wikipedia.org/wiki/Memoization @endlink) supporting parallel
 * execution.
 *
 * A C++11-compliant compiler is required to compile this file.
 *
 * Please read the documentation (including important caveats) on the project website:
 * http://projects.giacomodrago.com/c++memo
 *
 * This product includes Fcmm, a software developed by Giacomo Drago.
 * Website: http://projects.giacomodrago.com/fcmm
 *
 */

#ifndef CPPMEMO_H_
#define CPPMEMO_H_

#include <vector> // std::vector
#include <random> // std::minstd_rand
#include <thread> // std::thread
#include <functional> // std::function
#include <algorithm> // std::shuffle

#ifdef CPPMEMO_DETECT_CIRCULAR_DEPENDENCIES
#include <unordered_set>
#include <stdexcept>
#endif

#include <fcmm/fcmm.hpp>

namespace cppmemo {

template<typename Key>
struct GenericHash1 {
    std::size_t operator()(const Key& key) const {
        const std::size_t prime = 2147483647;
        std::size_t hash = 2166136261;
        const char* bytes = (const char*) &key;
        for (std::size_t i = 0; i < sizeof(Key); i++) {
            hash = (hash * prime) ^ bytes[i];
        }
        return hash;
    }
};

template<typename Key>
struct GenericHash2 {
    std::size_t operator()(const Key& key) const {
        std::size_t hash = 0;
        const char* bytes = (const char*) &key;
        for (std::size_t i = 0; i < sizeof(Key); i++) {
            hash = bytes[i] + (hash << 6) + (hash << 16) - hash;
        }
        return hash;
    }
};

template<typename Key>
struct GenericEqual {
    bool operator()(const Key& key1, const Key& key2) const {
        const char* bytes1 = (const char*) &key1;
        const char* bytes2 = (const char*) &key2;
        for (std::size_t i = 0; i < sizeof(Key); i++) {
            if (bytes1[i] != bytes2[i]) return false;
        }
        return true;
    }
};

/**
 * This class implements a generic framework for memoization supporting
 * parallel execution.
 *
 * IMPORTANT WARNING: default template parameters are NOT always appropriate.
 * Improper use of the defaults for KeyHash1, KeyHash2 and KeyEqual will cause
 * unpredictable results.
 *
 * Before using this class, please read the complete documentation
 * (including important caveats) on the project website:
 * http://projects.giacomodrago.com/c++memo
 */
template<
    typename Key,
    typename Value,
    typename KeyHash1 = GenericHash1<Key>,
    typename KeyHash2 = GenericHash2<Key>,
    typename KeyEqual = GenericEqual<Key>
>
class CppMemo {

public:

    typedef std::function<void(const Key&, std::function<void(const Key&)>)> DeclarePrerequisites;
    typedef std::function<Value(const Key&, std::function<const Value&(const Key&)>)> Compute;

private:

    int defaultNumThreads;
    Fcmm<Key, Value, KeyHash1, KeyHash2, KeyEqual> values;

    void runThread(int threadNo,
                   const Key& key,
                   Compute compute,
                   DeclarePrerequisites declarePrerequisites) {

        struct StackItem {
            Key key;
            bool ready;
        };

        std::minstd_rand randGen(threadNo);
        std::vector<StackItem> stack;

#ifdef CPPMEMO_DETECT_CIRCULAR_DEPENDENCIES
        std::unordered_set<Key, KeyHash1, KeyEqual> stackItems;
        const auto push = [&](const Key& key) -> bool {
            if (stackItems.find(key) != stackItems.end()) {
                throw std::logic_error("Circular dependency detected.");
            }
            stack.push_back({ key, false });
            return true;
        };
        const auto pop = [&] {
            const StackItem& item = stack.back();
            stackItems.erase(item.key);
            stack.pop_back();
        };
#else
        const auto push = [&](const Key& key) -> bool {
            if (values.find(key) == values.end()) {
                stack.push_back({ key, false });
                return true;
            }
            return false;
        };
        const auto pop = [&] {
            stack.pop_back();
        };
#endif

        push(key);

        while (!stack.empty()) {

            StackItem& item = stack.back();
            const Key itemKey = item.key;
            const bool itemReady = item.ready;

            if (itemReady) {
                pop();
            } else {
                item.ready = true;
            }

            if (itemReady) {

                const auto prereqs = [&](const Key& key) -> const Value& {
                    return values[key];
                };

                values.insert(itemKey, [&](const Key& key) -> Value {
                    return compute(key, prereqs);
                });

            } else if (values.find(itemKey) == values.end()) {

                int noPrerequisites = 0;

                if (declarePrerequisites != nullptr) { // execute the declarePrerequisites function
                                                       // to get prerequisites

                    declarePrerequisites(itemKey, [&](const Key& prerequisite) {
                        if (push(prerequisite)) noPrerequisites++;
                    });

                } else { // dry-run the compute function to capture prerequisites

                    const Value dummy = Value();
                    compute(itemKey, [&](const Key& prerequisite) -> const Value& {
                        if (push(prerequisite)) noPrerequisites++;
                        return dummy;
                    });

                }

                if (threadNo > 0 && noPrerequisites > 1) {
                    // shuffle the added prerequisites for improving parallel speedup
                    std::shuffle(stack.end() - noPrerequisites, stack.end(), randGen);
                }

#ifdef CPPMEMO_DETECT_CIRCULAR_DEPENDENCIES
                for (int i = 1; i <= noPrerequisites; i++) {
                    const StackItem& item = *(stack.end() - i);
                    stackItems.insert(item.key);
                }
#endif

            }

        }

    }

public:

    CppMemo(int defaultNumThreads = 1, std::size_t estimatedNumEntries = 0) : values(estimatedNumEntries) {
        setDefaultNumThreads(defaultNumThreads);
    }

    int getDefaultNumThreads() const {
        return defaultNumThreads;
    }

    void setDefaultNumThreads(int defaultNumThreads) {
        if (defaultNumThreads < 1) {
            throw std::logic_error("The default number of threads must be >= 1");
        }
        this->defaultNumThreads = defaultNumThreads;
    }

    Value getValue(const Key& key,
                   Compute compute,
                   DeclarePrerequisites declarePrerequisites,
                   int numThreads) {

        const auto it = values.find(key);
        if (it != values.end()) {
            return it->second;
        }

        if (compute == nullptr) {
            throw std::logic_error("The compute function must be specified when the value is not yet computed");
        }

        if (numThreads > 1) { // multi-thread execution

            std::vector<std::thread> threads;
            threads.reserve(numThreads);

            for (int threadNo = 0; threadNo < numThreads; threadNo++) {

                std::thread thread(&CppMemo<Key, Value, KeyHash1, KeyHash2, KeyEqual>::runThread,
                                   this, threadNo, std::ref(key), compute, declarePrerequisites);

                threads.push_back(std::move(thread));

            }

            for (std::thread& thread : threads) {
                thread.join();
            }

        } else { // single thread execution

            runThread(0, key, compute, declarePrerequisites);

        }

        return values[key];

    }

    Value getValue(const Key& key,
                   Compute compute = nullptr,
                   DeclarePrerequisites declarePrerequisites = nullptr) {
        return getValue(key, compute, declarePrerequisites, getDefaultNumThreads());
    }

    Value operator()(const Key& key,
                     Compute compute,
                     DeclarePrerequisites declarePrerequisites,
                     int numThreads) {
        return getValue(key, compute, declarePrerequisites, numThreads);
    }

    Value operator()(const Key& key,
                     Compute compute = nullptr,
                     DeclarePrerequisites declarePrerequisites = nullptr) {
        return getValue(key, compute, declarePrerequisites);
    }

    // the class shall not be copied or moved (because of Fcmm)
    CppMemo(const CppMemo&) = delete;
    CppMemo(CppMemo&&) = delete;

};

} // namespace cppmemo

#endif // CPPMEMO_H_
