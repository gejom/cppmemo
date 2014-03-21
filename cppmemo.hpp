/**
 * @file
 * @author  Giacomo Drago <giacomo@giacomodrago.com>
 * @version 1.0-RC
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
 * http://en.wikipedia.org/wiki/Memoization) supporting parallel
 * execution.
 *
 * A C++11-compliant compiler is required to compile this file.
 *
 * Please read the documentation on the project website:
 * http://projects.giacomodrago.com/c++memo
 *
 * This product includes Fcmm, a software developed by Giacomo Drago.
 * Website: http://projects.giacomodrago.com/fcmm
 *
 */

#ifndef CPPMEMO_H_
#define CPPMEMO_H_

// The noexcept specifier is unsupported in Visual Studio
#ifndef _MSC_VER
#define CPPMEMO_NOEXCEPT noexcept
#else
#define CPPMEMO_NOEXCEPT
#endif

#include <vector> // std::vector
#include <unordered_set> // std::unordered_set
#include <random> // std::minstd_rand
#include <thread> // std::thread
#include <algorithm> // std::shuffle
#include <cstddef> // std::nullptr_t
#include <stdexcept> // std::logic_error, std::runtime_error

#include <fcmm/fcmm.hpp>

namespace cppmemo {

/**
 * @brief This exception is thrown when a circular dependency among the keys is detected.
 *
 * This requires <i>circular dependency detection</i> to be enabled (via the
 * `detectCircularDependencies` argument of @link CppMemo @endlink constructor).
 *
 * @tparam Key the type of the keys
 */
template<typename Key>
class CircularDependencyException : public std::exception {
    
    template<typename K, typename V, typename KH1, typename KH2, typename KE>
    friend class CppMemo;
    
private:
    
    std::vector<Key> keysStack;
    
    CircularDependencyException(const std::vector<Key>& keysStack) : keysStack(keysStack) {
    }
    
public:
    
    /**
     * @brief Returns a C string identifying the exception.
     */
    const char* what() const CPPMEMO_NOEXCEPT {
        return "Circular dependency detected.";
    }
    
    /**
     * @brief Returns the keys stack at the time the circular dependency was detected.
     *
     * @return The keys stack (from bottom to top) as a vector
     */
    const std::vector<Key>& getKeysStack() const {
        return keysStack;
    }
    
};

/**
 * @brief This class implements a generic framework for memoization supporting
 * automatic parallel execution.
 * 
 * Please read the documentation on the project website: http://projects.giacomodrago.com/c++memo
 *
 * @tparam Key       the type of the key (default-constructible and copy-constructible)
 * @tparam Value     the type of the value (default-constructible and copy-constructible)
 * @tparam KeyHash1  the type of a function object that calculates the hash of the key;
 *                   it should have the same interface as
 *                   <a href="http://en.cppreference.com/w/cpp/utility/hash">std::hash<T></a>
 * @tparam KeyHash2  the type of another function object that calculates the hash of the key:
 *                   it should be <i>completely independent from KeyHash1</i>;
 *                   the default for this template parameter is only available for
 *                   <a href="http://en.cppreference.com/w/cpp/types/is_integral">integral types</a>.
 * @tparam KeyEqual  the type of the function object that checks the equality of the two keys;
 *                   it should have the same interface as
 *                   <a href="http://en.cppreference.com/w/cpp/utility/functional/equal_to">std::equal_to<T></a>
 */
template<
    typename Key,
    typename Value,
    typename KeyHash1 = std::hash<Key>,
    typename KeyHash2 = fcmm::DefaultKeyHash2<Key>,
    typename KeyEqual = std::equal_to<Key>
>
class CppMemo {
    
private:
    
    typedef fcmm::Fcmm<Key, Value, KeyHash1, KeyHash2, KeyEqual> Values;
    
    int defaultNumThreads;
    Values values;
    bool detectCircularDependencies;
    
