#include "matching_engine.hpp"

using TradeDS::Order;

[[maybe_unused]] MatchingEngine::MatchingEngine(const vector<Book *> books) {
    // initialisation indexing
    for (auto const book: books) {
        // symbol - Book
        this->books[book->symbol] = book;

        // order_id - Book
        auto orders = book->get_orders();
        for (auto it: orders) {
            this->order_book_map[it.second->order_id] = book;
        }
    }
}


bool MatchingEngine::add_order(uint64_t order_id, const string &symbol, Side side, int64_t price, int64_t volume,
                               vector<Fill> &fills) {
    if (order_id == 0) return false;
    if (this->order_book_map.find(order_id) != this->order_book_map.end()) return false; // order exists
    if (symbol.empty()) return false;
    if (price <= 0) return false;
    if (volume <= 0) return false;

    if (this->books.find(symbol) == this->books.end()) {
        /**
         * book doesn't exist: creat book and insert order right away
         */
        auto new_book = new Book(symbol, 1);    // unit is hard-coded here for now
        this->books[symbol] = new_book;
        new_book->insert(new Order(order_id, side, (double) price, volume));
        this->order_book_map[order_id] = new_book;
        return true;
    } else {
        /**
        * Book exists, attempt to exhaust the new order volume and insert what's left
        */
        Book *target_book = this->books[symbol];
        while (volume > 0) {
            const Order best_match_copy = target_book->get_order_by_id(target_book->get_best_offer_id(side));
            if (best_match_copy.order_id == 0) break;   // no counter orders exists

            bool buySatisfy = (side == Side::Buy) && (best_match_copy.price <= (double) price);
            bool sellSatisfy = (side == Side::Sell) && (best_match_copy.price >= (double) price);

            if (buySatisfy || sellSatisfy) {
                const uint64_t best_id = best_match_copy.order_id;
                const auto best_price = (uint64_t) best_match_copy.price;
                uint64_t best_volume = best_match_copy.volume;

                if (best_match_copy.volume > volume) {
                    // fulfill with single counter order
                    target_book->amend(best_id, (double) best_price, best_volume - volume);
                    fills.push_back(Fill{best_id, static_cast<int64_t>(best_price), volume});
                    volume = 0;
                } else {
                    // multiple orders needed to fulfill
                    volume -= (int64_t) best_volume;
                    target_book->remove(best_id);
                    fills.push_back(Fill{best_id, static_cast<int64_t>(best_price), static_cast<int64_t>(best_volume)});
                }
            } else break;
        }

        if (volume > 0) {
            // best offer can not satisfy, insert new_order into book and quit
            target_book->insert(new Order(order_id, side, (double) price, volume));
            this->order_book_map[order_id] = target_book;
        }
    }

    return true;
}

bool MatchingEngine::amend_order(uint64_t order_id, int64_t new_price, int64_t new_active_volume,
                                 vector<Fill> &fills) {
    if (this->order_book_map.find(order_id) == this->order_book_map.end()) return false; // no such order
    if (new_price <= 0) return false;
    if (new_price <= 0) return false;

    Book *target_book = this->order_book_map[order_id];
    Order target_order_copy = target_book->get_order_by_id(order_id);

    // order only lose priority on 1.price change or 2.increase volume;
    if (target_order_copy.price == (double) new_price && target_order_copy.volume >= new_active_volume) {
        target_book->amend(order_id, (double) new_price, new_active_volume);
    } else {
        // else, remove the order and reinsert one
        const string symbol = target_book->symbol;
        const Side side = target_order_copy.side;

        this->pull_order(order_id);
        this->add_order(order_id, symbol, side, new_price, new_active_volume, fills);
    }

    return true;
}

bool MatchingEngine::pull_order(uint64_t order_id) {
    Book *target_book = (this->order_book_map.find(order_id) != this->order_book_map.end()
                         ? this->order_book_map[order_id] : nullptr);

    if (target_book == nullptr) {
        return false;
    } else {
        this->order_book_map.erase(order_id);
        target_book->remove(order_id);
        return true;
    }

}

Book *MatchingEngine::get_book(string const &symbol) {
    return (this->books.find(symbol) != this->books.end() ? this->books[symbol] : nullptr);
}

BestBidOffer MatchingEngine::get_top_of_book(const string &symbol) const {
    auto it = this->books.find(symbol);
    if (it == this->books.end()) {
        return BestBidOffer{0, 0, 0, 0};
    }

    auto target_book = it->second;

    auto best_bid_price = (double) target_book->get_highest_price();
    auto best_ask_price = (double) target_book->get_lowest_price();

    return BestBidOffer{
            static_cast<int64_t>(target_book->get_volume_by_limit(Side::Buy, best_bid_price)),
            static_cast<int64_t>(best_bid_price),
            static_cast<int64_t>(target_book->get_volume_by_limit(Side::Sell, best_ask_price)),
            static_cast<int64_t>(best_ask_price)
    };
}



//
// Created by TK on 21/09/2022.
//

