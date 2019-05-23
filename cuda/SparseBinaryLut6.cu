#include <iostream>
#include <chrono>
#include <algorithm>

#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "bbcu/bbcu.h"
#include "bbcu/bbcu_util.h"


#include "Common.cuh"



// -------------------------------------------------
//  Forward
// -------------------------------------------------

template<int MAX_NODE_UNIT=32>
__device__ float device_fp32_SparseBinaryLut6_NodeForward
        (
            int     node_id,
            float   x[6],
            float   W[64][MAX_NODE_UNIT]
        )
{
    float   xp[6], xn[6];
    for ( int i = 0; i < 6; ++i) {
        float x_val = x[i];
        xp[i] = x_val;
        xn[i] = 1.0 - x_val;
    }

    float x0_00 = xn[1] * xn[0];
    float x0_01 = xn[1] * xp[0];
    float x0_10 = xp[1] * xn[0];
    float x0_11 = xp[1] * xp[0];
    float x1_00 = xn[3] * xn[2];
    float x1_01 = xn[3] * xp[2];
    float x1_10 = xp[3] * xn[2];
    float x1_11 = xp[3] * xp[2];
    float x2_00 = xn[5] * xn[4];
    float x2_01 = xn[5] * xp[4];
    float x2_10 = xp[5] * xn[4];
    float x2_11 = xp[5] * xp[4];

    float y = 0;
    float x2_00_x1_00 = x2_00 * x1_00;
    y += W[0 ][node_id] * x2_00_x1_00 * x0_00;
    y += W[1 ][node_id] * x2_00_x1_00 * x0_01;
    y += W[2 ][node_id] * x2_00_x1_00 * x0_10;
    y += W[3 ][node_id] * x2_00_x1_00 * x0_11;
    float x2_00_x1_01 = x2_00 * x1_01;
    y += W[4 ][node_id] * x2_00_x1_01 * x0_00;
    y += W[5 ][node_id] * x2_00_x1_01 * x0_01;
    y += W[6 ][node_id] * x2_00_x1_01 * x0_10;
    y += W[7 ][node_id] * x2_00_x1_01 * x0_11;
    float x2_00_x1_10 = x2_00 * x1_10;
    y += W[8 ][node_id] * x2_00_x1_10 * x0_00;
    y += W[9 ][node_id] * x2_00_x1_10 * x0_01;
    y += W[10][node_id] * x2_00_x1_10 * x0_10;
    y += W[11][node_id] * x2_00_x1_10 * x0_11;
    float x2_00_x1_11 = x2_00 * x1_11;
    y += W[12][node_id] * x2_00_x1_11 * x0_00;
    y += W[13][node_id] * x2_00_x1_11 * x0_01;
    y += W[14][node_id] * x2_00_x1_11 * x0_10;
    y += W[15][node_id] * x2_00_x1_11 * x0_11;
    float x2_01_x1_00 = x2_01 * x1_00;
    y += W[16][node_id] * x2_01_x1_00 * x0_00;
    y += W[17][node_id] * x2_01_x1_00 * x0_01;
    y += W[18][node_id] * x2_01_x1_00 * x0_10;
    y += W[19][node_id] * x2_01_x1_00 * x0_11;
    float x2_01_x1_01 = x2_01 * x1_01;
    y += W[20][node_id] * x2_01_x1_01 * x0_00;
    y += W[21][node_id] * x2_01_x1_01 * x0_01;
    y += W[22][node_id] * x2_01_x1_01 * x0_10;
    y += W[23][node_id] * x2_01_x1_01 * x0_11;
    float x2_01_x1_10 = x2_01 * x1_10;
    y += W[24][node_id] * x2_01_x1_10 * x0_00;
    y += W[25][node_id] * x2_01_x1_10 * x0_01;
    y += W[26][node_id] * x2_01_x1_10 * x0_10;
    y += W[27][node_id] * x2_01_x1_10 * x0_11;
    float x2_01_x1_11 = x2_01 * x1_11;
    y += W[28][node_id] * x2_01_x1_11 * x0_00;
    y += W[29][node_id] * x2_01_x1_11 * x0_01;
    y += W[30][node_id] * x2_01_x1_11 * x0_10;
    y += W[31][node_id] * x2_01_x1_11 * x0_11;
    float x2_10_x1_00 = x2_10 * x1_00;
    y += W[32][node_id] * x2_10_x1_00 * x0_00;
    y += W[33][node_id] * x2_10_x1_00 * x0_01;
    y += W[34][node_id] * x2_10_x1_00 * x0_10;
    y += W[35][node_id] * x2_10_x1_00 * x0_11;
    float x2_10_x1_01 = x2_10 * x1_01;
    y += W[36][node_id] * x2_10_x1_01 * x0_00;
    y += W[37][node_id] * x2_10_x1_01 * x0_01;
    y += W[38][node_id] * x2_10_x1_01 * x0_10;
    y += W[39][node_id] * x2_10_x1_01 * x0_11;
    float x2_10_x1_10 = x2_10 * x1_10;
    y += W[40][node_id] * x2_10_x1_10 * x0_00;
    y += W[41][node_id] * x2_10_x1_10 * x0_01;
    y += W[42][node_id] * x2_10_x1_10 * x0_10;
    y += W[43][node_id] * x2_10_x1_10 * x0_11;
    float x2_10_x1_11 = x2_10 * x1_11;
    y += W[44][node_id] * x2_10_x1_11 * x0_00;
    y += W[45][node_id] * x2_10_x1_11 * x0_01;
    y += W[46][node_id] * x2_10_x1_11 * x0_10;
    y += W[47][node_id] * x2_10_x1_11 * x0_11;
    float x2_11_x1_00 = x2_11 * x1_00;
    y += W[48][node_id] * x2_11_x1_00 * x0_00;
    y += W[49][node_id] * x2_11_x1_00 * x0_01;
    y += W[50][node_id] * x2_11_x1_00 * x0_10;
    y += W[51][node_id] * x2_11_x1_00 * x0_11;
    float x2_11_x1_01 = x2_11 * x1_01;
    y += W[52][node_id] * x2_11_x1_01 * x0_00;
    y += W[53][node_id] * x2_11_x1_01 * x0_01;
    y += W[54][node_id] * x2_11_x1_01 * x0_10;
    y += W[55][node_id] * x2_11_x1_01 * x0_11;
    float x2_11_x1_10 = x2_11 * x1_10;
    y += W[56][node_id] * x2_11_x1_10 * x0_00;
    y += W[57][node_id] * x2_11_x1_10 * x0_01;
    y += W[58][node_id] * x2_11_x1_10 * x0_10;
    y += W[59][node_id] * x2_11_x1_10 * x0_11;
    float x2_11_x1_11 = x2_11 * x1_11;
    y += W[60][node_id] * x2_11_x1_11 * x0_00;
    y += W[61][node_id] * x2_11_x1_11 * x0_01;
    y += W[62][node_id] * x2_11_x1_11 * x0_10;
    y += W[63][node_id] * x2_11_x1_11 * x0_11;

    // clamp
    y = max(0.0, y);
    y = min(1.0, y);

    return y;
}


