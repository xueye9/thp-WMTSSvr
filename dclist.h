/*!
 * \file dclist.h
 * \author Xuebingbing
 * \brief 双向循环列表
 *
 * TODO: long description
 * \date 七月 2015
 * \copyright	Copyright (c) 2015, 北京国科恒通电气自动化科技有限公司\n
	All rights reserved.
 */

#ifndef DC_LIST_H_
#define DC_LIST_H_
#include <list>

template<class T, class _Ax = std::allocator<T> >
class dclist : public std::list<T, _Ax>
{
public:
	void move_head_forward()
	{
		T v = _Myhead->_Myval;
		_Myhead->_Myval = _Myhead->_Prev->_Myval;
		_Myhead->_Prev->_Myval = v;
		_Myhead = _Myhead->_Prev;
	}

	iterator insert_n(size_type _pos, const T& _Val)
	{
		if(0 == _pos)
		{
			push_front(_Val);
			return begin();
		}

		if(_pos >= _Mysize)
			return end();

		const_iterator it = begin();
		for(size_type i = 0; i < _pos; ++i)
			++it;

		return insert(it, _Val);
	}
};

#endif// DC_LIST_H_