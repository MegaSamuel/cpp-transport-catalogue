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

        std::string str_cnt;

        std::getline(fin, str_cnt);
        query_input::queryDataBaseUpdate(catalogue, std::stoi(str_cnt), fin);

        std::getline(fin, str_cnt);
        query_output::queryDataBase(catalogue, std::stoi(str_cnt), fin, fout);
    } else {
        // работа со стандартными потоками ввода/вывода
        std::string str_cnt;

        std::getline(std::cin, str_cnt);
        query_input::queryDataBaseUpdate(catalogue, std::stoi(str_cnt), std::cin);

        std::getline(std::cin, str_cnt);
        query_output::queryDataBase(catalogue, std::stoi(str_cnt), std::cin, std::cout);
    }
}
