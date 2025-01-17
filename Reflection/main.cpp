#include "function_traits.h"
#include "variable_traits.h"
#include "srfle.h"
#include "type_list.h"

#include <string>
#include <iostream>
#include <vector>
#include <list>
#include <cassert>

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

class any;

template <typename T>
any make_copy(const T&);

template <typename T>
any make_steal(T&&);

template <typename T>
any make_ref(T&);

template <typename T>
any make_cref(const T&);

class any final {
public:
	enum class storage_type {
		Empty,
		Copy,
		Steal,
		Ref,
		ConstRef,
	};

	struct operations {
		any(*copy) (const any&) = {}; // 函数指针
		any(*steal) (any&) = {};
		void(*release) (any&) = {};
	};

	any() = default;

	any(const any& o) : typeinfo(o.typeinfo), store_type(o.store_type), ops(o.ops) {
		if (ops.copy) {
			auto new_any = ops.copy(o);
			payload = new_any.payload;
			new_any.payload = nullptr;
			new_any.ops.release = nullptr;
		}
		else {
			store_type = storage_type::Empty;
			typeinfo = nullptr;
		}
	}

	any(any&& o)
		: typeinfo(std::move(o.typeinfo)), payload(std::move(o.payload)),
		store_type(std::move(o.store_type)), ops(std::move(o.ops)) {}

	any& operator = (const any& o) {
		if (&o != this) {
			typeinfo = o.typeinfo;
			store_type = o.store_type;
			ops = o.ops;
			if (ops.copy) {
				auto new_any = ops.copy(o);
				payload = new_any.payload;
				new_any.payload = nullptr;
				new_any.ops.release = nullptr;
			}
			else {
				store_type = storage_type::Empty;
				typeinfo = nullptr;
			}
		}

		return *this;
	}

	any& operator = (any&& o) {
		if (&o != this) {
			typeinfo = std::move(o.typeinfo);
			store_type = std::move(o.store_type);
			ops = std::move(o.ops);
			payload = std::move(o.payload);
		}
		return *this;
	}

	~any() {
		if (ops.release && (store_type == storage_type::Copy || store_type == storage_type::Steal)) {
			ops.release(*this);
		}
	}


	Type* typeinfo{};
	void* payload{};
	storage_type store_type = storage_type::Empty;
	operations ops;
};

template <typename T>
struct operations_traits {
	static any copy(const any& elem) {
		assert(elem.typeinfo == GetType<T>());

		any return_value;
		return_value.payload = new T{ *static_cast<T*>(elem.payload) };
		return_value.typeinfo = elem.typeinfo;
		return_value.store_type = any::storage_type::Copy;
		return_value.ops = elem.ops;
		return return_value;
	}

	static any steal(any& elem) {
		assert(elem.typeinfo == GetType<T>());

		any return_value;
		return_value.payload = new T{ std::move(*static_cast<T*>(elem.payload)) };
		return_value.typeinfo = elem.typeinfo;
		return_value.store_type = any::storage_type::Copy;
		elem.store_type = any::storage_type::Steal;
		return_value.ops = elem.ops;
		return return_value;
	}

	static void release(any& elem) {
		assert(elem.typeinfo == GetType<T>());

		delete (T*)(elem.payload);
		elem.payload = nullptr;
		elem.store_type = any::store_type::Empty;
		elem.typeinfo = nullptr;
	}
};

template <typename T>
any make_copy(const T& elem) {
	any return_value;
	return_value.payload = new T{ elem };
	return_value.typeinfo = GetType<T>();
	return_value.store_type = any::storage_type::Copy;
	if constexpr (std::is_copy_constructible_v<T>) {
		return_value.ops.copy = &operations_traits<T>::copy;
	}
	if constexpr (std::is_move_constructible_v) {
		return_value.ops.steal = &operations_traits<T>::steal;
	}
	if constexpr (std::is_destructible_v) {
		return_value.ops.release = &operations_traits<T>::release;
	}
	return return_value;
}

template <typename T>
any make_steal(T&& elem) {
	any return_value;
	return_value.payload = new T{ std::move(elem) };
	return_value.typeinfo = GetType<T>();
	return_value.store_type = any::storage_type::Steal;
	if constexpr (std::is_copy_constructible_v<T>) {
		return_value.ops.copy = &operations_traits<T>::copy;
	}
	if constexpr (std::is_move_constructible_v) {
		return_value.ops.steal = &operations_traits<T>::steal;
	}
	if constexpr (std::is_destructible_v) {
		return_value.ops.release = &operations_traits<T>::release;
	}
	return return_value;
}

