/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                          License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Copyright (C) 2013, OpenCV Foundation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#pragma once

#ifndef __OPENCV_CUDEV_GRID_REDUCE_TO_COLUMN_DETAIL_HPP__
#define __OPENCV_CUDEV_GRID_REDUCE_TO_COLUMN_DETAIL_HPP__

#include "../../common.hpp"
#include "../../util/saturate_cast.hpp"
#include "../../block/reduce.hpp"

namespace cv { namespace cudev {

namespace grid_reduce_to_vec_detail
{
    template <class Reductor, int BLOCK_SIZE, class SrcPtr, typename ResType, class MaskPtr>
    __global__ void reduceToColumn(const SrcPtr src, ResType* dst, const MaskPtr mask, const int cols)
    {
        typedef typename Reductor::work_type work_type;

        __shared__ work_type smem[BLOCK_SIZE];

        const int y = blockIdx.x;

        work_type myVal = Reductor::initialValue();

        Reductor op;

        for (int x = threadIdx.x; x < cols; x += BLOCK_SIZE)
        {
            if (mask(y, x))
            {
                myVal = op(myVal, saturate_cast<work_type>(src(y, x)));
            }
        }

        blockReduce<BLOCK_SIZE>(smem, myVal, threadIdx.x, op);

        if (threadIdx.x == 0)
            dst[y] = saturate_cast<ResType>(Reductor::result(myVal, cols));
    }

    template <class Reductor, class Policy, class SrcPtr, typename ResType, class MaskPtr>
    __host__ void reduceToColumn(const SrcPtr& src, ResType* dst, const MaskPtr& mask, int rows, int cols, cudaStream_t stream)
    {
        const int BLOCK_SIZE_X = Policy::block_size_x;
        const int BLOCK_SIZE_Y = Policy::block_size_y;

        const int BLOCK_SIZE = BLOCK_SIZE_X * BLOCK_SIZE_Y;

        const dim3 block(BLOCK_SIZE);
        const dim3 grid(rows);

        reduceToColumn<Reductor, BLOCK_SIZE><<<grid, block, 0, stream>>>(src, dst, mask, cols);
        CV_CUDEV_SAFE_CALL( cudaGetLastError() );

        if (stream == 0)
            CV_CUDEV_SAFE_CALL( cudaDeviceSynchronize() );

    }
}

}}

#endif
