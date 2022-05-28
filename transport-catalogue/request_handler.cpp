#include "request_handler.h"

namespace request_handler {

RequestHandler::RequestHandler(const transport_catalogue::TransportCatalogue& catalogue, 
                               map_renderer::MapRenderer& renderer) : 
                               catalogue_(catalogue), renderer_(renderer) {

}

void RequestHandler::RenderMap(std::ostream& out) const {
    RenderSetBuses();
    RenderSetStopToBuses();

    renderer_.Render(out);
}

void RequestHandler::RenderSetBuses() const {
    std::map<std::string_view, const domain::Bus*> buses;
    for (const auto& [name, bus] : catalogue_.getBuses()) {
        buses[name] = bus;
    }
    renderer_.SetBuses(buses);
}

void RequestHandler::RenderSetStopToBuses() const {
    renderer_.SetStopToBuses(catalogue_.getStopToBuses());
}

} // namespace request_handler
