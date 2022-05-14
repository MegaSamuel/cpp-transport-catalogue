#include <cassert>
#include <iostream>
#include <string>
#include <fstream>

#include "transport_catalogue.h"
#include "input_reader.h"
#include "stat_reader.h"

int main(int argc, char** argv) {
    transport_catalogue::TransportCatalogue catalogue;

    if(1 < argc) {
        // работа с файлами ввода/вывода
        std::ifstream fin(argv[1], std::ios::in);
        std::ofstream fout(argv[2], std::ios::out);

        query_input::queryDataBaseUpdate(catalogue, fin);

        query_output::queryDataBase(catalogue, fin, fout);
    } else {
        // работа со стандартными потоками ввода/вывода
        query_input::queryDataBaseUpdate(catalogue, std::cin);

        query_output::queryDataBase(catalogue, std::cin, std::cout);
    }
}
