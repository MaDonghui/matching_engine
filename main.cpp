#include <iostream>

#include "matching_engine.hpp"

int main() {
    // prep
    auto matching_engine = new MatchingEngine();
    auto fills = std::vector<Fill>();
    auto symbol = std::string("webb");
    matching_engine->add_order(1, symbol, Side::Buy, 1, 1, fills);

    double total_time = 0;
    for (int i = 2; i < 10002; ++i) {
        // operation starts
        clock_t start_time = clock();

        matching_engine->add_order(2, symbol, Side::Buy, 1, 1, fills);

        // end work here
        double elapsed_time = (double) (clock() - start_time) / CLOCKS_PER_SEC * 1000;
        total_time += elapsed_time;
    }
    printf("Inserting done in %f ms\n", total_time / 10000);

    total_time = 0;
    for (int i = 10003; i < 20003; ++i) {
        // operation starts
        clock_t start_time = clock();

        matching_engine->add_order(i, symbol, Side::Sell, 1, 1, fills);

        // end work here
        double elapsed_time = (double) (clock() - start_time) / CLOCKS_PER_SEC * 1000;
        total_time += elapsed_time;
    }

    printf("Matched in %f ms\n", total_time / 10000);


    return 0;
}