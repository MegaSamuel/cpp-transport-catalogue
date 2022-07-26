#pragma once

#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

#include <unordered_map>
#include <vector>
#include <memory>

namespace transport_router {

struct RouterSettings {
    int bus_wait_time;
    double bus_velocity;
};

struct RouteProperties {
    int stops_number = 0;
    double waiting_time = 0;
    double travel_time = 0;

    RouteProperties() = default;
    RouteProperties(int stops_number, double waiting_time, double travel_time);
};

RouteProperties operator+(const RouteProperties& lhs, const RouteProperties& rhs);

bool operator<(const RouteProperties& lhs, const RouteProperties& rhs);

bool operator>(const RouteProperties& lhs, const RouteProperties& rhs);

class DistanceCalculator {
public:
    explicit DistanceCalculator(const transport_catalogue::TransportCatalogue& catalogue,
                                const domain::Bus* route);

    int GetDistanceBetween(int from, int to);

private:
    std::vector<int> forward_dist_;
    std::vector<int> reverse_dist_;
};

struct RouteConditions {
    const domain::Stop* from = nullptr;
    const domain::Stop* to = nullptr;
    const domain::Bus* route = nullptr;
    RouteProperties trip = {};

    RouteConditions() = default;
    RouteConditions(const domain::Stop* from, const domain::Stop* to,
                    const domain::Bus* route, const RouteProperties route_prop);
};

class TransportRouter {
public:
    explicit TransportRouter(const transport_catalogue::TransportCatalogue& catalogue);

    void SetSettings(const RouterSettings& settings);
    const RouterSettings& GetSettings() const;

    void CalcRoute();

    std::optional<std::vector<const RouteConditions*>> GetRoute(std::string_view from, std::string_view to) const;

private:
    const transport_catalogue::TransportCatalogue& catalogue_;

    RouterSettings settings_ = {};

    std::unique_ptr<graph::DirectedWeightedGraph<RouteProperties>> graph_ = nullptr;
    std::unique_ptr<graph::Router<RouteProperties>> router_ = nullptr;
    std::unordered_map<const domain::Stop*, graph::VertexId> graph_vertexes_ = {};
    std::vector<RouteConditions> graph_edges_ = {};
};

} // namespace transport_router
