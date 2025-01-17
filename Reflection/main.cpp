#include "function_traits.h"
#include "variable_traits.h"
#include "srfle.h"
#include "type_list.h"

#include <string>
#include <iostream>
#include <vector>
#include <list>

struct Person final {
	std::string familyName;
	float height;
	bool isFemale;

	void IntroduceMtself() const {
		std::cout << "hhh" << std::endl;
	}
	bool IsFemale() const { return false; }
	bool GetMarried(Person& other) {
		bool success = other.isFemale != isFemale;
		if (isFemale) {
			familyName = "Mrs." + other.familyName;
		}
		else {
			familyName = "Mr." + familyName;
		}
		return success;
	}
};

BEGIN_CLASS(Person)
functions(
	func(&Person::GetMarried),
	func(&Person::IntroduceMtself),
	func(&Person::IsFemale)
)
END_CLASS()

class Numeric;
class Enum;
class Class;
class Type;

std::list<const Type*> gTypeList;

class Type {
public:
	template <typename T>
	friend class EnumFactory;

	template <typename T>
	friend class ClassFactory;

	enum class Kind {
		Numeric,
		Enum,
		Class,
	};

	virtual ~Type() = default;

	Type(const std::string& name, Kind kind): name_(name), kind_(kind) {}

	auto& GetName() const { return name_; }
	auto GetKind() const { return kind_; }

	const Numeric* AsNumeric() const {
		if (kind_ == Kind::Numeric) {
			return (const Numeric*)(this);
		} else {
			return nullptr;
		}
	}

	const Enum* AsEnum() const {
		if (kind_ == Kind::Enum) {
			return (const Enum*)(this);
		}
		else {
			return nullptr;
		}
	}

	const Class* AsClass() const {
		if (kind_ == Kind::Class) {
			return (const Class*)(this);
		}
		else {
			return nullptr;
		}
	}

private:
	std::string name_;
	Kind kind_;
};

class Numeric : public Type {
public:
	enum class Kind {
		Unknow,
		Int8,
		Int16,
		Int32,
		Int64,
		Int128,
		Float,
		Double,
	};

	Numeric(Kind kind, bool isSigned): Type{getName(kind), Type::Kind::Numeric}, kind_{kind}, isSinged_{isSigned} {}

	auto GetKind() const { return kind_; }
	bool IsSinged() const { return isSinged_; }

	template <typename T>
	static Numeric Create() {
		return Numeric{ detectKind<T>(), std::is_signed_v<T> };
	}

private:
	Kind kind_;
	bool isSinged_;

	static std::string getName(Kind kind) {
		switch (kind)
		{
		case Kind::Int8:
			return "Int8";
		case Kind::Int16:
			return "Int16";
		case Kind::Int32:
			return "Int32";
		case Kind::Int64:
			return "Int64";
		case Kind::Int128:
			return "Int128";
		case Kind::Float:
			return "Float";
		case Kind::Double:
			return "Double";
		}

		return "Unknow";
	}

	template <typename T>
	static Kind detectKind() {
		if constexpr (std::is_same_v<T, char>) {
			return Kind::Int8;
		} else 	if constexpr (std::is_same_v<T, short>) {
			return Kind::Int16;
		} else 	if constexpr (std::is_same_v<T, int>) {
			return Kind::Int32;
		} else 	if constexpr (std::is_same_v<T, long>) {
			return Kind::Int64;
		} else 	if constexpr (std::is_same_v<T, long long>) {
			return Kind::Int128;
		} else 	if constexpr (std::is_same_v<T, float>) {
			return Kind::Float;
		} else 	if constexpr (std::is_same_v<T, double>) {
			return Kind::Double;
		} else {
			return Kind::Unknow
		}
	}
};

class Enum : public Type {
public:
	struct Item {
		using value_type = long;

		std::string name;
		value_type value;
	};

	Enum() : Type{"Unknow_Enum", Type::Kind::Enum } {}
	Enum(const std::string& name): Type(name, Type::Kind::Enum) {}

	template <typename T>
	void Add(const std::string& name, T value) {
		items_.emplace_back(Item{ name, static_cast<typename Item::value_type>(value) });
	}

	auto& GetItems() const { return items_; }

private:
	std::vector<Item> items_;
};

struct MerberVariable {
	std::string name;
	const Type* type;

