/*
 * LevelInformation.h
 *
 *  Created on: 10.03.2011
 *      Author: dennis
 */

#ifndef LEVELINFORMATION_H_
#define LEVELINFORMATION_H_

#include <vector>

class LevelInformation {
public:
	LevelInformation() {
		levelInfos = new std::vector<std::vector<unsigned> >();
	}
	~LevelInformation() {
		delete levelInfos;
	}

	void Add(const unsigned level, const unsigned entry) {
		if(levelInfos->size() <= level)
			levelInfos->resize(level+1);
		assert(levelInfos->size() >= level);
		(*levelInfos)[level].push_back(entry);
	}

	unsigned GetNumberOfLevels() const {
		return levelInfos->size();
	}

	std::vector<unsigned> & GetLevel(const unsigned level) {
		if(levelInfos->size() <= level)
			levelInfos->resize(level+1);
		assert(levelInfos->size() >= level);
		return (*levelInfos)[level];
	}

	void Reset() {
		delete levelInfos;
		levelInfos = new std::vector<std::vector<unsigned> >();
	}

private:
	std::vector<std::vector<unsigned> > * levelInfos;
};

#endif /* LEVELINFORMATION_H_ */
