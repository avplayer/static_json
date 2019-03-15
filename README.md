
# c++/json serialization

[![Build Status](https://travis-ci.org/avplayer/static_json.svg?branch=master)](https://travis-ci.org/avplayer/static_json)

## Introduction

Allows conversion from a C++ structure to json, or from a json to a C++ structure.

## Motivation

In some of the past projects, json is often used as a protocol, and when dealing with the corresponding protocol, it is often necessary to reference the value object parsed by json in each required place, and then go through the interface provided by the value json library. Access to the fields needed in the business, which leads to the json library-related code everywhere in the code, adding coupling to the json parsing library.

After understanding the basic principles of boost.serialization, I designed this library. static_json in the process of serialization and deserialization, the overhead is almost negligible, depending on the efficiency of the rapididjson library.

With this library, we only need to define the C++ data structure, and then serialize the json data into the structure, so that when accessing, it is no longer through the interface of the json library, but the structure of c++, thus avoiding json library and The degree of coupling of the project.

## How to use

You can simply include static_json.hpp and rapidjson libraries (rapidjson is also header only), or you can copy the code in the include directory to your own project, and then include it, you can start using it.


## Get started quickly

```cpp
#include "static_json.hpp"

int main() {
	using namespace static_json;

	std::vector<int> ai = {1, 3, 4, 7, 9};
	std::string a = to_json_string(ai);

	std::cout << a << std::endl;

	std::vector<int> af;
	from_json_string(af, a);
}
```


## Invasive c++ structure serialization

```cpp
struct proto {
	int type;
	std::string name;
	double height;
	
	template <typename Archive>
	void serialize(Archive &ar)
	{
		ar	& JSON_SERIALIZATION_NVP(type)
			& JSON_SERIALIZATION_NVP(name)
			& JSON_SERIALIZATION_NVP(height);
	}
};


proto test1 {1, "abcd", 1.83};
std::string a = to_json_string(test1);

proto test2;
from_json_string(test2, a); // test2 same as test1.
```


## Non-intrusive c++ structure serialization


```cpp
struct proto {
	int type;
	std::string name;
	double height;
};

template<class Archive>
void serialize(Archive& ar, proto& a)
{
	ar	& JSON_NI_SERIALIZATION_NVP(a, type)
		& JSON_NI_SERIALIZATION_NVP(a, name)
		& JSON_NI_SERIALIZATION_NVP(a, height);
}

proto test1 {1, "abcd", 1.83};
std::string a = to_json_string(test1);

proto test2;
from_json_string(test2, a); // test2 same as test1.
```


For more usage, see src/main.cpp

