#pragma once

#include <unordered_set>
#include <optional>

#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"

namespace request_handler {

class RequestHandler {
public:
    explicit RequestHandler(const transport_catalogue::TransportCatalogue& catalogue,
                            map_renderer::MapRenderer& renderer,
                            transport_router::TransportRouter& router);

    void RenderMap(std::ostream& out) const;

private:
    // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
    const transport_catalogue::TransportCatalogue& catalogue_;
    map_renderer::MapRenderer& renderer_;
    transport_router::TransportRouter& router_;

    void RenderSetBuses() const;

    void RenderSetStopToBuses() const;
};

} // namespace request_handler
