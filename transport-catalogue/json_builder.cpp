#include "json_builder.h"

namespace json {

DictKeyContext Builder::Key(std::string key) {
    if ((std::nullopt == m_key) && IsPrevDict()) {
        // нет ключа и добавление идет в словарь
        m_key = std::move(key);
        return *this;
    } else {
        using namespace std::literals;
        throw std::logic_error("Wrong Key call"s);
    }
}

Builder& Builder::Value(json::Node::Value value) {
    AddValue(value);
    return *this;
}

DictItemContext Builder::StartDict() {
    if (IsPrevNull() ||
        IsPrevArray() ||
        (std::nullopt != m_key)) {
        m_vct_node.emplace_back(std::move(AddValue(json::Dict())));
        dict_started = true;
        return *this;
    } else {
        using namespace std::literals;
        throw std::logic_error("Wrong StartDict call"s);
    }
}

ArrayItemContext Builder::StartArray() {
    if (IsPrevNull() ||
        IsPrevArray() ||
        (std::nullopt != m_key)) {
        m_vct_node.emplace_back(std::move(AddValue(json::Array())));
        array_started = true;
        return *this;
    } else {
        using namespace std::literals;
        throw std::logic_error("Wrong StartArray call"s);
    }
}

Builder& Builder::EndDict() {
    if (IsPrevDict()) {
        // словарь можно закончить только если он начинался
        m_vct_node.pop_back();
        dict_started = false;
        return *this;
    } else {
        using namespace std::literals;
        throw std::logic_error("Wrong EndDict call"s);
    }
}

Builder& Builder::EndArray() {
    if (IsPrevArray()) {
        // массив можно закончить только если он начинался
        m_vct_node.pop_back();
        array_started = false;
        return *this;
    } else {
        using namespace std::literals;
        throw std::logic_error("Wrong EndArray call"s);
    }
}

json::Node Builder::Build() {
    if (IsObjExist() && !dict_started && !array_started) {
        // строить можно только если есть из чего
        return m_root;
    } else {
        using namespace std::literals;
        throw std::logic_error("Wrong Build call"s);
    }
}

json::Node* Builder::AddValue(json::Node::Value value) {
    if (IsPrevNull()) {
        // добавляем значение
        m_vct_node.back()->GetValue() = std::move(value);
        return m_vct_node.back();
    } else if ((std::nullopt != m_key) && IsPrevDict()) {
        // добавляем значение в словарь
        json::Dict& dict = std::get<json::Dict>(m_vct_node.back()->GetValue());
        auto entry = dict.emplace(m_key.value(), value);
        // обнуляем ключ!
        m_key = std::nullopt;
        return &entry.first->second;
    } else if (IsPrevArray()) {
        // добавляем значение в массив
        json::Array& array = std::get<json::Array>(m_vct_node.back()->GetValue());
        return &array.emplace_back(value);
    } else {
        using namespace std::literals;
        throw std::logic_error("Wrong Value call"s);
    }
}

bool Builder::IsObjExist() const {
    return (1 == m_vct_node.size()) && !m_vct_node.back()->IsNull();
}

bool Builder::IsPrevNull() const {
    return !m_vct_node.empty() && m_vct_node.back()->IsNull();
}

bool Builder::IsPrevArray() const {
    return !m_vct_node.empty() && m_vct_node.back()->IsArray();
}

bool Builder::IsPrevDict() const {
    return !m_vct_node.empty() && m_vct_node.back()->IsDict();
}

// -->

DictItemContext DictKeyContext::Value(json::Node::Value value) {
    return DictItemContext(m_builder.Value(value));
}

DictItemContext DictKeyContext::StartDict() {
    return m_builder.StartDict();
}

DictItemContext ArrayItemContext::StartDict() {
    return m_builder.StartDict();
}

DictKeyContext DictItemContext::Key(std::string key) {
    return m_builder.Key(key);
}


ArrayItemContext DictKeyContext::StartArray() {
    return m_builder.StartArray();
}

ArrayItemContext ArrayItemContext::Value(json::Node::Value value) {
    return ArrayItemContext(m_builder.Value(value));
}

ArrayItemContext ArrayItemContext::StartArray() {
    return m_builder.StartArray();
}

Builder& DictItemContext::EndDict() {
    return m_builder.EndDict();
}

Builder& ArrayItemContext::EndArray() {
    return m_builder.EndArray();
}

// <--

} // namespace json
