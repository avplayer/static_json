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

		template<typename...> using __void_t = void;

		struct nonesuch
		{
			nonesuch() = delete;
			~nonesuch() = delete;
			nonesuch(nonesuch const&) = delete;
			void operator=(nonesuch const&) = delete;
		};

		template<typename _Default, typename _AlwaysVoid,
			template<typename...> class _Op, typename... _Args>
		struct __detector
		{
			using value_t = std::false_type;
			using type = _Default;
		};

		template<typename _Default, template<typename...> class _Op,
			typename... _Args>
			struct __detector<_Default, __void_t<_Op<_Args...>>, _Op, _Args...>
		{
			using value_t = std::true_type;
			using type = _Op<_Args...>;
		};

		template<typename _Default, template<typename...> class _Op,
			typename... _Args>
			using __detected_or = __detector<_Default, void, _Op, _Args...>;

		template<typename _Default, template<typename...> class _Op,
			typename... _Args>
			using __detected_or_t
			= typename __detected_or<_Default, _Op, _Args...>::type;

		template<template<typename...> class _Op, typename... _Args>
		using is_detected
			= typename __detector<nonesuch, void, _Op, _Args...>::value_t;

		template<template<typename...> class _Op, typename... _Args>
		constexpr bool is_detected_v = is_detected<_Op, _Args...>::value;

		template<template<typename...> class _Op, typename... _Args>
		using detected_t
			= typename __detector<nonesuch, void, _Op, _Args...>::type;

		template<typename _Default, template<typename...> class _Op, typename... _Args>
		using detected_or = __detected_or<_Default, _Op, _Args...>;

		template<typename _Default, template<typename...> class _Op, typename... _Args>
		using detected_or_t = typename detected_or<_Default, _Op, _Args...>::type;

		template<typename Expected, template<typename...> class _Op, typename... _Args>
		using is_detected_exact = std::is_same<Expected, detected_t<_Op, _Args...>>;

		template<typename Expected, template<typename...> class _Op, typename... _Args>
		constexpr bool is_detected_exact_v
			= is_detected_exact<Expected, _Op, _Args...>::value;

		template<typename T>
		using has_value_type_t = typename T::value_type;
		template<typename T>
		constexpr bool has_value_type_v = is_detected_v<has_value_type_t, T>;

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

		template<class T, class Archive>
		using has_serialize = decltype(std::declval<T&>().serialize(std::declval<Archive&>()));
		template<class Archive, class T>
		using has_nonmember_serialize = decltype(serialize(std::declval<Archive&>(), std::declval<T&>()));

		template<class B, class D>
		using base_cast = std::conditional<std::is_const_v<D>, const B, B>;

		template<class T, class U>
		static T & cast_reference(U & u) {
			return static_cast<T &>(u);
		}

		template<class Archive, class T>
		static void serialize_entry(Archive & ar, T & t)
		{
			if constexpr (detail::is_detected_exact_v<void, has_serialize, T, Archive>)
			{
				t.serialize(ar);
			}
			else if constexpr (detail::is_detected_exact_v<void, has_nonmember_serialize, Archive, T>)
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

// 用于非侵入式，避免类成员名字生成不正确.
#define JSON_NI_SERIALIZATION_NVP(classname, name)	\
    make_nvp(JSON_PP_STRINGIZE(name), classname . name)

#define JSON_NI_SERIALIZATION_BASE_OBJECT_NVP(classname, name)	\
    make_nvp(JSON_PP_STRINGIZE(name), base_object<classname>(*this))

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
			assert(json_.HasMember(name) && "has member must true!");

			auto& v = json_[name];
			switch (v.GetType())
			{
			case rapidjson::kNullType:
				break;
			case rapidjson::kObjectType:
			{
				if constexpr (!std::is_arithmetic_v<std::decay_t<T>>
					&& !std::is_same_v<std::decay_t<T>, std::string>
					&& !detail::has_value_type_v<T>)
				{
					json_iarchive ja(v);
					ja >> b;
				}
			}
			break;
			case rapidjson::kArrayType:
			{
				if constexpr (detail::has_value_type_v<T>) // 如果是vector数组, 则判断类型.
				{
					if (std::is_same_v<std::decay_t<T>,
						std::vector<typename T::value_type, typename T::allocator_type>>)
					{
						for (auto& a : v.GetArray())
						{
							using array_type = std::decay_t<typename T::value_type>;
							if constexpr (detail::is_detected_exact_v<void, access::has_serialize, array_type, json_iarchive>
								|| detail::is_detected_exact_v<void, access::has_nonmember_serialize, json_iarchive, array_type>)
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
			}
			break;
			case rapidjson::kFalseType:
			case rapidjson::kTrueType:
			case rapidjson::kNumberType:
			{
				if constexpr (std::is_arithmetic_v<std::decay_t<T>>)
				{
					json_input_builtin<T> ja(v);
					ja >> b;
				}
			}
			break;
			case rapidjson::kStringType:
			{
				if constexpr (std::is_same_v<std::decay_t<T>, std::string>)
				{
					json_input_builtin<std::string> ja(v);
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
				switch (0) { case 0:
					if constexpr (detail::has_value_type_v<T>) // 如果是vector数组, 则判断类型.
					{
#if 0
						if (!std::is_same_v<std::decay_t<T>,
							std::vector<typename T::value_type, typename T::allocator_type>>)
						{
// 							temp.SetObject();
// 							json_oarchive ja(temp);
// 							ja << b;
							break;
						}
#endif
						temp.SetArray();
						for (auto& n : b)
						{
							rapidjson::Value arr;

							using array_type = std::decay_t<typename T::value_type>;
							if constexpr (detail::is_detected_exact_v<void, access::has_serialize, array_type, json_oarchive>
								|| detail::is_detected_exact_v<void, access::has_nonmember_serialize, json_oarchive, array_type>)
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

						// 完成添加数组, 直接返回.
						json_.AddMember(rapidjson::StringRef(name), temp, access::rapidjson_ugly_document_alloc());
						return;
					}
					else
					{
						temp.SetObject();
						json_oarchive ja(temp);
						ja << b;
					}
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

#if 1
	// 非侵入式.
	template<class Archive, class Value>
	void serialize(Archive& ar, std::vector<Value>& v)
	{
		// TODO: 直接序列化vector到json暂时不作实现, 仅用于json object中的数组时, 匹配使用.
	}
#endif

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
