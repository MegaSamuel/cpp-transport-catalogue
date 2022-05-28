#include <cassert>
#include <iostream>
#include <string>
#include <fstream>

#include "transport_catalogue.h"
#include "json_reader.h"
#include "request_handler.h"

int main() {
    transport_catalogue::TransportCatalogue catalogue; // каталог
    map_renderer::MapRenderer renderer;

    json_reader::JsonReader json_reader(catalogue, renderer);

    // ввод
    json_reader.Load(std::cin);

    // парсим ввод (заполняем каталог)
    json_reader.Parse();

    // запросы к каталогу
    request_handler::RequestHandler request_handler(catalogue, renderer);

    // вывод
    json_reader.Print(std::cout, request_handler);
}
