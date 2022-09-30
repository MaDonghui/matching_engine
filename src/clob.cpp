#include "clob.hpp"

using TradeDS::Order, TradeDS::Limit, TradeDS::Book;

// Order fine-print helper
ostream &operator<<(ostream &os, const TradeDS::Order &o) {
    return (os << "Order\t[ "
               << "-id: " << o.order_id
               << "\t-Side: " << (o.side == Side::Buy ? "Buy" : "Sell")
               << "\t-Price: " << o.price
               << "\t-Volume: " << o.volume
               << "\t-Prev: " << ((o.prev == nullptr) ? "Null" : to_string(o.prev->order_id))
               << "\t-Next: " << ((o.next == nullptr) ? "Null" : to_string(o.next->order_id))
               << " ]");
}

string Order::toString() const {
    stringstream ss;
    ss << (*this);
    return ss.str();
}

// Book fine-print
ostream &operator<<(ostream &os, const TradeDS::Book &o) {
    os << "Book\t[ "
       << "-symbol: " << o.symbol
       << "\t-uint: " << o.unit
       << "\t-order count: " << o.get_order_count()
       << "\t-buy volume: " << o.get_buy_volume()
       << "\t-sell volume: " << o.get_sell_volume()
       << "\t-highest_buy: " << o.get_highest_price()
       << "\t-lowest_sell: " << o.get_lowest_price()
       << " ]" << endl;

    unordered_map<uint64_t, Order *> orders = o.get_orders();
    for (auto it: orders) {
        os << it.second->toString() << endl;
    }

    return os;
}

string Book::to_string() {
    stringstream ss;
    ss << (*this);
    return ss.str();
}

TradeDS::Book::~Book() {
    for (auto i: this->orders) delete i.second;

    for (int i = 0; i < this->buy_set->size(); i++) {
        delete this->buy_set->operator[](i);
    }

    for (int i = 0; i < this->sell_set->size(); i++) {
        delete this->sell_set->operator[](i);
    }

    delete this->buy_set;
    delete this->sell_set;
}

/**
 * 1. insert into correct limit
 * 2. adjust Book's meta data
 * @param new_order
 * @return reference to new_order
 */
Order *Book::insert(Order *const new_order) {
    // reject if order_id already exists
    if (this->exist(new_order->order_id)) return nullptr;
    // reject order if price is in wrong unit
    if (fmod(new_order->price, this->unit) != 0) return nullptr;

    // insert into right limit
    SparseSet < Limit * > *target_side = (new_order->side == Side::Buy) ? (this->buy_set) : (this->sell_set);
    const auto limit_idx = (int64_t) (new_order->price / this->unit);
    Limit *target_limit = target_side->operator[](limit_idx);

    // create limit if not exist
    if (target_limit == nullptr) {
        target_limit = new Limit(new_order->price);
        target_side->insert(limit_idx, target_limit);
    }

    // append order to the tail of limit
    if (target_limit->size == 0) {
        target_limit->front_order = new_order;
        target_limit->tail_order = new_order;
    } else {
        target_limit->tail_order->next = new_order;
        new_order->prev = target_limit->tail_order;
        target_limit->tail_order = new_order;
    }
    target_limit->size++;
    target_limit->volume += new_order->volume;

    new_order->limit = target_limit;

    // adjust Book's meta data
    this->orders[new_order->order_id] = new_order;
    this->order_count++;
    (new_order->side == Side::Buy) ? (this->buy_volume += new_order->volume) : (this->sell_volume += new_order->volume);

    // Adjust Best offer
    if (new_order->side == Side::Buy) {
        if (this->highest_buy == nullptr) {
            this->highest_buy = target_limit;
        } else {
            this->highest_buy = (this->highest_buy->price >= target_limit->price) ? this->highest_buy
                                                                                  : target_limit;
        }
    } else {
        if (this->lowest_sell == nullptr) {
            this->lowest_sell = target_limit;
        } else {
            this->lowest_sell = (this->lowest_sell->price <= target_limit->price) ? this->lowest_sell
                                                                                  : target_limit;
        }
    }

    return new_order;
}

Order *Book::amend(const uint64_t order_id, const double new_price, const uint64_t new_volume) {
    // reject if order doesn't exist
    if (!this->exist(order_id)) return nullptr;
    // reject order if price is in wrong unit
    if (fmod(new_price, this->unit) != 0) return nullptr;

    Order *target_order = this->orders[order_id];
    Limit *target_limit = target_order->limit;

    if (target_order->price != new_price) {
        // new price, detach order, modify and reinsert
        this->detach(target_order->order_id);   // metadata updated
        target_order->price = new_price;
        target_order->volume = new_volume;
        this->insert(target_order);           // metadata updated
    } else {
        // same price, simply change volume and metadata
        auto volume_diff = (int64_t) (new_volume - target_order->volume);
        target_limit->volume += volume_diff;
        (target_order->side == Side::Buy) ? (this->buy_volume += volume_diff) : (this->sell_volume += volume_diff);

        target_order->volume = new_volume;
    }

    return target_order;
}

