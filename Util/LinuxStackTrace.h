/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef LINUXSTACKTRACE_H_
#define LINUXSTACKTRACE_H_

#include <string>

#ifdef __linux__
#include <cxxabi.h>
#include <execinfo.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sstream>

std::string binaryName;

std::string getFileAndLine (char * offset_end) {
    std::string a(offset_end);
    std::string result;
    a = a.substr(2,a.length()-3);
    static char buf[256];
    std::string command("/usr/bin/addr2line -C -e " + binaryName + " -f -i " + a);
    // prepare command to be executed
    // our program need to be passed after the -e parameter
    FILE* f = popen (command.c_str(), "r");

    if (f == NULL) {
        perror (buf);
        return "";
    }
    // get function name
    if ( NULL != fgets (buf, 256, f) ) {

        // get file and line
        if ( NULL != fgets (buf, 256, f) ) {

            if (buf[0] != '?') {
                result = ( buf);
            } else {
                result = "unkown";
            }
        } else { result = ""; }
    } else { result = ""; }
    pclose(f);
    return result;
}


void crashHandler(int sig_num, siginfo_t * info, void * ) {
    const size_t maxDepth = 100;
    //size_t stackDepth;

    void *stackAddrs[maxDepth];
    backtrace(stackAddrs, maxDepth);

    std::cerr << "signal " << sig_num << " (" << strsignal(sig_num) << "), address is " << info->si_addr << " from " << stackAddrs[0] << std::endl;

    void * array[50];
    int size = backtrace(array, 50);

    array[1] = stackAddrs[0];

    char ** messages = backtrace_symbols(array, size);

    // skip first stack frame (points here)
    for (int i = 1; i < size-1 && messages != NULL; ++i) {
        char *mangledname = 0, *offset_begin = 0, *offset_end = 0;

        // find parantheses and +address offset surrounding mangled name
        for (char *p = messages[i+1]; *p; ++p) {
            if (*p == '(') {
                mangledname = p;
            } else if (*p == '+') {
                offset_begin = p;
            } else if (*p == ')') {
                offset_end = p;
                break;
            }
        }

        // if the line could be processed, attempt to demangle the symbol
        if (mangledname && offset_begin && offset_end && mangledname < offset_begin) {
            *mangledname++ = '\0';
            *offset_begin++ = '\0';
            *offset_end++ = '\0';

            int status;
            char * real_name = abi::__cxa_demangle(mangledname, 0, 0, &status);

            // if demangling is successful, output the demangled function name
            if (status == 0) {
                std::cerr << "[bt]: (" << i << ") " << messages[i+1] << " : " << real_name << " " << getFileAndLine(offset_end);
            }
            // otherwise, output the mangled function name
            else {
                std::cerr << "[bt]: (" << i << ") " << messages[i+1] << " : "
                        << mangledname << "+" << offset_begin << offset_end
                        << std::endl;
            }
            free(real_name);
        }
        // otherwise, print the whole line
        else {
            std::cerr << "[bt]: (" << i << ") " << messages[i+1] << std::endl;
        }
    }
    std::cerr << std::endl;

    free(messages);

    exit(EXIT_FAILURE);
}

void installCrashHandler(std::string b) {
    binaryName = b;
#ifndef NDEBUG
    struct sigaction sigact;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_sigaction = crashHandler;
    sigact.sa_flags = SA_RESTART | SA_SIGINFO;

    if (sigaction(SIGSEGV, &sigact, (struct sigaction *)NULL) != 0) {
        std::cerr <<  "error setting signal handler for " << SIGSEGV << " " << strsignal(SIGSEGV) << std::endl;

        exit(EXIT_FAILURE);
    }
#endif
}
#else
inline void installCrashHandler(std::string ) {}
#endif
#endif /* LINUXSTACKTRACE_H_ */
