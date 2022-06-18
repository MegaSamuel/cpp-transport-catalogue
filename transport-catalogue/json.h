#pragma once

#include <istream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <variant>
#include <stdexcept>

namespace json {

class Node;
using Array = std::vector<Node>;
using Dict = std::map<std::string, Node>;
using Number = std::variant<int, double>;

class ParsingError : public std::runtime_error {
public:
    //! делаем доступными все конструкторы родительского класса
    using runtime_error::runtime_error;
};

class Node : private std::variant<std::nullptr_t, int, double, std::string, bool, Array, Dict> {
public:
    //! делаем доступными все конструкторы родительского класса
    using variant::variant;

    using Value = variant;

    Node(Value& value);

    bool IsInt() const;
    bool IsDouble() const;
    bool IsPureDouble() const;
    bool IsBool() const;
    bool IsString() const;
    bool IsNull() const;
    bool IsArray() const;
    bool IsMap() const;

    int AsInt() const;
    bool AsBool() const;
    double AsDouble() const;
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;

    Value& GetValue();
    const Value& GetValue() const;
};

bool operator==(const Node& lhs, const Node& rhs);
bool operator!=(const Node& lhs, const Node& rhs);

class Document {
public:
    Document() = default;
    explicit Document(Node root);

    const Node& GetRoot() const;

private:
    Node root_;
};

bool operator==(const Document& lhs, const Document& rhs);
bool operator!=(const Document& lhs, const Document& rhs);

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

} // namespace json
