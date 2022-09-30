#ifndef SPARSE_SET_H
#define SPARSE_SET_H

#include <cstdint>

#include <cmath>
#include <string>
#include <vector>
#include <iostream>
#include <cstring>


using std::string, std::vector, std::cout, std::endl;

namespace TradeDS {
/**
 * A SparseSet can hold arbitrary value with self-increasing size\n
 * Internal storage is organised in term of Pages for improved space-complexity\n
 * Page is a static array with customizable size that must be power of 2\n
 * \n
 * Time-Complexity\n
 * - retrieve O(1)\n
 * - write O(1), worst case O(n) if page pool needs to be resized\n
 * - remove O(1)\n
 *
 * @tparam T Any type of element to be stored
 */
    template<typename T>
    class SparseSet {
    private:
        /**
         * SparseSet internal storage unit\n
         * @tparam T Any type of element to be stored
         */
        struct Page {
            T *container;

            explicit Page(size_t size) {
                container = new T[size];
                memset(container, 0, size);
            };

            ~Page() { delete container; };
        };

        const uint64_t PAGE_IDX_SHIFTER;
        // const uint64_t PAGE_IDX_MASK;
        const size_t PAGE_SIZE;
        vector<Page *> pages;

    public:
        /**
         * construct a empty SparseSet\n
         * @param size the size of initial SparseSet
         * @param page_size optional, 4096 by default, must be power of 2
         */
        explicit SparseSet(size_t size, size_t page_size = 4096);

        /**
         * destruct SparseSet and free all Page\n
         * reference type of elements in storage will NOT be freed
         */
        ~SparseSet();

        /**
         * Insert an element at a given index in SparseSet\n
         * If necessary, SparseSet will resize and create new corresponding page\n
         * @param index target position in SparseSet
         * @param element element to be inserted
         * @return
         */
        T insert(uint64_t index, T element);

        /**
         * Remove an element at a given index and free inactive Page if necessary
         * \todo Implement element removing and inactive Page freeing behaviour
         * @param index position to remove element at
         * @return element removed
         */
        __attribute__((unused)) T remove(uint64_t index);

        /**
         * return the size of SparseSet (Not the count of element inserted)\n
         * @return the size of SparseSet
         */
        size_t size() { return this->PAGE_SIZE * this->pages.size(); }

        /**
         * access element at a given index\n
         * Return 0-Equivalent value on out-of-bound access
         * @param index position of desired element
         * @return element at given index
         * @return 0-Equivalent value on on uninitialised value
         * @return 0-Equivalent value on out-of-bound access
         */
        T operator[](uint64_t);

        // tester
        __attribute__((unused)) vector<Page *> getPages() { return this->pages; }
    };


    template<typename T>
    SparseSet<T>::SparseSet(size_t size, size_t page_size)
            : PAGE_SIZE{page_size}, PAGE_IDX_SHIFTER{static_cast<uint64_t>(log2(page_size))} {
        // create at least 1 SparseSet Page
        const uint64_t page_count = (size / page_size) + 1;
        for (size_t i = 0; i < page_count; i++) {
            this->pages.push_back(new Page(page_size));
        }
    }

    template<typename T>
    SparseSet<T>::~SparseSet() {
        // remove all existing pages
        for (auto page: pages) delete page;
    }

    template<typename T>
    T SparseSet<T>::insert(uint64_t index, T element) {
        // essentially (index / page_size) but faster
        const size_t PAGE_IDX = index >> this->PAGE_IDX_SHIFTER;
        const size_t INPAGE_IDX = index % this->PAGE_SIZE;

        //const size_t INPAGE_IDX = index & this->PAGE_IDX_MASK;    // todo: using bit-mask

        // expand page space if needed
        if (PAGE_IDX >= this->pages.size()) {
            this->pages.resize(PAGE_IDX * 2);
            cout << "Resized SparseSet Pages to " << this->pages.size() << endl;
        }

        // create page if needed
        if (this->pages[PAGE_IDX] == nullptr) {
            cout << "Creating New Page\n";
            this->pages[PAGE_IDX] = new Page(this->PAGE_SIZE);
        }

        // insert
        this->pages[PAGE_IDX]->container[INPAGE_IDX] = element;

        return element;
    }

    template<typename T>
    T SparseSet<T>::operator[](uint64_t index) {
        // essentially (index / page_size) but faster
        const size_t PAGE_IDX = index >> this->PAGE_IDX_SHIFTER;
        const size_t INPAGE_IDX = index % this->PAGE_SIZE;

        //const size_t INPAGE_IDX = index & this->PAGE_IDX_MASK;    // todo: using bit-mask

        if (PAGE_IDX >= this->pages.size()) return 0;
        if (this->pages[PAGE_IDX] == nullptr) return 0;

        return this->pages[PAGE_IDX]->container[INPAGE_IDX];
    }

    template<typename T>
    __attribute__((unused)) T SparseSet<T>::remove(uint64_t index) {
        return nullptr;
    }
}

#endif  // !SPARSE_SET_H
