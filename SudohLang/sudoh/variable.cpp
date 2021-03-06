#include "variable.h"
#include "runtime_ex.h"
#include <string>
#include <sstream>
#include <iostream>
#include <memory>
#include <cmath>

// epsilon used for checking if a number (inherently type double) can be said to be an integer
constexpr double EPSILON = 0.0001;

// +------------------------------------------+
// |   Helper functions for asserting valid   |
// |   types for various operations           |
// +------------------------------------------+

bool Variable::stringCheck(const Variable& var, const std::string*& out)
{
	if (var.type == Type::string)
	{
		out = &var.val.stringVal;
		return true;
	}
	return false;
}

bool Variable::indexCheck(const Variable& var, size_t& out)
{
	if (var.type == Type::number)
	{
		double rounded = round(var.val.numVal);
		if (rounded >= 0.0 && rounded < SIZE_MAX && abs(rounded - var.val.numVal) < EPSILON)
		{
			out = (size_t)rounded;
			return true;
		}
	}
	return false;
}

bool Variable::numCheck(const Variable& var, double& out)
{
	if (var.type == Type::number)
	{
		out = var.val.numVal;
		return true;
	}
	return false;
}

bool Variable::listCheck(const Variable& var, List*& out)
{
	if (var.type == Type::list)
	{
		out = var.val.listRef.get();
		return true;
	}
	return false;
}

size_t assertValidIndex(const std::string& containerType, const Variable& index)
{
	size_t idx;
	if (!Variable::indexCheck(index, idx))
	{
		runtimeException("specified index into " + containerType + " must be a positive integer in the range [0, 2^" +
			std::to_string(sizeof(size_t) * 8) + " - 1)");
	}
	return idx;
}

// +-----------------------------+
// |   Variable implementation   |
// +-----------------------------+

std::string Variable::typeString() const
{
	switch (type)
	{
	case Type::number:
		return "number";
	case Type::boolean:
		return "boolean";
	case Type::string:
		return "string";
	case Type::list:
		return "list";
	case Type::object:
		return "object";
	default:
		return "null";
	}
}

Variable::Val::Val() : boolVal(false) {}
Variable::Val::Val(bool val) : boolVal(val) {}
Variable::Val::Val(double val) : numVal(val) {}
Variable::Val::Val(std::string val) : stringVal(val) {}
Variable::Val::Val(std::shared_ptr<List> val) : listRef(val) {}
Variable::Val::Val(std::shared_ptr<Object> val) : objRef(val) {}
Variable::Val::~Val() {}

Variable::Variable() : type(Type::null) {}
Variable::Variable(double n) : type(Type::number), val(n) {}
Variable::Variable(int n) : type(Type::number), val((double)n) {}
Variable::Variable(bool b) : type(Type::boolean), val(b) {}
Variable::Variable(std::string s) : type(Type::string), val(s) {}
Variable::Variable(std::shared_ptr<List> l) : type(Type::list), val(l) {}
Variable::Variable(std::shared_ptr<Object> m) : type(Type::object), val(m)
{
	for (auto& kv : *m)
	{
		if (kv.first.type != Type::string)
		{
			runtimeException("object field identifier must be a string; got type '" + kv.first.typeString() + "'");
		}
	}
}

Variable::Variable(const Variable& other) : type(other.type) { setValue(other); }

Variable::~Variable() { freeMem(); }

// helper function for copy constructor/assignment operator to copy value from other Variable
void Variable::setValue(const Variable& other)
{
	switch (other.type)
	{
	case Type::number:
		val.numVal = other.val.numVal;
		break;
	case Type::boolean:
		val.boolVal = other.val.boolVal;
		break;
	case Type::string:
		new(&val.stringVal) std::string(other.val.stringVal);
		break;
	case Type::list:
		new(&val.listRef) std::shared_ptr<List>(other.val.listRef);
		break;
	case Type::object:
		new(&val.listRef) std::shared_ptr<Object>(other.val.objRef);
		break;
	}
}

