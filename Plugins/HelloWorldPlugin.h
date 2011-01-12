/*
 * LocatePlugin.h
 *
 *  Created on: 01.01.2011
 *      Author: dennis
 */

#ifndef HELLOWORLDPLUGIN_H_
#define HELLOWORLDPLUGIN_H_

#include <sstream>

#include "BasePlugin.h"

class HelloWorldPlugin : public BasePlugin {
public:
	HelloWorldPlugin() {}
	~HelloWorldPlugin() { /*std::cout << GetDescriptor() << " destructor" << std::endl;*/ }
	std::string GetDescriptor() { return std::string("hello"); }
	void HandleRequest(std::vector<std::string> parameters, http::Reply& reply) {
		std::cout << "[hello world]: runnning handler" << std::endl;
		reply.status = http::Reply::ok;
		reply.content.append("<html><head><title>Hello World Demonstration Document</title></head><body><h1>Hello, World!</h1>");
		std::stringstream content;
		content << "Number of parameters: " << parameters.size() << "<br>";
		for(unsigned i = 0; i < parameters.size(); i++) {
			content << parameters[i] << "<br>";
		}
		reply.content.append(content.str());
		reply.content.append("</body></html>");
	}
	std::string GetVersionString() { return std::string("0.1a"); }
};

#endif /* HELLOWORLDPLUGIN_H_ */
