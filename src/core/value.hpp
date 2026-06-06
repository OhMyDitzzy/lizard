#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <sstream>

/* Forward declarations for circular references between LizardValue and collections. */
struct LizardArray;
struct LizardObject;

/*
 * LizardValue is the runtime representation of every value in Lizard.
 *
 * Arrays and Objects hold a shared_ptr so that assignment copies the
 * pointer — giving reference semantics. Two variables can point at the
 * same array or object, and a mutation through one is visible from the other.
 */
struct LizardValue {
    enum class Kind { String, Integer, Float, Bool, Null, Array, Object };

    Kind        kind       = Kind::String;
    std::string raw;                             // storage for primitive types
    std::shared_ptr<LizardArray>  array_data;   // used when kind == Array
    std::shared_ptr<LizardObject> object_data;  // used when kind == Object

    std::string display() const;

    /* Primitive constructors — shared_ptrs explicitly null-initialized. */
    static LizardValue makeString(const std::string& s) { return {Kind::String,  s, {}, {}}; }
    static LizardValue makeInt   (const std::string& s) { return {Kind::Integer, s, {}, {}}; }
    static LizardValue makeFloat (const std::string& s) { return {Kind::Float,   s, {}, {}}; }
    static LizardValue makeBool  (bool b)               { return {Kind::Bool, b ? "1" : "0", {}, {}}; }
    static LizardValue makeNull  ()                     { return {Kind::Null, "", {}, {}}; }

    static LizardValue makeArray (std::shared_ptr<LizardArray>  data);
    static LizardValue makeObject(std::shared_ptr<LizardObject> data);
};

struct LizardArray {
    std::vector<LizardValue> elements;
};

struct LizardObject {
    std::vector<std::string>                     key_order; // insertion order for display
    std::unordered_map<std::string, LizardValue> fields;

    /* Set or overwrite a key, tracking insertion order for new keys. */
    void set(const std::string& key, LizardValue val) {
        if (!fields.count(key)) key_order.push_back(key);
        fields[key] = std::move(val);
    }

    bool has(const std::string& key) const { return fields.count(key) > 0; }

    LizardValue get(const std::string& key) const {
        auto it = fields.find(key);
        return it != fields.end() ? it->second : LizardValue::makeNull();
    }
};

/* Defined here (after both LizardArray and LizardObject are complete). */

inline LizardValue LizardValue::makeArray(std::shared_ptr<LizardArray> data) {
    LizardValue v; v.kind = Kind::Array; v.array_data = std::move(data); return v;
}

inline LizardValue LizardValue::makeObject(std::shared_ptr<LizardObject> data) {
    LizardValue v; v.kind = Kind::Object; v.object_data = std::move(data); return v;
}

/*
 * String representation used by show() and string concatenation.
 * Strings inside arrays/objects are quoted to give a JSON-like view.
 */
inline std::string LizardValue::display() const {
    if (kind == Kind::Bool) return raw == "1" ? "true" : "false";
    if (kind == Kind::Null) return "null";

    if (kind == Kind::Array) {
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < array_data->elements.size(); i++) {
            if (i > 0) oss << ", ";
            const auto& el = array_data->elements[i];
            if (el.kind == Kind::String) oss << '"' << el.raw << '"';
            else                         oss << el.display();
        }
        oss << "]";
        return oss.str();
    }

    if (kind == Kind::Object) {
        std::ostringstream oss;
        oss << "{ ";
        bool first = true;
        for (const auto& key : object_data->key_order) {
            if (!first) oss << ", ";
            first = false;
            const auto& val = object_data->fields.at(key);
            oss << key << ": ";
            if (val.kind == Kind::String) oss << '"' << val.raw << '"';
            else                          oss << val.display();
        }
        oss << " }";
        return oss.str();
    }

    return raw;
}