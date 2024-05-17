#pragma once

#include <chrono>
#include <iostream>
#include <type_traits>

constexpr auto trackTime = [](std::string_view tag, auto func) {
    auto before = std::chrono::high_resolution_clock::now();

    if constexpr (std::is_same_v<decltype(func()), void>)
    {
        func();
        auto after = std::chrono::high_resolution_clock::now();
        std::cout << tag << ": "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count()
                  << "ms\n";
    }
    else
    {
        auto result = func();
        auto after = std::chrono::high_resolution_clock::now();
        std::cout << tag << ": "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count()
                  << "ms\n";
        return result;
    }
};