    class ThreadItemsStack {
        
    public:
        
        struct Item {
            Key key;
            bool ready;
        };
        
    private:
        
        int threadNo;
        std::minstd_rand randGen;
        std::size_t groupSize;
        bool detectCircularDependencies;

        std::vector<Item> items;        
        std::unordered_set<Key, KeyHash1, KeyEqual> itemsSet;
        
        std::vector<Key> getKeysStack() const {
            std::vector<Key> result;
            result.reserve(items.size());
            for (const Item& item : items) {
                result.push_back(item.key);
            }
            return result;
        }
        
    public:
        
        ThreadItemsStack(int threadNo, bool detectCircularDependencies) :
                threadNo(threadNo), randGen(threadNo), groupSize(0),
                detectCircularDependencies(detectCircularDependencies) {
        }
        
        void push(const Key& key) {
            items.push_back({ key, false });
            if (detectCircularDependencies) {
                if (itemsSet.find(key) != itemsSet.end()) {
                    throw CircularDependencyException<Key>(getKeysStack());
                }
            }
            groupSize++;
        }

        void pop() {
            if (groupSize != 0) {
                throw std::runtime_error("The current group is not finalized.");
            }
            if (detectCircularDependencies) {
                const Item& item = items.back();
                itemsSet.erase(item.key);
            }
            items.pop_back();
        }

        void finalizeGroup() {
            if (threadNo != 0 && groupSize > 1) {
                if (threadNo == 1) {
                    // reverse the added prerequisites of improving parallel speedup
                    std::reverse(items.end() - groupSize, items.end());
                } else {
                    // shuffle the added prerequisites of improving parallel speedup
                    std::shuffle(items.end() - groupSize, items.end(), randGen);                    
                }
            }
            if (detectCircularDependencies) {
                for (std::size_t i = 1; i <= groupSize; i++) {
                    const Item& addedItem = *(items.end() - i);
                    itemsSet.insert(addedItem.key);
                }
            }
            groupSize = 0;
        }
        
        bool empty() const {
            return items.empty();
        }
        
        const Item& back() const {
            return items.back();
        }
        
        Item& back() {
            return items.back();
        }
        
        std::size_t getGroupSize() const {
            return groupSize;
        }

    };
    
public:
    
    /**
     * @brief The function object providing prerequisites to the `Compute` function passed to
     * an appropriate `CppMemo::getValue()` overload.
     */
    class PrerequisitesProvider {
        
        friend class CppMemo<Key, Value, KeyHash1, KeyHash2, KeyEqual>;
        
    private:

        enum Mode { NORMAL, DRY_RUN };

        const Values& values;
        ThreadItemsStack& stack;
        Mode mode;
        Value dummyValue;

        PrerequisitesProvider(const Values& values, ThreadItemsStack& stack) :
                values(values), stack(stack), mode(NORMAL), dummyValue() {
        }

        void setMode(Mode mode) {
            this->mode = mode;
        }

    public:

        /**
         * @brief Provides the value corresponding to the given key.
         *
         * <span style="font-weight: bold; color: red">Important note</span>.
         * If a `CppMemo::getValue()` overload was called that does not accept a
         * `DeclarePrerequisites` function, then this method may return an invalid, default-constructed value,
         * and "track" the request as an indirect means to gather prerequisites of a given key
         * (via a dry run of the `Compute` funtion).
         *
         * @see `CppMemo::getValue()`
         *
         * @param  key the requested key
         *
         * @return the value corresponding to the requested key, or an invalid, default-constructed value
         *         (if dry running the `Compute` function)
         */
        const Value& operator()(const Key& key) {
            if (mode == NORMAL) {
                return values[key];
            } else { // dry running
                const auto findIt = values.find(key);
                if (findIt == values.end()) {
                    stack.push(key);
                    return dummyValue; // return an invalid value
                } else {
                    return findIt->second; // return a valid value
                }
            }
        }

    };

