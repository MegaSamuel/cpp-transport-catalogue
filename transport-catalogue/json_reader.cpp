#include <algorithm>
#include <sstream>

#include "json_reader.h"

namespace json_reader {

using namespace std::literals;

namespace details {

std::string space_trimmer(const std::string& name) {
    size_t from = name.find_first_not_of(' ', 0);
    size_t to = name.substr(from, name.size()-from).find_last_not_of(' ', name.size()-from)+1;
    return name.substr(from, to);
}

StopQuery QueryStop(const json::Dict& dict) {
    StopQuery stop;

    stop.type = query_type::STOP;
    stop.name = space_trimmer(dict.at("name"s).AsString());
    stop.coordinates.lat = dict.at("latitude"s).AsDouble();
    stop.coordinates.lng = dict.at("longitude"s).AsDouble();

    const json::Dict & ref_to_dict = dict.at("road_distances"s).AsMap();

    stop.distances.reserve(ref_to_dict.size());
    for (const auto& [name, node] : ref_to_dict) {
        stop.distances.emplace_back(make_pair(node.AsInt(), name));
    } 

    return stop;
}

BusQuery QueryBus(const json::Dict& dict) {
    BusQuery bus;

    bus.type = query_type::BUS;
    bus.name = space_trimmer(dict.at("name"s).AsString());

    bus.is_roundtrip = dict.at("is_roundtrip"s).AsBool();

    const json::Array& ref_to_array = dict.at("stops"s).AsArray();

    // резервируем место
    bus.stops.reserve(bus.is_roundtrip ? ref_to_array.size() : 2*ref_to_array.size());
    for (const auto& name : ref_to_array) {
        bus.stops.emplace_back(name.AsString());
    }

    // имя конечной остановки
    bus.name_last_stop = bus.stops.back();

    // добавляем обратный маршрут если не кольцевой маршрут
    if(false == bus.is_roundtrip) {
        for(int i = static_cast<int>(bus.stops.size()) - 2; i >= 0; --i) {
            bus.stops.emplace_back(bus.stops[i]);
        }
    }

    return bus;
}

MapQuery QueryMap(const json::Dict& dict) {
    MapQuery map;

    map.type = query_type::MAP;

    // у этого запроса нет данных
    (void)dict;

    return map;
}

StatQuery QueryStat(const json::Dict& dict) {
    StatQuery stat;

    // общее поле
    stat.id = dict.at("id"s).AsInt();

    if (!dict.at("type"s).AsString().compare("Stop"s)) {
        stat.type = query_type::STOP;
        stat.name = space_trimmer(dict.at("name"s).AsString());
    } else if (!dict.at("type"s).AsString().compare("Bus"s)) {
        stat.type = query_type::BUS;
        stat.name = space_trimmer(dict.at("name"s).AsString());
    } else if (!dict.at("type"s).AsString().compare("Map"s)) {
        stat.type = query_type::MAP;
    }

    return stat;
}

} // namespace details

JsonReader::JsonReader(transport_catalogue::TransportCatalogue& catalogue, 
                       map_renderer::MapRenderer& renderer) : 
                       catalogue_(catalogue), renderer_(renderer) {

}

void JsonReader::Load(std::istream& input) {
    document_ = json::Load(input);

    size_t total_size = 0;

    // запрашиваем размеры
    for (const auto& [name, node] : document_.GetRoot().AsMap()) {
        if (!name.compare("base_requests"s)) {
            total_size += node.AsArray().size();
        } else if (!name.compare("stat_requests"s)) {
            total_size += node.AsArray().size();
        } else if (!name.compare("render_settings"s)) {
            total_size += node.AsMap().size();
        }
    }

    // резервируем место
    queries_.reserve(total_size);

    // загружаем данные
    for (const auto& [name, node] : document_.GetRoot().AsMap()) {
        if (!name.compare("base_requests"s)) {
            LoadBase(node.AsArray());
        } else if (!name.compare("stat_requests"s)) {
            LoadStat(node.AsArray());
        } else if (!name.compare("render_settings"s)) {
            LoadRender(node.AsMap());
        }
    }
}

void JsonReader::LoadBase(const json::Array& vct) {
    for (const auto& it : vct) {
        if (0 == it.AsMap().count("type"s)) {
            continue;
        }

        if (!it.AsMap().at("type"s).AsString().compare("Stop"s)) {
            // остановка
            queries_.emplace_back(std::make_unique<details::StopQuery>(details::QueryStop(it.AsMap())));
        } else if (!it.AsMap().at("type"s).AsString().compare("Bus"s)) {
            // маршрут
            queries_.emplace_back(std::make_unique<details::BusQuery>(details::QueryBus(it.AsMap())));
        }
    }
}

void JsonReader::LoadStat(const json::Array& vct) {
    for (const auto& it : vct) {
        if (0 == it.AsMap().count("type"s)) {
            continue;
        }
        if (!it.AsMap().at("type"s).AsString().compare("Stop"s)) {
            // остановка
            queries_.emplace_back(std::make_unique<details::StatQuery>(details::QueryStat(it.AsMap())));
        } else if (!it.AsMap().at("type"s).AsString().compare("Bus"s)) {
            // маршрут
            queries_.emplace_back(std::make_unique<details::StatQuery>(details::QueryStat(it.AsMap())));
        } else if (!it.AsMap().at("type"s).AsString().compare("Map"s)) {
            // карта
            queries_.emplace_back(std::make_unique<details::StatQuery>(details::QueryStat(it.AsMap())));
        }
    }
}

svg::Color JsonReader::LoadColor(const json::Node& node) {
    if (node.IsArray()) {
        // если массив
        if (3 == node.AsArray().size()) {
            // размер 3 - обычный цвет
            svg::Rgb rgb;
            rgb.red = node.AsArray()[0].AsInt();
            rgb.green = node.AsArray()[1].AsInt();
            rgb.blue = node.AsArray()[2].AsInt();
            return rgb;
        } else {
            // размер 4 - цвет с прозрачностью
            svg::Rgba rgba;
            rgba.red = node.AsArray()[0].AsInt();
            rgba.green = node.AsArray()[1].AsInt();
            rgba.blue = node.AsArray()[2].AsInt();
            rgba.opacity = node.AsArray()[3].AsDouble();
            return rgba;
        }
    } else {
        // если строка
        return node.AsString();
    }
}

void JsonReader::LoadRender(const json::Dict& dict) {
    map_renderer::RenderSettings settings;

    settings.width = dict.at("width"s).AsDouble();
    settings.height = dict.at("height"s).AsDouble();
    settings.padding = dict.at("padding"s).AsDouble();
    settings.line_width = dict.at("line_width"s).AsDouble();
    settings.stop_radius = dict.at("stop_radius"s).AsDouble();
    settings.bus_label_font_size = dict.at("bus_label_font_size"s).AsInt();
    settings.bus_label_offset[0] = dict.at("bus_label_offset"s).AsArray()[0].AsDouble();
    settings.bus_label_offset[1] = dict.at("bus_label_offset"s).AsArray()[1].AsDouble();
    settings.stop_label_font_size = dict.at("stop_label_font_size"s).AsInt();
    settings.stop_label_offset[0] = dict.at("stop_label_offset"s).AsArray()[0].AsDouble();
    settings.stop_label_offset[1] = dict.at("stop_label_offset"s).AsArray()[1].AsDouble();
    settings.underlayer_color = LoadColor(dict.at("underlayer_color"s));
    settings.underlayer_width = dict.at("underlayer_width"s).AsDouble();

    const json::Array& array = dict.at("color_palette"s).AsArray();
    settings.color_palette.reserve(array.size());
    for (const auto& it : array) {
        settings.color_palette.emplace_back(LoadColor(it));
    }

    renderer_.SetSettings(settings);
}

void JsonReader::Parse() {
    // проходим по всем запросам, обрабатываем только StopQuery
    for (const auto& it : queries_) {
        if (details::StopQuery* stop_query = dynamic_cast<details::StopQuery*>(it.get())) {
            // добавляем в каталог остановки
            catalogue_.addStop(stop_query->name, stop_query->coordinates);
        }
    }

    // проходим по всем запросам, обрабатываем только StopQuery
    for (const auto& it : queries_) {
        if (details::StopQuery* stop_query = dynamic_cast<details::StopQuery*>(it.get())) {
            for(auto& [dist, stop_to] : stop_query->distances) {
                // добавляем в каталог расстояния между остановками
                catalogue_.setDistance(stop_query->name, stop_to, dist);
            }
        }
    }

    // проходим по всем запросам, обрабатываем только BusQuery
    for (const auto& it : queries_) {
        if (details::BusQuery* bus_query = dynamic_cast<details::BusQuery*>(it.get())) {
            // добавляем маршруты
            catalogue_.addBus(bus_query->name, bus_query->name_last_stop, bus_query->is_roundtrip, bus_query->stops);

            // формируем информацию о маршруте
            transport_catalogue::BusInfo info;

            const domain::Bus* bus = catalogue_.findBus(bus_query->name);

            std::unordered_set<std::string_view> stop_storage;

            info.stop_number = static_cast<int>(bus->stops.size());

            double geo_distance = 0;

            const domain::Stop* previous = nullptr;
            for(const domain::Stop* current : bus->stops) {
                stop_storage.emplace(current->name);
                if(previous) {
                    info.distance += catalogue_.getDistance(previous, current);
                    geo_distance += ComputeDistance(previous->coordinates, current->coordinates);
                }
                previous = current;
            }

            info.unique_stop_number = static_cast<int>(stop_storage.size());

            info.curvature = info.distance/geo_distance;

            catalogue_.addBusInfo(bus, info);
        }
    }

    // проходим по всем запросам, обрабатываем только MapQuery
    // for (const auto& it : queries_) {
    //     if (details::MapQuery* map_query = dynamic_cast<details::MapQuery*>(it.get())) {
    //     }
    // }
}

void JsonReader::Print(std::ostream& out, request_handler::RequestHandler& request_handler) {
    out << "["s << std::endl;
    bool first = true;
    for (const auto& it : queries_) {
        if (details::StatQuery* stat_query = dynamic_cast<details::StatQuery*>(it.get())) {
            if (!first) {
                out << ',' << std::endl;
            }
            first = false;

            if (stat_query->type == details::query_type::STOP) {
                // формируем словарь
                json::Dict dict;

                try {
                    const domain::Stop* stop = catalogue_.findStop(stat_query->name);

                    if(0 == catalogue_.getBusesNumOnStop(stop)) {
                        // остановка объявлена, но не входит ни в один из маршрутов

                        // формируем ноды
                        json::Node request_id{stat_query->id};
                        json::Array arr_buses;

                        dict["buses"] = arr_buses;
                        dict["request_id"s] = request_id;
                    } else {
                        std::vector<const domain::Bus*> buses = catalogue_.getBusesOnStop(stop);

                        // должен быть алфавитный порядок
                        std::sort(buses.begin(), buses.end(), 
                                  [](const domain::Bus* bus1, const domain::Bus* bus2) { 
                                      return bus1->name < bus2->name; 
                                  });

                        // формируем ноды
                        json::Node request_id{stat_query->id};
                        json::Array arr_buses;

                        for (const auto& it : buses) {
                            arr_buses.emplace_back(static_cast<std::string>(it->name));
                        }

                        dict["buses"] = arr_buses;
                        dict["request_id"s] = request_id;
                    }
                }
                catch(std::invalid_argument&) {
                    // формируем ноды
                    json::Node request_id{stat_query->id};
                    json::Node error{"not found"s};

                    dict["request_id"s] = request_id;
                    dict["error_message"s] = error;
                }


                // формируем документ
                json::Document document{dict};

                // печатаем документ
                json::Print(document, out);
            }
            else if (stat_query->type == details::query_type::BUS) {
                // формируем словарь
                json::Dict dict;

                try {
                    const transport_catalogue::BusInfo bus_info = catalogue_.getBusInfo(stat_query->name);

                    // формируем ноды
                    json::Node curvature{bus_info.curvature};
                    json::Node request_id{stat_query->id};
                    json::Node route_length{bus_info.distance};
                    json::Node stop_count{bus_info.stop_number};
                    json::Node unique_stop_count{bus_info.unique_stop_number};

                    dict["curvature"s] = curvature;
                    dict["request_id"s] = request_id;
                    dict["route_length"s] = route_length;
                    dict["stop_count"s] = stop_count;
                    dict["unique_stop_count"s] = unique_stop_count;
                }
                catch(std::invalid_argument&) {
                    // формируем ноды
                    json::Node request_id{stat_query->id};
                    json::Node error{"not found"s};

                    dict["request_id"s] = request_id;
                    dict["error_message"s] = error;
                }

                // формируем документ
                json::Document document{dict};

                // печатаем документ
                json::Print(document, out);
            } else if (stat_query->type == details::query_type::MAP) {
                // формируем словарь
                json::Dict dict;

                // формируем карту
                std::stringstream stream;
                request_handler.RenderMap(stream);

                // формируем ноды
                json::Node request_id{stat_query->id};

                json::Node svg_string{stream.str()};

                dict["request_id"s] = request_id;
                dict["map"s] = svg_string;

                // формируем документ
                json::Document document{dict};

                // печатаем документ
                json::Print(document, out);
            }
        }
    }
    out << std::endl;
    out << "]"s << std::endl;
}

} // namespace json_reader