Order *Book::detach(const uint64_t order_id) {
    // reject if order doesn't exist
    if (this->orders.find(order_id) == this->orders.end()) return nullptr;

    Order *target_order = this->orders[order_id];
    Limit *target_limit = target_order->limit;

    // 1. detach from orders
    this->orders.erase(order_id);

    // 2. detach from limit linked list
    // middle order, most likely a hit, check first
    if (target_limit->front_order != target_order && target_limit->tail_order != target_order) {
        target_order->prev->next = target_order->next;
        target_order->next->prev = target_order->prev;

        target_order->prev = nullptr;
        target_order->next = nullptr;
        goto CHANGE_META;
    }

    // only order in Limit
    if (target_limit->size == 1) {
        target_order->limit->front_order = nullptr;
        target_order->limit->tail_order = nullptr;
        goto CHANGE_META;
    }

    // head order, with other order behind
    if (target_limit->front_order == target_order) {
        target_order->next->prev = nullptr;
        target_limit->front_order = target_order->next;

        target_order->next = nullptr;
        goto CHANGE_META;
    }

    // tail order
    if (target_limit->tail_order == target_order) {
        target_order->prev->next = nullptr;
        target_limit->tail_order = target_order->prev;

        target_order->prev = nullptr;
        goto CHANGE_META;
    }

    // modify Limit & Book's metadata
    CHANGE_META:
    target_limit->size--;
    target_limit->volume -= target_order->volume;

    this->order_count--;
    target_order->side == Side::Buy ? this->buy_volume -= target_order->volume
                                    : this->sell_volume -= target_order->volume;

    // if best offer is exhausted, update it
    auto limit_idx = (int64_t) (target_order->price / this->unit);
    if (target_order->side == Side::Buy && this->highest_buy->size == 0) {
        // reset best buy-offer (highest)
        this->highest_buy = nullptr;
        // find next highest
        while (limit_idx >= 0) {
            limit_idx--;
            Limit *currLimit = this->buy_set->operator[](limit_idx);

            if (currLimit != nullptr && currLimit->size > 0) {
                this->highest_buy = currLimit;
                break;
            }
        }   // if no suitable limit is found, best offer stays as nullptr

    } else if (target_order->side == Side::Sell && this->lowest_sell->size == 0) {
        // reset best sell-offer (lowest)
        this->lowest_sell = nullptr;
        const uint64_t MAX_LIMIT_IDX = this->sell_set->size() - 1;
        while (limit_idx <= MAX_LIMIT_IDX) {
            limit_idx++;
            Limit *currLimit = this->sell_set->operator[](limit_idx);

            if (currLimit != nullptr && currLimit->size > 0) {
                this->lowest_sell = currLimit;
                break;
            }
        }   // if no suitable limit is found, best offer stays as nullptr
    }


    return target_order;
}

bool Book::exist(const uint64_t order_id) const {
    return this->orders.find(order_id) != this->orders.end();
}

bool Book::remove(uint64_t order_id) {
    Order *to_remove = this->detach(order_id);

    if (to_remove != nullptr) {
        delete to_remove;
        return true;
    } else { return false; }
}

uint64_t Book::get_best_offer_id(Side side) const {
    if (side == Side::Buy) {
        if (this->lowest_sell == nullptr) return 0;
        if (this->lowest_sell->front_order == nullptr) return 0;     // think never triggered
        return this->lowest_sell->front_order->order_id;
    } else {
        if (this->highest_buy == nullptr) return 0;
        if (this->highest_buy->front_order == nullptr) return 0;     // think never triggered
        return this->highest_buy->front_order->order_id;
    }
}

unordered_map<uint64_t, Order *> Book::get_orders() const {
    return this->orders;
}

Order Book::get_order_by_id(uint64_t order_id) const {
    auto it = this->orders.find(order_id);

    if (it == this->orders.end()) {
        return Order{0, Side::Buy, 0, 0};
    } else {
        return Order{it->second->order_id, it->second->side, it->second->price, it->second->volume};
    }
}


uint64_t Book::get_order_count() const {
    return this->order_count;
}

int64_t Book::get_buy_volume() const {
    return this->buy_volume;
}

int64_t Book::get_sell_volume() const {
    return this->sell_volume;
}

uint64_t Book::get_volume_by_limit(Side side, double price) const {
    auto target_set = side == Side::Buy ? this->buy_set : this->sell_set;
    const auto limit_idx = (int64_t) (price / this->unit);
    auto target_limit = target_set->operator[](limit_idx);
    if (target_limit == nullptr) {
        return 0;
    } else {
        return target_limit->volume;
    }
}

uint64_t Book::get_highest_price() const {
    return this->highest_buy != nullptr ? (uint64_t) this->highest_buy->price : 0;
}

uint64_t Book::get_lowest_price() const {
    return this->lowest_sell != nullptr ? (uint64_t) this->lowest_sell->price : 0;
}

