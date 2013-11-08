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

#ifndef PERCENT_H
#define PERCENT_H

#include "../Util/OpenMPWrapper.h"
#include <iostream>

class Percent {
public:
    /**
     * Constructor.
     * @param maxValue the value that corresponds to 100%
     * @param step the progress is shown in steps of 'step' percent
     */
    Percent(unsigned maxValue, unsigned step = 5) {
        reinit(maxValue, step);
    }

    /** Reinitializes this object. */
    void reinit(unsigned maxValue, unsigned step = 5) {
        _maxValue = maxValue;
        _current_value = 0;
        _intervalPercent = _maxValue / 100;
        _nextThreshold = _intervalPercent;
        _lastPercent = 0;
        _step = step;
    }

    /** If there has been significant progress, display it. */
    void printStatus(unsigned currentValue) {
        if (currentValue >= _nextThreshold) {
            _nextThreshold += _intervalPercent;
            printPercent( currentValue / (double)_maxValue * 100 );
        }
        if (currentValue + 1 == _maxValue)
            std::cout << " 100%" << std::endl;
    }

    void printIncrement() {
#pragma omp atomic
        ++_current_value;
        printStatus(_current_value);
    }

    void printAddition(const unsigned addition) {
#pragma omp atomic
        _current_value += addition;
        printStatus(_current_value);
    }
private:
    unsigned _current_value;
    unsigned _maxValue;
    unsigned _intervalPercent;
    unsigned _nextThreshold;
    unsigned _lastPercent;
    unsigned _step;

    /** Displays the new progress. */
    void printPercent(double percent) {
        while (percent >= _lastPercent+_step) {
            _lastPercent+=_step;
            if (_lastPercent % 10 == 0) {
                std::cout << " " << _lastPercent << "% ";
            }
            else {
                std::cout << ".";
            }
            std::cout.flush();
        }
    }
};

#endif // PERCENT_H