template<int MAX_FRAME_UNIT=32, int MAX_NODE_UNIT=32>
__global__ void kernal_bit_fp32_SparseBinaryLut6_Forward(
            int   const     *x_buf,
            int             *y_buf,
            int   const     *input_index,
            float const     *W_buf,
            float           *mean_buf,
            float           *rstd_buf,
            float           *running_mean_buf,
            float           *running_var_buf,
            float           gamma,
            float           beta,
            float           momentum,
            float           reciprocal_frame_size,
            int             node_size,
            int             frame_size,
            int             frame_stride,
            int             lut_binarize
        )
{
//  int unit_id = ((threadIdx.x % MAX_FRAME_UNIT) & ~0x1f);
    int node_id = threadIdx.y;
    int node    = blockIdx.y * blockDim.y + threadIdx.y;
    int id      = threadIdx.x;
    int id_step = blockDim.x;

    __shared__  float       sbuf[MAX_NODE_UNIT][MAX_FRAME_UNIT];

    __shared__  float       W[64][MAX_NODE_UNIT];
                int   const *x_ptr[6];
                int         *y_ptr;
    
    if ( node < node_size ) {
        // read W
        for ( int i = id; i < 64; i += id_step ) {
            W[i][node_id] = W_buf[node * 64 + i];
            if ( lut_binarize ) {
                W[i][node_id] = W[i][node_id] > 0.5 ? 1.0 : 0.0;
            }
        }
        
        // read input index
        for ( int i = 0; i < 6; ++i ) {
            x_ptr[i] = &x_buf[frame_stride * input_index[6*node + i]];
        }
                     
        y_ptr = &y_buf[node * frame_stride];
    }

    __syncthreads();
    
    // ���ςƕ��U�v��
    float s1 = 0, c1 = 0, y1, t1;
    float s2 = 0, c2 = 0, y2, t2;
    for (int frame = id; frame < frame_size; frame += id_step) {
        if ( node < node_size ) {
            // Forward�v�Z
            int bit  = (1 << (frame & 0x1f));
            int unit = (frame >> 5);
            float x[6];
            for ( int i = 0; i < 6; ++i) {
                x[i] = (x_ptr[i][unit] & bit) ? 0.7 : 0.3;
            }
            float y = device_fp32_SparseBinaryLut6_NodeForward<MAX_NODE_UNIT>(node_id, x, W);

            // �W�v
            y1 = y - c1;
            t1 = s1 + y1;
            c1 = (t1 - s1) - y1;
            s1 = t1;

            y2 = (y * y) - c2;
            t2 = s2 + y2;
            c2 = (t2 - s2) - y2;
            s2 = t2;
        }
    }

    s1 = device_fp32_LocalSum(s1, sbuf[node_id]);
    s2 = device_fp32_LocalSum(s2, sbuf[node_id]);
    float mean = s1 * reciprocal_frame_size;
    float var = max(1.0e-7f, (s2 * reciprocal_frame_size) - (mean * mean));
    
    float rstd = rsqrt(var);

    // ��������
    if (id == 0) {
        if ( node < node_size ) {
            running_mean_buf[node] = running_mean_buf[node] * momentum + mean * (1.0f - momentum);
            running_var_buf[node]  = running_var_buf[node] * momentum + var * (1.0f - momentum);
            mean_buf[node] = mean;
            rstd_buf[node] = rstd;
        }
    }

    // ���K��
    int loop_size = ((frame_size + blockDim.x - 1) & ~(blockDim.x - 1));
    for ( int frame = id; frame < loop_size; frame += id_step) {
        int unit     = (frame >> 5);
        int bit      = (frame & 0x1f);
        int bit_mask = (1 << bit);

        int y_mask = 0;
        if ( node < node_size && frame < frame_size) {
            // Forward�v�Z
            float x[6];
            for ( int i = 0; i < 6; ++i) {
                x[i] = (x_ptr[i][unit] & bit_mask) ? 0.7 : 0.3;
            }
            float y = device_fp32_SparseBinaryLut6_NodeForward<MAX_NODE_UNIT>(node_id, x, W);

            y = (y - mean) * rstd;
            y = y * gamma + beta;

            if ( y > 0.5 ) {
                y_mask = bit_mask;
            }
        }

//      y_mask = device_int_LocalOr(y_mask, bit, (int *)&sbuf[node_id][unit_id]);
        y_mask = device_int_ShuffleOr(y_mask);

        if ( bit == 0 ) {
            if ( node < node_size && frame < frame_size ) {
                y_ptr[unit] = y_mask;
            }
        }
    }
}


