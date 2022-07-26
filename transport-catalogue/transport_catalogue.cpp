#include <algorithm>
#include <numeric>
#include <stdexcept>

#include "transport_catalogue.h"
#include "geo.h"

namespace transport_catalogue {

using namespace std::string_literals;

BusInfo::BusInfo() {
    stop_number = 0;
    unique_stop_number = 0;
    distance = 0;
    curvature = 0.0;
}

size_t HasherPair::operator()(const std::pair<const domain::Stop*, const domain::Stop*>& p) const {
    return std::hash<const void*>{}(p.first) + std::hash<const void*>{}(p.second);
}

void TransportCatalogue::addStop(std::string_view name, geo_coord::Coordinates& coord) {
    std::string_view vname = getName(name);

    // добавляем остановку
    m_stops.emplace_back(domain::Stop{vname, coord});

    // добавляем остановку
    m_name_to_stop[vname] = &m_stops.back();
}

void TransportCatalogue::addBus(std::string_view name, std::string_view name_last_stop, bool is_roundtrip, const std::vector<std::string>& stops) {
    std::vector<const domain::Stop*> vector_stops;
    vector_stops.reserve(stops.size());
    const domain::Stop* last_stop = nullptr;

    for(const auto& stop_name : stops) {
        const domain::Stop* stop = findStop(stop_name);
        if(!stop) {
            throw std::invalid_argument(__func__ + " invalid stop pointer"s);
        }
        if(!stop_name.compare(name_last_stop)) {
            last_stop = stop;
        }
        vector_stops.emplace_back(stop);
    }

    std::string_view vname = getName(name);

    // добавляем маршрут
    m_buses.emplace_back(domain::Bus{vname, last_stop, is_roundtrip, vector_stops});

    // добавляем маршрут
    m_name_to_bus[vname] = &m_buses.back();

    // добавляем автобус к остановке
    for(auto stop : vector_stops) {
        m_stop_to_bus[stop].emplace(&m_buses.back());
    }
}

void TransportCatalogue::addBusInfo(const domain::Bus* bus, const BusInfo& info) {
    m_bus_to_info[bus] = info;
}

const domain::Bus* TransportCatalogue::findBus(std::string_view name) const {
    return (m_name_to_bus.count(name) > 0) ? m_name_to_bus.at(name) : nullptr;
}

const domain::Stop* TransportCatalogue::findStop(std::string_view name) const {
    return (m_name_to_stop.count(name) > 0) ? m_name_to_stop.at(name) : nullptr;
}

const BusInfo TransportCatalogue::getBusInfo(const domain::Bus* bus) {
    if(!bus) {
        throw std::invalid_argument(__func__ + " invalid bus pointer"s);
    }
    return m_bus_to_info.at(bus);
}

const BusInfo TransportCatalogue::getBusInfo(const std::string_view name) {
    return getBusInfo(findBus(name));
}

int TransportCatalogue::getBusesNumOnStop(const domain::Stop* stop) const {
    if(!stop) {
        throw std::invalid_argument(__func__ + " invalid stop pointer"s);
    }
    return static_cast<int>(m_stop_to_bus.count(stop));
}

std::vector<const domain::Bus*> TransportCatalogue::getBusesOnStop(const domain::Stop* stop) {
    return {m_stop_to_bus.at(stop).begin(), m_stop_to_bus.at(stop).end()};
}

void TransportCatalogue::setDistance(const std::string& str_stop_from, const std::string& str_stop_to, int distance) {
    // указатели на остановки
    const domain::Stop* stop_from = findStop(str_stop_from);
    const domain::Stop* stop_to = findStop(str_stop_to);

    if((nullptr == stop_from) || (nullptr == stop_to)) {
        return;
    }

    // если остановки нашлись, то добавляем расстояние
    m_distance.emplace(std::make_pair(std::make_pair(stop_from, stop_to), distance));
}

int TransportCatalogue::getDistance(const domain::Stop* stop_from, const domain::Stop* stop_to) const {
    std::pair<const domain::Stop*, const domain::Stop*> key_forward = std::make_pair(stop_from, stop_to);

    if(0 == m_distance.count(key_forward)) {
        std::pair<const domain::Stop*, const domain::Stop*> key_backward = std::make_pair(stop_to, stop_from);

        if(0 == m_distance.count(key_backward)) {
            // не нашли расстояние
            return 0;
        }

        // расстояние от stop_to до stop_from
        return m_distance.at(key_backward);
    }

    // расстояние от stop_from до stop_to
    return m_distance.at(key_forward);
}

const std::unordered_map<std::string_view, const domain::Stop*>& TransportCatalogue::getStops() const {
    return m_name_to_stop;
}

const std::unordered_map<std::string_view, const domain::Bus*>& TransportCatalogue::getBuses() const {
    return m_name_to_bus;
}

const std::unordered_map<const domain::Stop*, std::unordered_set<domain::Bus*>>& TransportCatalogue::getStopToBuses() const {
    return m_stop_to_bus;
}

std::string_view TransportCatalogue::getName(std::string_view str) {
    m_name_to_storage.emplace_back(static_cast<std::string>(str));
    return m_name_to_storage.back();
}

void TransportCatalogue::addStopDistance(const std::string& stop_name, std::vector<std::pair<int, std::string>>& vct_distance) {
    m_stop_to_dist[stop_name] = move(vct_distance);
}

const std::unordered_map<std::string, std::vector<std::pair<int, std::string>>>& TransportCatalogue::getStopDistance() {
    return m_stop_to_dist;
}

} // namespace transport_catalogue
