﻿// --------------------------------------------------------------------------
//  Binary Brain  -- binary neural net framework
//
//                                     Copyright (C) 2018 by Ryuji Fuchikami
//                                     https://github.com/ryuz
//                                     ryuji.fuchikami@nifty.com
// --------------------------------------------------------------------------



#pragma once


#include "bb/Layer.h"


namespace bb {


// ReLU(活性化関数)
template <typename T = float>
class ReLU : public Layer
{
protected:
    FrameBuffer m_x;
    FrameBuffer m_y;
    FrameBuffer m_dx;

    bool    m_binary_mode = false;

protected:
	ReLU() {}

public:
    static std::shared_ptr<ReLU> Create(void)
    {
        auto self = std::shared_ptr<ReLU>(new ReLU);
        return self;
    }

	~ReLU() {}

	std::string GetClassName(void) const { return "ReLU"; }

	void SetBinaryMode(bool enable)
	{
		m_binary_mode = enable;
	}

    /**
     * @brief  入力形状設定
     * @detail 入力形状を設定する
     *         内部変数を初期化し、以降、GetOutputShape()で値取得可能となることとする
     *         同一形状を指定しても内部変数は初期化されるものとする
     * @param  shape      1フレームのノードを構成するshape
     * @return 出力形状を返す
     */
    indices_t SetInputShape(indices_t shape)
    {
        return shape;
    }


      /**
     * @brief  forward演算
     * @detail forward演算を行う
     * @param  x     入力データ
     * @param  train 学習時にtrueを指定
     * @return forward演算結果
     */
    FrameBuffer Forward(FrameBuffer x, bool train = true)
    {
        m_x = x;
        m_y.Resize(x.GetType(), x.GetFrameSize(), x.GetShape());

        index_t frame_size = m_x.GetFrameSize();
        index_t node_size = m_x.GetNodeSize();

		auto x_ptr = m_x.GetConstPtr<T>();
		auto y_ptr = m_y.GetPtr<T>();

		if (m_binary_mode) {
			// Binarize
#pragma omp parallel for
			for (index_t node = 0; node < node_size; ++node) {
				for (index_t frame = 0; frame < frame_size; ++frame) {
					y_ptr.Set(frame, node, x_ptr.Get(frame, node) >(T)0.0 ? (T)1.0 : (T)0.0);
				}
			}
		}
		else {
			index_t  m256_frame_size = (int)(((frame_size + 7) / 8) * 8);
			__m256 zero = _mm256_set1_ps(0);
			for (index_t node = 0; node < node_size; ++node) {
				auto x_addr = (T*)x_ptr.GetAddr(node);
				auto y_addr = (T*)y_ptr.GetAddr(node);
				for (index_t frame = 0; frame < m256_frame_size; frame += 8) {
					__m256 in_sig = _mm256_load_ps(&x_addr[frame]);
					in_sig = _mm256_max_ps(in_sig, zero);
					_mm256_store_ps(&y_addr[frame], in_sig);
				}
			}
		}
        return m_y;
    }


   /**
     * @brief  backward演算
     * @detail backward演算を行う
     *         
     * @return backward演算結果
     */
	FrameBuffer Backward(FrameBuffer dy)
    {
        m_dx.Resize(dy.GetType(), dy.GetFrameSize(), dy.GetShape());

        index_t frame_size = m_dx.GetFrameSize();
        index_t node_size = m_dx.GetNodeSize();

		auto x_ptr  = m_x.GetConstPtr<T>();
		auto y_ptr  = m_y.GetConstPtr<T>();
		auto dy_ptr = dy.GetConstPtr<T>();
		auto dx_ptr = m_dx.GetPtr<T>();

        if (m_binary_mode) {
#pragma omp parallel for
			for (index_t node = 0; node < node_size; ++node) {
				for (index_t frame = 0; frame < frame_size; ++frame) {
					// hard-tanh
					auto grad = dy_ptr.Get(frame, node);
					auto sig  = x_ptr.Get(frame, node);
					dx_ptr.Set(frame, node, (sig >= (T)-1.0 && sig <= (T)1.0) ? grad : 0);
				}
			}
		}
		else {
			index_t  m256_frame_size = (int)(((frame_size + 7) / 8) * 8);

			__m256 zero = _mm256_set1_ps(0);
			for (index_t node = 0; node < node_size; ++node) {
				auto y_addr  = (T*)y_ptr.GetAddr(node);
				auto dy_addr = (T*)dy_ptr.GetAddr(node);
				auto dx_addr = (T*)dx_ptr.GetAddr(node);
				for (index_t frame = 0; frame < m256_frame_size; frame += 8) {
					__m256 y    = _mm256_load_ps(&y_addr[frame]);
					__m256 dy   = _mm256_load_ps(&dy_addr[frame]);
					__m256 mask = _mm256_cmp_ps(y, zero, _CMP_GT_OS);
					__m256 dx   = _mm256_and_ps(dy, mask);
					_mm256_store_ps(&dx_addr[frame], dx);
				}
			}
		}

        return m_dx;
    }
};

}
