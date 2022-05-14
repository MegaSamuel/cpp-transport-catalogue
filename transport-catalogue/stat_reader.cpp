#include <iostream>
#include <stdexcept>
#include <iomanip>  //for std::setprecision
#include <algorithm>

#include "stat_reader.h"

namespace query_output {

using namespace std::string_literals;

void printBusInfo(std::ostream& output, std::string_view name, const transport_catalogue::BusInfo& info) {
    output << "Bus "s << name << ": "s 
           << info.stop_number << " stops on route, "s 
           << info.unique_stop_number << " unique stops, "s 
           << std::setprecision(6) << static_cast<double>(info.distance) << " route length, "s
           << std::setprecision(6) << info.curvature << " curvature\n"s;
}

void parseBusQuery(transport_catalogue::TransportCatalogue& catalogue, std::ostream& output, std::string& line) {
    std::string name;
    // убираем пробелы в начале и конце маршрута
    auto from = line.find_first_not_of(' ', 3);
    auto to = line.substr(from, line.size()-from).find_last_not_of(' ', line.size()-from)+1;
    name = line.substr(from, to);

    try {
        transport_catalogue::BusInfo info = catalogue.getBusInfo(name);
        printBusInfo(output, name, info);
    }
    catch(std::invalid_argument&) {
        output << "Bus "s << name << ": not found\n"s;
    }
}

void parseStopQuery(transport_catalogue::TransportCatalogue& catalogue, std::ostream& output, std::string& line) {
    std::string name;
    // убираем пробелы в начале и конце остановки
    auto from = line.find_first_not_of(' ', 4);
    auto to = line.substr(from, line.size()-from).find_last_not_of(' ', line.size()-from)+1;
    name = line.substr(from, to);

    try {
        const transport_catalogue::Stop* stop = catalogue.findStop(name);
        if(0 == catalogue.getBusesNumOnStop(stop)) {
            // через остановку не проходят автобусы
            output << "Stop "s << name << ": no buses\n"s;
            return;
        }
        std::vector<const transport_catalogue::Bus*> buses = catalogue.getBusesOnStop(stop);
        // должен быть алфавитный порядок
        std::sort(buses.begin(), buses.end(), 
                  [](const transport_catalogue::Bus* bus1, const transport_catalogue::Bus* bus2) { 
                      return bus1->name < bus2->name; 
                  });
        output << "Stop "s << name << ": buses"s;
        for(const transport_catalogue::Bus* bus : buses) {
            output << " "s << bus->name ;
        }
        output << "\n"s;
    }
    catch(std::invalid_argument&) {
        output << "Stop "s << name << ": not found\n"s;
    }
}

void queryDataBase(transport_catalogue::TransportCatalogue& catalogue, std::istream& input, std::ostream& output) {
    const std::string cmd_stop = "Stop"s;
    const std::string cmd_bus = "Bus"s;

    std::string str_line;

    // количество запросов - строка
    std::getline(input, str_line);

    // количество запросов - число
    int count = std::stoi(str_line);

    for(int i = 0; i < count; i++) {
        // построчно читаем запросы
        std::getline(input, str_line);

        if(!cmd_stop.compare(str_line.substr(0, cmd_stop.size()))) {
            parseStopQuery(catalogue, output, str_line);
        } else if(!cmd_bus.compare(str_line.substr(0, cmd_bus.size()))) {
            parseBusQuery(catalogue, output, str_line);
        } else {
            output << __func__ << " something went wrong\n";
        }
    }
}

} // namespace query_output
