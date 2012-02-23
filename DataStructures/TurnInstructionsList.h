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

	// Clase en la que recorreremos la estructura en directorios
	list<TurnInstructionsClass> tilist;
	vector<string> languages;

	// Constructor vacio
	TurnInstructionsListClass(){};

	// Constructor
	TurnInstructionsListClass(std::string path){
		vector<string> archivos;
		vector<string>::iterator posArchivo;
		posArchivo = archivos.begin();
		FindFile(path.c_str(), archivos);
		TurnInstructionsClass ti;
		// Vector con los nombres de los archivos a leer
		for (int i = 0; i < archivos.size(); ++i){
			ti = TurnInstructionsClass(archivos.at(i));
			posArchivo++;
			tilist.push_back(ti);
		}
	};

	void FindFile (const char dirname[], vector<string> &archivos){
		static DIR *dir;
		static struct dirent *mi_dirent;
		// Comprobar que se abre el directorio
		if((dir = opendir(dirname)) == NULL)
			cout << "Fault in opendir" << endl;;
		// Recorremos el contenido del directorio
		while ((mi_dirent = readdir(dir)) != NULL){
			// Si no es él mismo (.) o el padre (..)
			if (strcmp (mi_dirent->d_name, ".") != 0 && strcmp (mi_dirent->d_name, "..") != 0){
				struct stat structura;
				// Leer la ruta completa
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
		// Cuando no se le pasa ningún idioma
		if(index == -1 && language == ""){
			for(int i=0; i<languages.size(); i++){
				if(languages.at(i).find("EN")==0){
					index = i;
					break;
				}
			}
		}
		// Cuando se le pasa por parametro lang=ES_ES y lo que tenemos es ES_ES
		for(int i=0; i<languages.size(); i++){
			if(language.compare(languages.at(i)) == 0){
				index = i;
				break;
			}
		}
		// Cuando se le pasa por parametro lang=ES y lo que tenemos es ES_ES
		if(index == -1){
			for(int i=0; i<languages.size(); i++){
				if(language != "" && languages.at(i).find(language)==0){
					index = i;
					break;
				}
			}
		}
		// Cuando se le pasa por parámetro lang=ES_ES y lo que tenemos es ES
		if(index == -1){
			for(int i=0; i<languages.size(); i++){
				if(language.find(languages.at(i))==0){
					index = i;
					break;
				}
			}
		}
		// Cuando se le pasa por parámetro ES_AR y tenemos ES debe devolver ES
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
		// Cuando no encuentra idioma que tome el por defecto EN
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