// helper function for freeing any allocated memory if needed; used by
// destructor and on old value for assignment operator
void Variable::freeMem()
{
	if (type == Type::list)
	{
		val.listRef.~shared_ptr();
	}
	else if (type == Type::object)
	{
		val.objRef.~shared_ptr();
	}
	else if (type == Type::string)
	{
		val.stringVal.~basic_string();
	}
}

std::string Variable::toString() const
{
	switch (type)
	{
	case Type::number:
	{
		std::stringstream s;
		double d = val.numVal;
		if (abs(round(d) - d) < EPSILON)
		{
			s.precision(0);
		}
		s << std::fixed << d;
		return s.str();
	}
	case Type::boolean:
		return val.boolVal ? "true" : "false";
	case Type::string:
		return val.stringVal;
	case Type::list:
	{
		std::string contents = "[ ";
		bool first = true;
		for (const Variable& e : *val.listRef)
		{
			if (!first)
			{
				contents += ", ";
			}
			contents += e.toString();
			first = false;
		}
		contents += " ]";
		return contents;
	}
	case Type::object:
	{
		std::string contents = "{ ";
		bool first = true;
		for (auto& e : *val.objRef)
		{
			if (!first)
			{
				contents += ", ";
			}
			contents += e.first.val.stringVal + " <- " + e.second.toString();
			first = false;
		}
		contents += " }";
		return contents;
	}
	default:
		return "null";
	}
}

// +------------------------------------------------------------+
// |   Binary arithmetic operators; all arithmetic operators    |
// |   except '+' only valid between 2 numbers ('+' also used   |
// |   for string concatenation                                 |
// +------------------------------------------------------------+

Variable Variable::operator+(const Variable& other) const
{
	switch (type)
	{
	case Type::number:
		if (other.type == Type::string)
		{
			return toString() + other.val.stringVal;
		}
		if (other.type == Type::number)
		{
			return val.numVal + other.val.numVal;
		}
		break;
	case Type::string:
		return val.stringVal + other.toString();
	}

	runtimeException("illegal operation '+' between types " + typeString() + " and " + other.typeString());
	return Variable();
}

Variable Variable::operator-(const Variable& other) const
{
	if (type == Type::number && other.type == Type::number)
	{
		return val.numVal - other.val.numVal;
	}

	runtimeException("illegal operation '-' between types " + typeString() + " and " + other.typeString());
	return Variable();
}

Variable Variable::operator*(const Variable& other) const
{
	if (type == Type::number && other.type == Type::number)
	{
		return val.numVal * other.val.numVal;
	}

	runtimeException("illegal operation '*' between types " + typeString() + " and " + other.typeString());
	return Variable();
}

Variable Variable::operator/(const Variable& other) const
{
	if (type == Type::number && other.type == Type::number)
	{
		return val.numVal / other.val.numVal;
	}

	runtimeException("illegal operation '/' between types " + typeString() + " and " + other.typeString());
	return Variable();
}

Variable Variable::operator%(const Variable& other) const
{
	if (type == Type::number && other.type == Type::number)
	{
		return fmod(val.numVal, other.val.numVal);
	}

	runtimeException("illegal operation 'mod' between types " + typeString() + " and " + other.typeString());
	return Variable();
}

// +--------------------------------------------------------------+
// |   Compound assignment operators; there is no direct          |
// |   syntax for accomplishing these in Sudoh, but something     |
// |   like 'a <- a + 1' will be translated to use '+=' for 'a'   |
// +--------------------------------------------------------------+

void Variable::operator+=(const Variable& other)
{
	if (type == Type::number && other.type == Type::number)
	{
		val.numVal += other.val.numVal;
	}
	else if (type == Type::string && other.type == Type::string)
	{
		val.stringVal += other.val.stringVal;
	}
	else
	{
		runtimeException("illegal compound addition assignment operation between types " + typeString() + " and " + other.typeString());
	}
}

