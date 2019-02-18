//
// Copyright (C) 2019 Jack.
//
// Author: jack
// Email:  jack.wgm at gmail dot com
//

#pragma once

#include <cassert>
#include <string>
#include <memory>
#include <vector>
#include <type_traits>

#define BACKEND_RAPIDJSON

namespace static_json {

	namespace detail {
		template<class T>
		using has_push_back_t = decltype(std::declval<T&>().push_back(std::declval<typename T::value_type&>()));

		template<class T, bool result = std::is_same_v<void, has_push_back_t<T>>>
		constexpr bool has_push_back_helper(int) { return result; }
		template<class T>
		static constexpr bool has_push_back_helper(...) { return false; }
		template<class T>
		static constexpr bool has_push_back() { return has_push_back_helper<T>(0); }
	} // namespace detail

	template<class T>
	struct nvp :
		public std::pair<const char *, T *>
	{
		explicit nvp(const char * name_, T & t) :
			std::pair<const char *, T *>(name_, std::addressof(t))
		{}

		const char* name() const {
			return this->first;
		}

		T & value() const {
			return *(this->second);
		}

		const T & const_value() const {
			return *(this->second);
		}
	};

	template<class T>
	inline const nvp< T > make_nvp(const char * name, T & t) {
		return nvp< T >(name, t);
	}

	struct access {
		template<class B, class D>
		using base_cast = std::conditional<std::is_const_v<D>, const B, B>;

		template<class T, class Archive>
		using has_serialize_t = decltype(std::declval<T&>().serialize(std::declval<Archive&>()));
		template<class Archive, class T>
		using has_nonmember_serialize_t = decltype(serialize(std::declval<Archive&>(), std::declval<T&>()));

		// has_serialize_member 实现.
		template<class Archive, class T, bool result = std::is_same_v<void, has_serialize_t<T, Archive>>>
		static constexpr bool has_serialize_helper(int) { return result; }
		template<class Archive, class T>
		static constexpr bool has_serialize_helper(...) { return false; }
		template<class Archive, class T>
		static constexpr bool has_serialize_member() { return has_serialize_helper<Archive, T>(0); }

		// has_nonmember_serialize 实现.
		template<class Archive, class T, bool result = std::is_same_v<void, has_nonmember_serialize_t<Archive, T>>>
		static constexpr bool has_nonmember_serialize_helper(int) { return result; }
		template<class Archive, class T>
		static constexpr bool has_nonmember_serialize_helper(...) { return false; }
		template<class Archive, class T>
		static constexpr bool has_nonmember_serialize() { return has_nonmember_serialize_helper<Archive, T>(0); }

		template<class T, class U>
		static T & cast_reference(U & u) {
			return static_cast<T &>(u);
		}

		template<class Archive, class T>
		static void serialize_entry(Archive & ar, T & t)
		{
			if constexpr (has_serialize_member<Archive, T>())
			{
				t.serialize(ar);
			}
			else if constexpr (has_nonmember_serialize<Archive, T>())
			{
				serialize(ar, t);
			}
			else
			{
				assert(false && "require serialize! require serialize!");
			}
		}
	};

	template<class Base, class Derived>
	typename static_json::access::base_cast<Base, Derived>::type&
		base_object(Derived &d)
	{
		typedef typename static_json::access::base_cast<Base, Derived>::type type;
		return static_json::access::cast_reference<type, Derived>(d);
	}

#define JSON_PP_STRINGIZE(text) JSON_PP_STRINGIZE_I(text)
#define JSON_PP_STRINGIZE_I(...) #__VA_ARGS__


// 用于serialize成员函数中声明要序列化的成员.
#define JSON_SERIALIZATION_NVP(name)	\
    make_nvp(JSON_PP_STRINGIZE(name), name)

#define JSON_SERIALIZATION_BASE_OBJECT_NVP(name) \
	make_nvp(JSON_PP_STRINGIZE(name), base_object<name>(*this))

// 侵入式, 指定key.
#define JSON_SERIALIZATION_KEY_NVP(key, name)	\
    make_nvp(key, name)

// 侵入式, 指定key和基类.
#define JSON_SERIALIZATION_KEY_BASE_OBJECT_NVP(key, name) \
	make_nvp(key, base_object<name>(*this))

// 非侵入式，避免类成员名字生成不正确.
#define JSON_NI_SERIALIZATION_NVP(classname, name)	\
    make_nvp(JSON_PP_STRINGIZE(name), classname . name)

// 非侵入式, 指定基类.
#define JSON_NI_SERIALIZATION_BASE_OBJECT_NVP(classname, name)	\
    make_nvp(JSON_PP_STRINGIZE(name), base_object<classname>(*this))

// 非侵入式, 指定key.
#define JSON_NI_SERIALIZATION_KEY_NVP(key, classname, name)	\
    make_nvp(key, classname . name)

// 非侵入式, 指定基类和key.
#define JSON_NI_SERIALIZATION_KEY_BASE_OBJECT_NVP(key, classname, name)	\
    make_nvp(key, base_object<classname>(*this))
}


#if defined(BACKEND_RAPIDJSON)

#include "backend_rapidjson.hpp"

namespace static_json {
	template<class T>
	void to_json(const T& a, rapidjson::Value& json)
	{
		rapidjson_oarchive ja(json);
		ja << a;
	}

	template<class T>
	void from_json(T& a, const rapidjson::Value& json)
	{
		rapidjson_iarchive ja(json);
		ja >> a;
	}

	template<class T>
	void from_json_string(T& a, std::string_view str)
	{
		rapidjson::Document doc;
		doc.Parse(str.data());

		rapidjson::Value v;
		doc.Swap(v);

		from_json(a, v);
	}

	template<class T>
	std::string to_json_string(T& a)
	{
		rapidjson::Value v{ rapidjson::kObjectType };
		to_json(a, v);

		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		v.Accept(writer);

		return buffer.GetString();
	}
}
#endif


