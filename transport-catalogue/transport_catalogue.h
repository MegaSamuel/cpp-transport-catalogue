#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

#include "domain.h"

namespace transport_catalogue {

// информация о маршруте
struct BusInfo {
    int stop_number;
    int unique_stop_number;
    int distance;
    double curvature;

    BusInfo();
};

struct HasherPair {
    size_t operator()(const std::pair<const domain::Stop*, const domain::Stop*>& p) const;
};

class TransportCatalogue {
public:
    TransportCatalogue() = default;

    // добавить остановку
    void addStop(std::string_view name, geo_coord::Coordinates& coord);

    // добавить маршрут
    void addBus(std::string_view name, std::string_view name_last_stop, bool is_roundtrip, const std::vector<std::string>& stops);

    // добавить информацию о маршруте
    void addBusInfo(const domain::Bus* bus, const BusInfo& info);

    // поиск остановки по имени
    const domain::Stop* findStop(std::string_view name) const;

    // поиск маршрута по имени
    const domain::Bus* findBus(std::string_view name) const;

    // получение информации о маршруте
    const BusInfo getBusInfo(const domain::Bus* bus);
    const BusInfo getBusInfo(const std::string_view name);

    // получение информации о автобусах проходящих через остановку
    int getBusesNumOnStop(const domain::Stop* stop) const;
    std::vector<const domain::Bus*> getBusesOnStop(const domain::Stop* stop);

    // установить расстояние между остановок
    void setDistance(const std::string& stop_from, const std::string& stop_to, int distance);

    // получить рассояние между остановок
    int getDistance(const domain::Stop* stop_from, const domain::Stop* stop_to) const;

    // получить мапу для остановки: имя - указатель
    const std::unordered_map<std::string_view, const domain::Stop*>& getStops() const;

    // получить мапу для маршрута: имя - указатель
    const std::unordered_map<std::string_view, const domain::Bus*>& getBuses() const;

    // получить мапу для остановки: остановка - маршруты
    const std::unordered_map<const domain::Stop*, std::unordered_set<domain::Bus*>>& getStopToBuses() const;

private:
    // здесь живут все имена (сюда показывает string_view)
    std::deque<std::string> m_name_to_storage;

    // остановки
    std::deque<domain::Stop> m_stops;
    std::unordered_map<std::string_view, const domain::Stop*> m_name_to_stop;

    // маршруты
    std::deque<domain::Bus> m_buses;
    std::unordered_map<std::string_view, const domain::Bus*> m_name_to_bus;

    // расстояния между остановками
    std::unordered_map<std::pair<const domain::Stop*, const domain::Stop*>, int, HasherPair> m_distance;

    // таблица для получения автобусов проходящих через остановку
    std::unordered_map<const domain::Stop*, std::unordered_set<domain::Bus*>> m_stop_to_bus;

    // таблица для получения информации о маршруте
    std::unordered_map<const domain::Bus*, BusInfo> m_bus_to_info;

    std::string_view getName(std::string_view str);
};

} // namespace transport_catalogue