void Variable::operator-=(const Variable& other)
{
	if (type == Type::number && other.type == Type::number)
	{
		val.numVal -= other.val.numVal;
	}
	else
	{
		runtimeException("illegal compound subtraction assignment between types " + typeString() + " and " + other.typeString());
	}
}

void Variable::operator*=(const Variable& other)
{
	if (type == Type::number && other.type == Type::number)
	{
		val.numVal *= other.val.numVal;
	}
	else
	{
		runtimeException("illegal compound multiplication assignment between types " + typeString() + " and " + other.typeString());
	}
}

void Variable::operator/=(const Variable& other)
{
	if (type == Type::number && other.type == Type::number)
	{
		val.numVal /= other.val.numVal;
	}
	else
	{
		runtimeException("illegal compound division assignment between types " + typeString() + " and " + other.typeString());
	}
}

void Variable::operator%=(const Variable& other)
{
	if (type == Type::number && other.type == Type::number)
	{
		val.numVal = fmod(val.numVal, other.val.numVal);
	}
	else
	{
		runtimeException("illegal operation 'mod' between types " + typeString() + " and " + other.typeString());
	}
}

Variable& Variable::operator=(const Variable& other)
{
	if (this != &other)
	{
		freeMem();
		type = other.type;
		setValue(other);
	}
	return *this;
}

// +-----------------------------------------------------------+
// |   Comparison operators; all Sudoh comparison ops except   |
// |   for '=' and '!=' are only valid between values of the   |
// |   same type ('='/'!=' valid for null as well              |
// +-----------------------------------------------------------+

double maybeIntVal(double val)
{
	double rounded = round(val);
	return abs(rounded - val) < EPSILON ? rounded : val;
}

bool Variable::operator==(const Variable& other) const
{
	if (type == Type::null || other.type == Type::null)
	{
		return type == other.type;
	}

	if (other.type != type) // TODO maybe make valid between different types; just return false
	{
		runtimeException("illegal comparison between types " + typeString() + " and " + other.typeString());
	}

	switch (other.type)
	{
	case Type::number:
		return maybeIntVal(val.numVal) == maybeIntVal(other.val.numVal);
	case Type::boolean:
		return val.boolVal == other.val.boolVal;
	case Type::string:
		return val.stringVal == other.val.stringVal;
	case Type::list:
		return val.listRef == other.val.listRef;
	case Type::object:
		return val.objRef == other.val.objRef;
	}
	return false;
}

bool Variable::operator!=(const Variable& other) const
{
	return !(*this == other);
}

bool Variable::operator<(const Variable& other) const
{
	if (other.type != type)
	{
		runtimeException("illegal comparison between types " + typeString() + " and " + other.typeString());
	}

	switch (other.type)
	{
	case Type::number:
		return maybeIntVal(val.numVal) < maybeIntVal(other.val.numVal);
	case Type::string:
		return val.stringVal < other.val.stringVal;
	}

	runtimeException("illegal comparison between types " + typeString() + " and " + other.typeString());
	return false;
}

bool Variable::operator<=(const Variable& other) const
{
	if (other.type != type)
	{
		runtimeException("illegal comparison between types " + typeString() + " and " + other.typeString());
	}

	switch (other.type)
	{
	case Type::number:
		return maybeIntVal(val.numVal) <= maybeIntVal(other.val.numVal);
	case Type::string:
		return val.stringVal <= other.val.stringVal;
	}

	runtimeException("illegal comparison between types " + typeString() + " and " + other.typeString());
	return false;
}

bool Variable::operator>(const Variable& other) const
{
	return !operator<=(other);
}

bool Variable::operator>=(const Variable& other) const
{
	return !operator<(other);
}

// +-----------------------------------------------------------+
// |   Indexing operators valid for string, list, and object   |
// |   values. Transpiled [] returns a reference and is        |
// |   used for an index operation on the left side of an      |
// |   assignment, .at() otherwise                             |
// +-----------------------------------------------------------+

