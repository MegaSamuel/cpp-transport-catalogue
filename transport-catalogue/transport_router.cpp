#include "transport_router.h"

namespace transport_router {

// ---> RouteProperties

RouteProperties::RouteProperties(int stops_number, double waiting_time, double travel_time) :
                                 stops_number(stops_number),
                                 waiting_time(waiting_time),
                                 travel_time(travel_time) {
}

RouteProperties operator+(const RouteProperties& lhs, const RouteProperties& rhs)
{
    return {lhs.stops_number + rhs.stops_number,
            lhs.waiting_time + rhs.waiting_time,
            lhs.travel_time + rhs.travel_time};
}

bool operator<(const RouteProperties& lhs, const RouteProperties& rhs)
{
    return (lhs.waiting_time + lhs.travel_time) < (rhs.waiting_time + rhs.travel_time);
}

bool operator>(const RouteProperties& lhs, const RouteProperties& rhs)
{
    return (lhs.waiting_time + lhs.travel_time) > (rhs.waiting_time + rhs.travel_time);
}

// <--- RouteProperties

// ---> DistanceCalculator

DistanceCalculator::DistanceCalculator(const transport_catalogue::TransportCatalogue& catalogue,
                                       const domain::Bus* route) :
                                       forward_dist_(route->stops.size(), 0),
                                       reverse_dist_(route->stops.size(), 0) {
    int forward = 0;
    int reverse = 0;

    for (int i = 0; i < static_cast<int>(route->stops.size()-1); ++i) {
        forward += catalogue.getDistance(route->stops[i], route->stops[i + 1]);
        forward_dist_[i + 1] = forward;

        if (!route->is_roundtrip) {
            reverse += catalogue.getDistance(route->stops[i + 1], route->stops[i]);
            reverse_dist_[i + 1] = reverse;
        }
    }
}

int DistanceCalculator::GetDistanceBetween(int from, int to) {
    if (from < to) {
        return forward_dist_[to] - forward_dist_[from];
    }
    else {
        return reverse_dist_[from] - reverse_dist_[to];
    }
}

// <--- DistanceCalculator

// ---> RouteConditions

RouteConditions::RouteConditions(const domain::Stop* from, const domain::Stop* to,
                                 const domain::Bus* route, RouteProperties route_prop) :
                                 from(from),
                                 to(to),
                                 route(route),
                                 trip(route_prop) {
}
// <--- RouteConditions

// ---> TransportRouter

TransportRouter::TransportRouter(const transport_catalogue::TransportCatalogue& catalogue) :
                                 catalogue_(catalogue) {
}

void TransportRouter::SetSettings(const RouterSettings& settings) {
    settings_ = settings;
}

void TransportRouter::CalcRoute() {
    // все остановки
    const auto& stops = catalogue_.getStops();

    graph_ = std::make_unique<graph::DirectedWeightedGraph<RouteProperties>>(stops.size());

    graph::VertexId vertex_counter = 0;

    // добавляем все остановки
    for (const auto& [stop_name, stop_ptr] : stops) {
        graph_vertexes_.insert({stop_ptr, vertex_counter++});
    }

    // все маршруты
    const auto& buses = catalogue_.getBuses();

    // добавляем все маршруты
    for (const auto& [bus_name, bus_ptr] : buses) {
        const auto& stops = bus_ptr->stops;

        DistanceCalculator distance_calc(catalogue_, bus_ptr);

        for (int i = 0; i < static_cast<int>(stops.size()-1); ++i) {
            for (int j = i + 1; j < static_cast<int>(stops.size()); ++j) {
                RouteProperties route_prop(j-i,
                                           settings_.bus_wait_time,
                                           distance_calc.GetDistanceBetween(i, j)/settings_.bus_velocity);

                RouteConditions route_cond{stops[i], stops[j], bus_ptr, route_prop};

                graph_->AddEdge(graph::Edge<RouteProperties>{graph_vertexes_[route_cond.from],
                                                             graph_vertexes_[route_cond.to],
                                                             route_prop});
                graph_edges_.emplace_back(std::move(route_cond));

                if (!bus_ptr->is_roundtrip) {
                    RouteProperties route_prop(j-i,
                                               settings_.bus_wait_time,
                                               distance_calc.GetDistanceBetween(j, i)/settings_.bus_velocity);

                    RouteConditions route_cond{stops[i], stops[j], bus_ptr, route_prop};

                    graph_->AddEdge(graph::Edge<RouteProperties>{graph_vertexes_[route_cond.to],
                                                                 graph_vertexes_[route_cond.from],
                                                                 route_prop});
                    graph_edges_.emplace_back(std::move(route_cond));
                }
            }
        }
    }

    router_ = std::make_unique<graph::Router<RouteProperties>>(*graph_);
}

std::optional<std::vector<const RouteConditions*>>
TransportRouter::GetRoute(std::string_view from, std::string_view to) const {
    const domain::Stop* stop_from = catalogue_.findStop(from);
    const domain::Stop* stop_to = catalogue_.findStop(to);

    // не смогли найти одну из остановок
    if ((nullptr == stop_from) ||
        (nullptr == stop_to)) {
        return std::nullopt;
    }

    std::vector<const RouteConditions*> result;

    // первая и последняя остановки совпадают
    if (stop_from == stop_to) {
        return result;
    }

    if (graph_vertexes_.empty()) {
        return std::nullopt;
    }

    // вершины графа
    graph::VertexId vertex_from = graph_vertexes_.at(stop_from);
    graph::VertexId vertex_to = graph_vertexes_.at(stop_to);

    // строим граф
    auto route = router_->BuildRoute(vertex_from, vertex_to);

    // если не смогли построить
    if (!route.has_value()) {
        return std::nullopt;
    }

    // выделяем память
    result.reserve(route.value().edges.size());

    // если смогли построить
    for (const auto& edge : route.value().edges) {
        result.emplace_back(&graph_edges_.at(edge));
    }

    return result;
}

// <--- TransportRouter

} // namespace transport_router
