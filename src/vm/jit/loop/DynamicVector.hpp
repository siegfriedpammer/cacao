#ifndef _DYNAMIC_VECTOR_HPP
#define _DYNAMIC_VECTOR_HPP

#include <vector>

template<class T>
class DynamicVector
{
	std::vector<T> _vec;

public:

	typedef typename std::vector<T>::iterator iterator;
	typedef typename std::vector<T>::const_iterator const_iterator;
	
	iterator begin();
	const_iterator begin() const;

	iterator end();
	const_iterator end() const;

	/**
	 * Returns the element at the specified index.
	 * Resizes the vector if the index does not exist yet.
	 */
	T& operator[](size_t index);
	const T& operator[](size_t index) const;

	size_t size() const;

	void resize(size_t size);

/*
	void reserve(size_t size);
	void push_back(const T&);
*/
};

template<class T>
inline typename DynamicVector<T>::iterator DynamicVector<T>::begin()
{
	return _vec.begin();
}

template<class T>
inline typename DynamicVector<T>::const_iterator DynamicVector<T>::begin() const
{
	return _vec.begin();
}

template<class T>
inline typename DynamicVector<T>::iterator DynamicVector<T>::end()
{
	return _vec.end();
}

template<class T>
inline typename DynamicVector<T>::const_iterator DynamicVector<T>::end() const
{
	return _vec.end();
}

template<class T>
inline T& DynamicVector<T>::operator[](size_t index)
{
	if (index >= _vec.size())
		_vec.resize(index + 1);
	return _vec[index];
}

template<class T>
inline const T& DynamicVector<T>::operator[](size_t index) const
{
	assert(index < _vec.size());
	return _vec[index];
}

template<class T>
inline size_t DynamicVector<T>::size() const
{
	return _vec.size();
}

template<class T>
inline void DynamicVector<T>::resize(size_t size)
{
	_vec.resize(size);
}

/*
template<class T>
inline void DynamicVector<T>::reserve(size_t size)
{
	_vec.reserve(size);
}

template<class T>
inline void DynamicVector<T>::push_back(const T& elem)
{
	_vec.push_back(elem);
}*/

#endif
