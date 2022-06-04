#pragma once

#include <vector>
#include <optional>

#include "json.h"

namespace json {

class DictItemContext;
class DictKeyContext;
class ArrayItemContext;

class Builder {
public:
    Builder() : m_root(nullptr), m_key(std::nullopt), m_vct_node{&m_root} {
    }
    
    DictKeyContext Key(std::string key);
    
    Builder& Value(json::Node::Value value);
    
    DictItemContext StartDict();
    
    ArrayItemContext StartArray();
    
    Builder& EndDict();
    
    Builder& EndArray();
    
    json::Node Build();

private:
    json::Node m_root;
    std::optional<std::string> m_key;
    std::vector<json::Node*> m_vct_node;

    bool array_started = false;
    bool dict_started = false;

    json::Node* AddValue(json::Node::Value value);

    bool IsObjExist() const;

    bool IsPrevNull() const;

    bool IsPrevDict() const;

    bool IsPrevArray() const;
};

class DictItemContext {
public:
    DictItemContext(Builder& builder) : m_builder(builder) {
    }

    DictKeyContext Key(std::string key);

    Builder& EndDict();

private:
    Builder& m_builder;
};

class DictKeyContext {
public:
    DictKeyContext(Builder& builder) : m_builder(builder) {
    }

    DictItemContext Value(json::Node::Value value);

    DictItemContext StartDict();

    ArrayItemContext StartArray();

private:
    Builder& m_builder;
};

class ArrayItemContext {
public:
    ArrayItemContext(Builder& builder) : m_builder(builder) {
    }

    ArrayItemContext Value(json::Node::Value value);

    DictItemContext StartDict();

    ArrayItemContext StartArray();

    Builder& EndArray();

private:
    Builder& m_builder;
};

} // namespace json
