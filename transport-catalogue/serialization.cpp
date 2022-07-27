#include "serialization.h"

#include <fstream>
#include <sstream>

using namespace std;

namespace serialization {

Serializator::Serializator(transport_catalogue::TransportCatalogue& catalogue,
                           map_renderer::MapRenderer& renderer,
                           transport_router::TransportRouter& router) :
                           catalogue_(catalogue),
                           renderer_(renderer),
                           router_(router) {

}

void Serializator::SetSettings(const SerializatorSettings& settings) {
    settings_ = settings;
}

void Serializator::Serialize() {
    ofstream out_file(settings_.path, ios::binary);

    // подготовка к записи
    WriteStops();
    WriteBuses();
    WriteRender();
    WriteRouter();

    // пишем в файл
    pr_catalogue_.SerializeToOstream(&out_file);
}

void Serializator::Deserialize() {
    ifstream in_file(settings_.path, ios::binary);

    // читаем из файла
    pr_catalogue_.ParseFromIstream(&in_file);

    // разбираем что прочитали
    ReadStops();
    ReadBuses();
    ReadRender();
    ReadRouter();

    // строим маршрут
    router_.CalcRoute();
}

// --> serialization

// конвертация остановки каталога в прото остановку
pr_transport_catalogue::Stop Serializator::StopToPrStop(const domain::Stop& stop) const {
    pr_transport_catalogue::Stop pr_stop;

    pr_stop.set_name(static_cast<string>(stop.name));

    pr_transport_catalogue::Coordinates pr_coordinates;
    pr_coordinates.set_lat(stop.coordinates.lat);
    pr_coordinates.set_lng(stop.coordinates.lng);
    *pr_stop.mutable_coordinates() = move(pr_coordinates);

    unordered_map<string, vector<pair<int, string>>> map_stop_dist = catalogue_.getStopDistance();
    auto it = map_stop_dist.find(static_cast<string>(stop.name));
    if (it != map_stop_dist.end()) {
        for (const auto& [dist, stop_to] : it->second) {
            pr_transport_catalogue::Distance pr_distance;
            pr_distance.set_dist(dist);
            pr_distance.set_stop_to(stop_to);
            *pr_stop.add_distances() = move(pr_distance);
        }
    }

    return pr_stop;
}

// конвертация автобуса каталога в прото автобус
pr_transport_catalogue::Bus Serializator::BusToPrBus(const domain::Bus& bus) const {
    pr_transport_catalogue::Bus pr_bus;

    pr_bus.set_name(static_cast<string>(bus.name));
    pr_bus.set_last_stop(static_cast<string>(bus.last_stop->name));
    pr_bus.set_is_roundtrip(bus.is_roundtrip);

    for (const auto& stop : bus.stops) {
        pr_bus.add_bus_stops(static_cast<string>(stop->name));
    }

    return pr_bus;
}

void Serializator::UnderlayerColorToPrColor(pr_map_renderer::RenderSettings& pr_settings, const svg::Color& color) {
    if (holds_alternative<svg::Rgb>(color)) {
        pr_svg::Rgb color_rgb;
        svg::Rgb rgb = get<svg::Rgb>(color);
        color_rgb.set_red(rgb.red);
        color_rgb.set_green(rgb.green);
        color_rgb.set_blue(rgb.blue);
        *pr_settings.mutable_underlayer_color()->mutable_color_rgb() = color_rgb;
    }
    else if (holds_alternative<svg::Rgba>(color)){
        pr_svg::Rgba color_rgba;
        svg::Rgba rgba = get<svg::Rgba>(color);
        color_rgba.set_red(rgba.red);
        color_rgba.set_green(rgba.green);
        color_rgba.set_blue(rgba.blue);
        color_rgba.set_opacity(rgba.opacity);
        *pr_settings.mutable_underlayer_color()->mutable_color_rgba() = color_rgba;
    }
    else {
        string color_string = get<string>(color);
        pr_settings.mutable_underlayer_color()->set_color_string(color_string);
    }
}

void Serializator::PaletteColorToPrColor(pr_map_renderer::RenderSettings& pr_settings, const vector<svg::Color>& Colors) {
    for (const svg::Color& color : Colors) {
        if (holds_alternative<svg::Rgb>(color)) {
            pr_svg::Rgb color_rgb;
            svg::Rgb rgb = get<svg::Rgb>(color);
            color_rgb.set_red(rgb.red);
            color_rgb.set_green(rgb.green);
            color_rgb.set_blue(rgb.blue);
            pr_svg::Color pr_color;
            *pr_color.mutable_color_rgb() = color_rgb;
            *pr_settings.add_color_palette() = pr_color;
        }
        else if (holds_alternative<svg::Rgba>(color)){
            pr_svg::Rgba color_rgba;
            svg::Rgba rgba = get<svg::Rgba>(color);
            color_rgba.set_red(rgba.red);
            color_rgba.set_green(rgba.green);
            color_rgba.set_blue(rgba.blue);
            color_rgba.set_opacity(rgba.opacity);
            pr_svg::Color pr_color;
            *pr_color.mutable_color_rgba() = color_rgba;
            *pr_settings.add_color_palette() = pr_color;
        }
        else {
            string color_str = get<string>(color);
            pr_svg::Color pr_color;
            pr_color.set_color_string(color_str);
            *pr_settings.add_color_palette() = pr_color;
        }
    }
}

// конвертация настроек карты в прото настройки
pr_map_renderer::RenderSettings Serializator::RenderToPrRender(const map_renderer::RenderSettings& settings) {
    pr_map_renderer::RenderSettings pr_renderer_settings;

    pr_renderer_settings.set_width(settings.width);
    pr_renderer_settings.set_height(settings.height);
    pr_renderer_settings.set_padding(settings.padding);
    pr_renderer_settings.set_line_width(settings.line_width);
    pr_renderer_settings.set_stop_radius(settings.stop_radius);
    pr_renderer_settings.set_bus_label_font_size(settings.bus_label_font_size);

    pr_svg::Point bus_offset;
    bus_offset.set_x(settings.bus_label_offset[0]);
    bus_offset.set_y(settings.bus_label_offset[1]);
    *pr_renderer_settings.mutable_bus_label_offset() = bus_offset;

    pr_renderer_settings.set_stop_label_font_size(settings.stop_label_font_size);

    pr_svg::Point stop_offset;
    stop_offset.set_x(settings.stop_label_offset[0]);
    stop_offset.set_y(settings.stop_label_offset[1]);
    *pr_renderer_settings.mutable_stop_label_offset() = stop_offset;

    UnderlayerColorToPrColor(pr_renderer_settings, settings.underlayer_color);

    pr_renderer_settings.set_underlayer_width(settings.underlayer_width);

    PaletteColorToPrColor(pr_renderer_settings, settings.color_palette);

    return pr_renderer_settings;
}

// конвертация настроек роутера в прото настройки
pr_transport_router::RouterSettings Serializator::RouterToPrRouter(const transport_router::RouterSettings& settings) {
    pr_transport_router::RouterSettings pr_settings;

    pr_settings.set_bus_wait_time(settings.bus_wait_time);
    pr_settings.set_bus_velocity(settings.bus_velocity);

    return pr_settings;
}

// берем все остановки из каталога и кладем в файл
void Serializator::WriteStops() {
    for (const auto [name, stop] : catalogue_.getStops()) {
        *pr_catalogue_.add_stops() = move(StopToPrStop(*stop));
    }
}

// берем все автобусы из каталога и кладем в файл
void Serializator::WriteBuses() {
    for (const auto [name, bus] : catalogue_.getBuses()) {
        *pr_catalogue_.add_buses() = move(BusToPrBus(*bus));
    }
}

// берем настройки карты и кладем в файл
void Serializator::WriteRender() {
    *pr_catalogue_.mutable_render_settings() = move(RenderToPrRender(renderer_.GetSettings()));
}

// берем настройки роутера и кладем в файл
void Serializator::WriteRouter() {
    *pr_catalogue_.mutable_router_settings() = move(RouterToPrRouter(router_.GetSettings()));
}

// <-- serialization

// --> deserialization

// конвертация прото остановки в остановку каталога
void Serializator::PrStopToStop(const pr_transport_catalogue::Stop& pr_stop) {
    geo_coord::Coordinates coordinates;

    string name = pr_stop.name();
    coordinates.lat = pr_stop.coordinates().lat();
    coordinates.lng = pr_stop.coordinates().lng();

    catalogue_.addStop(name, coordinates);
}

// конвертация прото автобуса в автобус каталога
void Serializator::PrBusToBus(const pr_transport_catalogue::Bus& pr_bus) {
    string name = pr_bus.name();
    string last_stop = pr_bus.last_stop();
    bool is_roundtrip = pr_bus.is_roundtrip();

    vector<string> stops;
    size_t sz = pr_bus.bus_stops_size();
    stops.reserve(sz);

    for (int i = 0; i < sz; ++i) {
        stops.push_back(pr_bus.bus_stops(i));
    }

    // добавляем маршруты
    catalogue_.addBus(name, last_stop, is_roundtrip, stops);

    // формируем информацию о маршруте
    transport_catalogue::BusInfo info;

    const domain::Bus* bus = catalogue_.findBus(name);

    unordered_set<string_view> stop_storage;

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

svg::Color Serializator::PrColorToColor(const pr_svg::Color& pr_color) {
    if (pr_color.has_color_rgb()) {
        return svg::Color{svg::Rgb{static_cast<uint8_t>(pr_color.color_rgb().red()),
                                   static_cast<uint8_t>(pr_color.color_rgb().green()),
                                   static_cast<uint8_t>(pr_color.color_rgb().blue())}};
    }
    else if (pr_color.has_color_rgba()) {
        return svg::Color{svg::Rgba{static_cast<uint8_t>(pr_color.color_rgba().red()),
                                    static_cast<uint8_t>(pr_color.color_rgba().green()),
                                    static_cast<uint8_t>(pr_color.color_rgba().blue()),
                                    pr_color.color_rgba().opacity()}};
    }
    else {
        return svg::Color{pr_color.color_string()};
    }

    return svg::Color{};
}

// конвертация прото настроек карты в просто настройки
void Serializator::PrRenderToRender(const pr_map_renderer::RenderSettings& pr_settings) {
    map_renderer::RenderSettings settings;

    settings.width = pr_settings.width();
    settings.height = pr_settings.height();
    settings.padding = pr_settings.padding();
    settings.line_width = pr_settings.line_width();
    settings.stop_radius = pr_settings.stop_radius();
    settings.bus_label_font_size = pr_settings.bus_label_font_size();

    const pr_svg::Point& bus_offset = pr_settings.bus_label_offset();
    settings.bus_label_offset[0] = bus_offset.x();
    settings.bus_label_offset[1] = bus_offset.y();

    settings.stop_label_font_size = pr_settings.stop_label_font_size();

    const pr_svg::Point& stop_offset = pr_settings.stop_label_offset();
    settings.stop_label_offset[0] = stop_offset.x();
    settings.stop_label_offset[1] = stop_offset.y();

    settings.underlayer_color = PrColorToColor(pr_settings.underlayer_color());

    settings.underlayer_width = pr_settings.underlayer_width();

    size_t color_palette_size = pr_settings.color_palette_size();
    settings.color_palette.reserve(color_palette_size);

    for (int i = 0; i < color_palette_size; ++i) {
        settings.color_palette.emplace_back(PrColorToColor(pr_settings.color_palette(i)));
    }

    renderer_.SetSettings(settings);
}

// конвертация прото настроек роутера в просто настройки
void Serializator::PrRouterToRouter(const pr_transport_router::RouterSettings& pr_settings) {
    transport_router::RouterSettings settings;

    settings.bus_wait_time = pr_settings.bus_wait_time();
    settings.bus_velocity = pr_settings.bus_velocity();

    router_.SetSettings(settings);
}

// берем все прото остановки и кладем в каталог
void Serializator::ReadStops() {
    for (int i = 0; i < pr_catalogue_.stops_size(); ++i) {
        // добавляем в каталог остановки
        PrStopToStop(pr_catalogue_.stops(i));
    }

    for (int i = 0; i < pr_catalogue_.stops_size(); ++i) {
        const pr_transport_catalogue::Stop& pr_stop = pr_catalogue_.stops(i);

        for (int j = 0; j < pr_stop.distances_size(); ++j) {
            // добавляем в каталог расстояния между остановками
            const pr_transport_catalogue::Distance& pr_distance = pr_stop.distances(j);

            string name = pr_stop.name();
            int dist = pr_distance.dist();
            string stop_to = pr_distance.stop_to();

            catalogue_.setDistance(name, stop_to, dist);
        }
    }
}

// берем все прото автобусы и кладем в каталог
void Serializator::ReadBuses() {
    for (int i = 0; i < pr_catalogue_.buses_size(); ++i) {
        PrBusToBus(pr_catalogue_.buses(i));
    }
}

void Serializator::ReadRender() {
    PrRenderToRender(pr_catalogue_.render_settings());
}

void Serializator::ReadRouter() {
    PrRouterToRouter(pr_catalogue_.router_settings());
}

// <-- deserialization

} // namespace serialization
