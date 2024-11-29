//===== Copyright © 1999-2024, Kawe Mazidjatari, All rights reserved. =======//
//
// Purpose: A set of utilities for RapidJSON
//
//===========================================================================//
#ifndef TIER2_JSONUTILS_H
#define TIER2_JSONUTILS_H

//-----------------------------------------------------------------------------
// JSON member enumerations
//-----------------------------------------------------------------------------
enum class JSONFieldType_e
{
    kNull = 0,
    kObject,

    kBool,
    kNumber,

    kSint32,
    kUint32,

    kSint64,
    kUint64,

    kFloat,
    kLFloat,

    kDouble,
    kLDouble,

    kString,
    kArray
};

//-----------------------------------------------------------------------------
// Purpose: gets the object type as string
//-----------------------------------------------------------------------------
inline const char* JSON_TypeToString(const rapidjson::Type type)
{
    switch (type)
    {
    case rapidjson::kNullType: return "null";
    case rapidjson::kFalseType: case rapidjson::kTrueType: return "bool";
    case rapidjson::kObjectType: return "object";
    case rapidjson::kArrayType: return "array";
    case rapidjson::kStringType: return "string";
    case rapidjson::kNumberType: return "number";
    default: return "unknown";
    }
}

//-----------------------------------------------------------------------------
// Purpose: checks if the member's value is of type provided
//-----------------------------------------------------------------------------
template <class T>
inline bool JSON_IsOfType(const T& data, const JSONFieldType_e type)
{
    switch (type)
    {
    case JSONFieldType_e::kNull:
        return data.IsNull();
    case JSONFieldType_e::kObject:
        return data.IsObject();
    case JSONFieldType_e::kBool:
        return data.IsBool();
    case JSONFieldType_e::kNumber:
        return data.IsNumber();
    case JSONFieldType_e::kSint32:
        return data.IsInt();
    case JSONFieldType_e::kUint32:
        return data.IsUint();
    case JSONFieldType_e::kSint64:
        return data.IsInt64();
    case JSONFieldType_e::kUint64:
        return data.IsUint64();
    case JSONFieldType_e::kFloat:
        return data.IsFloat();
    case JSONFieldType_e::kLFloat:
        return data.IsLosslessFloat();
    case JSONFieldType_e::kDouble:
        return data.IsDouble();
    case JSONFieldType_e::kLDouble:
        return data.IsLosslessDouble();
    case JSONFieldType_e::kString:
        return data.IsString();
    case JSONFieldType_e::kArray:
        return data.IsArray();
    default:
        return false;
    }
}

//-----------------------------------------------------------------------------
// Purpose: gets json type for data type at compile time
//-----------------------------------------------------------------------------
template <class T>
inline JSONFieldType_e JSON_GetTypeForType()
{
    if constexpr (std::is_same<T, bool>::value)
        return JSONFieldType_e::kBool;
    else if constexpr (std::is_same<T, int32_t>::value)
        return JSONFieldType_e::kSint32;
    else if constexpr (std::is_same<T, int64_t>::value)
        return JSONFieldType_e::kSint64;
    else if constexpr (std::is_same<T, uint32_t>::value)
        return JSONFieldType_e::kUint32;
    else if constexpr (std::is_same<T, uint64_t>::value)
        return JSONFieldType_e::kUint64;
    else if constexpr (std::is_same<T, float>::value)
        return JSONFieldType_e::kFloat;
    else if constexpr (std::is_same<T, double>::value)
        return JSONFieldType_e::kDouble;
    else if constexpr (std::is_same<T, const char*>::value)
        return JSONFieldType_e::kString;
    else if constexpr (std::is_same<T, std::string>::value)
        return JSONFieldType_e::kString;
    else
        static_assert(false, "Cannot classify data type; unsupported.");
}

//-----------------------------------------------------------------------------
// Purpose: checks if the member exists and if its value is of type provided
//-----------------------------------------------------------------------------
template <class T>
inline bool JSON_HasMemberAndIsOfType(const T& data, const char* const member, const JSONFieldType_e type)
{
    const T::ConstMemberIterator it = data.FindMember(member);

    if (it != data.MemberEnd())
    {
        return JSON_IsOfType(it->value, type);
    }

    return false;
}

//-----------------------------------------------------------------------------
// Purpose: checks if the member exists and if its value is of specified type,
//          and sets 'out' to its value if all aforementioned conditions
//          are met
//-----------------------------------------------------------------------------
template <class T, class V>
inline bool JSON_GetValue(const T& data, const char* const member, const JSONFieldType_e type, V& out)
{
    const T::ConstMemberIterator it = data.FindMember(member);

    if (it != data.MemberEnd())
    {
        const rapidjson::Value& val = it->value;

        if (JSON_IsOfType(val, type))
        {
            out = val.Get<V>();
            return true;
        }
    }

    // Not found or didn't match specified type.
    return false;
}

