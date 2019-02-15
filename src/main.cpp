#include <iostream>
#include "static_json.hpp"

using namespace static_json;

class animal {
public:
	animal() {}
	animal(int l) : legs(l) {}
	void set_leg(int l) { legs = l; };
	void set_name(std::string s) { name = s; };
	void set_ismammal(bool b) { is_mammal = b; };
	void set_height(double h) { height = h; }

	//private:
	//	friend struct static_json::access;

#if 0
	// 侵入式使用JSON_SERIALIZATION_NVP 或 JSON_SERIALIZATION_BASE_OBJECT_NVP
	template <typename Archive>
	void serialize(Archive &ar)
	{
		ar	& JSON_SERIALIZATION_NVP(legs)
			& JSON_SERIALIZATION_NVP(is_mammal)
			& JSON_SERIALIZATION_NVP(height)
			& JSON_SERIALIZATION_NVP(name);
	}
#else
	// 如果要序列化的成员在private中, 则需要声明serialize是class的友员函数, 以便访问成员.
	template<class Archive>
	friend void serialize(Archive& ar, animal& a);
#endif

private:
	int legs;
	bool is_mammal;
	std::string name;
	double height;
};

#if 1
// 非侵入式, 使用JSON_NI_SERIALIZATION_NVP 或 JSON_NI_SERIALIZATION_BASE_OBJECT_NVP
template<class Archive>
void serialize(Archive& ar, animal& a)
{
	ar	& JSON_NI_SERIALIZATION_NVP(a, legs)
		& JSON_NI_SERIALIZATION_NVP(a, is_mammal)
		& JSON_NI_SERIALIZATION_NVP(a, height)
		& JSON_NI_SERIALIZATION_NVP(a, name);
}
#endif

struct human {
	std::string hand;

	template<class Archive>
	void serialize(Archive& ar)
	{
		ar	& JSON_SERIALIZATION_NVP(hand);
	}
};

// 非侵入式.

#if 0
template<class Archive>
void serialize(Archive& ar, human& a)
{
	ar	& JSON_NI_SERIALIZATION_NVP(a, hand);
}
#endif

class bird : public animal
{
public:
	bird() = default;
	bird(int legs, bool fly) :
		animal{ legs }, can_fly{ fly } {

		// 添加一些测试数据.
		auto tmp = *this;
		tmp.set_leg(10);
		fat.push_back(tmp);
		tmp.set_leg(11);
		fat.push_back(tmp);
		tmp.set_leg(12);
		fat.push_back(tmp);

		money.push_back(100);
		money.push_back(200);
		money.push_back(20);
		money.push_back(21);

		man.hand = u8"鸟人的paw";
	}

private:
	friend struct static_json::access; // 放入private中时需要添加为友员.

	template <typename Archive>
	void serialize(Archive &ar)
	{
		// 侵入式使用JSON_SERIALIZATION_NVP 或 JSON_SERIALIZATION_BASE_OBJECT_NVP
		ar	& JSON_SERIALIZATION_BASE_OBJECT_NVP(animal)
			& JSON_SERIALIZATION_NVP(can_fly)
			& JSON_SERIALIZATION_NVP(fat)
			& JSON_SERIALIZATION_NVP(money)
			& JSON_SERIALIZATION_NVP(man);
	}

	bool can_fly;
	std::vector<animal> fat;
	std::vector<int> money;
	human man;
};

int main()
{
	rapidjson::Value json{ rapidjson::kObjectType };

	{
		// 普通结构体序列化到json对象.
		animal a;
		a.set_name("Horse");
		a.set_leg(4);
		a.set_ismammal(true);
		a.set_height(9.83);

		to_json(a, json);

		{
			rapidjson::StringBuffer buffer;
			rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
			json.Accept(writer);

			std::cout << buffer.GetString() << std::endl;
		}

		// 反序列化json到普通结构体, b 此时和 a 的值完全相同.
		animal b;
		from_json(b, json);
	}

	// 清空一下json, 用于下面测试.
	rapidjson::Value empty{ rapidjson::kObjectType };
	json.Swap(empty);

	{
		bird b(4, true);

		b.set_name("Horse");
		b.set_ismammal(true);
		b.set_height(9.83);
		to_json(b, json);

		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		json.Accept(writer);

		std::cout << buffer.GetString() << std::endl;

		bird d;
		from_json(d, json);

		// 这里d的值和b的值完全相同.
	}

	return 0;
}