int bbcu_bit_fp32_SparseBinaryLut6_Forward
        (
            int   const     *dev_x_buf,
            int             *dev_y_buf,
            int   const     *dev_input_index,
            float const     *dev_W,
            float           *mean_buf,
            float           *rstd_buf,
            float           *running_mean_buf,
            float           *running_var_buf,
            float           gamma,
            float           beta,
            float           momentum,
            int             node_size,
            int             frame_size,
            int             frame_stride,
            int             lut_binarize,
            cudaStream_t    streamId
        )
{
    BBCU_DEBUG_ASSERT(bbcu_IsDeviceAvailable());

    unsigned int const THREAD_SIZE    = 512;
    unsigned int const MAX_FRAME_UNIT = 256;
    unsigned int const MAX_NODE_UNIT  = 16;

#if 0
    dim3    block(MAX_FRAME_UNIT, THREAD_SIZE / MAX_FRAME_UNIT);
    while ( (int)block.x / 2 >= frame_size && block.x > 32 ) { block.x /= 2; block.y *= 2; }
    while ( (int)block.y / 2 >= node_size                  ) { block.y /= 2; }
#else
    dim3    block(THREAD_SIZE / MAX_NODE_UNIT, MAX_NODE_UNIT);
    while ( (int)block.y / 2 >= node_size  )                { block.y /= 2; block.x *= 2;}
    while ( (int)block.x / 2 >= frame_size && block.x > 32) { block.x /= 2; }
#endif

    block.x = std::min(block.x, MAX_FRAME_UNIT);
    block.y = std::min(block.y, MAX_NODE_UNIT);
    dim3    grid(1, (node_size + (block.y - 1)) / block.y);
    
    kernal_bit_fp32_SparseBinaryLut6_Forward<MAX_FRAME_UNIT, MAX_NODE_UNIT><<<grid, block, 0, streamId>>>(
            dev_x_buf,
            dev_y_buf,
            dev_input_index,
            dev_W,
            mean_buf,
            rstd_buf,
            running_mean_buf,
            running_var_buf,
            gamma,
            beta,
            momentum,
            1.0f / (float)frame_size,
            node_size,
            frame_size,
            frame_stride,
            lut_binarize
        );
    BB_CUDA_CHECK_LAST_ERROR();
    
    return 0;
}


// -------------------------------------------------
//  Forward Inference
// -------------------------------------------------


template<int MAX_FRAME_UNIT=32, int MAX_NODE_UNIT=32>
__global__ void kernal_bit_fp32_SparseBinaryLut6_ForwardInference
        (
            int   const     *x_buf,
            int             *y_buf,
            int   const     *input_index,
            float const     *W_buf,
            float const     *running_mean_buf,
            float const     *running_var_buf,
            float           gamma,
            float           beta,
            int             node_size,
            int             frame_size,
            int             frame_stride,
            int             lut_binarize
        )
{
    int node_id = threadIdx.y;
    int node    = blockIdx.y * blockDim.y + threadIdx.y;
    int id      = threadIdx.x;
    int id_step = blockDim.x;

    __shared__  float       W[64][MAX_NODE_UNIT];
                int   const *x_ptr[6];
                int         *y_ptr;
    
    if ( node < node_size ) {
        // read W
        for ( int i = id; i < 64; i += id_step ) {
            W[i][node_id] = W_buf[node * 64 + i];
            if ( lut_binarize ) {
                W[i][node_id] = W[i][node_id] > 0.5 ? 1.0 : 0.0;
            }
        }
        
        // read input index
        for ( int i = 0; i < 6; ++i ) {
            x_ptr[i] = &x_buf[frame_stride * input_index[6*node + i]];
        }
                     
        y_ptr = &y_buf[node * frame_stride];
    }

    __syncthreads();
    
    if ( node < node_size ) {
        float mean  = running_mean_buf[node];
        float var   = running_var_buf[node];
        float rstd = 1.0 / (sqrt(var) + 1.0e-7);

        int loop_size = ((frame_size + blockDim.x - 1) & ~(blockDim.x - 1));
        for ( int frame = id; frame < loop_size; frame += id_step) {
            int unit     = (frame >> 5);
            int bit      = (frame & 0x1f);
            int bit_mask = (1 << bit);

            int y_mask = 0;
            if ( node < node_size && frame < frame_size) {
                // Forward�v�Z
                float x[6];
                for ( int i = 0; i < 6; ++i) {
                    x[i] = (x_ptr[i][unit] & bit_mask) ? 0.7 : 0.3;
                }
                float y = device_fp32_SparseBinaryLut6_NodeForward<MAX_NODE_UNIT>(node_id, x, W);

                y = ((y - mean) * rstd) * gamma + beta;

                if ( y > 0.5 ) {
                    y_mask = bit_mask;
                }
            }

            y_mask = device_int_ShuffleOr(y_mask);

            if ( bit == 0 ) {
                if ( node < node_size && frame < frame_size ) {
                    y_ptr[unit] = y_mask;
                }
            }
        }
    }
}


int bbcu_bit_fp32_SparseBinaryLut6_ForwardInference
        (
            int   const     *dev_x_buf,
            int             *dev_y_buf,
            int   const     *dev_input_index,
            float const     *dev_W,
            float const     *running_mean_buf,
            float const     *running_var_buf,
            float           gamma,
            float           beta,
            int             node_size,
            int             frame_size,
            int             frame_stride,
            int             lut_binarize,
            cudaStream_t    streamId
        )
{
    BBCU_DEBUG_ASSERT(bbcu_IsDeviceAvailable());

    unsigned int const THREAD_SIZE    = 512;
    unsigned int const MAX_FRAME_UNIT = 256;
    unsigned int const MAX_NODE_UNIT  = 16;

#if 0
    dim3    block(MAX_FRAME_UNIT, THREAD_SIZE / MAX_FRAME_UNIT);
    while ( (int)block.x / 2 >= frame_size && block.x > 32 ) { block.x /= 2; block.y *= 2; }
    while ( (int)block.y / 2 >= node_size                  ) { block.y /= 2; }
#else
    dim3    block(THREAD_SIZE / MAX_NODE_UNIT, MAX_NODE_UNIT);
    while ( (int)block.y / 2 >= node_size  )                { block.y /= 2; block.x *= 2;}
    while ( (int)block.x / 2 >= frame_size && block.x > 32) { block.x /= 2; }
#endif

    block.x = std::min(block.x, MAX_FRAME_UNIT);
    block.y = std::min(block.y, MAX_NODE_UNIT);
    dim3    grid(1, (node_size + (block.y - 1)) / block.y);
    
    kernal_bit_fp32_SparseBinaryLut6_ForwardInference<MAX_FRAME_UNIT, MAX_NODE_UNIT><<<grid, block, 0, streamId>>>(
            dev_x_buf,
            dev_y_buf,
            dev_input_index,
            dev_W,
            running_mean_buf,
            running_var_buf,
            gamma,
            beta,
            node_size,
            frame_size,
            frame_stride,
            lut_binarize
        );
    BB_CUDA_CHECK_LAST_ERROR();
    
    return 0;
}




#if 0

