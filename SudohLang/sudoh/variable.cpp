#include "variable.h"
#include "runtime_ex.h"
#include <string>
#include <sstream>
#include <iostream>
#include <memory>

size_t Variable::VariableHash::operator()(const Variable& v) const
{
	switch (v.type)
	{
	case Type::number:
		return std::hash<double>()(v.val.numVal.val);
	case Type::boolean:
		return std::hash<bool>()(v.val.boolVal);
	case Type::map:
		return std::hash<Ref<Map>*>()(v.val.mapRef);
	case Type::list:
		return std::hash<Ref<List>*>()(v.val.listRef);
	case Type::string:
		return std::hash<std::string>()(v.val.stringVal);
	default:
		runtimeException("cannot add key 'null' to map");
	}
}

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
	case Type::map:
		return "map";
	case Type::null:
		return "null";
	}
}

Variable::Val::Val() : boolVal(false) {}
Variable::Val::Val(Number val) : numVal(val) {}
Variable::Val::Val(bool val) : boolVal(val) {}
Variable::Val::Val(std::string val) : stringVal(val) {}
Variable::Val::Val(Ref<List>* val) : listRef(val) {}
Variable::Val::Val(Ref<Map>* val) : mapRef(val) {}
Variable::Val::~Val() {}

Variable::Variable() : type(Type::null) {}
Variable::Variable(double n) : type(Type::number), val(Number(n)) {}
Variable::Variable(Number n) : type(Type::number), val(n) {}
Variable::Variable(bool b) : type(Type::boolean), val(b) {}
Variable::Variable(std::string s) : type(Type::string), val(s) {}
Variable::Variable(List l) : type(Type::list), val(new Ref<List>(l)) {}
Variable::Variable(Map m) : type(Type::map), val(new Ref<Map>(m)) {}

Variable::Variable(const Variable& other) : type(other.type) { setValue(other); }

Variable::~Variable() { freeMem(); }

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
		val.listRef = other.val.listRef;
		val.listRef->refCount++;
		break;
	case Type::map:
		val.mapRef = other.val.mapRef;
		val.mapRef->refCount++;
		break;
	}
}

void Variable::freeMem()
{
	if (type == Type::list)
	{
		if (--val.listRef->refCount == 0)
		{
			delete val.listRef;
		}
	}
	else if (type == Type::map)
	{
		if (--val.mapRef->refCount == 0)
		{
			delete val.mapRef;
		}
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
		if (val.numVal.isInt)
		{
			s.precision(0);
		}
		s << std::fixed << val.numVal.val;
		return s.str();
	}
	case Type::boolean:
		return std::to_string(val.boolVal);
	case Type::string:
		return val.stringVal;
	case Type::list:
	{
		std::string contents = "[ ";
		bool first = true;
		for (const Variable& e : val.listRef->val)
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
	case Type::map:
	{
		std::string contents = "{ ";
		bool first = true;
		for (std::pair<const Variable, Variable>& e : val.mapRef->val)
		{
			if (!first)
			{
				contents += ", ";
			}
			contents += e.first.toString() + " <- " + e.second.toString();
			first = false;
		}
		contents += " }";
		return contents;
	}
	case Type::null:
		return "null";
	}
}

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
}

Variable Variable::operator-(const Variable& other) const
{
	if (type == Type::number && other.type == Type::number)
	{
		return val.numVal - other.val.numVal;
	}

	runtimeException("illegal operation '-' between types " + typeString() + " and " + other.typeString());
}

Variable Variable::operator*(const Variable& other) const
{
	if (type == Type::number && other.type == Type::number)
	{
		return val.numVal * other.val.numVal;
	}

	runtimeException("illegal operation '*' between types " + typeString() + " and " + other.typeString());
}

Variable Variable::operator/(const Variable& other) const
{
	if (type == Type::number && other.type == Type::number)
	{
		return val.numVal / other.val.numVal;
	}

	runtimeException("illegal operation '/' between types " + typeString() + " and " + other.typeString());
}

Variable Variable::operator%(const Variable& other) const
{
	if (type == Type::number && other.type == Type::number)
	{
		return val.numVal % other.val.numVal;
	}

	runtimeException("illegal operation 'mod' between types " + typeString() + " and " + other.typeString());
}

