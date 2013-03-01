/*
 * ContainerUtils.h
 *
 *  Created on: 02.02.2013
 *      Author: dennis
 */

#ifndef CONTAINERUTILS_H_
#define CONTAINERUTILS_H_

#include <algorithm>
#include <vector>

template<typename T>
inline void sort_unique_resize(std::vector<T> & vector) {
	std::sort(vector.begin(), vector.end());
	unsigned number_of_unique_elements = std::unique(vector.begin(), vector.end()) - vector.begin();
	vector.resize(number_of_unique_elements);
}

template<typename T>
inline void sort_unique_resize_shrink_vector(std::vector<T> & vector) {
	sort_unique_resize(vector);
	std::vector<T>().swap(vector);
}

template<typename T>
inline void remove_consecutive_duplicates_from_vector(std::vector<T> & vector) {
    unsigned number_of_unique_elements = std::unique(vector.begin(), vector.end()) - vector.begin();
    vector.resize(number_of_unique_elements);
}


#endif /* CONTAINERUTILS_H_ */