// -------------------------------------------------
//  Backward
// -------------------------------------------------


template<int MAX_FRAME_UNIT=256, int MAX_NODE_UNIT=16>
__global__ void kernal_bit_fp32_SparseBinaryLut6_Backward
        (
            int   const     *x_buf,
            float const     *dy_buf,
            float           *dx_buf,
            int   const     *input_index,
            float const     *W_buf,
            float           *dW_buf,
            float const     *mean_buf,
            float const     *rstd_buf,
            float           gamma,
            float           reciprocal_frame_size,
            int             node_size,
            int             frame_size,
            int             frame_stride,
            int             bin_frame_stride,
            int             lut_binarize
        )
{

    int node_id = threadIdx.y;
    int node    = blockIdx.y * blockDim.y + threadIdx.y;
    int id      = threadIdx.x;
    int id_step = blockDim.x;

    __shared__  float       sbuf[MAX_NODE_UNIT][MAX_FRAME_UNIT];
    __shared__  float       dW_prev[64][MAX_NODE_UNIT];
    __shared__  float       W[64][MAX_NODE_UNIT];
                float       dW[64];
                int   const *x_ptr[6];
                float const *dy_ptr;
    
    // initialize dW
    if ( node < node_size ) {
        for ( int i = 0; i < 64; ++i) {
            dW[i] = 0;
        }

        for ( int i = id; i < 64; i += id_step ) {
            dW_prev[i][node_id] = dW_buf[node * 64 + i];
        }

        // read W
        for ( int i = id; i < 64; i += id_step ) {
            W[i][node_id] = W_buf[node * 64 + i];
            if ( lut_binarize ) {
                W[i][node_id] = W[i][node_id] > 0.5 ? 1.0 : 0.0;
            }
        }
        
        // init pointer
        for ( int i = 0; i < 6; ++i ) {
            int input_node = input_index[6*node + i];
            x_ptr[i]  = &x_buf[input_node * bin_frame_stride];
        }

        dy_ptr = &dy_buf[node * frame_stride];
    }

    __syncthreads();
    

    float dmeanx = 0;
    float dstd = 0;

    if ( node < node_size ) {
        float mean = mean_buf[node];
        float rstd = rstd_buf[node];
        float rstd2 = rstd * rstd;

        for ( int frame = id; frame < frame_size; frame += id_step ) {
            int bit  = (1 << (frame & 0x1f));
            int unit = (frame >> 5);
            
            // x ���Čv�Z
            float x_vec[6];
            for ( int i = 0; i < 6; ++i) {
                x_vec[i] = (x_ptr[i][unit] & bit) ? 0.7 : 0.3;
            }
            float x = device_fp32_SparseBinaryLut6_NodeForward<MAX_NODE_UNIT>(node_id, x_vec, W);

            // BatchNorm
            float dy = dy_ptr[frame];
            float xc = x - mean;
    //      float xn = xc * rstd;

            float dxn = gamma * dy;
            dstd   += -(dxn * xc * rstd2);
            dmeanx += -(dxn * rstd);

            float dy = dy_ptr[frame];
            float x  = x_ptr[frame];
            float dxn = dy * gamma;
            float dxc = dxn * rstd;
            float dx  = dxc + dmean + (x * dvar * reciprocal_frame_size);
            dx_ptr[frame] = dx;


            float x0_00 = xn[1] * xn[0];
            float x0_01 = xn[1] * xp[0];
            float x0_10 = xp[1] * xn[0];
            float x0_11 = xp[1] * xp[0];
            float x1_00 = xn[3] * xn[2];
            float x1_01 = xn[3] * xp[2];
            float x1_10 = xp[3] * xn[2];
            float x1_11 = xp[3] * xp[2];
            float x2_00 = xn[5] * xn[4];
            float x2_01 = xn[5] * xp[4];
            float x2_10 = xp[5] * xn[4];
            float x2_11 = xp[5] * xp[4];

            float grad = dy_ptr[frame];

            float  x2_00_x1_00 =  x2_00 * x1_00;
            float  x2_00_x1_01 =  x2_00 * x1_01;
            float  x2_00_x1_10 =  x2_00 * x1_10;
            float  x2_00_x1_11 =  x2_00 * x1_11;
            float  x2_01_x1_00 =  x2_01 * x1_00;
            float  x2_01_x1_01 =  x2_01 * x1_01;
            float  x2_01_x1_10 =  x2_01 * x1_10;
            float  x2_01_x1_11 =  x2_01 * x1_11;
            float  x2_10_x1_00 =  x2_10 * x1_00;
            float  x2_10_x1_01 =  x2_10 * x1_01;
            float  x2_10_x1_10 =  x2_10 * x1_10;
            float  x2_10_x1_11 =  x2_10 * x1_11;
            float  x2_11_x1_00 =  x2_11 * x1_00;
            float  x2_11_x1_01 =  x2_11 * x1_01;
            float  x2_11_x1_10 =  x2_11 * x1_10;
            float  x2_11_x1_11 =  x2_11 * x1_11;

            dW[ 0] += x2_00_x1_00 * x0_00 * grad;
            dW[ 1] += x2_00_x1_00 * x0_01 * grad;
            dW[ 2] += x2_00_x1_00 * x0_10 * grad;
            dW[ 3] += x2_00_x1_00 * x0_11 * grad;
            dW[ 4] += x2_00_x1_01 * x0_00 * grad;
            dW[ 5] += x2_00_x1_01 * x0_01 * grad;
            dW[ 6] += x2_00_x1_01 * x0_10 * grad;
            dW[ 7] += x2_00_x1_01 * x0_11 * grad;
            dW[ 8] += x2_00_x1_10 * x0_00 * grad;
            dW[ 9] += x2_00_x1_10 * x0_01 * grad;
            dW[10] += x2_00_x1_10 * x0_10 * grad;
            dW[11] += x2_00_x1_10 * x0_11 * grad;
            dW[12] += x2_00_x1_11 * x0_00 * grad;
            dW[13] += x2_00_x1_11 * x0_01 * grad;
            dW[14] += x2_00_x1_11 * x0_10 * grad;
            dW[15] += x2_00_x1_11 * x0_11 * grad;
            dW[16] += x2_01_x1_00 * x0_00 * grad;
            dW[17] += x2_01_x1_00 * x0_01 * grad;
            dW[18] += x2_01_x1_00 * x0_10 * grad;
            dW[19] += x2_01_x1_00 * x0_11 * grad;
            dW[20] += x2_01_x1_01 * x0_00 * grad;
            dW[21] += x2_01_x1_01 * x0_01 * grad;
            dW[22] += x2_01_x1_01 * x0_10 * grad;
            dW[23] += x2_01_x1_01 * x0_11 * grad;
            dW[24] += x2_01_x1_10 * x0_00 * grad;
            dW[25] += x2_01_x1_10 * x0_01 * grad;
            dW[26] += x2_01_x1_10 * x0_10 * grad;
            dW[27] += x2_01_x1_10 * x0_11 * grad;
            dW[28] += x2_01_x1_11 * x0_00 * grad;
            dW[29] += x2_01_x1_11 * x0_01 * grad;
            dW[30] += x2_01_x1_11 * x0_10 * grad;
            dW[31] += x2_01_x1_11 * x0_11 * grad;
            dW[32] += x2_10_x1_00 * x0_00 * grad;
            dW[33] += x2_10_x1_00 * x0_01 * grad;
            dW[34] += x2_10_x1_00 * x0_10 * grad;
            dW[35] += x2_10_x1_00 * x0_11 * grad;
            dW[36] += x2_10_x1_01 * x0_00 * grad;
            dW[37] += x2_10_x1_01 * x0_01 * grad;
            dW[38] += x2_10_x1_01 * x0_10 * grad;
            dW[39] += x2_10_x1_01 * x0_11 * grad;
            dW[40] += x2_10_x1_10 * x0_00 * grad;
            dW[41] += x2_10_x1_10 * x0_01 * grad;
            dW[42] += x2_10_x1_10 * x0_10 * grad;
            dW[43] += x2_10_x1_10 * x0_11 * grad;
            dW[44] += x2_10_x1_11 * x0_00 * grad;
            dW[45] += x2_10_x1_11 * x0_01 * grad;
            dW[46] += x2_10_x1_11 * x0_10 * grad;
            dW[47] += x2_10_x1_11 * x0_11 * grad;
            dW[48] += x2_11_x1_00 * x0_00 * grad;
            dW[49] += x2_11_x1_00 * x0_01 * grad;
            dW[50] += x2_11_x1_00 * x0_10 * grad;
            dW[51] += x2_11_x1_00 * x0_11 * grad;
            dW[52] += x2_11_x1_01 * x0_00 * grad;
            dW[53] += x2_11_x1_01 * x0_01 * grad;
            dW[54] += x2_11_x1_01 * x0_10 * grad;
            dW[55] += x2_11_x1_01 * x0_11 * grad;
            dW[56] += x2_11_x1_10 * x0_00 * grad;
            dW[57] += x2_11_x1_10 * x0_01 * grad;
            dW[58] += x2_11_x1_10 * x0_10 * grad;
            dW[59] += x2_11_x1_10 * x0_11 * grad;
            dW[60] += x2_11_x1_11 * x0_00 * grad;
            dW[61] += x2_11_x1_11 * x0_01 * grad;
            dW[62] += x2_11_x1_11 * x0_10 * grad;
            dW[63] += x2_11_x1_11 * x0_11 * grad;

            float  x2_00_x0_00 =  x2_00 * x0_00;
            float  x2_00_x0_01 =  x2_00 * x0_01;
            float  x2_00_x0_10 =  x2_00 * x0_10;
            float  x2_00_x0_11 =  x2_00 * x0_11;
            float  x2_01_x0_00 =  x2_01 * x0_00;
            float  x2_01_x0_01 =  x2_01 * x0_01;
            float  x2_01_x0_10 =  x2_01 * x0_10;
            float  x2_01_x0_11 =  x2_01 * x0_11;
            float  x2_10_x0_00 =  x2_10 * x0_00;
            float  x2_10_x0_01 =  x2_10 * x0_01;
            float  x2_10_x0_10 =  x2_10 * x0_10;
            float  x2_10_x0_11 =  x2_10 * x0_11;
            float  x2_11_x0_00 =  x2_11 * x0_00;
            float  x2_11_x0_01 =  x2_11 * x0_01;
            float  x2_11_x0_10 =  x2_11 * x0_10;
            float  x2_11_x0_11 =  x2_11 * x0_11;

            float  x1_00_x0_00 =  x1_00 * x0_00;
            float  x1_00_x0_01 =  x1_00 * x0_01;
            float  x1_00_x0_10 =  x1_00 * x0_10;
            float  x1_00_x0_11 =  x1_00 * x0_11;
            float  x1_01_x0_00 =  x1_01 * x0_00;
            float  x1_01_x0_01 =  x1_01 * x0_01;
            float  x1_01_x0_10 =  x1_01 * x0_10;
            float  x1_01_x0_11 =  x1_01 * x0_11;
            float  x1_10_x0_00 =  x1_10 * x0_00;
            float  x1_10_x0_01 =  x1_10 * x0_01;
            float  x1_10_x0_10 =  x1_10 * x0_10;
            float  x1_10_x0_11 =  x1_10 * x0_11;
            float  x1_11_x0_00 =  x1_11 * x0_00;
            float  x1_11_x0_01 =  x1_11 * x0_01;
            float  x1_11_x0_10 =  x1_11 * x0_10;
            float  x1_11_x0_11 =  x1_11 * x0_11;


            float dxi;
            float dx0_00 = 0;
            float dx0_01 = 0;
            float dx0_10 = 0;
            float dx0_11 = 0;
            float dx1_00 = 0;
            float dx1_01 = 0;
            float dx1_10 = 0;
            float dx1_11 = 0;
            float dx2_00 = 0;
            float dx2_01 = 0;
            float dx2_10 = 0;
            float dx2_11 = 0;
            dxi = W[ 0][node_id];  dx0_00 += dxi * x2_00_x1_00;  dx1_00 += dxi * x2_00_x0_00;  dx2_00 += dxi * x1_00_x0_00;
            dxi = W[ 1][node_id];  dx0_01 += dxi * x2_00_x1_00;  dx1_00 += dxi * x2_00_x0_01;  dx2_00 += dxi * x1_00_x0_01;
            dxi = W[ 2][node_id];  dx0_10 += dxi * x2_00_x1_00;  dx1_00 += dxi * x2_00_x0_10;  dx2_00 += dxi * x1_00_x0_10;
            dxi = W[ 3][node_id];  dx0_11 += dxi * x2_00_x1_00;  dx1_00 += dxi * x2_00_x0_11;  dx2_00 += dxi * x1_00_x0_11;
            dxi = W[ 4][node_id];  dx0_00 += dxi * x2_00_x1_01;  dx1_01 += dxi * x2_00_x0_00;  dx2_00 += dxi * x1_01_x0_00;
            dxi = W[ 5][node_id];  dx0_01 += dxi * x2_00_x1_01;  dx1_01 += dxi * x2_00_x0_01;  dx2_00 += dxi * x1_01_x0_01;
            dxi = W[ 6][node_id];  dx0_10 += dxi * x2_00_x1_01;  dx1_01 += dxi * x2_00_x0_10;  dx2_00 += dxi * x1_01_x0_10;
            dxi = W[ 7][node_id];  dx0_11 += dxi * x2_00_x1_01;  dx1_01 += dxi * x2_00_x0_11;  dx2_00 += dxi * x1_01_x0_11;
            dxi = W[ 8][node_id];  dx0_00 += dxi * x2_00_x1_10;  dx1_10 += dxi * x2_00_x0_00;  dx2_00 += dxi * x1_10_x0_00;
            dxi = W[ 9][node_id];  dx0_01 += dxi * x2_00_x1_10;  dx1_10 += dxi * x2_00_x0_01;  dx2_00 += dxi * x1_10_x0_01;
            dxi = W[10][node_id];  dx0_10 += dxi * x2_00_x1_10;  dx1_10 += dxi * x2_00_x0_10;  dx2_00 += dxi * x1_10_x0_10;
            dxi = W[11][node_id];  dx0_11 += dxi * x2_00_x1_10;  dx1_10 += dxi * x2_00_x0_11;  dx2_00 += dxi * x1_10_x0_11;
            dxi = W[12][node_id];  dx0_00 += dxi * x2_00_x1_11;  dx1_11 += dxi * x2_00_x0_00;  dx2_00 += dxi * x1_11_x0_00;
            dxi = W[13][node_id];  dx0_01 += dxi * x2_00_x1_11;  dx1_11 += dxi * x2_00_x0_01;  dx2_00 += dxi * x1_11_x0_01;
            dxi = W[14][node_id];  dx0_10 += dxi * x2_00_x1_11;  dx1_11 += dxi * x2_00_x0_10;  dx2_00 += dxi * x1_11_x0_10;
            dxi = W[15][node_id];  dx0_11 += dxi * x2_00_x1_11;  dx1_11 += dxi * x2_00_x0_11;  dx2_00 += dxi * x1_11_x0_11;
            dxi = W[16][node_id];  dx0_00 += dxi * x2_01_x1_00;  dx1_00 += dxi * x2_01_x0_00;  dx2_01 += dxi * x1_00_x0_00;
            dxi = W[17][node_id];  dx0_01 += dxi * x2_01_x1_00;  dx1_00 += dxi * x2_01_x0_01;  dx2_01 += dxi * x1_00_x0_01;
            dxi = W[18][node_id];  dx0_10 += dxi * x2_01_x1_00;  dx1_00 += dxi * x2_01_x0_10;  dx2_01 += dxi * x1_00_x0_10;
            dxi = W[19][node_id];  dx0_11 += dxi * x2_01_x1_00;  dx1_00 += dxi * x2_01_x0_11;  dx2_01 += dxi * x1_00_x0_11;
            dxi = W[20][node_id];  dx0_00 += dxi * x2_01_x1_01;  dx1_01 += dxi * x2_01_x0_00;  dx2_01 += dxi * x1_01_x0_00;
            dxi = W[21][node_id];  dx0_01 += dxi * x2_01_x1_01;  dx1_01 += dxi * x2_01_x0_01;  dx2_01 += dxi * x1_01_x0_01;
            dxi = W[22][node_id];  dx0_10 += dxi * x2_01_x1_01;  dx1_01 += dxi * x2_01_x0_10;  dx2_01 += dxi * x1_01_x0_10;
            dxi = W[23][node_id];  dx0_11 += dxi * x2_01_x1_01;  dx1_01 += dxi * x2_01_x0_11;  dx2_01 += dxi * x1_01_x0_11;
            dxi = W[24][node_id];  dx0_00 += dxi * x2_01_x1_10;  dx1_10 += dxi * x2_01_x0_00;  dx2_01 += dxi * x1_10_x0_00;
            dxi = W[25][node_id];  dx0_01 += dxi * x2_01_x1_10;  dx1_10 += dxi * x2_01_x0_01;  dx2_01 += dxi * x1_10_x0_01;
            dxi = W[26][node_id];  dx0_10 += dxi * x2_01_x1_10;  dx1_10 += dxi * x2_01_x0_10;  dx2_01 += dxi * x1_10_x0_10;
            dxi = W[27][node_id];  dx0_11 += dxi * x2_01_x1_10;  dx1_10 += dxi * x2_01_x0_11;  dx2_01 += dxi * x1_10_x0_11;
            dxi = W[28][node_id];  dx0_00 += dxi * x2_01_x1_11;  dx1_11 += dxi * x2_01_x0_00;  dx2_01 += dxi * x1_11_x0_00;
            dxi = W[29][node_id];  dx0_01 += dxi * x2_01_x1_11;  dx1_11 += dxi * x2_01_x0_01;  dx2_01 += dxi * x1_11_x0_01;
            dxi = W[30][node_id];  dx0_10 += dxi * x2_01_x1_11;  dx1_11 += dxi * x2_01_x0_10;  dx2_01 += dxi * x1_11_x0_10;
            dxi = W[31][node_id];  dx0_11 += dxi * x2_01_x1_11;  dx1_11 += dxi * x2_01_x0_11;  dx2_01 += dxi * x1_11_x0_11;
            dxi = W[32][node_id];  dx0_00 += dxi * x2_10_x1_00;  dx1_00 += dxi * x2_10_x0_00;  dx2_10 += dxi * x1_00_x0_00;
            dxi = W[33][node_id];  dx0_01 += dxi * x2_10_x1_00;  dx1_00 += dxi * x2_10_x0_01;  dx2_10 += dxi * x1_00_x0_01;
            dxi = W[34][node_id];  dx0_10 += dxi * x2_10_x1_00;  dx1_00 += dxi * x2_10_x0_10;  dx2_10 += dxi * x1_00_x0_10;
            dxi = W[35][node_id];  dx0_11 += dxi * x2_10_x1_00;  dx1_00 += dxi * x2_10_x0_11;  dx2_10 += dxi * x1_00_x0_11;
            dxi = W[36][node_id];  dx0_00 += dxi * x2_10_x1_01;  dx1_01 += dxi * x2_10_x0_00;  dx2_10 += dxi * x1_01_x0_00;
            dxi = W[37][node_id];  dx0_01 += dxi * x2_10_x1_01;  dx1_01 += dxi * x2_10_x0_01;  dx2_10 += dxi * x1_01_x0_01;
            dxi = W[38][node_id];  dx0_10 += dxi * x2_10_x1_01;  dx1_01 += dxi * x2_10_x0_10;  dx2_10 += dxi * x1_01_x0_10;
            dxi = W[39][node_id];  dx0_11 += dxi * x2_10_x1_01;  dx1_01 += dxi * x2_10_x0_11;  dx2_10 += dxi * x1_01_x0_11;
            dxi = W[40][node_id];  dx0_00 += dxi * x2_10_x1_10;  dx1_10 += dxi * x2_10_x0_00;  dx2_10 += dxi * x1_10_x0_00;
            dxi = W[41][node_id];  dx0_01 += dxi * x2_10_x1_10;  dx1_10 += dxi * x2_10_x0_01;  dx2_10 += dxi * x1_10_x0_01;
            dxi = W[42][node_id];  dx0_10 += dxi * x2_10_x1_10;  dx1_10 += dxi * x2_10_x0_10;  dx2_10 += dxi * x1_10_x0_10;
            dxi = W[43][node_id];  dx0_11 += dxi * x2_10_x1_10;  dx1_10 += dxi * x2_10_x0_11;  dx2_10 += dxi * x1_10_x0_11;
            dxi = W[44][node_id];  dx0_00 += dxi * x2_10_x1_11;  dx1_11 += dxi * x2_10_x0_00;  dx2_10 += dxi * x1_11_x0_00;
            dxi = W[45][node_id];  dx0_01 += dxi * x2_10_x1_11;  dx1_11 += dxi * x2_10_x0_01;  dx2_10 += dxi * x1_11_x0_01;
            dxi = W[46][node_id];  dx0_10 += dxi * x2_10_x1_11;  dx1_11 += dxi * x2_10_x0_10;  dx2_10 += dxi * x1_11_x0_10;
            dxi = W[47][node_id];  dx0_11 += dxi * x2_10_x1_11;  dx1_11 += dxi * x2_10_x0_11;  dx2_10 += dxi * x1_11_x0_11;
            dxi = W[48][node_id];  dx0_00 += dxi * x2_11_x1_00;  dx1_00 += dxi * x2_11_x0_00;  dx2_11 += dxi * x1_00_x0_00;
            dxi = W[49][node_id];  dx0_01 += dxi * x2_11_x1_00;  dx1_00 += dxi * x2_11_x0_01;  dx2_11 += dxi * x1_00_x0_01;
            dxi = W[50][node_id];  dx0_10 += dxi * x2_11_x1_00;  dx1_00 += dxi * x2_11_x0_10;  dx2_11 += dxi * x1_00_x0_10;
            dxi = W[51][node_id];  dx0_11 += dxi * x2_11_x1_00;  dx1_00 += dxi * x2_11_x0_11;  dx2_11 += dxi * x1_00_x0_11;
            dxi = W[52][node_id];  dx0_00 += dxi * x2_11_x1_01;  dx1_01 += dxi * x2_11_x0_00;  dx2_11 += dxi * x1_01_x0_00;
            dxi = W[53][node_id];  dx0_01 += dxi * x2_11_x1_01;  dx1_01 += dxi * x2_11_x0_01;  dx2_11 += dxi * x1_01_x0_01;
            dxi = W[54][node_id];  dx0_10 += dxi * x2_11_x1_01;  dx1_01 += dxi * x2_11_x0_10;  dx2_11 += dxi * x1_01_x0_10;
            dxi = W[55][node_id];  dx0_11 += dxi * x2_11_x1_01;  dx1_01 += dxi * x2_11_x0_11;  dx2_11 += dxi * x1_01_x0_11;
            dxi = W[56][node_id];  dx0_00 += dxi * x2_11_x1_10;  dx1_10 += dxi * x2_11_x0_00;  dx2_11 += dxi * x1_10_x0_00;
            dxi = W[57][node_id];  dx0_01 += dxi * x2_11_x1_10;  dx1_10 += dxi * x2_11_x0_01;  dx2_11 += dxi * x1_10_x0_01;
            dxi = W[58][node_id];  dx0_10 += dxi * x2_11_x1_10;  dx1_10 += dxi * x2_11_x0_10;  dx2_11 += dxi * x1_10_x0_10;
            dxi = W[59][node_id];  dx0_11 += dxi * x2_11_x1_10;  dx1_10 += dxi * x2_11_x0_11;  dx2_11 += dxi * x1_10_x0_11;
            dxi = W[60][node_id];  dx0_00 += dxi * x2_11_x1_11;  dx1_11 += dxi * x2_11_x0_00;  dx2_11 += dxi * x1_11_x0_00;
            dxi = W[61][node_id];  dx0_01 += dxi * x2_11_x1_11;  dx1_11 += dxi * x2_11_x0_01;  dx2_11 += dxi * x1_11_x0_01;
            dxi = W[62][node_id];  dx0_10 += dxi * x2_11_x1_11;  dx1_11 += dxi * x2_11_x0_10;  dx2_11 += dxi * x1_11_x0_10;
            dxi = W[63][node_id];  dx0_11 += dxi * x2_11_x1_11;  dx1_11 += dxi * x2_11_x0_11;  dx2_11 += dxi * x1_11_x0_11;
        
            float *dx_ptr = &dx_buf[(node*6)*frame_stride + frame];
            float dxn;
            float dxp;
            float dx;
            dxn  = dx0_00 * xn[1];    dxn += dx0_10 * xp[1];
            dxp  = dx0_01 * xn[1];    dxp += dx0_11 * xp[1];
            dx = (dxp - dxn) * grad;
            if ( xp[0] == 0.0 || xp[0] == 1.0 ) { dx = 0; }
            dx_ptr[0 * frame_stride] = dx;

            dxn  = dx0_00 * xn[0];
            dxn += dx0_01 * xp[0];
            dxp  = dx0_10 * xn[0];
            dxp += dx0_11 * xp[0];
            dx = (dxp - dxn) * grad;
            if ( xp[0] == 0.0 || xp[0] == 1.0 ) { dx = 0; }
            dx_ptr[1 * frame_stride] = dx;

            dxn  = dx1_00 * xn[3];     
            dxp  = dx1_01 * xn[3];     
            dxn += dx1_10 * xp[3];     
            dxp += dx1_11 * xp[3];     
            dx = (dxp - dxn) * grad;
            if ( xp[0] == 0.0 || xp[0] == 1.0 ) { dx = 0; }
            dx_ptr[2 * frame_stride] = dx;

            dxn  = dx1_00 * xn[2];
            dxn += dx1_01 * xp[2];
            dxp  = dx1_10 * xn[2];
            dxp += dx1_11 * xp[2];
            dx = (dxp - dxn) * grad;
            if ( xp[0] == 0.0 || xp[0] == 1.0 ) { dx = 0; }
            dx_ptr[3 * frame_stride] = dx;

            dxn  = dx2_00 * xn[5];     
            dxp  = dx2_01 * xn[5];     
            dxn += dx2_10 * xp[5];     
            dxp += dx2_11 * xp[5];     
            dx = (dxp - dxn) * grad;
            if ( xp[0] == 0.0 || xp[0] == 1.0 ) { dx = 0; }
            dx_ptr[4 * frame_stride] = dx;

            dxn  = dx2_00 * xn[4];
            dxn += dx2_01 * xp[4];
            dxp  = dx2_10 * xn[4];
            dxp += dx2_11 * xp[4];
            dx = (dxp - dxn) * grad;
            if ( xp[0] == 0.0 || xp[0] == 1.0 ) { dx = 0; }
            dx_ptr[5 * frame_stride] = dx;
        }
    }

    for ( int i = 0; i < 64; ++i ) {
        dW[i] = device_fp32_LocalSum(dW[i], sbuf[node_id]);
    }

    if ( node < node_size ) {
        if ( id == 0 ) {
            for ( int i = 0; i < 64; ++i) {
                dW_buf[node*64 + i] = dW[i] + dW_prev[i][node_id];
            }
        }
    }
}