Variable& Variable::operator[](const Variable& index)
{
	switch (type)
	{
	case Type::string:
		runtimeException("illegal attempt to modify string");
	case Type::list:
	{
		List& list = *val.listRef;

		size_t idx = assertValidIndex("string", index);

		// expand list if index above length
		for (size_t i = list.size(); i <= idx; i++)
		{
			list.push_back(Variable());
		}

		return list[idx];
	}
	case Type::object:
		if (index.type != Type::string)
		{
			runtimeException("index into object must be of type 'string'");
		}
		return (*val.objRef)[index];
	}

	runtimeException("cannot index into type " + typeString());
	return *this;
}

Variable Variable::at(const Variable& index) const
{
	switch (type)
	{
	case Type::string:
	{
		size_t idx = assertValidIndex("string", index);
		if (idx >= val.stringVal.length())
		{
			runtimeException("specified index '" + std::to_string(idx) + "' out of bounds of string (length " +
				std::to_string(val.stringVal.length()) + ")");
		}
		return std::string(1, val.stringVal[idx]);
	}
	case Type::list:
	{
		size_t idx = assertValidIndex("list", index);
		if (idx >= val.listRef->size())
		{
			runtimeException("specified index '" + std::to_string(idx) + "' out of bounds of list (length " +
				std::to_string(val.listRef->size()) + ")");
		}
		return (*val.listRef)[idx];
	}
	case Type::object:
	{
		Object& o = *val.objRef;
		if (index.type != Type::string)
		{
			runtimeException("index into object must be of type 'string'");
		}

		auto item = o.find(index);
		if (item == o.end())
		{
			runtimeException("field '" + index.val.stringVal + "' does not exist in the object");
		}
		return item->second;
	}
	}
	
	runtimeException("cannot index into type " + typeString());
	return Variable();
}

// for converting a boolean variable to type bool for a condition
Variable::operator bool() const
{
	if (type == Type::boolean)
	{
		return val.boolVal;
	}
	runtimeException("expected boolean type");
	return false;
}

// +-------------------------------------+
// |   VariableIterator implementation   |
// +-------------------------------------+

Variable::VariableIterator::VariableIterator(Variable* var, bool begin) : container(var)
{
	switch (var->type)
	{
	case Type::string:
		stringIt = begin ? var->val.stringVal.begin() : var->val.stringVal.end();
		break;
	case Type::list:
		listIt = begin ? var->val.listRef->begin() : var->val.listRef->end();
		break;
	case Type::object:
		objIt = begin ? var->val.objRef->begin() : var->val.objRef->end();
		break;
	default:
		runtimeException("cannot iterate over type " + var->typeString());
	}
}

void Variable::VariableIterator::operator++()
{
	switch (container->type)
	{
	case Type::string:
		stringIt++;
		break;
	case Type::list:
		listIt++;
		break;
	case Type::object:
		objIt++;
		break;
	}
}

Variable Variable::VariableIterator::operator*()
{
	if (container->type == Type::string)
	{
		return std::string(1, *stringIt);
	}
	if (container->type == Type::list)
	{
		return *listIt;
	}

	return objIt->first;
}

bool Variable::VariableIterator::operator!=(const VariableIterator& other)
{
	if (container->type == Type::string)
	{
		return stringIt != other.stringIt;
	}
	if (container->type == Type::list)
	{
		return listIt != other.listIt;
	}

	return objIt != other.objIt;
}

Variable::VariableIterator Variable::begin()
{
	return VariableIterator(this, true);
}

Variable::VariableIterator Variable::end()
{
	return VariableIterator(this, false);
}

bool Variable::ObjectComp::operator()(const Variable& left, const Variable& right) const
{
	if (left.type != Type::string)
	{
		runtimeException("object field identifier must be a string; got type '" + left.typeString() + "'");
	}
	if (right.type != Type::string)
	{
		runtimeException("object field identifier must be a string; got type '" + right.typeString() + "'");
	}
	return left.val.stringVal < right.val.stringVal;
}
