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

size_t HasherPair::operator()(const std::pair<const Stop*, const Stop*>& p) const {
    return std::hash<const void*>{}(p.first) + std::hash<const void*>{}(p.second);
}

void TransportCatalogue::addStop(std::string_view name, geo_coord::Coordinates& coord) {
    std::string_view vname = getName(name);

    // добавляем остановку
    m_stops.push_back({vname, coord});

    // добавляем остановку
    m_name_to_stop[vname] = &m_stops.back();
}

void TransportCatalogue::addBus(std::string_view name, const std::vector<std::string>& stops) {
    std::vector<const Stop*> vector_stops;
    vector_stops.reserve(stops.size());

    for(auto stop_name : stops) {
        const Stop* stop = findStop(stop_name);
        if(!stop) {
            throw std::invalid_argument(__func__ + " invalid stop pointer"s);
        }
        vector_stops.push_back(stop);
    }

    std::string_view vname = getName(name);

    // добавляем маршрут
    m_buses.push_back({vname, vector_stops});

    // добавляем маршрут
    m_name_to_bus[vname] = &m_buses.back();

    // добавляем автобус к остановке
    for(auto stop : vector_stops) {
        m_stop_to_bus[stop].insert(&m_buses.back());
    }
}

void TransportCatalogue::addBusInfo(const Bus* bus, const BusInfo& info) {
    m_bus_to_info[bus] = info;
}

const Bus* TransportCatalogue::findBus(std::string_view name) const {
    return (m_name_to_bus.count(name) > 0) ? m_name_to_bus.at(name) : nullptr;
}

const Stop* TransportCatalogue::findStop(std::string_view name) const {
    return (m_name_to_stop.count(name) > 0) ? m_name_to_stop.at(name) : nullptr;
}

const BusInfo TransportCatalogue::getBusInfo(const Bus* bus) {
    if(!bus) {
        throw std::invalid_argument(__func__ + " invalid bus pointer"s);
    }
    return m_bus_to_info.at(bus);
}

const BusInfo TransportCatalogue::getBusInfo(const std::string_view name) {
    return getBusInfo(findBus(name));
}

int TransportCatalogue::getBusesNumOnStop(const Stop* stop) {
    if(!stop) {
        throw std::invalid_argument(__func__ + " invalid stop pointer"s);
    }
    return m_stop_to_bus.count(stop);
}

std::vector<const Bus*> TransportCatalogue::getBusesOnStop(const Stop* stop) {
    return {m_stop_to_bus.at(stop).begin(), m_stop_to_bus.at(stop).end()};
}

void TransportCatalogue::setDistance(const std::string& str_stop_from, const std::string& str_stop_to, int distance) {
    // указатели на остановки
    const Stop* stop_from = findStop(str_stop_from);
    const Stop* stop_to = findStop(str_stop_to);

    if((nullptr == stop_from) || (nullptr == stop_to)) {
        return;
    }

    // если остановки нашлись, то добавляем расстояние
    m_distance.insert({std::make_pair(stop_from, stop_to), distance});
}

int TransportCatalogue::getDistance(const Stop* stop_from, const Stop* stop_to) const {
    std::pair<const Stop*, const Stop*> key_forward = std::make_pair(stop_from, stop_to);

    if(0 == m_distance.count(key_forward)) {
        std::pair<const Stop*, const Stop*> key_backward = std::make_pair(stop_to, stop_from);

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

std::string_view TransportCatalogue::getName(std::string_view str) {
    m_name_to_storage.push_back(static_cast<std::string>(str));
    return m_name_to_storage.back();
}

} // namespace transport_catalogue
