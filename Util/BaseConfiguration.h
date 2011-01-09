/*
 * BaseConfiguration.h
 *
 *  Created on: 26.11.2010
 *      Author: dennis
 */

#ifndef BASECONFIGURATION_H_
#define BASECONFIGURATION_H_

#include <iostream>
#include <string>
#include <set>
#include <sstream>
#include <exception>
#include <fstream>

#include <boost/config.hpp>
#include <boost/program_options/detail/config_file.hpp>
#include <boost/program_options/parsers.hpp>

#include "../DataStructures/HashTable.h"

class BaseConfiguration {
public:
	BaseConfiguration(const char * configFile) {
		std::ifstream config( configFile );
		if(!config) {
			std::cerr << "[config] .ini not found" << std::endl;
			return;
		}

		//parameters
		options.insert("*");

		try {
			for (boost::program_options::detail::config_file_iterator i(config, options), e ; i != e; ++i) {
				std::cout << "[config] " << i->string_key << " = " << i->value[0] << std::endl;
				parameters.Add(i->string_key, i->value[0]);
			}
		} catch(std::exception& e) {
			std::cerr << "[config] .ini not found -> Exception: " <<e.what() << std::endl;
		}
	}

	std::string GetParameter(const char * key){
		return GetParameter(std::string(key));
	}

	std::string GetParameter(std::string key) {
		return parameters.Find(key);
	}

	void SetParameter(const char* key, const char* value) {
		SetParameter(std::string(key), std::string(value));
	}

	void SetParameter(std::string key, std::string value) {
		parameters.Set(key, value);
	}

private:
	std::set<std::string> options;
	HashTable<std::string, std::string> parameters;
	//Speichert alle Einträge aus INI Datei
};

#endif /* BASECONFIGURATION_H_ */
