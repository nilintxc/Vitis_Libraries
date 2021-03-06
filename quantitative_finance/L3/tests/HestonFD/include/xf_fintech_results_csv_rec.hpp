/*
 * Copyright 2019 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _XF_FINTECH_RESULTS_CSV_REC_HPP_
#define _XF_FINTECH_RESULTS_CSV_REC_HPP_

#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

class GoldenResultsCSVRecord {
   public:
    std::vector<double> csvTableEntry;
    int NumberOfRows;

    std::vector<double> showCSVTableEntry() { return (csvTableEntry); }

    int showNumberOfRows() { return (NumberOfRows); }

}; // class GoldenResultsCSVRecord

#endif
