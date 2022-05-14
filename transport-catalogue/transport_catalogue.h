#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

#include "geo.h"

namespace transport_catalogue {

// остановка
struct Stop {
    std::string_view name;
    geo_coord::Coordinates coordinates;
};

// маршрут
struct Bus {
    std::string_view name;
    std::vector<const Stop*> stops;
};

// информация о маршруте
struct BusInfo {
    int stop_number;
    int unique_stop_number;
    int distance;
    double curvature;

    BusInfo();
};

struct HasherPair {
    size_t operator()(const std::pair<const Stop*, const Stop*>& p) const;
};

class TransportCatalogue {
public:
    TransportCatalogue() = default;

    // добавить остановку
    void addStop(std::string_view name, geo_coord::Coordinates& coord);

    // добавить маршрут
    void addBus(std::string_view name, const std::vector<std::string>& stops);

    // добавить информацию о маршруте
    void addBusInfo(const Bus* bus, const BusInfo& info);

    // поиск остановки по имени
    const Stop* findStop(std::string_view name) const;

    // поиск маршрута по имени
    const Bus* findBus(std::string_view name) const;

    // получение информации о маршруте
    const BusInfo getBusInfo(const Bus* bus);
    const BusInfo getBusInfo(const std::string_view name);

    // получение информации о автобусах проходящих через остановку
    int getBusesNumOnStop(const Stop* stop);
    std::vector<const Bus*> getBusesOnStop(const Stop* stop);

    // установить расстояние между остановок
    void setDistance(const std::string& stop_from, const std::string& stop_to, int distance);

    // получить рассояние между остановок
    int getDistance(const Stop* stop_from, const Stop* stop_to) const;

private:
    // здесь живут все имена (сюда показывает string_view)
    std::deque<std::string> m_name_to_storage;

    // остановки
    std::deque<Stop> m_stops;
    std::unordered_map<std::string_view, const Stop*> m_name_to_stop;

    // маршруты
    std::deque<Bus> m_buses;
    std::unordered_map<std::string_view, const Bus*> m_name_to_bus;

    // расстояния между остановками
    std::unordered_map<std::pair<const Stop*, const Stop*>, int, HasherPair> m_distance;

    // таблица для получения автобусов проходящих через остановку
    std::unordered_map<const Stop*, std::unordered_set<Bus*>> m_stop_to_bus;

    // таблица для получения информации о маршруте
    std::unordered_map<const Bus*, BusInfo> m_bus_to_info;

    std::string_view getName(std::string_view str);
};

} // namespace transport_catalogue
