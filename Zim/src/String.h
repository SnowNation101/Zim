#pragma once
#include <iostream>
#include <cstring>
#include <tchar.h>

#ifndef Z_STRING
#define Z_STRING

class String {
	// Prototype for stream insertion
	friend std::wostream& operator<<(std::wostream& os, const String& obj);

	// Prototype for stream extraction
	friend std::wistream& operator>>(std::wistream& is, String& obj);

	// Prototype for '+'
	// operator overloading
	friend String operator+(const String& str1, const String& str2);

private:
	TCHAR* str = new TCHAR;

public:
	// constructor
	String();
	String(const String& _str);
	String(const TCHAR* s);
	// destructor
	~String();
	// operator=
	String& operator= (const String& _str);
	String& operator= (const TCHAR* _str);
	String& operator= (const TCHAR c);

	friend std::wostream& operator<< (std::wostream& os, const String& st);
	// Capacity:
	// size
	size_t size() const { return _tcslen(str); }
	// length
	size_t length() const { return _tcslen(str); }
	// max_size
	// resize
	// capacity
	// reserve
	// clear
	// empty
	// shrink_to_fit
	// Element access:
	// operator[]
	TCHAR& operator[](size_t pos) const;
	// at
	TCHAR& at(size_t pos) const;
	// back
	TCHAR& back() const;
	// front
	TCHAR& front() const;

	// Modifiers:
	// operator+=
	// append
	String& append(const String& _str);
	String& append(const TCHAR* s);
	// push_back
	void push_back(const TCHAR c);
	// assign
	// insert
	void insert(size_t pos, const TCHAR c);
	// erase
	String& erase(size_t pos, size_t len);
	// replace
	// swap
	// pop_back

	// String operators
	// c_str
	const TCHAR* c_str() const;
	// data
	// get_allocator
	// copy
	// find
	// Searches the string for the first occurrence of the sequence specified by its arguments.
	size_t find(const String& str, size_t pos = 0) const;
	size_t find(const TCHAR* s, size_t pos = 0) const;
	size_t find(TCHAR c, size_t pos = 0) const;
	// rfind
	// find_first_of
	// find_last_of
	// find_first_not_of
	// find_last_not_of
	// substr
	// compare
};

#endif