	template <typename T>
	static MerberVariable Create(const std::string& name);
};

struct MerberFunction {
	std::string name;
	const Type* retType;
	std::vector<const Type*> paramTypes;

	template <typename T>
	static MerberFunction Create(const std::string& name) {
		using traits = function_traits<T>;
		using args = typename traits::args;
		return MerberFunction{ name, GetType<typename traits::return_type>(),
			cvtTypelist2vector<args>(std::make_index_sequence<std::tuple_size_v<args>>)
		};
	}

private:
	template <typename Params, size_t... Idx>
	std::vector<const Type*> cvtTypelist2vector(std::index_sequence<Idx...>) {
		return { GetType<std::tuple_element_t<Idx>(Params)>(), ... };
	}
}; 

class Class: public Type {
public:
	Class(): Type{"Unknown_Class", Type::Kind::Class} {}
	Class(const std::string& name): Type{name, Type::Kind::Class} {}

	void AddVar(MerberVariable&& var) {
		vars_.emplace_back(std::move(var));
	}

	void AddFunc(MerberFunction&& func) {
		funcs_.emplace_back(std::move(func));
	}

private:
	std::vector<MerberVariable> vars_;
	std::vector<MerberFunction> funcs_;
};

template <typename T> 
class NumericFactory final {
public:
	static NumericFactory& Instance() {
		static NumericFactory inst{ Numeric::Create<T>() };
		static bool isSaved = false;
		if (!isSaved) {
			gTypeList.push_back(&inst.Info());
			isSaved = true;
		}
		return inst;
	}

	auto& Info() const { return info_; }
private:
	Numeric info_;

	NumericFactory(Numeric&& info) : info_(std::move(info)) {}
};

template <typename T>
class EnumFactory final {
public:
	static EnumFactory& Instance() {
		static EnumFactory inst;
		static bool isSaved = false;
		if (!isSaved) {
			gTypeList.push_back(&inst.Info());
			isSaved = true;
		}
		return inst;
	}

	auto& Info() const { return info_; }

	EnumFactory& Regist(const std::string& name) {
		info_.name_ = name;
		return *this;
	}

	template <typename U>
	EnumFactory& Add(const std::string& name, U value) {
		info_.Add(name, value);
		return *this;
	}
	
private:

	Enum info_;
};


template <typename T>
class ClassFactory final {
public:
	static ClassFactory& Instance() {
		static ClassFactory inst;
		static bool isSaved = false;
		if (!isSaved) {
			gTypeList.push_back(&inst.Info());
			isSaved = true;
		}
		return inst;
	}

	auto& Info() const { return info_; }

	ClassFactory& Regist(const std::string& name) {
		info_.name_ = name;
		return *this;
	}

	template <typename U>
	ClassFactory& AddVariable(const std::string& name) {
		info_.AddVar(MerberVariable::Create<U>(name));
		return *this;
	}

	template <typename U>
	ClassFactory& AddFunction(const std::string& name) {
		info_.AddVar(MerberFunction::Create<U>(name));
		return *this;
	}

	void Unregist() {
		info_ = Class{};
	}

private:
	Class info_;
};

class TrivialFactory {
public:
	static TrivialFactory& Instance() {
		static TrivialFactory inst;
		return inst;
	}
};

template <typename T> 
class Factory final {
public:
	static auto& GetFactory() {
		if constexpr (std::is_fundamental_v<T>) {
			return NumericFactory<T>::Instance();
		} else if constexpr (std::is_enum_v<T>) {
			return EnumFactory<T>::Instance();
		} else if constexpr (std::is_class_v<T>) {
			// return 
		} else {
			return TrivialFactory::Instance();
		}
 	}
};

template <typename T>
auto& Register() {
	return Factory<T>::GetFactory();
}

template <typename T>
const Type* GetType() {
	return &Factory<T>::GetFactory().Info();
}

const Type* GetType(const std::string& name) {
	for (auto typeinfo : gTypeList) {
		if (typeinfo->GetName() == name) {
			return typeinfo;
		}
	}

	return nullptr;
}

template <typename T>
MerberVariable MerberVariable::Create(const std::string& name) {
	using type = typename variable_traits<T>::type;
	return MerberVariable{ name, GetType<type>() };
}

int main() {
	return 0;
}