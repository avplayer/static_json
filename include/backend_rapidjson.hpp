//
// Copyright (C) 2019 Jack.
//
// Author: jack
// Email:  jack.wgm at gmail dot com
//

#pragma once

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

// 支持普通c++数据结构和rapidjson之间的互相序列化.
// 序列化到c++类型和rapidjson之间转换仅使用以下类型：
// int, unsigned int, int64_t, uint64_t, bool, float, double, std::string, std::vector
// 分别对应rapidjson中
// Int, Uint,         Int64,   Uint64,   Bool, Float, Double, String,      Array
//

namespace static_json {

	template<class T>
	struct nvp;

}

static std::unique_ptr<rapidjson::Document> document_;
static rapidjson::Document::AllocatorType&
	rapidjson_ugly_document_alloc() {
	if (!document_)
		document_.reset(new rapidjson::Document{ rapidjson::kObjectType });
	return document_->GetAllocator();
}

static void set_default_ugly_document(rapidjson::Document* doc) {
	document_.reset(doc);
}

namespace archive {

	// rapidjson 到 普通数据结构.
	//
	struct rapidjson_iarchive
	{
		rapidjson_iarchive(const rapidjson::Value& json)
			: json_(json)
		{}

		template <typename T>
		rapidjson_iarchive& operator>>(static_json::nvp<T> const& wrap)
		{
			load(wrap.name(), wrap.value());
			return *this;
		}

		template <typename T>
		rapidjson_iarchive& operator>>(T const& value)
		{
			return operator>>(const_cast<T&>(value));
		}

		template <typename T>
		rapidjson_iarchive& operator>>(T& value)
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
			else if constexpr (std::is_same_v<std::decay_t<T>, char>)
				// char 类型直接跳过不作处理, json没有char类型与之对应.
				;
			else if constexpr (static_json::traits::is_std_optional_v<T>)
			{
				rapidjson_iarchive ja(json_);
				typename T::value_type v;
				ja >> v;
				value = v;
			}
			else if constexpr (static_json::traits::has_push_back<T>())
			{
				for (auto& a : json_.GetArray())
				{
					std::decay_t<typename T::value_type> tmp;
					rapidjson_iarchive ja(a);
					ja >> tmp;
					value.push_back(tmp);
				}
			}
			else
				load(value);
			return *this;
		}

		template <typename T>
		rapidjson_iarchive& operator&(T const& v)
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
					&& !static_json::traits::has_push_back<T>())
				{
					rapidjson_iarchive ja(value);
					ja >> b;
				}
			}
			break;
			case rapidjson::kArrayType:
			{
				if constexpr (static_json::traits::has_push_back<T>()) // 如果是兼容数组类型, 则按数组使用push_back保存.
				{
					for (auto& a : value.GetArray())
					{
						std::decay_t<typename T::value_type> tmp;
						rapidjson_iarchive ja(a);
						ja >> tmp;
						b.push_back(tmp);
					}
				}
			}
			break;
			case rapidjson::kFalseType:
			case rapidjson::kTrueType:
			case rapidjson::kNumberType:
			{
				rapidjson_iarchive ja(value);
				ja >> b;
			}
			break;
			case rapidjson::kStringType:
			{
				rapidjson_iarchive ja(value);
				ja >> b;
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
			static_json::serialize_adl(*this, v);
		}

		const rapidjson::Value& json_;
	};

	// 普通数据结构 到 rapidjson.
	//
	struct rapidjson_oarchive
	{
		rapidjson_oarchive(rapidjson::Value& json)
			: json_(json)
		{}

		template <typename T>
		rapidjson_oarchive& operator<<(static_json::nvp<T> const& wrap)
		{
			save(wrap.name(), wrap.value());
			return *this;
		}

		template <typename T>
		rapidjson_oarchive& operator<<(T const& value)
		{
			return operator<<(const_cast<T&>(value));
		}

		template <typename T>
		rapidjson_oarchive& operator<<(T& value)
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
			else if constexpr (std::is_same_v<std::decay_t<T>, char>)
				// char 类型直接跳过不作处理, json没有char类型与之对应.
				;
			else if constexpr (static_json::traits::is_std_optional_v<T>)
			{
				if (value)
				{
					rapidjson_oarchive ja(json_);
					ja << value.value();
				}
			}
			else if constexpr (static_json::traits::has_push_back<T>())
			{
				json_.SetArray();
				for (auto& n : value)
				{
					rapidjson::Value arr;
					rapidjson_oarchive ja(arr);
					ja << n;
					json_.PushBack(arr, rapidjson_ugly_document_alloc());
				}
			}
			else
			{
				json_.SetObject();
				save(value);
			}
			return *this;
		}

		template <typename T>
		rapidjson_oarchive& operator&(T const& v)
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
				rapidjson_oarchive ja(temp);
				ja << b;
			}
			else
			{
				if constexpr (static_json::traits::has_push_back<T>()) // 如果是兼容数组类型, 则按数组来序列化.
				{
					temp.SetArray();
					for (auto& n : b)
					{
						rapidjson::Value arr;
						rapidjson_oarchive ja(arr);
						ja << n;
						temp.PushBack(arr, rapidjson_ugly_document_alloc());
					}
				}
				else if constexpr (static_json::traits::is_std_optional_v<T>)
				{
					if (b)
					{
						rapidjson_oarchive ja(temp);
						ja << b.value();
					}
					else
					{
						return;
					}
				}
				else
				{
					rapidjson_oarchive ja(temp);
					ja << b;
				}
			}

			json_.AddMember(rapidjson::StringRef(name), temp, rapidjson_ugly_document_alloc());
		}

		template <typename T>
		void save(T& v)
		{
			static_json::serialize_adl(*this, v);
		}

		rapidjson::Value& json_;
	};
}
