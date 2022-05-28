#pragma once

#include <vector>
#include <string_view>

#include "geo.h"

namespace domain {

// остановка
struct Stop {
    std::string_view name;
    geo_coord::Coordinates coordinates;
};

// маршрут
struct Bus {
    std::string_view name;
    const Stop* last_stop;
    bool is_roundtrip = false;
    std::vector<const Stop*> stops;
};

} // namespace domain
