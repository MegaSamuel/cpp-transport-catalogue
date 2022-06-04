#include "json.h"

using namespace std::literals;

namespace json {

// ---> Node

Node::Node(Value& value) {
    this->swap(value);
}

bool Node::IsInt() const {
    return std::holds_alternative<int>(*this);
}

bool Node::IsDouble() const {
    return std::holds_alternative<int>(*this) || std::holds_alternative<double>(*this);
}

bool Node::IsPureDouble() const {
    return std::holds_alternative<double>(*this);
}

bool Node::IsBool() const {
    return std::holds_alternative<bool>(*this);
}

bool Node::IsString() const {
    return std::holds_alternative<std::string>(*this);
}

bool Node::IsNull() const {
    return std::holds_alternative<std::nullptr_t>(*this);
}

bool Node::IsArray() const {
    return std::holds_alternative<Array>(*this);
}

bool Node::IsMap() const {
    return std::holds_alternative<Dict>(*this);
}

int Node::AsInt() const {
    if(!IsInt()) {
        throw std::logic_error("data is not an int"s);
    }
    return std::get<int>(*this);
}

bool Node::AsBool() const {
    if(!IsBool()) {
        throw std::logic_error("data is not a bool"s);
    }
    return std::get<bool>(*this);
}

double Node::AsDouble() const {
    if(!IsDouble()) {
        throw std::logic_error("data is not a double"s);
    }
    return IsPureDouble() ? std::get<double>(*this) : AsInt();
}

const std::string& Node::AsString() const {
    if(!IsString()) {
        throw std::logic_error("data is not a string"s);
    }
    return std::get<std::string>(*this);
}

const Array& Node::AsArray() const {
    if(!IsArray()) {
        throw std::logic_error("data is not an array"s);
    }
    return std::get<Array>(*this);
}

const Dict& Node::AsMap() const {
    if(!IsMap()) {
        throw std::logic_error("data is not a map"s);
    }
    return std::get<Dict>(*this);
}

Node::Value& Node::GetValue() {
    return *this;
}

const Node::Value& Node::GetValue() const {
    return *this;
}

// <--- Node


bool operator==(const Node& lhs, const Node& rhs) {
    return lhs.GetValue() == rhs.GetValue();
}

bool operator!=(const Node& lhs, const Node& rhs) {
    return !(lhs == rhs);
}

namespace {

Number LoadNumber(std::istream& input) {
    std::string parsed_num;

    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }
    // Парсим целую часть числа
    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    } else {
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    // Парсим экспоненциальную часть числа
    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                return std::stoi(parsed_num);
            } catch (...) {
                // В случае неудачи, например, при переполнении,
                // код ниже попробует преобразовать строку в double
            }
        }
        return std::stod(parsed_num);
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

// Считывает содержимое строкового литерала JSON-документа
// Функцию следует использовать после считывания открывающего символа ":
std::string LoadString(std::istream& input) {
    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            // Поток закончился до того, как встретили закрывающую кавычку?
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            // Встретили закрывающую кавычку
            ++it;
            break;
        } else if (ch == '\\') {
            // Встретили начало escape-последовательности
            ++it;
            if (it == end) {
                // Поток завершился сразу после символа обратной косой черты
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
            switch (escaped_char) {
                case 'n':
                    s.push_back('\n');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case '"':
                    s.push_back('"');
                    break;
                case '\\':
                    s.push_back('\\');
                    break;
                default:
                    // Встретили неизвестную escape-последовательность
                    throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            // Строковый литерал внутри- JSON не может прерываться символами \r или \n
            throw ParsingError("Unexpected end of line"s);
        } else {
            // Просто считываем очередной символ и помещаем его в результирующую строку
            s.push_back(ch);
        }
        ++it;
    }

    return s;
}

Node LoadNode(std::istream& input);

Node LoadArray(std::istream& input) {
    Array result;

    for (char c; input >> c && c != ']';) {
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }

    if (!input) {
        throw ParsingError("Failed to parse array node"s);
    }

    return Node(std::move(result));
}

Node LoadNum(std::istream& input) {
    auto num = LoadNumber(input);
    if (std::holds_alternative<double>(num)) {
        return Node(std::get<double>(num));
    } else {
        return Node(std::get<int>(num));
    }
}

Node LoadStr(std::istream& input) {
    std::string line = LoadString(input);

    return Node(move(line));
}

Node LoadDict(std::istream& input) {
    Dict dict;

    for (char c; input >> c && c != '}';) {
        if (c == '"') {
            std::string key = LoadStr(input).AsString();
            if (input >> c && c == ':') {
                if (dict.find(key) != dict.end()) {
                    throw ParsingError("Duplicate key '"s + key + "' has been found");
                }
                dict.emplace(std::move(key), LoadNode(input));
            }
            else {
                throw ParsingError(": is expected but '"s + c + "' has been found"s);
            }
        }
        else if (c != ',') {
            throw ParsingError(R"(',' is expected but ')"s + c + "' has been found"s);
        }
    }
    if (!input) {
        throw ParsingError("Failed to parse dict node"s);
    }
    return Node(std::move(dict));
}

