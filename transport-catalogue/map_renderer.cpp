#include <iostream>

#include "map_renderer.h"

namespace map_renderer {

RenderSettings::RenderSettings() :
    width(600.0),
    height(400.0),
    padding(50.0),
    line_width(14.0),
    stop_radius(5.0),
    bus_label_font_size(20),
    bus_label_offset{7.0, 15.0},
    stop_label_font_size(20),
    stop_label_offset{7.0, -3.0},
    underlayer_color(svg::Rgba{255, 255, 255, 0.85}),
    underlayer_width(3.0)
    {
}

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

svg::Point SphereProjector::operator()(geo_coord::Coordinates coords) const {
    return {
        (coords.lng - min_lon_) * zoom_coeff_ + padding_,
        (max_lat_ - coords.lat) * zoom_coeff_ + padding_
    };
}

void MapRenderer::SetSettings(const RenderSettings& settings) {
    settings_ = settings;
}

const RenderSettings& MapRenderer::GetSettings() const {
    return settings_;
}

void MapRenderer::SetBuses(const std::map<std::string_view, const domain::Bus*> buses) {
    buses_ = buses;
}

void MapRenderer::SetStopToBuses(const std::unordered_map<const domain::Stop*, std::unordered_set<domain::Bus*>>& stop_to_buses) {
    stop_to_buses_ = stop_to_buses;
}

void MapRenderer::Render(std::ostream& out) {
    std::vector<geo_coord::Coordinates> coordinates;

    // уникальные остановки по всем маршрутам
    for (const auto& [stop, buses] : stop_to_buses_) {
        if (!buses.empty()) {
            unique_stops_.emplace(stop);
        }
    }

    // координаты остановок
    for(const domain::Stop* stop : unique_stops_) {
        coordinates.emplace_back(stop->coordinates);
    }

    SphereProjector projector(coordinates.begin(), coordinates.end(), settings_.width, settings_.height, settings_.padding);

    std::set<const domain::Bus*, BusSort> buses_to_render;
    for (const auto& [name, bus] : buses_) {
        if (!bus->stops.empty()) {
            buses_to_render.emplace(bus);
        }
    }

    RenderBuses(projector, buses_to_render);
    RenderBusesNames(projector, buses_to_render);

    RenderStops(projector);
    RenderStopsNames(projector);

    document_.Render(out);
}

bool MapRenderer::BusSort::operator()(const domain::Bus* lhs, const domain::Bus* rhs) const {
    return std::lexicographical_compare(lhs->name.begin(), lhs->name.end(),
        rhs->name.begin(), rhs->name.end());
}

bool MapRenderer::StopSort::operator()(const domain::Stop* lhs, const domain::Stop* rhs) const {
    return std::lexicographical_compare(lhs->name.begin(), lhs->name.end(),
        rhs->name.begin(), rhs->name.end());
}

void MapRenderer::RenderBuses(SphereProjector& projector, const std::set<const domain::Bus*, BusSort>& buses_to_render) {
    int color_index = 0;

    for (const auto& bus : buses_to_render) {
        svg::Polyline line;

        for (const domain::Stop* stop : bus->stops) {
            auto stop_it = unique_stops_.find(stop);
            if (stop_it != unique_stops_.end()) {
                line.AddPoint(projector((*stop_it)->coordinates));
            }
        }

        line.SetFillColor("none");
        line.SetStrokeColor(settings_.color_palette[color_index % settings_.color_palette.size()]);
        line.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
        line.SetStrokeWidth(settings_.line_width);
        line.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        color_index++;

        document_.Add(line);
    }
}

void MapRenderer::RenderBusesNames(SphereProjector& projector, const std::set<const domain::Bus*, BusSort>& buses_to_render) {
    int color_index = 0;

    for (const auto& bus : buses_to_render) {
        std::vector<const domain::Stop*> stops;

        if (bus->is_roundtrip) {
            // кольцевой маршрут
            stops.push_back(bus->stops.front());
        } else {
            // не кольцевой маршрут
            stops.push_back(bus->stops.front());

            if(bus->stops.front() != bus->last_stop) {
                // конечные не совпадают
                stops.push_back(bus->last_stop);
            }
        }

        for (const auto& stop : stops) {
            svg::Text text_upper;
            svg::Text text_bottom;

            text_bottom.SetFontFamily("Verdana"s);
            text_bottom.SetOffset({settings_.bus_label_offset[0], settings_.bus_label_offset[1]});
            text_bottom.SetFontSize(settings_.bus_label_font_size);
            text_bottom.SetFontWeight("bold"s);
            text_bottom.SetStrokeColor(settings_.underlayer_color);
            text_bottom.SetFillColor(settings_.underlayer_color);
            text_bottom.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
            text_bottom.SetStrokeWidth(settings_.underlayer_width);
            text_bottom.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
            text_bottom.SetPosition(projector(stop->coordinates));
            text_bottom.SetData(static_cast<std::string>(bus->name));

            text_upper.SetFontFamily("Verdana"s);
            text_upper.SetOffset({settings_.bus_label_offset[0], settings_.bus_label_offset[1]});
            text_upper.SetFontSize(settings_.bus_label_font_size);
            text_upper.SetFontWeight("bold"s);
            text_upper.SetFillColor(settings_.color_palette[color_index % settings_.color_palette.size()]);
            text_upper.SetPosition(projector(stop->coordinates));
            text_upper.SetData(static_cast<std::string>(bus->name));

            document_.Add(text_bottom);
            document_.Add(text_upper);
        }

        color_index++;
    }
}

void MapRenderer::RenderStops(SphereProjector& projector) {
    for (const domain::Stop* stop : unique_stops_) {
        svg::Circle circle;

        circle.SetRadius(settings_.stop_radius);
        circle.SetFillColor("white"s);
        circle.SetCenter(projector(stop->coordinates));

        document_.Add(circle);
    }
}

void MapRenderer::RenderStopsNames(SphereProjector& projector) {
    for (const domain::Stop* stop : unique_stops_) {
        svg::Text text_upper;
        svg::Text text_bottom;

        text_bottom.SetFontFamily("Verdana"s);
        text_bottom.SetOffset({settings_.stop_label_offset[0], settings_.stop_label_offset[1]});
        text_bottom.SetFontSize(settings_.stop_label_font_size);
        text_bottom.SetStrokeColor(settings_.underlayer_color);
        text_bottom.SetFillColor(settings_.underlayer_color);
        text_bottom.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
        text_bottom.SetStrokeWidth(settings_.underlayer_width);
        text_bottom.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        text_bottom.SetData(static_cast<std::string>(stop->name));
        text_bottom.SetPosition(projector(stop->coordinates));

        text_upper.SetFontFamily("Verdana"s);
        text_upper.SetOffset({settings_.stop_label_offset[0], settings_.stop_label_offset[1]});
        text_upper.SetFontSize(settings_.stop_label_font_size);
        text_upper.SetFillColor("black");
        text_upper.SetData(static_cast<std::string>(stop->name));
        text_upper.SetPosition(projector(stop->coordinates));

        document_.Add(text_bottom);
        document_.Add(text_upper);
    }
}

} // namespace map_renderer
