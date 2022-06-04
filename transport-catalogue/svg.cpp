#include "svg.h"

namespace svg {

using namespace std::literals;

Rgb::Rgb(uint8_t r, uint8_t g, uint8_t b) : red(r), green(g), blue(b) {
}

Rgba::Rgba(uint8_t r, uint8_t g, uint8_t b, double a) : red(r), green(g), blue(b), opacity(a) {
}

std::ostream& operator<<(std::ostream& out, const StrokeLineCap stroke_line_cap) {
    switch (stroke_line_cap) {
        case StrokeLineCap::BUTT:
            out << "butt"sv;
            break;
        case StrokeLineCap::ROUND:
            out << "round"sv;
            break;
        case StrokeLineCap::SQUARE:
            out << "square"sv;
            break;
        }

    return out;
}

std::ostream& operator<<(std::ostream& out, const StrokeLineJoin stroke_line_join) {
    switch (stroke_line_join) {
        case StrokeLineJoin::ARCS:
            out << "arcs"sv;
            break;
        case StrokeLineJoin::BEVEL:
            out << "bevel"sv;
            break;
        case StrokeLineJoin::MITER:
            out << "miter"sv;
            break;
        case StrokeLineJoin::MITER_CLIP:
            out << "miter-clip"sv;
            break;
        case StrokeLineJoin::ROUND:
            out << "round"sv;
            break;
        }
    return out;
}

void SolutionColor::operator()(std::monostate) {
    out << "none"sv;
}

void SolutionColor::operator()(std::string color) {
    out << color;
}

void SolutionColor::operator()(Rgb rgb) {
    out << "rgb("sv << static_cast<int>(rgb.red) << ","sv << static_cast<int>(rgb.green);
    out << ","sv << static_cast<int>(rgb.blue) << ")"sv;
}

void SolutionColor::operator()(Rgba rgba) {
    out << "rgba("sv << static_cast<int>(rgba.red) << ","sv << static_cast<int>(rgba.green);
    out << ","sv << static_cast<int>(rgba.blue) << ","sv << rgba.opacity << ")"sv;
}

std::ostream& operator<<(std::ostream& out, const Color& color) {
    std::visit(SolutionColor{out}, color);
    return out;
}

// ---------- Point ------------------

Point::Point(double x, double y)
    : x(x)
    , y(y) {
}

// ---------- RenderContext ------------------

RenderContext::RenderContext(std::ostream& out)
    : out(out) {
}

RenderContext::RenderContext(std::ostream& out, int indent_step, int indent = 0)
    : out(out)
    , indent_step(indent_step)
    , indent(indent) {
}

RenderContext RenderContext::Indented() const {
    return {out, indent_step, indent + indent_step};
}

void RenderContext::RenderIndent() const {
    for (int i = 0; i < indent; ++i) {
        out.put(' ');
    }
}

// ---------- Object ------------------

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCircle(Point center, double radius)  {
    center_ = center;
    radius_ = radius;
    return *this;
}

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
    RenderAttrs(out);
    out << "/>"sv;
}

// ---------- Polyline ------------------

Polyline& Polyline::AddPoint(Point point) {
    points_.emplace_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\""sv;
    for (size_t i = 0; i < points_.size(); ++i) {
        out << points_[i].x << ","sv << points_[i].y;
        if (i != points_.size() - 1) {
            out << " "sv;
        }
    }
    out << "\""sv;
    RenderAttrs(out);
    out << "/>"sv;
}

// ---------- Text ------------------

Text& Text::SetPosition(Point pos) {
    position_ = pos;
    return *this;
}

Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(uint32_t size) {
    font_size_ = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family) {
    font_family_ = font_family;
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight) {
    font_weight_ = font_weight;
    return *this;
}

Text& Text::SetData(std::string data) {
    data_ = data;
    return *this;
}

void Text::RenderObject(const RenderContext& context) const {
    std::vector<std::pair<char, std::string>> spec_symbols{ {'&', "&amp;"s},{'"', "&quot;"s},{'<', "&lt;"s},{'>', "&gt;"s},{'\'', "&apos;"} };
    std::string out_str = data_;
    size_t pos_space;
    for (auto& [symbol, str] : spec_symbols) {
        while (true) {
            if (out_str.find("&amp;"s) != data_.npos && symbol == '&') {
                break;
            }
            pos_space = out_str.find(symbol);
            if (pos_space != data_.npos) {
                out_str.erase(pos_space, 1u);
                out_str.insert(pos_space, str);
            }
            else {
                break;
            }
        }
    }

    auto& out = context.out;
    out << "<text"sv;
    RenderAttrs(out);
    out << " x=\""sv << position_.x << "\" y=\""sv << position_.y << "\" "sv;
    out << "dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\" "sv;
    out << "font-size=\""sv << font_size_ << "\" "sv;
    if (!font_family_.empty()) {
        out << "font-family=\""sv << font_family_ << "\""sv;
    }
    if (!font_weight_.empty()) {
        out << " font-weight=\""sv << font_weight_ << "\""sv;
    }

    out << ">"sv << out_str << "</text>"sv;
}

// ---------- Document ------------------

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    objects_.emplace_back(std::move(obj));
}

void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"sv;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n"sv;
    for (size_t i = 0; i < objects_.size(); ++i) {
        objects_[i]->Render({ out, 2, 2 });
    }
    out << "</svg>"sv;
}

}  // namespace svg