__global__ void kernal_fp32_StochasticLut6_BackwardMarge(
            const float*    src_buf,
            float*          dst_buf,
            const int*      input_index,
            int             node_size,
            int             frame_size,
            int             frame_stride
        )
{
    int frame = blockDim.x * blockIdx.x + threadIdx.x;

    for ( int node = 0; node < node_size; ++node ) {
        if ( frame < frame_size ) {
            for ( int n = 0; n < 6; ++n ) {
                int in_idx = input_index[node*6 + n];
                float*       dst_buf_ptr = &dst_buf[frame_stride * in_idx];
                float        prev_data = dst_buf_ptr[frame];
                const float* src_buf_ptr = &src_buf[(6 * node + n) * frame_stride];
                
                dst_buf_ptr[frame] = prev_data + src_buf_ptr[frame];
            }
        }
        __syncthreads();
    }
}



int bbcu_bit_fp32_StochasticLut6_Backward(
            int   const     *dev_x_buf,
            float const     *dev_dy_buf,
            float           *dev_dx_buf,
            float           *dev_dx_tmp,
            int   const     *dev_input_index,
            float const     *dev_W,
            float           *dev_dW,
            int             input_node_size,
            int             output_node_size,
            int             frame_size,
            int             frame_stride,
            int             bin_frame_stride,
            int             lut_binarize,
            cudaStream_t    streamId
    )
{
    BBCU_DEBUG_ASSERT(bbcu_IsDeviceAvailable());

    {
        unsigned int const THREAD_SIZE    = 256;
        unsigned int const MAX_FRAME_UNIT = 256;
        unsigned int const MAX_NODE_UNIT  = 16;

#if 0
        dim3    block(MAX_FRAME_UNIT, THREAD_SIZE / MAX_FRAME_UNIT);
        while ( (int)block.x / 2 >= frame_size )       { block.x /= 2; block.y *= 2; }
        while ( (int)block.y / 2 >= output_node_size ) { block.y /= 2; }
#else
        dim3    block(THREAD_SIZE / MAX_NODE_UNIT, MAX_NODE_UNIT);
        while ( (int)block.y / 2 >= output_node_size) { block.y /= 2; block.x *= 2;}
        while ( (int)block.x / 2 >= frame_size      ) { block.x /= 2; }
#endif

        block.x = std::min(block.x, MAX_FRAME_UNIT);
        block.y = std::min(block.y, MAX_NODE_UNIT);
        dim3    grid(1, (output_node_size + (block.y - 1)) / block.y);

        kernal_bit_fp32_StochasticLut6_Backward<MAX_FRAME_UNIT, MAX_NODE_UNIT><<<grid, block, 0, streamId>>>(
                dev_x_buf,
                dev_dy_buf,
                dev_dx_tmp,
                dev_input_index,
                dev_W,
                dev_dW,
                output_node_size,
                frame_size,
                frame_stride,
                bin_frame_stride,
                lut_binarize
            );
        BB_CUDA_CHECK_LAST_ERROR();
    }
    

    {
        BB_CUDA_SAFE_CALL(cudaMemset(dev_dx_buf, 0, input_node_size * frame_stride * sizeof(float)));

        int block_x = frame_size;
        while ( block_x > 1024 ) { block_x /= 2; }

        dim3    grid((frame_size + block_x - 1) /block_x, 1);
        dim3    block(block_x, 1, 1);
        kernal_fp32_StochasticLut6_BackwardMarge<<<grid, block>>>(
                dev_dx_tmp,
                dev_dx_buf,
                dev_input_index,
                output_node_size,
                frame_size,
                frame_stride
            );
        BB_CUDA_CHECK_LAST_ERROR();
    }

    return 0;
}

#endif



// end of file
