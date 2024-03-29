#include <algorithm>
#include <sstream>

#include "json_reader.h"
#include "json_builder.h"

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
    } else if (!dict.at("type"s).AsString().compare("Route"s)) {
        stat.type = query_type::ROUTE;
        stat.from = space_trimmer(dict.at("from"s).AsString());
        stat.to = space_trimmer(dict.at("to"s).AsString());
    }

    return stat;
}

} // namespace details

JsonReader::JsonReader(transport_catalogue::TransportCatalogue& catalogue,
                       map_renderer::MapRenderer& renderer,
                       transport_router::TransportRouter& router,
                       serialization::Serializator& serializator) :
                       catalogue_(catalogue), renderer_(renderer),
                       router_(router), serializator_(serializator) {

}

void JsonReader::GeneralLoadBase(std::istream& input) {
    json::Document document = json::Load(input);

    size_t total_size = 0;

    // запрашиваем размеры
    for (const auto& [name, node] : document.GetRoot().AsMap()) {
        if (!name.compare("base_requests"s)) {
            total_size += node.AsArray().size();
        } else if (!name.compare("render_settings"s)) {
            total_size += node.AsMap().size();
        } else if (!name.compare("routing_settings"s)) {
            total_size += node.AsMap().size();
        }
    }

    // резервируем место
    queries_.reserve(total_size);

    // загружаем данные
    for (const auto& [name, node] : document.GetRoot().AsMap()) {
        if (!name.compare("base_requests"s)) {
            LoadBase(node.AsArray());
        } else if (!name.compare("render_settings"s)) {
            LoadRender(node.AsMap());
        } else if (!name.compare("routing_settings"s)) {
            LoadRouting(node.AsMap());
        } else if (!name.compare("serialization_settings"s)) {
            LoadSerialization(node.AsMap());
        }
    }
}

