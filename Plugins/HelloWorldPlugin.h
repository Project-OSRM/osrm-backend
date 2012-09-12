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
#include "RouteParameters.h"

class HelloWorldPlugin : public BasePlugin {
public:
	HelloWorldPlugin() {}
	virtual ~HelloWorldPlugin() { /*std::cout << GetDescriptor() << " destructor" << std::endl;*/ }
	std::string GetDescriptor() const { return std::string("hello"); }
	std::string GetVersionString() const { return std::string("0.1a"); }

	void HandleRequest(const RouteParameters & routeParameters, http::Reply& reply) {
		std::cout << "[hello world]: runnning handler" << std::endl;
		reply.status = http::Reply::ok;
		reply.content.append("<html><head><title>Hello World Demonstration Document</title></head><body><h1>Hello, World!</h1>");
		std::stringstream content;
        content << "Number of locations: " << routeParameters.coordinates.size() << "<br>\n";
        for(unsigned i = 0; i < routeParameters.coordinates.size(); ++i) {
            content << "  [" << i << "] " << routeParameters.coordinates[i].lat/100000. << "," << routeParameters.coordinates[i].lon/100000. << "<br>\n";
        }
        content << "Number of hints: " << routeParameters.hints.size() << "<br>\n";
        for(unsigned i = 0; i < routeParameters.hints.size(); ++i) {
            content << "  [" << i << "] " << routeParameters.hints[i] << "<br>\n";
        }
		reply.content.append(content.str());
		reply.content.append("</body></html>");
	}
};

#endif /* HELLOWORLDPLUGIN_H_ */
