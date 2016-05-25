#pragma once

#include <string>

//typedef boost::spirit::classic::position_iterator<std::string::const_iterator> pos_iterator_type;

typedef std::string::const_iterator str_it;

extern str_it g_lastIterator;

struct pos_iterator_type : public str_it
{
	pos_iterator_type(): isEnd(false){}
	pos_iterator_type(str_it& _it, bool _isEnd = false)
		: str_it(_it), isEnd(_isEnd){}

	bool isEnd;
	~pos_iterator_type()
	{
		if (!isEnd && this->_Ptr && g_lastIterator._Ptr && g_lastIterator < *this) g_lastIterator = *this;
	}

/*	pos_iterator_type& operator++()
	{
		g_iteratorPointer++;
		(*(str_it*)this)++;

		return *this;
	}

	pos_iterator_type& operator--()
	{
		g_iteratorPointer--;
		(*(str_it*)this)--;

		return *this;
	}*/
};