void JsonReader::GeneralLoadRequests(std::istream& input) {
    json::Document document = json::Load(input);

    size_t total_size = 0;

    // запрашиваем размеры
    for (const auto& [name, node] : document.GetRoot().AsMap()) {
        if (!name.compare("stat_requests"s)) {
            total_size += node.AsArray().size();
            stat_count  = node.AsArray().size();
        }
    }

    // резервируем место
    queries_.reserve(total_size);

    // загружаем данные
    for (const auto& [name, node] : document.GetRoot().AsMap()) {
        if (!name.compare("stat_requests"s)) {
            LoadStat(node.AsArray());
        } else if (!name.compare("serialization_settings"s)) {
            LoadSerialization(node.AsMap());
        }
    }

    // десериализация данных каталога
    serializator_.Deserialize();
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
        } else if (!it.AsMap().at("type"s).AsString().compare("Route"s)) {
            // построение маршрута
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

void JsonReader::LoadRouting(const json::Dict& dict) {
    transport_router::RouterSettings settings;

    settings.bus_wait_time = dict.at("bus_wait_time"s).AsInt();
    settings.bus_wait_time *= 60; // time in seconds
    settings.bus_velocity = dict.at("bus_velocity"s).AsInt();
    settings.bus_velocity /= 3.6; // velocity in meter per second

    router_.SetSettings(settings);
}

void JsonReader::LoadSerialization(const json::Dict& dict) {
    serialization::SerializatorSettings settings;

    settings.path = dict.at("file"s).AsString();

    serializator_.SetSettings(settings);
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
            std::vector<std::pair<int, std::string>> vct_distance;
            for(auto& [dist, stop_to] : stop_query->distances) {
                vct_distance.push_back(make_pair(dist, stop_to));
                // добавляем в каталог расстояния между остановками
                catalogue_.setDistance(stop_query->name, stop_to, dist);
            }
            // запоминаем расстояния для остановки (для сериализации)
            if (!vct_distance.empty()) {
                catalogue_.addStopDistance(stop_query->name, vct_distance);
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

    // строим маршрут
    router_.CalcRoute();

    // сериализация данных каталога
    serializator_.Serialize();
}

void JsonReader::Print(std::ostream& out, request_handler::RequestHandler& request_handler) {
    // результат должен быть в массиве
    json::Array Arr;

    // резервируем место
    Arr.reserve(stat_count);

    for (const auto& it : queries_) {
        if (details::StatQuery* stat_query = dynamic_cast<details::StatQuery*>(it.get())) {
            if (stat_query->type == details::query_type::STOP) {
                // формируем словарь
                json::Dict dict;

                try {
                    const domain::Stop* stop = catalogue_.findStop(stat_query->name);

                    if(0 == catalogue_.getBusesNumOnStop(stop)) {
                        // остановка объявлена, но не входит ни в один из маршрутов

                        dict = json::Builder{}.
                            StartDict().
                            Key("buses"s).Value(json::Array{}).
                            Key("request_id"s).Value(stat_query->id).
                            EndDict().
                            Build().AsMap();
                    } else {
                        std::vector<const domain::Bus*> buses = catalogue_.getBusesOnStop(stop);

                        // должен быть алфавитный порядок
                        std::sort(buses.begin(), buses.end(),
                                  [](const domain::Bus* bus1, const domain::Bus* bus2) {
                                      return bus1->name < bus2->name;
                                  });

                        json::Array arr_buses;

                        for (const auto& it : buses) {
                            arr_buses.emplace_back(static_cast<std::string>(it->name));
                        }

                        dict = json::Builder{}.
                            StartDict().
                            Key("buses"s).Value(arr_buses).
                            Key("request_id"s).Value(stat_query->id).
                            EndDict().
                            Build().AsMap();
                    }
                }
                catch(std::invalid_argument&) {
                    dict = json::Builder{}.
                        StartDict().
                        Key("request_id"s).Value(stat_query->id).
                        Key("error_message"s).Value("not found"s).
                        EndDict().
                        Build().AsMap();
                }

                Arr.emplace_back(std::move(dict));
            }
            else if (stat_query->type == details::query_type::BUS) {
                // формируем словарь
                json::Dict dict;

                try {
                    const transport_catalogue::BusInfo bus_info = catalogue_.getBusInfo(stat_query->name);

                    dict = json::Builder{}.
                        StartDict().
                        Key("curvature"s).Value(bus_info.curvature).
                        Key("request_id"s).Value(stat_query->id).
                        Key("route_length"s).Value(static_cast<double>(bus_info.distance)).
                        Key("stop_count"s).Value(bus_info.stop_number).
                        Key("unique_stop_count"s).Value(bus_info.unique_stop_number).
                        EndDict().
                        Build().AsMap();
                }
                catch(std::invalid_argument&) {
                    dict = json::Builder{}.
                        StartDict().
                        Key("request_id"s).Value(stat_query->id).
                        Key("error_message"s).Value("not found"s).
                        EndDict().
                        Build().AsMap();
                }

                Arr.emplace_back(std::move(dict));
            }
            else if (stat_query->type == details::query_type::MAP) {
                // формируем карту
                std::stringstream stream;
                request_handler.RenderMap(stream);

                // формируем словарь
                json::Dict dict = json::Builder{}.
                    StartDict().
                    Key("request_id"s).Value(stat_query->id).
                    Key("map"s).Value(stream.str()).
                    EndDict().
                    Build().AsMap();

                Arr.emplace_back(std::move(dict));
            }
            else if (stat_query->type == details::query_type::ROUTE) {
                // формируем словарь
                json::Dict dict;
                double total_time = 0.0;

                // формируем оптимальный маршрут
                auto route = router_.GetRoute(stat_query->from, stat_query->to);

                if (!route.has_value()) {
                    dict = json::Builder{}.
                        StartDict().
                        Key("request_id"s).Value(stat_query->id).
                        Key("error_message"s).Value("not found"s).
                        EndDict().
                        Build().AsMap();
                }
                else {
                    json::Array arr_items;

                    for (const auto it : route.value()) {
                        arr_items.push_back(json::Builder{}.
                            StartDict().
                            Key("type"s).Value("Wait"s).
                            Key("stop_name"s).Value(static_cast<std::string>(it->from->name)).
                            Key("time"s).Value(it->trip.waiting_time/60.0).
                            EndDict().
                            Build().AsMap());
                        total_time += it->trip.waiting_time/60.0;

                        arr_items.push_back(json::Builder{}.
                            StartDict().
                            Key("type"s).Value("Bus"s).
                            Key("bus"s).Value(static_cast<std::string>(it->route->name)).
                            Key("span_count"s).Value(it->trip.stops_number).
                            Key("time"s).Value(it->trip.travel_time/60.0).
                            EndDict().
                            Build().AsMap());
                        total_time += it->trip.travel_time/60.0;
                    }

                    dict = json::Builder{}.
                        StartDict().
                        Key("request_id"s).Value(stat_query->id).
                        Key("total_time"s).Value(total_time).
                        Key("items"s).Value(arr_items).
                        EndDict().
                        Build().AsMap();
                }

                Arr.emplace_back(std::move(dict));
            }
        }
    }

    // формируем документ
    json::Document document(Arr);

    // печатаем документ
    json::Print(document, out);
}

} // namespace json_reader
