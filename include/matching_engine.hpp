#ifndef MATCHING_ENGINE_H
#define MATCHING_ENGINE_H

#include <cstdint>

#include "clob.hpp"
#include <string>
#include <unordered_map>
#include <vector>

using std::string, std::unordered_map, std::vector, TradeDS::Book;

class MatchingEngine {
private:
    unordered_map<string, Book *> books;  // all books for all symbols
    unordered_map<uint64_t, Book *> order_book_map;

public:
    MatchingEngine() = default;

    /**
     * Unused, importing existing TradeDS::Books and indexing all orders
     * @param books existing order books
     */
    [[maybe_unused]] explicit MatchingEngine(vector<Book *> books);

    ~MatchingEngine() = default;

    MatchingEngine(MatchingEngine const &rhs) = delete;

    MatchingEngine &operator=(MatchingEngine const &rhs) = delete;

    MatchingEngine(MatchingEngine const &&rhs) = delete;

    MatchingEngine &operator=(MatchingEngine const &&rhs) = delete;

    /**
     * get the reference of a TradeDS::Book with a given symbol
     * @param symbol
     * @return the reference of a TradeDS::Book with a given symbol
     */
    Book *get_book(const string &symbol);

    /**
     * Attempt to fill then add an new order into it's corresponding Book\n
     * \n
     * time-complexity depends on: \n
     * Book's orders distribution\n
     * TradeDS::Book.insert, TradeDS::Book.amend, TradeDS::Book.remove\n
     * The denser Book's possible Limits are, the similar offer's volumes are
     * The closer it runs on O(1)
     *
     * @param order_id
     * @param symbol
     * @param side
     * @param price
     * @param volume
     * @param fills an vector passed by reference, all filled order needs to be written in there
     * @return True on successful (partial) fill or insertion
     * @return False on invalid order_id; 0, existing id
     * @return False on bad symbol
     * @return False on negative price or volume
     */
    bool add_order(uint64_t order_id, string const &symbol, Side side,
                   int64_t price, int64_t volume, vector<Fill> &fills);

    /**
     * update an existing order, then only attempt to fill it if price changed\n
     * if price changed, or volume is increased, order loses it's priority position; it will be re-evaluated\n
     *
     * @param order_id
     * @param new_price
     * @param new_active_volume
     * @param fills
     * @return True on successful update
     * @return False if order_id doesn't exist
     * @return False on invalid price or volume (<=0)
     */
    bool amend_order(uint64_t order_id, int64_t new_price, int64_t new_active_volume,
                     vector<Fill> &fills);

    /**
     * remove an existing order given an order_id, Order will also be destructed\n
     * @param order_id
     * @return True on successful removal
     * @return False if order_id doesn't exist
     */
    bool pull_order(uint64_t order_id);

    /**
     * get the best offer information from both side, at a given symbol book\n
     * @param symbol
     * @return an BestBidOffer struct with best offer information, if offer doesn't exists, the field will be set to 0
     */
    BestBidOffer get_top_of_book(string const &symbol) const;
};

#endif  // MATCHING_ENGINE_H