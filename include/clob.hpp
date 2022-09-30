#ifndef CLOB_H
#define CLOB_H

#include <cstddef>
#include <cstdint>
#include <unordered_map>

#include <utility>
#include <sstream>

#include "sparse_set.hpp"
#include "types.hpp"

using std::ostream, std::stringstream, std::to_string, std::unordered_map;

namespace TradeDS {
    struct Order;
    struct Limit;

    class Book;

    /**
     * an struct containing Order information and chaining information
     */
    struct Order {
        const uint64_t order_id;
        const Side side;
        double price;
        uint64_t volume;

        Limit *limit = nullptr;
        Order *prev = nullptr;
        Order *next = nullptr;

        Order(uint64_t order_id, Side side, double limitPrice, uint64_t volume) : order_id{order_id}, side{side},
                                                                                  price{limitPrice},
                                                                                  volume{volume} {};

        string toString() const;
    };


    /**
     * A Limit Price, containing all orders with respected price, chained in double linked list chronological order
     */
    struct Limit {
        const double price;
        size_t size = 0;
        unsigned long long int volume = 0;

        /**
         * Order in this Limit price with highest priority
         */
        Order *front_order = nullptr;
        /**
         * Order in this Limit price with lowest priority
        */
        Order *tail_order = nullptr;

        explicit Limit(double limitPrice) : price{limitPrice} {};
    };

    /**
     * An "Central Limit Order Book", contain all order from both side and metadata\n
     * Internal correctness is guaranteed via guarded modifying methods\n
     * Meaning Orders are in correct storage position and priority  and metadata is updated when necessary\n
     */
    class Book {
    private:
        unordered_map<uint64_t, Order *> orders;    // O(1) order access
        SparseSet<Limit *> *buy_set = new SparseSet<Limit *>(4096);
        SparseSet<Limit *> *sell_set = new SparseSet<Limit *>(4096);
        uint64_t order_count = 0;
        int64_t buy_volume = 0;
        int64_t sell_volume = 0;
        Limit *highest_buy = nullptr;
        Limit *lowest_sell = nullptr;

        /**
         * if an order_id exists\n
         * @param order_id target order_id
         * @return True if order exist, False otherwise
         */
        bool exist(uint64_t order_id) const;

    public:
        const string symbol;
        const double unit;  // unit between each price, 0.01 by default

        /**
         * create a new central limit order book, with a immutable symbol and price unit\n
         * @param symbol the symbol of product
         * @param unit the unit of order price; for example, price of 2.51 is in unit of 0.01
         */
        explicit Book(string symbol, double unit) : symbol{std::move(symbol)}, unit{unit} {};

        /**
         * deconstruct central limit order book\n
         * all TradeDS::Order within will also be destructed as well
         */
        ~Book();

        /**
         * insert a new TradeDS::Order into corresponding price TradeDS:Limit\n
         * update Book's metadata\n
         * a new TradeDS::Limit will be created if necessary\n
         * \n
         * Time-complexity O(1)\n
         *
         * @param new_order reference to TradeDS::Order to be inserted
         * @return reference to TradeDS::Order on successful insertion
         * @return nullptr if order_id already exists
         * @return nullptr if order.price is in wrong unit
         */
        Order *insert(Order *new_order);

        /**
         * update an existing TradeDS::Order within book\n
         * if order's price is modified, an Order will be detached and re-insert into new TradeDS::Limit\n
         * if only oder's volume is modified, Order will be modified directly in-place\n
         * \n
         * time-complexity O(1)
         *
         * @param order_id
         * @param new_price
         * @param new_volume
         * @return reference to TraderDS::Order on success
         * @return nullptr if order_id doesn't exist
         * @return nullptr if new_price is in wrong unit
         */
        Order *amend(uint64_t order_id, double new_price, uint64_t new_volume);

        /**
         * Remove an existing TradeDS::Order from it's TradeDS::Limit without destructing it\n
         * TraderDS::Order's chaining information will be reset\n
         * Book's metadata will be updated as the order is no longer in the book\n
         * \n
         * time-complexity\n
         * O(1) on average\n
         * O(L) on worst case; where L is the number of Limit on Order's Side.
         * this happens when Order is the only order in its TradeDS::Limit, and all Limit are empty\n
         * updating Book's metadata will resulting a full Limit iteration over one Side
         *
         * @param order_id
         * @return reference to TraderDS::Order on success
         * @return nullptr if order_id doesn't exist
         */
        Order *detach(uint64_t order_id);

        /**
         * Remove an existing TradeDS::Order from it's TradeDS::Limit and destructing it\n
         * \n
         * time-complexity\n
         * O(1) on average\n
         * O(L) on worst case; where L is the number of Limit on Order's Side.
         * this happens when Order is the only order in its TradeDS::Limit, and all Limit are empty\n
         * updating Book's metadata will resulting a full Limit iteration over one Side
         *
         * @param order_id
         * @return True on successful removal, False if order_id doesn't exist
         */
        bool remove(uint64_t order_id);

        /**
         * Return the best offer order_id on a given side
         * @param side
         * @return order_id if exists, otherwise 0
         */
        uint64_t get_best_offer_id(Side side) const;

        // getters
        /**
         * get a copy of all orders in book, in a unordered_map\n
         */
        unordered_map<uint64_t, Order *> get_orders() const;

        /**
         * get a copy of Order with a given order_id\n
         * @param order_id
         */
        Order get_order_by_id(uint64_t order_id) const;

        /**
         * get the number of orders in book
         */
        uint64_t get_order_count() const;

        /**
         * get the total volume of Buy side
         */
        int64_t get_buy_volume() const;

        /**
         * get the total volume of Sell Side;
         */
        int64_t get_sell_volume() const;

        /**
         * get the volume of a given limit price on a given side
         */
        uint64_t get_volume_by_limit(Side side, double price) const;

        /**
         * get the highest price among all Buy orders
         */
        uint64_t get_highest_price() const;

        /**
         * get the lowest price among all Sell orders
         * @return
         */
        uint64_t get_lowest_price() const;

        /**
         * fine print Order book and all it's Orders
         */
        string to_string();
    };
}

/**
 * Overloaded ostream << operator for quick fine-print
 * @param os ostream
 * @param o an Order
 * @return ostream
 */
ostream &operator<<(ostream &os, const TradeDS::Order &o);

#endif  // !CLOB_H