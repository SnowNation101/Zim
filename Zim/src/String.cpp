#include "String.h"

// Overloading the plus operator
String operator+(const String& str1, const String& str2) {
	auto* buff = new TCHAR[_tcslen(str1.str) + _tcslen(str2.str) + 1];

	// Copy the strings to buff[]
	_tcscpy_s(buff, str1.size() + 1, str1.str);
	_tcscat_s(buff, _tcslen(str1.str) + _tcslen(str2.str) + 1, str2.str);

	// String temp
	String temp{ buff };

	// delete the buff[]
	delete[] buff;

	// Return the concatenated string
	return temp;
}

// Overloading the stream
// extraction operator
std::wistream& operator>>(std::wistream& is, String& obj)
{
	TCHAR* buff = new TCHAR[1000];
	memset(&buff[0], 0, sizeof(buff));
	is >> buff;
	obj = String{ buff };
	delete[] buff;
	return is;
}

// Overloading the stream
// insertion operator
std::wostream& operator<<(std::wostream& os, const String& obj)
{
	os << obj.str;
	return os;
}

String::String() : str(new TCHAR[1])
{
	str[0] = '\0';
}

String::String(const String& _str) : str(new TCHAR[_str.size() + 1])
{
	_tcscpy_s(str, _str.size() + 1, _str.str);
}

String::String(const TCHAR* s)
{
	if (s == nullptr) {
		str = new TCHAR[1];
		str[0] = '\0';
	}
	else
	{
		str = new TCHAR[_tcslen(s) + 1];
		_tcscpy_s(str, _tcslen(s) + 1, s);
	}
	
}

String::~String()
{
	delete[] str;
}

String& String::operator=(const String& _str)
{
	if (this == &_str) return *this;
	delete[] str;
	str = new TCHAR[_tcslen(_str.str) + 1];
	_tcscpy_s(str, _str.size() + 1, _str.str);
	return *this;
}

String& String::operator=(const TCHAR* _str)
{
	delete[] str;
	str = new TCHAR[_tcslen(_str) + 1];
	_tcscpy_s(str, _tcslen(_str) + 1, _str);
	return *this;
}

TCHAR& String::operator[](size_t pos) const
{
	return str[pos];
}

TCHAR& String::at(size_t pos) const
{
	return str[pos];
}

TCHAR& String::back() const
{
	if (_tcslen(str) == 0) return str[0];
	return str[_tcslen(str) - 1];
}

TCHAR& String::front() const
{
	return str[0];
}

String& String::append(const String& _str)
{
	str = (TCHAR*)realloc(str, (_tcslen(str) + _str.size() + 1) * 2);
	if (str) { _tcscat_s(str, _tcslen(str) + _str.size() + 1, _str.str); }
	return *this;
}

String& String::append(const TCHAR* s)
{
	str = (TCHAR*)realloc(str, (_tcslen(str) + _tcslen(s) + 1) * 2);
	if (str) { _tcscat_s(str, _tcslen(str) + _tcslen(s) + 1, s); }
	return *this;
}

void String::push_back(const TCHAR c)
{
	auto* tmp = (TCHAR*)realloc(str, (_tcslen(str) + 2) * 2);
	if (tmp) str = tmp;
	const TCHAR temp[2] = { c, '\0' };
	if (str) { _tcscat_s(str, _tcslen(str) + 2, temp); }
}

void String::insert(size_t pos, const TCHAR c)
{
	auto* tmp = (TCHAR*)realloc(str, (_tcslen(str) + 2) * 2);
	if (tmp)
	{
		str = tmp;
	}

	if (str)
	{
		String temp;
		for (size_t i = pos; i < _tcslen(str); i++)
		{
			temp.push_back(str[i]);
		}
		str[pos] = c;
		str[pos + 1] = '\0';

		_tcscat_s(str, _tcslen(str) + temp.size() + 1, temp.c_str());
	}
}

String& String::erase(size_t pos, size_t len)
{
	if ((pos + len) > _tcslen(str))
	{
		str[pos] = '\0';
	}
	else
	{
		for (size_t i = (pos + len); i <= _tcslen(str); i++)
		{
			str[i - len] = str[i];
		}
	}
	return *this;
}

const TCHAR* String::c_str() const
{
	return str;
}

// Fills next[] for given pattern pat[0..M-1]
void get_next_array(const TCHAR* pat, size_t* next)
{
	const size_t pattern_size = _tcslen(pat);
	// length of the previous longest prefix suffix
	size_t len = 0;

	next[0] = 0; // next[0] is always 0

	// the loop calculates next[i] for i = 1 to M-1
	size_t i = 1;
	while (i < pattern_size) {
		if (pat[i] == pat[len]) {
			len++;
			next[i] = len;
			i++;
		}
		else
		{
			if (len != 0) {
				len = next[len - 1];
			}
			else // if (len == 0)
			{
				next[i] = 0;
				i++;
			}
		}
	}
}

size_t String::find(const String& _str, size_t pos) const
{
	const size_t text_size = _tcslen(str);
	const size_t pattern_size = _str.size();

	// create next[] that will hold the longest prefix suffix values for pattern
	auto* next = new size_t[pattern_size];

	// Preprocess the pattern (calculate next[] array)
	get_next_array(_str.c_str(), next);

	size_t i = pos; // index for str
	size_t j = 0; // index for pattern

	while (i < text_size) {
		if (_str[j] == str[i]) {
			j++;
			i++;
		}

		if (j == pattern_size) {
			return i - j;
		}

		// mismatch after j matches
		if (i < text_size && _str[j] != str[i]) {
			// Do not match next[0..next[j-1]] characters, they will match anyway
			if (j != 0) j = next[j - 1];
			else  i++;
		}
	}
	return -1;
}

size_t String::find(const TCHAR* s, size_t pos) const
{
	const size_t text_size = _tcslen(str);
	const size_t pattern_size = _tcslen(s);

	// create next[] that will hold the longest prefix suffix values for pattern
	auto* next = new size_t[pattern_size];

	// Preprocess the pattern (calculate next[] array)
	get_next_array(s, next);

	size_t i = pos; // index for str
	size_t j = 0; // index for pattern

	while (i < text_size) {
		if (s[j] == str[i]) {
			j++;
			i++;
		}

		if (j == pattern_size) {
			return i - j;
		}

		// mismatch after j matches
		if (i < text_size && s[j] != str[i]) {
			// Do not match next[0..next[j-1]] TCHARacters, they will match anyway
			if (j != 0) j = next[j - 1];
			else  i++;
		}
	}
	return -1;
}

size_t String::find(TCHAR c, size_t pos) const
{
	for (size_t i = pos; i < _tcslen(str); i++)
	{
		if (str[i] == c) return i;
	}
	return -1;
}