Node LoadNull(std::istream& input) {
    std::string res;

    char c;

    for (int i = 0; i < 4; ++i) {
        if(input.get(c)) {
            res+=c;
        }
    }

    if (res != "null") {
        throw ParsingError("Failed to parse null node");
    }

    return Node();
}

Node LoadBool(std::istream& input) {
    std::string res;
    char c;

    //определяю сколько символов считывать
    c = static_cast<char>(input.peek());
    int length = c == 't' ? 4 : 5;

    for (int i = 0; i < length; ++i) {
        if(input.get(c)) {
            res+=c;
        }
    }

    if (res != "true"s && res != "false"s) {
        throw ParsingError("Failed to parse bool node");
    }

    if (res == "true") {
        return Node(true);
    } else {
        return Node(false);
    }
}

Node LoadNode(std::istream& input) {
    char c;
    input >> c;

    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadStr(input);
    } else if (c == 't') { // true
        input.putback(c);
        return LoadBool(input);
    } else if (c == 'f') { // false
        input.putback(c);
        return LoadBool(input);
    } else if (c == 'n') { // null
        input.putback(c);
        return LoadNull(input);
    } else {
        input.putback(c);
        return LoadNum(input);
    }
}

} // namespace

Document::Document(Node root)
    : root_(move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

bool operator==(const Document& lhs, const Document& rhs) {
    return lhs.GetRoot() == rhs.GetRoot();
}

bool operator!=(const Document& lhs, const Document& rhs) {
    return !(lhs == rhs);
}

Document Load(std::istream& input) {
    return Document{LoadNode(input)};
}

namespace {

// -------------------------- печать нод ----------------------------

// Контекст вывода, хранит ссылку на поток вывода и текущий отсуп
struct PrintContext {
    std::ostream& out;
    int indent_step = 4;
    int indent = 0;

    void PrintIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    // Возвращает новый контекст вывода с увеличенным смещением
    PrintContext Indented() const {
        return {out, indent_step, indent_step + indent};
    }
};

void PrintNode(const Node &node, const PrintContext& ctx);

template <typename Value>
void PrintValue(const Value& value, const PrintContext& ctx) {
    ctx.out << value;
}

void PrintString(const std::string& value, std::ostream& out) {
    out.put('"');
    for (const char c : value) {
        switch (c) {
        case '\r':
            out << "\\r"sv;
            break;
        case '\n':
            out << "\\n"sv;
            break;
        case '"':
            // Символы " и \ выводятся как \" или \\, соответственно
            [[fallthrough]];
        case '\\':
            out.put('\\');
            [[fallthrough]];
        default:
            out.put(c);
            break;
        }
    }
    out.put('"');
}

template <>
void PrintValue<std::string>(const std::string& value, const PrintContext& ctx) {
    PrintString(value, ctx.out);
}

template <>
void PrintValue<std::nullptr_t>(const std::nullptr_t&, const PrintContext& ctx) {
    ctx.out << "null"sv;
}

template <>
void PrintValue<bool>(const bool& value, const PrintContext& ctx) {
    ctx.out << (value ? "true"sv : "false"sv);
}

template <>
void PrintValue<Array>(const Array& nodes, const PrintContext& ctx) {
    std::ostream& out = ctx.out;
    out << "[\n"sv;
    bool first = true;
    auto inner_ctx = ctx.Indented();
    for (const Node& node : nodes) {
        if (first) {
            first = false;
        }
        else {
            out << ",\n"sv;
        }
        inner_ctx.PrintIndent();
        PrintNode(node, inner_ctx);
    }
    out.put('\n');
    ctx.PrintIndent();
    out.put(']');
}

template <>
void PrintValue<Dict>(const Dict& nodes, const PrintContext& ctx) {
    std::ostream& out = ctx.out;
    out << "{\n"sv;
    bool first = true;
    auto inner_ctx = ctx.Indented();
    for (const auto& [key, node] : nodes) {
        if (first) {
            first = false;
        }
        else {
            out << ",\n"sv;
        }
        inner_ctx.PrintIndent();
        PrintString(key, ctx.out);
        out << ": "sv;
        PrintNode(node, inner_ctx);
    }
    out.put('\n');
    ctx.PrintIndent();
    out.put('}');
}

void PrintNode(const Node& node, const PrintContext& ctx) {
    std::visit([&ctx](const auto& value){PrintValue(value, ctx);}, node.GetValue());
}

} // namespace

void Print(const Document& doc, std::ostream& output) {
    PrintNode(doc.GetRoot(), PrintContext{ output });
}

} // namespace json
