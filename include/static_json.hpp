//
// Copyright (C) 2019 Jack.
//
// Author: jack
// Email:  jack.wgm at gmail dot com
//

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <type_traits>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

// 支持普通c++数据结构和rapidjson之间的互相序列化.
// 序列化到c++类型和rapidjson之间转换仅使用以下类型：
// int, unsigned int, int64_t, uint64_t, bool, float, double, std::string, std::vector
// 分别对应rapidjson中
// Int, Uint,         Int64,   Uint64,   Bool, Float, Double, String,      Array
//
// 限制条件:
// json必须是一个Object, 暂不支持json是单个数组或单个类型, 比如不能直接将 Array 序列化到
// std::vector, 而必须是在Object中的Array.

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

		static std::unique_ptr<rapidjson::Document> document_;

		static rapidjson::Document::AllocatorType&
			rapidjson_ugly_document_alloc() {
			if (!document_)
				document_.reset(new rapidjson::Document{ rapidjson::kObjectType });
			return document_->GetAllocator();
		}

		void set_default_ugly_document(rapidjson::Document* doc) {
			document_.reset(doc);
		}

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

	std::unique_ptr<rapidjson::Document> access::document_ = nullptr;

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

	template <class T>
	struct json_input_builtin
	{
		const rapidjson::Value& json_;
		json_input_builtin(const rapidjson::Value& json)
			: json_(json)
		{}

		json_input_builtin& operator>>(T& value)
		{
			if constexpr (std::is_same_v<std::decay_t<T>, int>)
				value = json_.GetInt();
			else if constexpr (std::is_same_v<std::decay_t<T>, unsigned int>)
				value = json_.GetUint();
			else if constexpr (std::is_same_v<std::decay_t<T>, int64_t>)
				value = json_.GetInt64();
			else if constexpr (std::is_same_v<std::decay_t<T>, uint64_t>)
				value = json_.GetUint64();
			else if constexpr (std::is_same_v<std::decay_t<T>, bool>)
				value = json_.GetBool();
			else if constexpr (std::is_same_v<std::decay_t<T>, float>)
				value = json_.GetFloat();
			else if constexpr (std::is_same_v<std::decay_t<T>, double>)
				value = json_.GetDouble();
			else if constexpr (std::is_same_v<std::decay_t<T>, std::string>)
				value.assign(json_.GetString(), json_.GetStringLength());
			return *this;
		}
	};

	// json 到 普通数据结构.
	//
	struct json_iarchive
	{
		json_iarchive(const rapidjson::Value& json)
			: json_(json)
		{}

		template <typename T>
		json_iarchive& operator>>(nvp<T> const& wrap)
		{
			load(wrap.name(), wrap.value());
			return *this;
		}

		template <typename T>
		json_iarchive& operator>>(T const& value)
		{
			return operator>>(const_cast<T&>(value));
		}

		template <typename T>
		json_iarchive& operator>>(T& value)
		{
			load(value);
			return *this;
		}

		template <typename T>
		json_iarchive& operator&(T const& v)
		{
			return operator>>(v);
		}

		template <typename T>
		void load(char const* name, T& b) const
		{
			if (!json_.HasMember(name))
				return;

			auto& value = json_[name];
			switch (value.GetType())
			{
			case rapidjson::kNullType:
				break;
			case rapidjson::kObjectType:
			{
				if constexpr (!std::is_arithmetic_v<std::decay_t<T>>
					&& !std::is_same_v<std::decay_t<T>, std::string>
					&& !detail::has_push_back<T>())
				{
					json_iarchive ja(value);
					ja >> b;
				}
			}
			break;
			case rapidjson::kArrayType:
			{
				if constexpr (detail::has_push_back<T>()) // 如果是兼容数组类型, 则按数组使用push_back保存.
				{
					for (auto& a : value.GetArray())
					{
						using array_type = std::decay_t<typename T::value_type>;

						if constexpr (access::has_serialize_member<json_iarchive, array_type>()
							|| access::has_nonmember_serialize<json_iarchive, array_type>())
						{
							json_iarchive ja(a);
							typename T::value_type item;
							ja >> item;
							b.push_back(item);
						}
						else
						{
							array_type tmp;
							json_input_builtin<array_type> ja(a);
							ja >> tmp;
							b.push_back(tmp);
						}
					}
				}
			}
			break;
			case rapidjson::kFalseType:
			case rapidjson::kTrueType:
			case rapidjson::kNumberType:
			{
				if constexpr (std::is_arithmetic_v<std::decay_t<T>>)
				{
					json_input_builtin<T> ja(value);
					ja >> b;
				}
			}
			break;
			case rapidjson::kStringType:
			{
				if constexpr (std::is_same_v<std::decay_t<T>, std::string>)
				{
					json_input_builtin<std::string> ja(value);
					ja >> b;
				}
			}
			break;
			default:
				assert(false && "json type not supported!");
				break;
			}
		}

		template <typename T>
		void load(T& v)
		{
			access::serialize_entry(*this, v);
		}

		const rapidjson::Value& json_;
	};


	template <class T>
	struct json_output_builtin
	{
		rapidjson::Value& json_;
		json_output_builtin(rapidjson::Value& json)
			: json_(json)
		{}

		json_output_builtin& operator<<(T& value)
		{
			if constexpr (std::is_same_v<std::decay_t<T>, int>)
				json_.SetInt(value);
			else if constexpr (std::is_same_v<std::decay_t<T>, unsigned int>)
				json_.SetUint(value);
			else if constexpr (std::is_same_v<std::decay_t<T>, int64_t>)
				json_.SetInt64(value);
			else if constexpr (std::is_same_v<std::decay_t<T>, uint64_t>)
				json_.SetUint64(value);
			else if constexpr (std::is_same_v<std::decay_t<T>, bool>)
				json_.SetBool(value);
			else if constexpr (std::is_same_v<std::decay_t<T>, float>)
				json_.SetFloat(value);
			else if constexpr (std::is_same_v<std::decay_t<T>, double>)
				json_.SetDouble(value);
			else if constexpr (std::is_same_v<std::decay_t<T>, std::string>)
				json_.SetString(value.c_str(), static_cast<rapidjson::SizeType>(value.size()));
			return *this;
		}
	};

	// 普通数据结构 到 json.
	//
	struct json_oarchive
	{
		json_oarchive(rapidjson::Value& json)
			: json_(json)
		{}

		template <typename T>
		json_oarchive& operator<<(nvp<T> const& wrap)
		{
			save(wrap.name(), wrap.value());
			return *this;
		}

		template <typename T>
		json_oarchive& operator<<(T const& value)
		{
			return operator<<(const_cast<T&>(value));
		}

		template <typename T>
		json_oarchive& operator<<(T& value)
		{
			save(value);
			return *this;
		}

		template <typename T>
		json_oarchive& operator&(T const& v)
		{
			return operator<<(v);
		}

		template <typename T>
		void save(char const* name, T& b)
		{
			rapidjson::Value temp;
			if constexpr (std::is_arithmetic_v<std::decay_t<T>>
				|| std::is_same_v<std::decay_t<T>, std::string>)
			{
				json_output_builtin<T> ja(temp);
				ja << b;
			}
			else
			{
				if constexpr (detail::has_push_back<T>()) // 如果是兼容数组类型, 则按数组来序列化.
				{
					temp.SetArray();
					for (auto& n : b)
					{
						rapidjson::Value arr;

						using array_type = std::decay_t<typename T::value_type>;

						if constexpr (access::has_serialize_member<json_iarchive, array_type>()
							|| access::has_nonmember_serialize<json_iarchive, array_type>())
						{
							arr.SetObject();
							json_oarchive ja(arr);
							ja << n;
						}
						else
						{
							json_output_builtin<array_type> ja(arr);
							ja << n;
						}

						temp.PushBack(arr, access::rapidjson_ugly_document_alloc());
					}
				}
				else
				{
					temp.SetObject();
					json_oarchive ja(temp);
					ja << b;
				}
			}

			json_.AddMember(rapidjson::StringRef(name), temp, access::rapidjson_ugly_document_alloc());
		}

		template <typename T>
		void save(T& v)
		{
			access::serialize_entry(*this, v);
		}

		rapidjson::Value& json_;
	};


	template<class T>
	void to_json(const T& a, rapidjson::Value& json)
	{
		json_oarchive ja(json);
		ja << a;
	}

	template<class T>
	void from_json(T& a, const rapidjson::Value& json)
	{
		json_iarchive ja(json);
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