//-----------------------------------------------------------------------------
// Purpose: checks if the member exists and if its value is of classified type,
//          and sets 'out' to its value if all aforementioned conditions are met
//-----------------------------------------------------------------------------
template <class T, class V>
inline bool JSON_GetValue(const T& data, const char* const member, V& out)
{
    const T::ConstMemberIterator it = data.FindMember(member);

    if (it != data.MemberEnd())
    {
        const rapidjson::Value& val = it->value;

        if (JSON_IsOfType(val, JSON_GetTypeForType<V>()))
        {
            out = val.Get<V>();
            return true;
        }
    }

    // Not found or didn't match classified type.
    return false;
}
template <class T>
inline bool JSON_GetValue(const T& data, const char* const member, std::string& out)
{
    const char* stringVal;

    if (JSON_GetValue(data, member, JSONFieldType_e::kString, stringVal))
    {
        out = stringVal;
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
// Purpose: checks if the member exists and if its value is of classified type,
//          and returns the value if all aforementioned conditions are met.
//          else the provided default gets returned
//-----------------------------------------------------------------------------
template <class T, class V>
inline V JSON_GetValueOrDefault(const T& data, const char* const member, const V def)
{
    V val;

    if (JSON_GetValue(data, member, JSON_GetTypeForType<V>(), val))
    {
        return val;
    }

    return def;
}

//-----------------------------------------------------------------------------
// Purpose: checks if the member exists and if its value is of type provided,
//          and sets 'out' to its iterator if all aforementioned conditions
//          are met
//-----------------------------------------------------------------------------
template <class T>
inline bool JSON_GetIterator(const T& data, const char* const member,
    const JSONFieldType_e type, typename T::ConstMemberIterator& out)
{
    const T::ConstMemberIterator it = data.FindMember(member);

    if (it != data.MemberEnd())
    {
        if (JSON_IsOfType(it->value, type))
        {
            out = it;
            return true;
        }
    }

    // Not found or didn't match specified type.
    return false;
}

//-----------------------------------------------------------------------------
// Purpose: checks if the member exists, and sets 'out' to its iterator if the
// aforementioned condition is met
//-----------------------------------------------------------------------------
template <class T>
inline bool JSON_GetIterator(const T& data, const char* const member, typename T::ConstMemberIterator& out)
{
    const T::ConstMemberIterator it = data.FindMember(member);

    if (it != data.MemberEnd())
    {
        out = it;
        return true;
    }

    // Not found.
    return false;
}

//-----------------------------------------------------------------------------
// Purpose: parses json number out of provided integer, float or string. string
//          can be a hex (0x<num>), octal (0<num>) or decimal (<num>)
//-----------------------------------------------------------------------------
template <class T, class V>
inline bool JSON_ParseNumber(const T& data, V& num)
{
    if (JSON_IsOfType(data, JSONFieldType_e::kNumber))
    {
        num = data.Get<V>();
        return true;
    }
    else if (JSON_IsOfType(data, JSONFieldType_e::kString))
    {
        const char* const string = data.GetString();
        char* end = nullptr;

        if constexpr (std::is_same<V, int32_t>::value)
            num = strtol(string, &end, 0);
        else if constexpr (std::is_same<V, int64_t>::value)
            num = strtoll(string, &end, 0);
        else if constexpr (std::is_same<V, uint32_t>::value)
            num = strtoul(string, &end, 0);
        else if constexpr (std::is_same<V, uint64_t>::value)
            num = strtoull(string, &end, 0);
        else if constexpr (std::is_same<V, float>::value)
            num = static_cast<float>(strtod(string, &end));
        else if constexpr (std::is_same<V, double>::value)
            num = strtod(string, &end);
        else
            static_assert(false, "Cannot classify numeric type; unsupported.");

        return end == &string[data.GetStringLength()];
    }

    return false;
}
template <class T, class V>
inline bool JSON_ParseNumber(const T& data, const char* const member, V& num)
{
    rapidjson::Document::ConstMemberIterator it;

    if (JSON_GetIterator(data, member, it))
    {
        return JSON_ParseNumber(it->value, num);;
    }

    return false;
}

//-----------------------------------------------------------------------------
// Purpose: checks if the member exists and if its value is of classified type,
//          and returns the number if all aforementioned conditions are met.
//          else the provided default gets returned
//-----------------------------------------------------------------------------
template <class T, class V>
inline V JSON_GetNumberOrDefault(const T& data, const char* const member, V def)
{
    V num;

    if (JSON_ParseNumber(data, member, num))
    {
        return num;
    }

    return def;
}

void JSON_DocumentToBufferDeserialize(const rapidjson::Document& document, rapidjson::StringBuffer& buffer, unsigned int indent = 4);

#endif // TIER2_JSONUTILS_H