void Variable::operator+=(const Variable& other)
{
	if (type == Type::number && other.type == Type::number)
	{
		val.numVal += other.val.numVal;
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
		val.numVal %= other.val.numVal;
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

bool Variable::operator==(const Variable& other) const
{
	if (type == Type::null || other.type == Type::null)
	{
		return type == other.type;
	}

	if (other.type != type)
	{
		runtimeException("illegal comparison between types " + typeString() + " and " + other.typeString());
	}

	switch (other.type)
	{
	case Type::number:
		return val.numVal == other.val.numVal;
	case Type::boolean:
		return val.boolVal == other.val.boolVal;
	case Type::string:
		return val.stringVal == other.val.stringVal;
	case Type::list:
		return val.listRef == other.val.listRef;
	case Type::map:
		return val.mapRef == other.val.mapRef;
	}
	return false;
}

bool Variable::operator!=(const Variable& other) const
{
	return !operator==(other);
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
		return val.numVal < other.val.numVal;
	case Type::string:
		return val.stringVal < other.val.stringVal;
	}

	runtimeException("illegal comparison between types " + typeString() + " and " + other.typeString());
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
		return val.numVal <= other.val.numVal;
	case Type::string:
		return val.stringVal <= other.val.stringVal;
	}

	runtimeException("illegal comparison between types " + typeString() + " and " + other.typeString());
}

bool Variable::operator>(const Variable& other) const
{
	return !operator<=(other);
}

bool Variable::operator>=(const Variable& other) const
{
	return !operator<(other);
}

// will only appear on left-hand side of expression
Variable& Variable::operator[](const Variable& index)
{
	switch (type)
	{
	case Type::string:
		runtimeException("illegal attempt to modify string");
	case Type::list:
	{
		if (index.type != Type::number || !index.val.numVal.isInt)
		{
			runtimeException("specified list index is not an integer");
		}
		if (index.val.numVal.val < 0)
		{
			runtimeException("cannot modify list at index below 0");
		}

		size_t idx = (size_t)index.val.numVal.val;
		List& list = val.listRef->val;

		// expand list if index above length
		for (size_t i = list.size(); i <= idx; i++)
		{
			list.push_back(null);
		}

		return list[idx];
	}
	case Type::map:
		return val.mapRef->val[index];
	}

	runtimeException("cannot index into type " + typeString());
}

// non-reference equivalent of [] operator; for accesses rather than modifications
Variable Variable::at(const Variable& index) const
{
	switch (type)
	{
	case Type::string:
	{
		if (index.type != Type::number || !index.val.numVal.isInt)
		{
			runtimeException("specified string index is not an integer");
		}
		double d = val.numVal.val;
		if (d < 0 || d >= val.stringVal.length())
		{
			runtimeException("specified string index out of bounds");
		}
		return std::string(1, val.stringVal[(size_t)index.val.numVal.val]);
	}
	case Type::list:
	{
		if (index.type != Type::number || !index.val.numVal.isInt)
		{
			runtimeException("specified list index is not an integer");
		}
		double d = val.numVal.val;
		if (d < 0 || d >= val.stringVal.length())
		{
			runtimeException("specified list index out of bounds");
		}
		return val.listRef->val[(size_t)index.val.numVal.val];
	}
	case Type::map:
		return val.mapRef->val[index];
	}
	
	runtimeException("cannot index into type " + typeString());
}

Variable::operator bool() const
{
	if (type == Type::boolean)
	{
		return val.boolVal;
	}
	runtimeException("expected boolean type");
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
		listIt = begin ? var->val.listRef->val.begin() : var->val.listRef->val.end();
		break;
	case Type::map:
		mapIt = begin ? var->val.mapRef->val.begin() : var->val.mapRef->val.end();
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
	case Type::map:
		mapIt++;
		break;
	}
}

Variable Variable::VariableIterator::operator*()
{
	switch (container->type)
	{
	case Type::string:
		return std::string(1, *stringIt);
	case Type::list:
		return *listIt;
	case Type::map:
		return mapIt->first;
	}
}

bool Variable::VariableIterator::operator!=(const VariableIterator& other)
{
	switch (container->type)
	{
	case Type::string:
		return stringIt != other.stringIt;
	case Type::list:
		return listIt != other.listIt;
	case Type::map:
		return mapIt != other.mapIt;
	}
}

Variable::VariableIterator Variable::begin()
{
	return VariableIterator(this, true);
}

Variable::VariableIterator Variable::end()
{
	return VariableIterator(this, false);
}
