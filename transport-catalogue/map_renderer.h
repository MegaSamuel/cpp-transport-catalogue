#pragma once

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>
#include <string_view>
#include <set>
#include <map>

#include "domain.h"
#include "geo.h"
#include "svg.h"
#include "transport_catalogue.h"

namespace map_renderer {

using namespace std::literals;

struct RenderSettings {
    double width;
    double height;
    double padding;
    double line_width;
    double stop_radius;
    int bus_label_font_size;
    double bus_label_offset[2];
    int stop_label_font_size;
    double stop_label_offset[2];
    svg::Color underlayer_color;
    double underlayer_width;
    std::vector<svg::Color> color_palette;

    RenderSettings();
};

inline const double EPSILON = 1e-6;
bool IsZero(double value);

class SphereProjector {
public:
    // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding);

    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(geo_coord::Coordinates coords) const;

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

class MapRenderer {
public:
    MapRenderer() = default;

    void SetSettings(const RenderSettings& settings);

    void SetBuses(const std::map<std::string_view, const domain::Bus*> buses);

    void SetStopToBuses(const std::unordered_map<const domain::Stop*, std::unordered_set<domain::Bus*>>& stop_to_buses);

    void Render(std::ostream& out);

private:
    RenderSettings settings_;
    svg::Document document_;
    std::map<std::string_view, const domain::Bus*> buses_;
    std::unordered_map<const domain::Stop*, std::unordered_set<domain::Bus*>> stop_to_buses_;

    struct BusSort {
        bool operator()(const domain::Bus* lhs, const domain::Bus* rhs) const {
            return std::lexicographical_compare(lhs->name.begin(), lhs->name.end(),
                rhs->name.begin(), rhs->name.end());
        }
    };

    struct StopSort {
        bool operator()(const domain::Stop* lhs, const domain::Stop* rhs) const {
            return std::lexicographical_compare(lhs->name.begin(), lhs->name.end(),
                rhs->name.begin(), rhs->name.end());
        }
    };

    std::set<const domain::Stop*, StopSort> unique_stops_;

    void RenderBuses(SphereProjector& projector, const std::set<const domain::Bus*, BusSort>& buses_to_render);
    void RenderBusesNames(SphereProjector& projector, const std::set<const domain::Bus*, BusSort>& buses_to_render);

    void RenderStops(SphereProjector& projector);
    void RenderStopsNames(SphereProjector& projector);
};

template <typename PointInputIt>
SphereProjector::SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                                 double max_width, double max_height, double padding)
    : padding_(padding) //
{
    // Если точки поверхности сферы не заданы, вычислять нечего
    if (points_begin == points_end) {
        return;
    }

    // Находим точки с минимальной и максимальной долготой
    const auto [left_it, right_it] = std::minmax_element(
        points_begin, points_end,
        [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
    min_lon_ = left_it->lng;
    const double max_lon = right_it->lng;

    // Находим точки с минимальной и максимальной широтой
    const auto [bottom_it, top_it] = std::minmax_element(
        points_begin, points_end,
        [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
    const double min_lat = bottom_it->lat;
    max_lat_ = top_it->lat;

    // Вычисляем коэффициент масштабирования вдоль координаты x
    std::optional<double> width_zoom;
    if (!IsZero(max_lon - min_lon_)) {
        width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
    }

    // Вычисляем коэффициент масштабирования вдоль координаты y
    std::optional<double> height_zoom;
    if (!IsZero(max_lat_ - min_lat)) {
        height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
    }

    if (width_zoom && height_zoom) {
        // Коэффициенты масштабирования по ширине и высоте ненулевые,
        // берём минимальный из них
        zoom_coeff_ = std::min(*width_zoom, *height_zoom);
    } else if (width_zoom) {
        // Коэффициент масштабирования по ширине ненулевой, используем его
        zoom_coeff_ = *width_zoom;
    } else if (height_zoom) {
        // Коэффициент масштабирования по высоте ненулевой, используем его
        zoom_coeff_ = *height_zoom;
    }
}

} // namespace map_renderer
