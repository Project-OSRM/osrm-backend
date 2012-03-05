#ifndef TURNINSTRUCTIONSLIST_H_
#define TURNINSTRUCTIONSLIST_H_

#include <list>
#include <iostream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cctype>
#include <boost/algorithm/string.hpp>
#include "./TurnInstructions.h"

struct TurnInstructionsListClass {

	// Class to go over directory hierachy
	list<TurnInstructionsClass> tilist;
	vector<string> languages;

	TurnInstructionsListClass(){};

	TurnInstructionsListClass(std::string path){
		vector<string> archivos;
		vector<string>::iterator posArchivo;
		posArchivo = archivos.begin();
		FindFile(path.c_str(), archivos);
		TurnInstructionsClass ti;
		// Vector with the file names to read
		for (int i = 0; i < archivos.size(); ++i){
			ti = TurnInstructionsClass(archivos.at(i));
			posArchivo++;
			tilist.push_back(ti);
		}
	};

	void FindFile (const char dirname[], vector<string> &archivos){
		static DIR *dir;
		static struct dirent *mi_dirent;
		// Check if the directory have opened correctly
		if((dir = opendir(dirname)) == NULL)
			cout << "Fault in opendir" << endl;;
		// Go over directory content
		while ((mi_dirent = readdir(dir)) != NULL){
			// If it's not (.) or (..)
			if (strcmp (mi_dirent->d_name, ".") != 0 && strcmp (mi_dirent->d_name, "..") != 0){
				struct stat structura;
				// Read the complete path
				string dirnameString = dirname;
				string nameFile = mi_dirent->d_name;
				string pathFile = dirnameString + "/" + nameFile;
				if (stat(pathFile.c_str(), &structura) < 0){
					cout << "Fault in stat" << endl;
				}else if (structura.st_mode & S_IFREG){
					archivos.reserve(archivos.size() + 2);
					archivos.push_back(pathFile);
					boost::to_upper(nameFile);
					languages.reserve(languages.size() + 2);
					languages.push_back(nameFile);
				}else if(structura.st_mode & S_IFDIR){
					FindFile(mi_dirent->d_name, archivos);
				}
			}
		}
		if(closedir(dir) == -1){
			cout << "Fault in closedir" << endl;
		}
	}
	
	TurnInstructionsClass getTurnInstructions(std::string language){
		boost::to_upper(language);
		list <TurnInstructionsClass>::iterator pos;
		int index = -1;
		// In case the language is empty
		if(index == -1 && language == ""){
			for(int i=0; i<languages.size(); i++){
				if(languages.at(i).find("EN")==0){
					index = i;
					break;
				}
			}
		}
		// In case the request language is the same name than we have in the directory hierachy
		for(int i=0; i<languages.size(); i++){
			if(language.compare(languages.at(i)) == 0){
				index = i;
				break;
			}
		}
		// In case the request language is the same language but the name is not exactly the same
		// Example: Request parameter: 'lang=ES' and file name is 'ES_ES'
		if(index == -1){
			for(int i=0; i<languages.size(); i++){
				if(language != "" && languages.at(i).find(language)==0){
					index = i;
					break;
				}
			}
		}
		// In case the request language is the same language but the name is not exactly the same
		// Example: Request parameter: 'lang=ES_ES' and file name is 'ES'
		if(index == -1){
			for(int i=0; i<languages.size(); i++){
				if(language.find(languages.at(i))==0){
					index = i;
					break;
				}
			}
		}
		// In case the request language was an included language in another language 
		// Example: Request parameter: 'lang=ES_AR' and file name is 'ES'
		if(index == -1){
			for(int i=0; i<languages.size(); i++){
				int pos = language.find("_");
				string lang = language.substr(0, pos);
				if(languages.at(i).find(lang)==0){
					index = i;
					break;
				}
			}
		}
		// In case the request parameter language doesn't exist, the default language is 'EN'
		if(index == -1){
			for(int i=0; i<languages.size(); i++){
				if(languages.at(i).find("EN")==0){
					index = i;
					break;
				}
			}
		}
		pos = tilist.begin();
		for(int i=0; i<index; i++){
			pos++;
		}
	    	return *pos;
	}
};

#endif /* TURNINSTRUCTIONSLIST_H_ */
