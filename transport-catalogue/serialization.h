#pragma once

#include <filesystem>
#include <graph.pb.h>
#include <map_renderer.pb.h>
#include <svg.pb.h>
#include <transport_catalogue.pb.h>
#include <transport_router.pb.h>

#include "map_renderer.h"
#include "transport_router.h"

namespace serialization {

struct SerializatorSettings {
    std::filesystem::path path;
};

class Serializator
{
public:
    Serializator(transport_catalogue::TransportCatalogue& catalogue,
                 map_renderer::MapRenderer& renderer,
                 transport_router::TransportRouter& router);

    void SetSettings(const SerializatorSettings& settings);

    void Serialize();

    void Deserialize();

private:
    transport_catalogue::TransportCatalogue& catalogue_;
    map_renderer::MapRenderer& renderer_;
    transport_router::TransportRouter& router_;
    SerializatorSettings settings_;
    mutable pr_transport_catalogue::TransportCatalogue pr_catalogue_;

    pr_transport_catalogue::Stop GetStop(const domain::Stop& stop) const;
    pr_transport_catalogue::Bus GetBus(const domain::Bus& bus) const;

    // кладем в файл
    void WriteStops();
    void WriteBuses();
    void WriteRender();
    void WriteRouter();

    // берем из файла
    void ReadStops();
    void ReadBuses();
    void ReadRender();
    void ReadRouter();

    // прото конвертеры
    pr_transport_catalogue::Stop StopToPrStop(const domain::Stop& stop) const;
    pr_transport_catalogue::Bus BusToPrBus(const domain::Bus& bus) const;
    void UnderlayerColorToPrColor(pr_map_renderer::RenderSettings& pr_settings, const svg::Color& color);
    void PaletteColorToPrColor(pr_map_renderer::RenderSettings& pr_settings, const std::vector<svg::Color>& Colors);
    pr_map_renderer::RenderSettings RenderToPrRender(const map_renderer::RenderSettings& settings);
    pr_transport_router::RouterSettings RouterToPrRouter(const transport_router::RouterSettings& settings);

    // конвертеры
    void PrStopToStop(const pr_transport_catalogue::Stop& pr_stop);
    void PrBusToBus(const pr_transport_catalogue::Bus& pr_bus);
    svg::Color PrColorToColor(const pr_svg::Color& pr_color);
    void PrRenderToRender(const pr_map_renderer::RenderSettings& pr_settings);
    void PrRouterToRouter(const pr_transport_router::RouterSettings& pr_settings);
};

} // namespace serialization