template <typename T>
any make_ref(T& elem) {
	any return_value;
	return_value.payload = &elem;
	return_value.typeinfo = GetType<T>();
	return_value.store_type = any::storage_type::Ref;
	if constexpr (std::is_copy_constructible_v<T>) {
		return_value.ops.copy = &operations_traits<T>::copy;
	}
	if constexpr (std::is_move_constructible_v) {
		return_value.ops.steal = &operations_traits<T>::steal;
	}
	if constexpr (std::is_destructible_v) {
		return_value.ops.release = &operations_traits<T>::release;
	}
	return return_value;
}

template <typename T>
any make_cref(const T& elem) {
	any return_value;
	return_value.payload = &elem;
	return_value.typeinfo = GetType<T>();
	return_value.store_type = any::storage_type::ConstRef;
	if constexpr (std::is_copy_constructible_v<T>) {
		return_value.ops.copy = &operations_traits<T>::copy;
	}
	if constexpr (std::is_move_constructible_v) {
		return_value.ops.steal = &operations_traits<T>::steal;
	}
	if constexpr (std::is_destructible_v) {
		return_value.ops.release = &operations_traits<T>::release;
	}
	return return_value;
}

template <typename T>
T* try_cast(any& elem) {
	if (elem.typeinfo = GetType<T>()) {
		return (T*)(elem.payload);
	}
	else {
		return nullptr;
	}
}

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

	void SetValue(double value, any& elem) {
		if (elem.typeinfo->GetKind() == Type::Kind::Numeric) {
			switch (elem.typeinfo->AsNumeric()->GetKind()) {
			case Kind::Int8:
				if (elem.typeinfo->AsNumeric()->IsSinged()) {
					*(int8_t*)elem.payload = static_cast<int8_t>(value);
				}
				else {
					*(uint8_t*)elem.payload = static_cast<uint8_t>(value);
				}
				break;
			case Kind::Int16:
				*(int16_t*)elem.payload = static_cast<int16_t>(value);
				break;
			case Kind::Int32:
				*(int32_t*)elem.payload = static_cast<int32_t>(value);
				break;
			case Kind::Int64:
				*(int64_t*)elem.payload = static_cast<int64_t>(value);
				break;
			case Kind::Float:
				*(float*)elem.payload = static_cast<float>(value);
				break;
			case Kind::Double:
				*(double*)elem.payload = value;
				break;
			case Kind::Unknow:
				// 处理未知类型，最好是抛出异常或记录日志，而不是断言
				throw std::runtime_error("Unknown numeric type");
				break;
			default:
				assert(false);  // 处理意外情况
			}
		}
	}

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

class Member {
public:
	virtual any call(const std::vector<any>& anies) = 0;
};

template <typename Clazz, typename Type>
struct MerberVariable : public Merber {
	std::string name;
	const Type* type;
	Type Clazz::* ptr;

	any call(const std::vector<any>& anies) override {
		assert(anies.size() == 1 && anies[0].typeinfo == GetType<Clazz>());
		Clazz* instance = (Clazz*)anies[0].payload;
		auto value = instance->*ptr;
		return make_copy(value);
	}

	template <typename T>
	static MerberVariable Create(const std::string& name);
};

template <typename T>
T& unwrap(any& value) {
	assert(value.typeinfo == GetType<T>());
	return *(T*)value.payload;
}

template <typename Clazz, typename RetType, size_t... Idx, typename... Args>
any inner_call(RetType(Clazz::* ptr)(Args...), const std::vector<any>& params, std::index_sequence<Idx...>) {
	auto return_value =
		((Clazz*)param[0].payload->*ptr)(unwrap<Args>(params[Idx + 1])...);
	return make_copy(return_value);
}

template <typename Clazz, typename Type, typename... Args>
struct MerberFunction : public Merber {
	std::string name;
	const Type* retType;
	std::vector<const Type*> paramTypes;
	RetType(Clazz::* ptr)(Args...);

	any call(const std::vector<any>& anies) override {
		assert(anies.size() == paramTypes.size() + 1);
		for (int i = 0; i < paramTypes.size(); i++) {
			assert(paramTypes[i] == anies[i + 1].typeinfo);
		}

		return inner_call(ptr, anies, std::make_index_sequence<sizeof...(Args)>());
	}

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