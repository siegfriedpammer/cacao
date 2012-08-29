#ifndef _DYNAMIC_VECTOR_HPP
#define _DYNAMIC_VECTOR_HPP

template<class T>
class DynamicVector
{
	std::vector<T> _vec;

public:
	
	/**
	 * Returns the element at the specified index.
	 * Resizes the vector if the index does not exist yet.
	 */
	T& operator[](size_t index);
	const T& operator[](size_t index) const;

	size_t size() const;

/*
	void reserve(size_t size);
	void push_back(const T&);
*/
};

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
	if (index >= _vec.size())
		_vec.resize(index + 1);
	return _vec[index];
}

template<class T>
inline size_t DynamicVector<T>::size() const
{
	return _vec.size();
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
