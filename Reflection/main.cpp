#include "function_traits.h"
#include "variable_traits.h"
#include "srfle.h"
#include "type_list.h"

#include <string>
#include <iostream>

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

class Type {
public:
	virtual ~Type() = default;

	Type(const std::string& name): name_(name) {}

private:
	std::string name_;
};

class Numeric : public Type {
public:
};

class Enum : public Type {
public:
};

class Class: public Type {
public:
};

int main() {
	return 0;
}