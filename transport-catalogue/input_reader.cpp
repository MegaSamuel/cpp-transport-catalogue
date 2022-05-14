#include <iostream>

#include "input_reader.h"

namespace query_input {

using namespace std::string_literals;

struct StopQuery{
    // название остановки
    std::string name;
    // координаты остановки
    geo_coord::Coordinates coordinates{0, 0};
    // вектор расстояний от остановки до других остановок
    std::vector<std::pair<int, std::string>> distances;
};

struct BusQuery {
    std::string name;
    std::vector<std::string> stops;
};

StopQuery parseStop(const std::string& line) {
    StopQuery result;

    // позиция первого не пробельного символа
    auto first = line.find_first_not_of(' ', 4);

    // находим позицию ':'
    auto colon = line.find(':', first+1);

    // позиции имени
    auto from1 = first;
    auto to1 = line.substr(from1, colon-from1).find_last_not_of(' ', colon-from1)+1;

    // имя
    result.name = line.substr(from1, to1);

    // позиция широты
    auto from2 = line.find_first_not_of(' ', colon+1);

    // находим позицию ','
    auto comma1 = line.find(',', from2);

    auto to2 = line.substr(from2, comma1-from2).find_last_not_of(' ', comma1-from2)+1;

    result.coordinates.lat = std::stod(static_cast<std::string>(line.substr(from2, to2)));

    // позиция долготы
    auto from3 = line.find_first_not_of(' ', comma1+1);

    // находим позицию ','
    auto comma2 = line.find(',', from3);

    auto to3 = line.substr(from3, comma2-from3).find_last_not_of(' ', comma2-from3)+1;

    result.coordinates.lng = std::stod(static_cast<std::string>(line.substr(from3, to3)));

    // находим позицию ','
    auto comma3 = line.find(',', from3);

    // если нет информации о расстояниях - выходим
    if(line.npos == comma3) {
        return result;
    }

    std::istringstream istream(line.substr(comma3+1, line.size()));
    std::string raw_str;
    std::string str;
    size_t from;
    size_t to;
    // проход по записям вида "DISTm to NAME"
    while(getline(istream, raw_str, ',')) {
        // первый не пробельный символ записи
        from = raw_str.find_first_not_of(' ', 0);

        // последний не пробельный символ записи
        to = raw_str.substr(from, raw_str.size()).find_last_not_of(' ', raw_str.size())+1;

        // запись без пробелов в начале и конце
        str = raw_str.substr(from, to);

        // находим позицию " to"
        auto sep = str.find(" to", 0);

        // позиции расстояния (без учета символа 'm')
        auto from1 = 0;
        auto to1 = str.substr(from1, sep).find_last_not_of(' ', sep);

        // позиции имени (от from2 до конца строки str)
        auto from2 = str.find_first_not_of(' ', sep+3);

        result.distances.push_back(make_pair(stoi(str.substr(from1, to1)), str.substr(from2, str.size())));
    }

    return result;
}

BusQuery parseBus(const std::string& line) {
    BusQuery result;

    // позиция первого не пробельного символа
    auto first = line.find_first_not_of(' ', 3);

    // находим позицию ':'
    auto colon = line.find(':', first+1);

    // позиции имени
    auto from1 = first;
    auto to1 = line.substr(from1, colon-from1).find_last_not_of(' ', colon-from1)+1;

    // имя
    result.name = line.substr(from1, to1);

    char separator = '>';

    // позиция на начало остановки
    auto pos_begin = line.find_first_not_of(' ', colon+1);

    // позиция первого разделителя
    auto pos_sep = line.find(separator, colon+1);

    if(pos_sep == line.npos) {
        separator = '-';
        pos_sep = line.find(separator, colon+1);
    }

    // позиция на конец остановки
    auto pos_end = line.substr(pos_begin, pos_sep-pos_begin).find_last_not_of(' ', pos_sep-pos_begin)+1;

    // читаем маршрут
    while(true) {
        result.stops.push_back(line.substr(pos_begin, pos_end));

        if(pos_sep == line.npos) {
            // конец запроса
            break;
        }

        // позиция начала следующей остановки
        pos_begin = line.find_first_not_of(' ', pos_sep+1);

        // позиция следующего разделителя
        pos_sep = line.find(separator, pos_sep+1);

        // позиция конца следующей остановки
        pos_end = line.substr(pos_begin, pos_sep-pos_begin).find_last_not_of(' ', pos_sep-pos_begin)+1;
    }

    // добавляем обратный маршрут
    if(separator == '-') {
        for(int i = result.stops.size() - 2; i >= 0; --i) {
            result.stops.push_back(result.stops[i]);
        }
    }

    return result;
}

std::istream& queryDataBaseUpdate(transport_catalogue::TransportCatalogue& catalogue, std::istream& input) {
    std::vector<StopQuery> stop_queries;
    std::vector<BusQuery> bus_queries;
    std::vector<std::string> buses;

    const std::string cmd_stop = "Stop"s;
    const std::string cmd_bus = "Bus"s;

    std::string str_line;

    // количество запросов - строка
    std::getline(input, str_line);

    // количество запросов - число
    int count = std::stoi(str_line);

    for(int i = 0; i < count; i++) {
        // построчно читаем запросы
        getline(input, str_line);

        if(!cmd_stop.compare(str_line.substr(0, cmd_stop.size()))) {
            StopQuery stop = parseStop(str_line);
            catalogue.addStop(stop.name, stop.coordinates);
            stop_queries.push_back(stop);
        } else if(!cmd_bus.compare(str_line.substr(0, cmd_bus.size()))) {
            // сначала читаем все маршруты
            buses.push_back(str_line);
            bus_queries.push_back(parseBus(buses.back()));
        } else {
            std::cout << __func__ << " something went wrong\n";
        }
    }

    for(auto& query : stop_queries) {
        for(auto& [dist, stop_to] : query.distances) {
            // добавляем расстояния
            catalogue.setDistance(query.name, stop_to, dist);
        }
    }

    for(auto& query : bus_queries) {
        // добавляем маршруты
        catalogue.addBus(query.name, query.stops);

        // формируем информацию о маршруте
        transport_catalogue::BusInfo info;

        const transport_catalogue::Bus* bus = catalogue.findBus(query.name);

        std::unordered_set<std::string_view> stop_storage;

        info.stop_number = bus->stops.size();

        double geo_distance = 0;

        const transport_catalogue::Stop* previous = nullptr;
        for(const transport_catalogue::Stop* current : bus->stops) {
            stop_storage.insert(current->name);
            if(previous) {
                info.distance += catalogue.getDistance(previous, current);
                geo_distance += ComputeDistance(previous->coordinates, current->coordinates);
            }
            previous = current;
        }

        info.unique_stop_number = stop_storage.size();

        info.curvature = info.distance/geo_distance;

        catalogue.addBusInfo(bus, info);
    }

    return input;
}

} // namespace query_input