    /**
     * @brief The function object gathering prerequisites from the `DeclarePrerequisites` function
     * passed to an appropriate `CppMemo::getValue()` overload.
     */
    class PrerequisitesGatherer {

        friend class CppMemo<Key, Value, KeyHash1, KeyHash2, KeyEqual>;

    private:

        const Values& values;
        ThreadItemsStack& stack;

        PrerequisitesGatherer(const Values& values, ThreadItemsStack& stack) : values(values), stack(stack) {
        }

    public:

        /**
         * @brief Gathers a prerequisite.
         *
         * @param key a prerequisite key
         */
        void operator()(const Key& key) {
            if (values.find(key) == values.end()) {
                stack.push(key);
            }
        }

    };

private:

    template<typename Compute, typename DeclarePrerequisites>
    void run(int threadNo, const Key& key, Compute compute, DeclarePrerequisites declarePrerequisites,
             bool providedDeclarePrerequisites) {

        ThreadItemsStack stack(threadNo, detectCircularDependencies);

        stack.push(key);
        stack.finalizeGroup();

        PrerequisitesProvider prerequisitesProvider(values, stack);
        PrerequisitesGatherer prerequisitesDeclarer(values, stack);

        while (!stack.empty()) {

            typename ThreadItemsStack::Item& item = stack.back();

            if (item.ready) {

                prerequisitesProvider.setMode(PrerequisitesProvider::NORMAL);
                values.insert(item.key, [&](const Key& key) -> Value {
                    return compute(key, prerequisitesProvider);
                });

                stack.pop();

            } else {

                item.ready = true;

                const Key itemKey = item.key; // copy item key

                if (values.find(itemKey) == values.end()) {

                    if (providedDeclarePrerequisites) {

                        // execute the declarePrerequisites function to get prerequisites

                        declarePrerequisites(itemKey, prerequisitesDeclarer);

                    } else {

                        // dry-run the compute function to capture prerequisites

                        prerequisitesProvider.setMode(PrerequisitesProvider::DRY_RUN);
                        const Value itemValue = compute(itemKey, prerequisitesProvider);

                        if (stack.getGroupSize() == 0) { // the computed value is valid
                            values.emplace(itemKey, itemValue);
                            stack.pop();
                        }

                    }

                    stack.finalizeGroup();

                }

            }

        }

    }

    // This method serves as a workaround for a Visual Studio bug
    template<typename Compute, typename DeclarePrerequisites>
    static void runWrapper(CppMemo<Key, Value, KeyHash1, KeyHash2, KeyEqual>& instance, int threadNo, const Key& key, Compute compute,
                    DeclarePrerequisites declarePrerequisites, bool providedDeclarePrerequisites) {
        instance.run(threadNo, key, compute, declarePrerequisites, providedDeclarePrerequisites);
    }

    template<typename Compute, typename DeclarePrerequisites>
    const Value& getValue(const Key& key, Compute compute, DeclarePrerequisites declarePrerequisites, int numThreads,
                          bool providedDeclarePrerequisites) {

        const auto findIt = values.find(key);
        if (findIt != values.end()) {
            return findIt->second;
        }

        if (numThreads > 1) { // multi-thread execution

            std::vector<std::thread> threads;
            threads.reserve(numThreads);

            for (int threadNo = 0; threadNo < numThreads; threadNo++) {
                
                std::thread thread(&CppMemo<Key, Value, KeyHash1, KeyHash2, KeyEqual>::runWrapper<Compute, DeclarePrerequisites>,
                        std::ref(*this), threadNo, std::cref(key), compute, declarePrerequisites, providedDeclarePrerequisites);

                threads.push_back(std::move(thread));
                
            }

            for (std::thread& thread : threads) {
                thread.join();
            }

        } else { // single thread execution

            run(0, key, compute, declarePrerequisites, providedDeclarePrerequisites);

        }

        return values[key];

    }

public:

