/*
 * (c) Copyright 2019 Xilinx, Inc. All rights reserved.
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
 *
 */
#ifndef _XFCOMPRESSION_AXI_STREAM_UTILS_HPP_
#define _XFCOMPRESSION_AXI_STREAM_UTILS_HPP_

#include "hls_stream.h"
#include "common.h"
#include "axi_stream_types.hpp"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <ap_int.h>

namespace xf {
namespace compression {

/**
 * @brief Read data from axi stream and write to internal hls stream.
 *
 * @param inputAxisStream   incoming axi stream
 * @param inputStream       output hls stream
 * @param inputSize         size of the data coming from input axi stream
 *
 */
void axis2hlsStreamFixedSize(hls::stream<hStream8b_t>& inputAxiStream,
                             hls::stream<streamDt>& inputStream,
                             uint32_t inputSize) {
    for (int i = 0; i < inputSize; i++) {
#pragma HLS PIPELINE II = 1
        hStream8b_t t1 = inputAxiStream.read();
        ap_uint<8> tmpOut;
        tmpOut = t1.get_data();
        inputStream << tmpOut;
    }
}

/**
 * @brief Read data from hls stream till the end of stream is indicated and write this data to axi stream.
 *        The total size of data is written 32-bit wide axi stream.
 *
 * @param outputStream      internal output hls stream
 * @param outStreamEos      stream to specify the end of stream
 * @param outputAxisStream  output stream going to axi
 * @param outStreamSize     size of the data coming to output from stream
 * @param outAxiStreamSize  size of the data to go through output axi stream
 *
 */
void hlsStream2axis(hls::stream<streamDt>& outputStream,
                    hls::stream<bool>& outStreamEos,
                    hls::stream<hStream8b_t>& outputAxiStream,
                    hls::stream<uint32_t>& outStreamSize,
                    hls::stream<hStream32b_t>& outAxiStreamSize) {
    uint8_t temp;
    bool flag;
    do {
        temp = outputStream.read();
        flag = outStreamEos.read();
        hStream8b_t tOut;
        tOut.set_data(temp);
        tOut.set_last(flag);
        tOut.set_keep(-1);
        outputAxiStream.write(tOut);
    } while (!flag);
    uint32_t outSize = outStreamSize.read();
    hStream32b_t tOutSize;
    tOutSize.set_data(outSize);
    tOutSize.set_last(1);
    tOutSize.set_keep(-1);
    outAxiStreamSize.write(tOutSize);
}

/**
 * @brief Read data from internal hls stream and write to output axi stream for the given size.
 *
 * @param hlsInStream       internal hls stream
 * @param outputAxisStream  output axi stream
 * @param originalSize      output data size to be written to output stream
 *
 */
void hlsStream2axiStreamFixedSize(hls::stream<streamDt>& hlsInStream,
                                  hls::stream<hStream8b_t>& outputAxiStream,
                                  uint32_t originalSize) {
    uint8_t temp;
    for (int i = 0; i < originalSize - 1; i++) {
        temp = hlsInStream.read();
        hStream8b_t tOut;
        tOut.set_data(temp);
        tOut.set_keep(-1);
        outputAxiStream.write(tOut);
    }
    // last byte
    temp = hlsInStream.read();
    hStream8b_t tOut;
    tOut.set_data(temp);
    tOut.set_last(1);
    tOut.set_keep(-1);
    outputAxiStream.write(tOut);
}

/**
 * @brief Read N-bit wide data from internal hls streams
 *        and write to output axi stream. N is passed as template parameter.
 *
 * @tparam STREAMDWIDTH stream data width
 *
 * @param inAxiStream   input kernel axi stream
 * @param outStream     output hls stream
 *
 */
template <uint16_t STREAMDWIDTH>
void axis2hlsStream(hls::stream<qdma_axis<STREAMDWIDTH, 0, 0, 0> >& inAxiStream,
                    hls::stream<ap_uint<STREAMDWIDTH> >& outStream) {
    while (true) {
#pragma HLS PIPELINE II = 1
        qdma_axis<STREAMDWIDTH, 0, 0, 0> t1 = inAxiStream.read();
        ap_uint<STREAMDWIDTH> tmp = t1.get_data();
        outStream << tmp;
        if (t1.get_last()) {
            break;
        }
    }
}

/**
 * @brief Write N-bit wide data of given size from hls stream to kernel axi stream.
 *        N is passed as template parameter.
 *
 * @tparam STREAMDWIDTH stream data width
 *
 * @param in            input hls stream
 * @param inStream_dm   output kernel stream
 * @param inputSize     size of data in to be transferred
 *
 */
template <uint16_t STREAMDWIDTH>
void streamDataDm2k(hls::stream<ap_uint<STREAMDWIDTH> >& in,
                    hls::stream<ap_axiu<STREAMDWIDTH, 0, 0, 0> >& inStream_dm,
                    uint32_t inputSize) {
    // read data from input hls to input stream for decompression kernel
    uint32_t itrLim = 1 + (inputSize - 1) / (STREAMDWIDTH / 8);
    for (uint32_t i = 0; i < itrLim; i++) {
#pragma HLS PIPELINE II = 1
        ap_uint<STREAMDWIDTH> temp = in.read();
        ap_axiu<STREAMDWIDTH, 0, 0, 0> dataIn;
        dataIn.data = temp; // kernel to kernel data transfer
        inStream_dm.write(dataIn);
    }
}

/**
 * @brief Read data from kernel axi stream byte by byte and write to hls stream and indicate end of stream.
 *
 * @param out           output hls stream
 * @param bytEos        internal stream which indicates end of data stream
 * @param dataSize      size of data in streams
 * @param dmOutStream   input kernel axi stream to be read
 *
 */
void streamDataK2dm(hls::stream<ap_uint<8> >& out,
                    hls::stream<bool>& bytEos,
                    hls::stream<uint32_t>& dataSize,
                    hls::stream<kStream8b_t>& dmOutStream) {
    // read data from decompression kernel output to global memory output
    kStream8b_t dataout;
    bool last = false;
    uint32_t outSize = 0;
    do {
#pragma HLS PIPELINE II = 1
        dataout = dmOutStream.read();
        last = dataout.last;
        bytEos << last;
        out << dataout.data;
        outSize++;
    } while (!last);
    dataSize << outSize - 1; // read encoded size from decompression kernel
}

/**
 * @brief Read data from kernel axi stream byte by byte and write to hls stream for given output size.
 *
 * @param out           output hls stream
 * @param dmOutStream   input kernel axi stream to be read
 * @param dataSize      size of data in streams
 *
 */
void streamDataK2dmFixedSize(hls::stream<ap_uint<8> >& out, hls::stream<kStream8b_t>& dmOutStream, uint32_t dataSize) {
    // read data from decompression kernel output to global memory output
    kStream8b_t dataout;
    for (uint32_t i = 0; i < dataSize; i++) {
#pragma HLS PIPELINE II = 1
        dataout = dmOutStream.read();
        out << dataout.data;
    }
}

} // end compression
} // end xf

#endif // _XFCOMPRESSION_AXI_STREAM_UTILS_HPP_
