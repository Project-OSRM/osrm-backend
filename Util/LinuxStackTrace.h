/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
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


void crashHandler(int sig_num, siginfo_t * info, void * ucontext) {
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
#ifndef NDEBUG
    binaryName = b;
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
inline void installCrashHandler(std::string b) {}
#endif
#endif /* LINUXSTACKTRACE_H_ */
