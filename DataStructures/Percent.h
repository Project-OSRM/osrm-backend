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

#ifndef PERCENT_H
#define PERCENT_H

#include <iostream>
#ifdef _OPENMP
#include <omp.h>
#endif

class Percent
{
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

    void printIncrement()
    {
#pragma omp atomic
        _current_value++;
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