    /**
     * @brief Constructor.
     *
     * @param defaultNumThreads           the default number of threads to be started
     * @param estimatedNumEntries         an estimate for the number of memoized entries that will be stored in
     *                                    this class instance
     * @param detectCircularDependencies  enable circular dependency detection
     */
    CppMemo(int defaultNumThreads = 1, std::size_t estimatedNumEntries = 0, bool detectCircularDependencies = false) :
            values(estimatedNumEntries),
            detectCircularDependencies(detectCircularDependencies) {
        setDefaultNumThreads(defaultNumThreads);
    }

    /**
     * @brief Returns the default number of threads to be started.
     */
    int getDefaultNumThreads() const {
        return defaultNumThreads;
    }

    /**
     * @brief Sets the default number of threads to be started.
     */
    void setDefaultNumThreads(int defaultNumThreads) {
        if (defaultNumThreads < 1) {
            throw std::logic_error("The default number of threads must be >= 1");
        }
        this->defaultNumThreads = defaultNumThreads;
    }

    /**
     * @brief Returns `true` if circular dependency detection is enabled, `false` otherwise.
     */
    bool getDetectCircularDependencies() const {
        return detectCircularDependencies;
    }

    /**
     * @brief Enables or disables circular dependency detection.
     *
     * @param detectCircularDependencies `true` to enable circular dependency detection, `false` to disable it
     */
    void setDetectCircularDependencies(bool detectCircularDependencies) {
        this->detectCircularDependencies = detectCircularDependencies;
    }

    /**
     * @brief Returns the value corresponding to the requested key, computing (and memoizing) it as needed.
     *
     * @param key                    the requested key
     * @param compute                a function or functor used to compute the value corresponding to a given key
     * @param declarePrerequisites   a function or functor used to gather the prerequisites of a given key
     * @param numThreads             the number of threads to be started
     *
     * @tparam Compute               function or functor implementing `Value operator()(const Key&, CppMemo<Key, Value, KeyHash1, KeyHash2, KeyEqual>::PrerequisitesProvider)`
     * @tparam DeclarePrerequisites  function or functor implementing `void operator()(const Key&, CppMemo<Key, Value, KeyHash1, KeyHash2, KeyEqual>::PrerequisitesGatherer)`
     *
     * @return the value corresponding to the requested key
     */
    template<typename Compute, typename DeclarePrerequisites>
    const Value& getValue(const Key& key, Compute compute, DeclarePrerequisites declarePrerequisites, int numThreads) {
        return getValue(key, compute, declarePrerequisites, numThreads, true);
    }

    /**
     * @brief Returns the value corresponding to the requested key, computing (and memoizing) it as needed.
     *
     * The default number of threads will be started (see setDefaultNumThreads()).
     *
     * @param key                    the requested key
     * @param compute                a function or functor used to compute the value corresponding to a given key
     * @param declarePrerequisites   a function or functor used to gather the prerequisites of a given key
     *
     * @tparam Compute               function or functor implementing `Value operator()(const Key&, CppMemo<Key, Value, KeyHash1, KeyHash2, KeyEqual>::PrerequisitesProvider)`
     * @tparam DeclarePrerequisites  function or functor implementing `void operator()(const Key&, CppMemo<Key, Value, KeyHash1, KeyHash2, KeyEqual>::PrerequisitesGatherer)`
     *
     * @return the value corresponding to the requested key
     */
    template<typename Compute, typename DeclarePrerequisites>
    const Value& getValue(const Key& key, Compute compute, DeclarePrerequisites declarePrerequisites) {
        return getValue(key, compute, declarePrerequisites, defaultNumThreads, true);
    }

