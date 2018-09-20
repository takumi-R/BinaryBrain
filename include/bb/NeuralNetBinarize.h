// --------------------------------------------------------------------------
//  Binary Brain  -- binary neural net framework
//
//                                     Copyright (C) 2018 by Ryuji Fuchikami
//                                     https://github.com/ryuz
//                                     ryuji.fuchikami@nifty.com
// --------------------------------------------------------------------------



#pragma once

//#ifndef EIGEN_MPL2_ONLY
//#define EIGEN_MPL2_ONLY
//#endif
#include <Eigen/Core>

#include "NeuralNetLayerBuf.h"


namespace bb {


// バイナリ化(活性化関数)
template <typename T = float, typename INDEX = size_t>
class NeuralNetBinarize : public NeuralNetLayerBuf<T, INDEX>
{
protected:
	INDEX		m_mux_size = 1;
	INDEX		m_frame_size = 1;
	INDEX		m_node_size = 0;
	bool		m_enable = true;

public:
	NeuralNetBinarize() {}

	NeuralNetBinarize(INDEX node_size)
	{
		Resize(node_size);
	}

	~NeuralNetBinarize() {}

	void Resize(INDEX node_size)
	{
		m_node_size = node_size;
	}

	void  SetBinaryEnable(bool enable)
	{
		m_enable = enable;
	}

	void  SetMuxSize(INDEX mux_size) { m_mux_size = mux_size; }
	void  SetBatchSize(INDEX batch_size) { m_frame_size = batch_size * m_mux_size; }

	INDEX GetInputFrameSize(void) const { return m_frame_size; }
	INDEX GetInputNodeSize(void) const { return m_node_size; }
	INDEX GetOutputFrameSize(void) const { return m_frame_size; }
	INDEX GetOutputNodeSize(void) const { return m_node_size; }

	int   GetInputSignalDataType(void) const { return NeuralNetType<T>::type; }
	int   GetInputErrorDataType(void) const { return NeuralNetType<T>::type; }
	int   GetOutputSignalDataType(void) const { return NeuralNetType<T>::type; }
	int   GetOutputErrorDataType(void) const { return NeuralNetType<T>::type; }

	void Forward(bool train = true)
	{
		auto x = GetInputSignalBuffer();
		auto y = GetOutputSignalBuffer();

		if (m_enable) {
			// バイナリ化
			for (INDEX node = 0; node < m_node_size; ++node) {
				for (INDEX frame = 0; frame < m_frame_size; ++frame) {
					y.Set<T>(frame, node, x.Get<T>(frame, node) > (T)0.0 ? (T)1.0 : (T)0.0);
				}
			}
		}
		else {
			// bypass
			for (INDEX node = 0; node < m_node_size; ++node) {
				for (INDEX frame = 0; frame < m_frame_size; ++frame) {
					y.Set<T>(frame, node, x.Get<T>(frame, node));
				}
			}
		}
	}

	void Backward(void)
	{
		auto dx = GetInputErrorBuffer();
		auto dy = GetOutputErrorBuffer();
		auto y = GetOutputSignalBuffer();

		if (m_enable) {
			// hard-tanh
			for (INDEX node = 0; node < m_node_size; ++node) {
				for (INDEX frame = 0; frame < m_frame_size; ++frame) {
					auto err = dy.Get<T>(frame, node);
					auto sig = y.Get<T>(frame, node);
					dx.Set<T>(frame, node, (sig >= (T)-1.0 && sig <= (T)1.0) ? err : 0);
				}
			}
		}
		else {
			// bypass
			for (INDEX node = 0; node < m_node_size; ++node) {
				for (INDEX frame = 0; frame < m_frame_size; ++frame) {
					dx.Set<T>(frame, node, dy.Get<T>(frame, node));
				}
			}
		}
	}

	void Update(void)
	{
	}
};

}
