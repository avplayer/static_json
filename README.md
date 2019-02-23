
# c++/json序列化

## 介绍

允许从一个c++的结构体转换到json，或者从一个json转换到c++结构.

## 动机

在曾经的一些项目中，常常会用到json做为协议，然后在处理对应协议的时候，经常需要把解析json出来的value对象引用在各个需要的地方，然后通过value的json库提供的接口去访问业务中需要的字段，这样导致代码中到处夹杂了json库相关的代码，增加了与json解析库的耦合。

我在了解了boost.serialization的基本原理后，从而设计了这个库，该库在序列化和反序列化过程中，其中开销几乎可以忽略不计，具体依赖rapidjson库的效率。

有了这个库，我们只需要定义c++数据结构，然后把json数据序列化到结构体中，这样访问的时候，不再是通过json库的接口了，而是c++的结构体，从而避免json和项目的耦合度。

## 使用

可以简单的包含static_json.hpp以及rapidjson库(rapidjson也是header only的)，或者也可以将include目录下的代码拷贝到你自己的项目当中，再include进来，就可以开始使用了。


## 快速上手

```
#include "static_json.hpp"

int main() {
	using namespace static_json;

	std::vector<int> ai = {1, 3, 4, 7, 9};
	std::string a = to_json_string(ai); // 将ai序列为到一个json数组字符串.

	std::cout << a << std::endl;

	std::vector<int> af;
	from_json_string(af, a);	// af 将从a字符串反序列化为一个具有 1 3 4 7 9的数组.
}
```


## 侵入式c++结构体序列化

```
struct proto {
	int type;
	std::string name;
	double height;
	
	template <typename Archive>
	void serialize(Archive &ar) // 注意serialize函数必须为public
	{
		ar	& JSON_SERIALIZATION_NVP(type)
			& JSON_SERIALIZATION_NVP(name)
			& JSON_SERIALIZATION_NVP(height);
	}
};


// 序列化
proto test1 {1, "abcd", 1.83};
std::string a = to_json_string(test1); // 将序列为到一个json字符串.

// 反序列化
proto test2;
from_json_string(test2, a); // test2拥有和test1一样的内容.
```


## 非侵入式c++结构体序列化


```
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

// 序列化
proto test1 {1, "abcd", 1.83};
std::string a = to_json_string(test1); // 将序列为到一个json字符串.

// 反序列化
proto test2;
from_json_string(test2, a); // test2拥有和test1一样的内容.
```


更多的用法参考src/main.cpp，

