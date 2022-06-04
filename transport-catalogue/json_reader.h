#pragma once

#include <memory>

#include "geo.h"
#include "request_handler.h"
#include "json.h"
#include "map_renderer.h"

namespace json_reader {

namespace details {

enum class query_type {
    EMPTY,
    STOP,
    BUS,
    MAP
};

struct Query {
    query_type type = query_type::EMPTY;

    virtual ~Query() = default;
};

struct StopQuery : public Query {
    std::string name;
    geo_coord::Coordinates coordinates{0, 0};
    std::vector<std::pair<int, std::string>> distances;
};

struct BusQuery : public Query {
    std::string name;
    bool is_roundtrip = false;
    std::string name_last_stop;
    std::vector<std::string> stops;
};

struct MapQuery : public Query {
    std::string name;
};

struct StatQuery : public Query {
    std::string name;
    int id = 0;
};

StopQuery QueryStop(const json::Dict& dict);

BusQuery QueryBus(const json::Dict& dict);

MapQuery QueryMap(const json::Dict& dict);

StatQuery QueryStat(const json::Dict& dict);

std::string space_trimmer(const std::string& name);

} // namespace details

class JsonReader {
public:
    explicit JsonReader(transport_catalogue::TransportCatalogue& catalogue, 
                        map_renderer::MapRenderer& renderer);

    void Load(std::istream& input);

    void Parse();

    void Print(std::ostream& out, request_handler::RequestHandler& request_handler);

private:
    transport_catalogue::TransportCatalogue& catalogue_;
    map_renderer::MapRenderer& renderer_;
    std::vector<std::unique_ptr<details::Query>> queries_;

    void LoadBase(const json::Array& vct);
    void LoadStat(const json::Array& vct);
    svg::Color LoadColor(const json::Node& node);
    void LoadRender(const json::Dict& dict);

    int stat_count = 0;
};

} // namespace json_reader
