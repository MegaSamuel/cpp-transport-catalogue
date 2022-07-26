#include <fstream>
#include <iostream>
#include <string_view>

#include "transport_catalogue.h"
#include "json_reader.h"
#include "request_handler.h"
#include "transport_router.h"

using namespace std::literals;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);

    transport_catalogue::TransportCatalogue catalogue; // каталог
    map_renderer::MapRenderer renderer;
    transport_router::TransportRouter router(catalogue);
    serialization::Serializator serializator(catalogue, renderer, router);
    json_reader::JsonReader json_reader(catalogue, renderer, router, serializator);

    if (mode == "make_base"sv) {
        // ввод
        json_reader.GeneralLoadBase(std::cin);
        // парсим ввод (заполняем каталог)
        json_reader.Parse();
    } else if (mode == "process_requests"sv) {
        json_reader.GeneralLoadRequests(std::cin);
        // запросы к каталогу
        request_handler::RequestHandler request_handler(catalogue, renderer, router);
        // вывод
        json_reader.Print(std::cout, request_handler);
    } else {
        PrintUsage();
        return 1;
    }
}