    /**
     * @brief Returns the value corresponding to the requested key, computing (and memoizing) it as needed.
     *
     * <span style="font-weight: bold; color: red">Important note</span>.
     * This overload omits the `DeclarePrerequisites` parameter: the prerequisites of a given key
     * are gathered indirectly by dry running the `Compute` function.
     * Please read the relevant documentation on the project website.
     *
     * @param key                    the requested key
     * @param compute                a function or functor used to compute the value corresponding to a given key
     * @param numThreads             the number of threads to be started
     *
     * @tparam Compute               function or functor implementing `Value operator()(const Key&, CppMemo<Key, Value, KeyHash1, KeyHash2, KeyEqual>::PrerequisitesProvider)`
     *
     * @return the value corresponding to the requested key
     */
    template<typename Compute>
    const Value& getValue(const Key& key, Compute compute, int numThreads) {
        const auto dummyDeclarePrerequisites = [](const Key&, PrerequisitesGatherer&) {};
        return getValue(key, compute, dummyDeclarePrerequisites, numThreads, false);
    }

    /**
     * @brief Returns the value corresponding to the requested key, computing (and memoizing) it as needed.
     *
     * The default number of threads will be started (see setDefaultNumThreads()).
     *
     * <span style="font-weight: bold; color: red">Important note</span>.
     * This overload omits the `DeclarePrerequisites` parameter: the prerequisites of a given key
     * are gathered indirectly by dry running the `Compute` function.
     * Please read the relevant documentation on the project website.
     *
     * @param key                    the requested key
     * @param compute                a function or functor used to compute the value corresponding to a given key
     *
     * @tparam Compute               function or functor implementing `Value operator()(const Key&, CppMemo<Key, Value, KeyHash1, KeyHash2, KeyEqual>::PrerequisitesProvider)`
     *
     * @return the value corresponding to the requested key
     */
    template<typename Compute>
    const Value& getValue(const Key& key, Compute compute) {
        return getValue(key, compute, defaultNumThreads);
    }

    /**
     * @brief Returns the memoized value corresponding to the requested key. If no value is memoized,
     * a `std::logic_error` exception is thrown.
     *
     * @param key the requested key
     *
     * @return the memoized value corresponding to the requested key
     *
     * @throw std::logic_error thrown if no value for the requested key is memoized
     */
    const Value& getValue(const Key& key) const {
        const auto findIt = values.find(key);
        if (findIt != values.end()) {
            return findIt->second;
        } else {
            throw std::logic_error("The value is not memoized");
        }
    }

    /**
     * @brief Alias for getValue(const Key&, Compute, DeclarePrerequisites, int)
     */
    template<typename Compute, typename DeclarePrerequisites>
    const Value& operator()(const Key& key, Compute compute, DeclarePrerequisites declarePrerequisites, int numThreads) {
        return getValue(key, compute, declarePrerequisites, numThreads);
    }

    /**
     * @brief Alias for getValue(const Key&, Compute, DeclarePrerequisites)
     */
    template<typename Compute, typename DeclarePrerequisites>
    const Value& operator()(const Key& key, Compute compute, DeclarePrerequisites declarePrerequisites) {
        return getValue(key, compute, declarePrerequisites);
    }

    /**
     * @brief Alias for getValue(const Key&, Compute, int)
     */
    template<typename Compute>
    const Value& operator()(const Key& key, Compute compute, int numThreads) {
        return getValue(key, compute, numThreads);
    }

    /**
     * @brief Alias for getValue(const Key&, Compute)
     */
    template<typename Compute>
    const Value& operator()(const Key& key, Compute compute) {
        return getValue(key, compute);
    }

    /**
     * @brief Alias for getValue(const Key&)
     */
    const Value& operator()(const Key& key) const {
        return getValue(key);
    }

    // the class shall not be copied or moved (because of fcmm)
    CppMemo(const CppMemo&) = delete;
    CppMemo(CppMemo&&) = delete;

};

} // namespace cppmemo

#undef CPPMEMO_NOEXCEPT

#endif // CPPMEMO_H_
