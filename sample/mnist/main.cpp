#include <iostream>
#include <fstream>
#include <numeric>
#include <random>
#include <chrono>

#include <opencv2/opencv.hpp>

#include <cereal/cereal.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/archives/json.hpp>

#include "bb/NeuralNet.h"
#include "bb/NeuralNetUtility.h"
#include "bb/NeuralNetAffine.h"
#include "bb/NeuralNetSigmoid.h"
#include "bb/NeuralNetSoftmax.h"
#include "bb/NeuralNetInputLimitedAffine.h"
#include "bb/NeuralNetBinarize.h"
#include "bb/NeuralNetUnbinarize.h"
#include "bb/NeuralNetBinaryLut6.h"
#include "bb/NeuralNetBinaryLut6VerilogXilinx.h"
#include "bb/NeuralNetBinaryFilter.h"
#include "bb/ShuffleSet.h"
#include "mnist_read.h"


static void image_show(std::string name, bb::NeuralNetBuffer<> buf, size_t f, size_t h, size_t w)
{
	cv::Mat img((int)h, (int)w, CV_8U);
	for (size_t y = 0; y < h; y++) {
		for (size_t x = 0; x < w; x++) {
			img.at<uchar>((int)y, (int)x) = buf.GetBinary(f, y*w + x) ? 255 : 0;
		}
	}
	cv::imshow(name, img);
}

void print_time(void)
{
	static bool first = true;
	static std::chrono::system_clock::time_point prev_time;

	auto now_time = std::chrono::system_clock::now();

	if (first) {
		first = false;
		prev_time = now_time;
	}

	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now_time - prev_time).count() / 1000.0;
	std::cout << elapsed << "[s]" << std::endl;

	prev_time = now_time;
}



// MNISTデータを使った評価用クラス
class EvaluateMnist
{
protected:

public:
	// 評価用データセット
	std::vector< std::vector<float> >	m_test_images;
	std::vector< std::uint8_t >			m_test_labels;
	std::vector< std::vector<float> >	m_test_onehot;

	// 学習用データセット
	std::vector< std::vector<float> >	m_train_images;
	std::vector< std::uint8_t >			m_train_labels;
	std::vector< std::vector<float> >	m_train_onehot;

	// 学習用バッチ
//	bb::ShuffleSet						m_shuffle_set;
//	std::vector<size_t>					m_train_batch_index;
//	std::vector< std::vector<float> >	m_train_batch_images;
//	std::vector< std::uint8_t >			m_train_batch_labels;
//	std::vector< std::vector<float> >	m_train_batch_onehot;
	
public:
	EvaluateMnist(int train_max_size = -1, int test_max_size = -1)
	{
		// MNISTデータ読み込み
		m_train_images = mnist_read_images_real<float>("train-images-idx3-ubyte", train_max_size);
		m_train_labels = mnist_read_labels("train-labels-idx1-ubyte", train_max_size);
		m_train_onehot = bb::LabelToOnehot<std::uint8_t, float>(m_train_labels, 10);

		m_test_images = mnist_read_images_real<float>("t10k-images-idx3-ubyte", test_max_size);
		m_test_labels = mnist_read_labels("t10k-labels-idx1-ubyte", test_max_size);
		m_test_onehot = bb::LabelToOnehot<std::uint8_t, float>(m_test_labels, 10);

		// インデックス作成
//		m_shuffle_set.Setup(m_train_images.size());
	
//		m_train_batch_index.resize(m_train_images.size());
//		for (size_t i = 0; i < m_train_batch_index.size(); ++i) {
//			m_train_batch_index[i] = i;
//		}
	}


#if 1
	// 浮動小数点版のフラットなネットを評価
	void RunFlatReal(int epoc_size, size_t max_batch_size, int test_rate=1)
	{
		std::mt19937_64 mt(1);

		// 実数版NET構築
		bb::NeuralNet<> net;
		bb::NeuralNetAffine<>  layer0_affine(28 * 28, 100);
		bb::NeuralNetSigmoid<> layer0_sigmoid(100);
		bb::NeuralNetAffine<>  layer1_affine(100, 10);
		bb::NeuralNetSoftmax<> layer1_softmax(10);
		net.AddLayer(&layer0_affine);
		net.AddLayer(&layer0_sigmoid);
		net.AddLayer(&layer1_affine);
		net.AddLayer(&layer1_softmax);

		for (int epoc = 0; epoc < epoc_size; ++epoc) {

			// 学習状況評価
			if (epoc % test_rate == 0) {
				std::cout << "epoc[" << epoc << "] accuracy : " << CalcAccuracy(net) << std::endl;
			}

			for (size_t x_index = 0; x_index < m_train_images.size(); x_index += max_batch_size) {
				// 末尾のバッチサイズクリップ
				size_t batch_size = std::min(max_batch_size, m_train_images.size() - x_index);

				// データセット
				net.SetBatchSize(batch_size);
				for (size_t frame = 0; frame < batch_size; ++frame) {
					net.SetInputValue(frame, m_train_images[x_index + frame]);
				}

				// 予測
				net.Forward();

				// 誤差逆伝播
				for (size_t frame = 0; frame < batch_size; ++frame) {
					auto values = net.GetOutputValue(frame);
					for (size_t node = 0; node < values.size(); ++node) {
						values[node] -= m_train_onehot[x_index + frame][node];
						values[node] /= (float)batch_size;
					}
					net.SetOutputError(frame, values);
				}
				net.Backward();

				// 更新
				net.Update(0.2);
			}
		}
	}
#endif

#if 1
	// 浮動小数点版のフラットなネットを評価
	void RunFlatIlReal(int epoc_size, size_t max_batch_size, int test_rate = 1)
	{
		std::mt19937_64 mt(1);

		// 実数版NET構築
		bb::NeuralNet<> net;
		size_t input_node_size = 28 * 28;
		size_t layer0_node_size = 10 * 6 * 6 * 3;
		size_t layer1_node_size = 10 * 6 * 6;
		size_t layer2_node_size = 10 * 6;
		size_t layer3_node_size = 10;
		size_t output_node_size = 10;
		bb::NeuralNetInputLimitedAffine<>	layer0_affine(28 * 28, layer0_node_size);
		bb::NeuralNetSigmoid<>				layer0_sigmoid(layer0_node_size);
		bb::NeuralNetInputLimitedAffine<>   layer1_affine(layer0_node_size, layer1_node_size);
		bb::NeuralNetSigmoid<>				layer1_sigmoid(layer1_node_size);
		bb::NeuralNetInputLimitedAffine<>   layer2_affine(layer1_node_size, layer2_node_size);
		bb::NeuralNetSigmoid<>				layer2_sigmoid(layer2_node_size);
		bb::NeuralNetInputLimitedAffine<>   layer3_affine(layer2_node_size, layer3_node_size);
		bb::NeuralNetSoftmax<>				layer3_softmax(layer3_node_size);
		net.AddLayer(&layer0_affine);
		net.AddLayer(&layer0_sigmoid);
		net.AddLayer(&layer1_affine);
		net.AddLayer(&layer1_sigmoid);
		net.AddLayer(&layer2_affine);
		net.AddLayer(&layer2_sigmoid);
		net.AddLayer(&layer3_affine);
		net.AddLayer(&layer3_softmax);

		for (int epoc = 0; epoc < epoc_size; ++epoc) {

			// 学習状況評価
			if (epoc % test_rate == 0) {
				std::cout << "epoc[" << epoc << "] accuracy : " << CalcAccuracy(net) << std::endl;
			}

			for (size_t x_index = 0; x_index < m_train_images.size(); x_index += max_batch_size) {
				// 末尾のバッチサイズクリップ
				size_t batch_size = std::min(max_batch_size, m_train_images.size() - x_index);

				// データセット
				net.SetBatchSize(batch_size);
				for (size_t frame = 0; frame < batch_size; ++frame) {
					net.SetInputValue(frame, m_train_images[x_index + frame]);
				}

				// 予測
				net.Forward();

				// 誤差逆伝播
				for (size_t frame = 0; frame < batch_size; ++frame) {
					auto values = net.GetOutputValue(frame);
					for (size_t node = 0; node < values.size(); ++node) {
						values[node] -= m_train_onehot[x_index + frame][node];
						values[node] /= (float)batch_size;
					}
					net.SetOutputError(frame, values);
				}
				net.Backward();
				
				/*
				auto err_buf = layer2_affine.GetOutputErrorBuffer();
				std::cout << err_buf.GetReal(0, 0) << " "
					<< err_buf.GetReal(1, 1) << " "
					<< err_buf.GetReal(2, 2) << " "
					<< err_buf.GetReal(3, 3) << std::endl;

				std::cout << err_buf.GetReal(0, 0) << "-"
					<< err_buf.GetReal(0, 1) << " "
					<< err_buf.GetReal(0, 2) << " "
					<< err_buf.GetReal(0, 3) << std::endl;

				std::cout << err_buf.GetReal(0, 0) << "+"
					<< err_buf.GetReal(1, 0) << " "
					<< err_buf.GetReal(2, 0) << " "
					<< err_buf.GetReal(3, 0) << std::endl;
					*/

				// 更新
				net.Update(0.2);
			}
		}
	}
#endif

	// バイナリ版のフラットなネットを評価
	void RunFlatBinary(int epoc_size, size_t max_batch_size, int test_rate = 1)
	{
		std::mt19937_64 mt(1);

		int train_mux_size = 1;
		int test_mux_size = 16;

		// バイナリ版NET構築
		bb::NeuralNet<> net;
		size_t input_node_size = 28 * 28;
		size_t layer0_node_size = 360 * 2;
		size_t layer1_node_size = 60 * 4;
		size_t layer2_node_size = 10 * 8;
		size_t output_node_size = 10;
		bb::NeuralNetBinarize<>   layer_binarize(input_node_size, input_node_size, mt());
		bb::NeuralNetBinaryLut6<> layer_lut0(input_node_size, layer0_node_size, mt());
		bb::NeuralNetBinaryLut6<> layer_lut1(layer0_node_size, layer1_node_size, mt());
		bb::NeuralNetBinaryLut6<> layer_lut2(layer1_node_size, layer2_node_size, mt());
		bb::NeuralNetUnbinarize<> layer_unbinarize(layer2_node_size, output_node_size, mt());
		auto last_lut_layer = &layer_lut2;
		net.AddLayer(&layer_binarize);
		net.AddLayer(&layer_lut0);
		net.AddLayer(&layer_lut1);
		net.AddLayer(&layer_lut2);
		//	net.AddLayer(&layer_unbinarize);

		// バイナリ版NET構築(評価用)
		bb::NeuralNet<> net_eva;
		net_eva.AddLayer(&layer_binarize);
		net_eva.AddLayer(&layer_lut0);
		net_eva.AddLayer(&layer_lut1);
		net_eva.AddLayer(&layer_lut2);
		net_eva.AddLayer(&layer_unbinarize);

		for (int epoc = 0; epoc < epoc_size; ++epoc) {
			// 学習状況評価
			if (epoc % test_rate == 0) {
				layer_binarize.InitializeCoeff(1);
				layer_unbinarize.InitializeCoeff(1);
				net_eva.SetMuxSize(test_mux_size);
				std::cout << "epoc[" << epoc << "] accuracy : " << CalcAccuracy(net_eva) << std::endl;
			}
			
			for (size_t x_index = 0; x_index < m_train_images.size(); x_index += max_batch_size) {
				// 末尾のバッチサイズクリップ
				size_t batch_size = std::min(max_batch_size, m_train_images.size() - x_index);

				// バッチ学習データの作成
				std::vector< std::vector<float> >	batch_images(m_train_images.begin() + x_index, m_train_images.begin() + x_index + batch_size);
				std::vector< std::uint8_t >			batch_labels(m_train_labels.begin() + x_index, m_train_labels.begin() + x_index + batch_size);

				// データセット
				net.SetMuxSize(train_mux_size);
				net.SetBatchSize(batch_size);
				for (size_t frame = 0; frame < batch_size; ++frame) {
					net.SetInputValue(frame, batch_images[frame]);
				}

				// 予測
				net.Forward();

				// バイナリ版フィードバック
				while (net.Feedback(last_lut_layer->GetOutputOnehotLoss<std::uint8_t, 10>(batch_labels)))
					;
				
				// 中間表示
				layer_binarize.InitializeCoeff(1);
				layer_unbinarize.InitializeCoeff(1);
				net_eva.SetMuxSize(test_mux_size);
				std::cout << "epoc[" << epoc << "] accuracy : " << CalcAccuracy(net_eva) << std::endl;
			}
		}
	}



#if 0
	// バイナリ版のフラットなネットを評価
	void RunFlatBinaryBackward(int epoc_size, size_t batch_size, int test_rate = 1)
	{
		std::mt19937_64 mt(1);
		batch_size = std::min(batch_size, m_train_images.size());

		int train_mux_size = 1;
		int test_mux_size = 16;

		// バイナリ版NET構築
		bb::NeuralNet<> net;
		size_t input_node_size = 28 * 28;
		size_t layer0_node_size = 120;
		size_t layer1_node_size = 120;
//		size_t layer2_node_size = 10;
		size_t output_node_size = 10;
		bb::NeuralNetBinarize<>   layer_binarize(input_node_size, input_node_size, mt());
		bb::NeuralNetBinaryLut6<> layer_lut0(input_node_size, layer0_node_size, mt());
		bb::NeuralNetBinaryLut6<> layer_lut1(layer0_node_size, layer1_node_size, mt());
//		bb::NeuralNetBinaryLut6<> layer_lut2(layer1_node_size, layer2_node_size, mt());
		auto last_lut_layer = &layer_lut1;
		bb::NeuralNetUnbinarize<> layer_unbinarize(layer1_node_size, output_node_size, mt());
		net.AddLayer(&layer_binarize);
		net.AddLayer(&layer_lut0);
		net.AddLayer(&layer_lut1);
	//	net.AddLayer(&layer_lut2);
	//	net.AddLayer(&layer_unbinarize);

		// バイナリ版NET構築(評価用)
		bb::NeuralNet<> net_eva;
		net_eva.AddLayer(&layer_binarize);
		net_eva.AddLayer(&layer_lut0);
		net_eva.AddLayer(&layer_lut1);
	//	net_eva.AddLayer(&layer_lut2);
		net_eva.AddLayer(&layer_unbinarize);

		for (int epoc = 0; epoc < epoc_size; ++epoc) {
			// 学習状況評価
			if (epoc % test_rate == 0) {

				layer_binarize.InitializeCoeff(1);
				layer_unbinarize.InitializeCoeff(1);

#if 0
				auto err_buf1 = layer_lut1.GetInputErrorBuffer();
				for (int i = 0; i < 10; i++) {
					for (int j = 0; j < 20; j++) {
						std::cout << err_buf1.GetReal(i, j) << " ";
					}
					std::cout << std::endl;
				}
				std::cout << std::endl;
				auto err_buf0 = layer_lut0.GetInputErrorBuffer();
				for (int i = 0; i < 10; i++) {
					for (int j = 0; j < 20; j++) {
						std::cout << err_buf0.GetReal(i, j) << " ";
					}
					std::cout << std::endl;
				}
#endif

				net_eva.SetMuxSize(test_mux_size);
				std::cout << "accuracy : " << CalcAccuracy(net_eva) << std::endl;
			}

			// 学習データセットシャッフル
			ShuffleTrainBatch(batch_size, mt());

			// データセット
			net_eva.SetMuxSize(train_mux_size);
			net.SetBatchSize(batch_size);
			for (size_t frame = 0; frame < batch_size; ++frame) {
				net.SetInputValue(frame, m_train_batch_images[frame]);
			}

			// 予測
			net.Forward();

			// バイナリ版逆誤差伝播
			auto err_buf = last_lut_layer->GetOutputErrorBuffer();
			for (size_t frame = 0; frame < err_buf.GetFrameSize(); ++frame) {
				for (size_t node = 0; node < err_buf.GetNodeSize(); ++node) {
					if (node % 10 == m_train_batch_labels[frame / train_mux_size]) {
						err_buf.SetReal(frame, node, +9);
					}
					else {
						err_buf.SetReal(frame, node, -1);
					}
				}
			}

#if 1
			// バイナリ版フィードバック
			while (net.Feedback(last_lut_layer->GetOutputOnehotLoss<std::uint8_t, 10>(m_train_batch_labels)))
				;
#endif

#if 0
			net.Backward();
			net.Update(0.1);
#endif

		}
	}




	// バイナリ版のCNV1層を評価
	void RunCnv1Binary(int epoc_size, size_t batch_size, int test_rate = 1)
	{
		std::cout << "[RunCnv1Binary]" << std::endl;

		std::mt19937_64 mt(1);
		batch_size = std::min(batch_size, m_train_images.size());

		int train_mux_size = 1;
		int test_mux_size = 3;
		
		// layer0 畳み込み層
		size_t layer0_channel = 3;

		size_t layer0_input_c_size = 1;
		size_t layer0_input_h_size = 28;
		size_t layer0_input_w_size = 28;
		size_t layer0_output_c_size = layer0_channel;
		size_t layer0_filter_h_size = 7;
		size_t layer0_filter_w_size = 7;
		size_t layer0_y_step = 3;
		size_t layer0_x_step = 3;
		size_t layer0_output_h_size = ((layer0_input_h_size - layer0_filter_h_size + 1) + (layer0_y_step - 1)) / layer0_y_step;
		size_t layer0_output_w_size = ((layer0_input_w_size - layer0_filter_w_size + 1) + (layer0_x_step - 1)) / layer0_x_step;

		size_t layer0_filter_input_node_size = layer0_input_c_size * layer0_filter_h_size * layer0_filter_h_size;
		size_t layer0_filter_lut0_node_size = layer0_channel * 6;
		size_t layer0_filter_lut1_node_size = layer0_output_c_size;

		size_t input_node_size  = layer0_input_h_size * layer0_input_w_size;
		size_t layer0_node_size = layer0_output_c_size * layer0_output_h_size * layer0_output_w_size;
		size_t layer1_node_size = 360 * 2;
		size_t layer2_node_size = 60 * 2;
		size_t layer3_node_size = 10 * 2;
		size_t output_node_size = 10;

		// 入力層(バイナライズ)
		bb::NeuralNetBinarize<>   layer_binarize(input_node_size, input_node_size, mt());

		// 畳み込み層0作成
		bb::NeuralNetBinaryLut6<true> layer0_filter_lut0(layer0_filter_input_node_size, layer0_filter_lut0_node_size, mt());
		bb::NeuralNetBinaryLut6<true> layer0_filter_lut1(layer0_filter_lut0_node_size, layer0_filter_lut1_node_size, mt());
//		bb::NeuralNetBinaryLut6<true> layer0_filter_lut2(layer0_filter_lut1_node_size, layer0_filter_lut2_node_size, mt());
		bb::NeuralNetGroup<> layer0_filter_net;
		layer0_filter_net.AddLayer(&layer0_filter_lut0);
		layer0_filter_net.AddLayer(&layer0_filter_lut1);
//		layer0_filter_net.AddLayer(&layer0_filter_lut2);
		bb::NeuralNetBinaryFilter<> layer0_cnv(
			&layer0_filter_net,
			layer0_input_c_size,
			layer0_input_h_size,
			layer0_input_w_size,
			layer0_output_c_size,
			layer0_filter_h_size,
			layer0_filter_w_size,
			layer0_y_step,
			layer0_x_step);

		// LUT層
		bb::NeuralNetBinaryLut6<> layer1_lut(layer0_node_size, layer1_node_size, mt());
		bb::NeuralNetBinaryLut6<> layer2_lut(layer1_node_size, layer2_node_size, mt());
		bb::NeuralNetBinaryLut6<> layer3_lut(layer2_node_size, layer3_node_size, mt());
		
		// Unbinarize
		bb::NeuralNetUnbinarize<> layer_unbinarize(layer3_node_size, output_node_size, train_mux_size);
		
		// 命名
		layer_binarize.SetLayerName("binarize");
		layer0_cnv.SetLayerName("layer0_cnv");
		layer1_lut.SetLayerName("layer1_lut");
		layer2_lut.SetLayerName("layer2_lut");
		layer3_lut.SetLayerName("layer3_lut");
		layer_unbinarize.SetLayerName("layer_unbinarize");
		layer0_filter_lut0.SetLayerName("layer0_filter_lut0");
		layer0_filter_lut1.SetLayerName("layer0_filter_lut1");
	//	layer0_filter_lut2.SetLayerName("layer0_filter_lut2");

		// 学習用ネット
		bb::NeuralNet<> net;
		net.AddLayer(&layer_binarize);
		net.AddLayer(&layer0_cnv);
		net.AddLayer(&layer1_lut);
		net.AddLayer(&layer2_lut);
		net.AddLayer(&layer3_lut);
	//	net.AddLayer(&layer_unbinarize);

		// 評価用ネット
		bb::NeuralNet<> net_eva;
		net_eva.AddLayer(&layer_binarize);
		net_eva.AddLayer(&layer0_cnv);
		net_eva.AddLayer(&layer1_lut);
		net_eva.AddLayer(&layer2_lut);
		net_eva.AddLayer(&layer3_lut);
		net_eva.AddLayer(&layer_unbinarize);
		
		double loss_min = 9999.9;
		for (int epoc = 0; epoc < epoc_size; ++epoc) {
			
			// 学習状況評価
			if (epoc % test_rate == 0) {
				print_time();

				net_eva.SetMuxSize(test_mux_size);
				std::cout << "accuracy : " << CalcAccuracy(net_eva, 1) << std::endl;
				
				print_time();
			}

			// 学習データセットシャッフル
			ShuffleTrainBatch(batch_size, mt());

			// データセット
			net.SetMuxSize(train_mux_size);
			net.SetBatchSize(batch_size);
			for (size_t frame = 0; frame < batch_size; ++frame) {
				net.SetInputValue(frame, m_train_batch_images[frame]);
			}

			// 予測
			net.Forward();

			// フィードバック
			std::vector <double> loss;
			do {
				loss = layer3_lut.GetOutputOnehotLoss<std::uint8_t, 10>(m_train_batch_labels);
				double loss_now = std::accumulate(loss.begin(), loss.end(), 0.0) / (double)loss.size();
				if (loss_now < loss_min) {
					loss_min = loss_now;
	//				std::cout << "loss = " << loss_min << std::endl;
					std::cout << "loss = " << loss_min << " (now : " << loss_now << ")" << std::endl;
				}

				static int z = 0;
				z++;
				if (z > 100) {
					z = 0;
					auto buf = net.GetOutputValueBuffer(1);
					image_show("buf0", buf, 0, layer0_output_h_size, layer0_output_w_size);
					image_show("buf1", buf, 1, layer0_output_h_size, layer0_output_w_size);
					image_show("buf2", buf, 2, layer0_output_h_size, layer0_output_w_size);
					image_show("buf3", buf, 3, layer0_output_h_size, layer0_output_w_size);
					cv::waitKey(1);
					std::cout << "loss = " << loss_min << " (now : " << loss_now << ")" << std::endl;
				}

			}  while (net.Feedback(loss));
		}
	}



	// バイナリ版の1層のCNN(Cnv+Pol)を評価
	void RunCnn1Binary(int epoc_size, size_t batch_size, int test_rate = 1)
	{
		std::mt19937_64 mt(1);
		batch_size = std::min(batch_size, m_train_images.size());

		int train_mux_size = 3;
		int test_mux_size = 16;

		size_t	channel = 20;

		// layer0 畳み込み層相当
		size_t layer0_input_c_size = 1;
		size_t layer0_input_h_size = 28;
		size_t layer0_input_w_size = 28;
		size_t layer0_output_c_size = channel;
		size_t layer0_filter_h_size = 5;
		size_t layer0_filter_w_size = 5;
		size_t layer0_y_step = 1;
		size_t layer0_x_step = 1;
		size_t layer0_output_h_size = ((layer0_input_h_size - layer0_filter_h_size + 1) + (layer0_y_step - 1)) / layer0_y_step;
		size_t layer0_output_w_size = ((layer0_input_w_size - layer0_filter_w_size + 1) + (layer0_x_step - 1)) / layer0_x_step;

		size_t layer0_filter_input_node_size = layer0_input_c_size * layer0_filter_h_size * layer0_filter_w_size;
		size_t layer0_filter_lut0_node_size = channel * 6 * 6;
		size_t layer0_filter_lut1_node_size = channel * 6;
		size_t layer0_filter_lut2_node_size = layer0_output_c_size;


		// layer1 プーリング層相当
		size_t layer1_input_c_size = layer0_output_c_size;
		size_t layer1_input_h_size = layer0_output_h_size;
		size_t layer1_input_w_size = layer0_output_w_size;
		size_t layer1_output_c_size = channel;
		size_t layer1_filter_h_size = 2;
		size_t layer1_filter_w_size = 2;
		size_t layer1_y_step = 2;
		size_t layer1_x_step = 2;
		size_t layer1_output_h_size = ((layer1_input_h_size - layer1_filter_h_size + 1) + (layer1_y_step - 1)) / layer1_y_step;
		size_t layer1_output_w_size = ((layer1_input_w_size - layer1_filter_w_size + 1) + (layer1_x_step - 1)) / layer1_x_step;

		size_t layer1_filter_input_node_size = layer1_input_c_size * layer1_filter_h_size * layer1_filter_w_size;
		size_t layer1_filter_lut0_node_size = channel * 6 * 6;
		size_t layer1_filter_lut1_node_size = channel * 6;
		size_t layer1_filter_lut2_node_size = layer1_output_c_size;
		
		// layer全体
		size_t input_node_size = layer0_input_h_size * layer0_input_w_size;
		size_t layer0_node_size = layer0_output_c_size * layer0_output_h_size * layer0_output_w_size;
		size_t layer1_node_size = layer1_output_c_size * layer1_output_h_size * layer1_output_w_size;
		size_t layer2_node_size = 360 * 3;
		size_t layer3_node_size = 60 * 3;
		size_t layer4_node_size = 10 * 3;
		size_t output_node_size = 10;

		// 入力層(バイナライズ)
		bb::NeuralNetBinarize<>   layer_binarize(input_node_size, input_node_size, train_mux_size);

		// layer0 畳み込み層作成
		bb::NeuralNetBinaryLut6<true> layer0_filter_lut0(layer0_filter_input_node_size, layer0_filter_lut0_node_size, train_mux_size);
		bb::NeuralNetBinaryLut6<true> layer0_filter_lut1(layer0_filter_lut0_node_size, layer0_filter_lut1_node_size, train_mux_size);
		bb::NeuralNetBinaryLut6<true> layer0_filter_lut2(layer0_filter_lut1_node_size, layer0_filter_lut2_node_size, train_mux_size);
		bb::NeuralNetGroup<> layer0_filter_net;
		layer0_filter_net.AddLayer(&layer0_filter_lut0);
		layer0_filter_net.AddLayer(&layer0_filter_lut1);
		layer0_filter_net.AddLayer(&layer0_filter_lut2);
		bb::NeuralNetBinaryFilter<> layer0_cnv(
			&layer0_filter_net,
			layer0_input_c_size,
			layer0_input_h_size,
			layer0_input_w_size,
			layer0_output_c_size,
			layer0_filter_h_size,
			layer0_filter_w_size,
			layer0_y_step,
			layer0_x_step);

		// layer1 プーリング層作成
		bb::NeuralNetBinaryLut6<true> layer1_filter_lut0(layer1_filter_input_node_size, layer1_filter_lut0_node_size, train_mux_size);
		bb::NeuralNetBinaryLut6<true> layer1_filter_lut1(layer1_filter_lut0_node_size, layer1_filter_lut1_node_size, train_mux_size);
		bb::NeuralNetBinaryLut6<true> layer1_filter_lut2(layer1_filter_lut1_node_size, layer1_filter_lut2_node_size, train_mux_size);
		bb::NeuralNetGroup<> layer1_filter_net;
		layer1_filter_net.AddLayer(&layer1_filter_lut0);
		layer1_filter_net.AddLayer(&layer1_filter_lut1);
		layer1_filter_net.AddLayer(&layer1_filter_lut2);
		bb::NeuralNetBinaryFilter<> layer1_pol(
			&layer1_filter_net,
			layer1_input_c_size,
			layer1_input_h_size,
			layer1_input_w_size,
			layer1_output_c_size,
			layer1_filter_h_size,
			layer1_filter_w_size,
			layer1_y_step,
			layer1_x_step);

		// Layer2-4 LUT層
		bb::NeuralNetBinaryLut6<> layer2_lut(layer1_node_size, layer2_node_size, train_mux_size);
		bb::NeuralNetBinaryLut6<> layer3_lut(layer2_node_size, layer3_node_size, train_mux_size);
		bb::NeuralNetBinaryLut6<> layer4_lut(layer3_node_size, layer4_node_size, train_mux_size);

		// バイナリの最終段
		bb::NeuralNetBinaryLut<>* lastBinLayer = &layer4_lut;

		// Unbinarize
		bb::NeuralNetUnbinarize<> layer_unbinarize(layer4_node_size, output_node_size, train_mux_size);

		// 学習用ネット
		bb::NeuralNet<> net;
		net.AddLayer(&layer_binarize);
		net.AddLayer(&layer0_cnv);
		net.AddLayer(&layer1_pol);
		net.AddLayer(&layer2_lut);
		net.AddLayer(&layer3_lut);
		net.AddLayer(&layer4_lut);
		//	net.AddLayer(&layer_unbinarize);

		// 評価用ネット
		bb::NeuralNet<> net_eva;
		net_eva.AddLayer(&layer_binarize);
		net_eva.AddLayer(&layer0_cnv);
		net_eva.AddLayer(&layer1_pol);
		net_eva.AddLayer(&layer2_lut);
		net_eva.AddLayer(&layer3_lut);
		net_eva.AddLayer(&layer4_lut);
		net_eva.AddLayer(&layer_unbinarize);

		for (int epoc = 0; epoc < epoc_size; ++epoc) {
			// 学習状況評価
			if (epoc % test_rate == 0) {
				net_eva.SetMuxSize(test_mux_size);
				std::cout << "accuracy : " << CalcAccuracy(net_eva) << std::endl;
			}

			// 学習データセットシャッフル
			ShuffleTrainBatch(batch_size, mt());

			// データセット
			net_eva.SetMuxSize(train_mux_size);
			net.SetBatchSize(batch_size);
			for (size_t frame = 0; frame < batch_size; ++frame) {
				net.SetInputValue(frame, m_train_batch_images[frame]);
			}

			// 予測
			net.Forward();

			// バイナリ版フィードバック
			while (net.Feedback(lastBinLayer->GetOutputOnehotLoss<std::uint8_t, 10>(m_train_batch_labels)))
				;
		}
	}


	// バイナリ版のCNNを評価
	void RunCnnBinary(int epoc_size, size_t batch_size, int test_rate = 1)
	{
		std::mt19937_64 mt(1);
		batch_size = std::min(batch_size, m_train_images.size());

		int train_mux_size = 3;
		int test_mux_size = 16;

		// layer0 畳み込み層相当
		size_t layer0_input_c_size = 1;
		size_t layer0_input_h_size = 28;
		size_t layer0_input_w_size = 28;
		size_t layer0_output_c_size = 8;
		size_t layer0_filter_h_size = 5;
		size_t layer0_filter_w_size = 5;
		size_t layer0_y_step = 1;
		size_t layer0_x_step = 1;
		size_t layer0_output_h_size = ((layer0_input_h_size - layer0_filter_h_size + 1) + (layer0_y_step - 1)) / layer0_y_step;
		size_t layer0_output_w_size = ((layer0_input_w_size - layer0_filter_w_size + 1) + (layer0_x_step - 1)) / layer0_x_step;

		size_t layer0_filter_input_node_size = layer0_input_c_size * layer0_filter_h_size * layer0_filter_w_size;
		size_t layer0_filter_lut0_node_size = 100;
		size_t layer0_filter_lut1_node_size = 48;
		size_t layer0_filter_lut2_node_size = layer0_output_c_size;
		
		// layer1 プーリング層相当
		size_t layer1_input_c_size = layer0_output_c_size;
		size_t layer1_input_h_size = layer0_output_h_size;
		size_t layer1_input_w_size = layer0_output_w_size;
		size_t layer1_output_c_size = 5;
		size_t layer1_filter_h_size = 2;
		size_t layer1_filter_w_size = 2;
		size_t layer1_y_step = 2;
		size_t layer1_x_step = 2;
		size_t layer1_output_h_size = ((layer1_input_h_size - layer1_filter_h_size + 1) + (layer1_y_step - 1)) / layer1_y_step;
		size_t layer1_output_w_size = ((layer1_input_w_size - layer1_filter_w_size + 1) + (layer1_x_step - 1)) / layer1_x_step;

		size_t layer1_filter_input_node_size = layer1_input_c_size * layer1_filter_h_size * layer1_filter_w_size;
		size_t layer1_filter_lut0_node_size = 30;
		size_t layer1_filter_lut1_node_size = layer1_output_c_size;


		// layer2 畳み込み層相当
		size_t layer2_input_c_size = layer1_output_c_size;
		size_t layer2_input_h_size = layer1_output_h_size;
		size_t layer2_input_w_size = layer1_output_w_size;
		size_t layer2_output_c_size = 10;
		size_t layer2_filter_h_size = 5;
		size_t layer2_filter_w_size = 5;
		size_t layer2_y_step = 1;
		size_t layer2_x_step = 1;
		size_t layer2_output_h_size = ((layer2_input_h_size - layer2_filter_h_size + 1) + (layer2_y_step - 1)) / layer2_y_step;
		size_t layer2_output_w_size = ((layer2_input_w_size - layer2_filter_w_size + 1) + (layer2_x_step - 1)) / layer2_x_step;

		size_t layer2_filter_input_node_size = layer2_input_c_size * layer2_filter_h_size * layer2_filter_w_size;
		size_t layer2_filter_lut0_node_size = 100;
		size_t layer2_filter_lut1_node_size = 48;
		size_t layer2_filter_lut2_node_size = layer2_output_c_size;
		
		// layer3 プーリング層相当
		size_t layer3_input_c_size = layer2_output_c_size;
		size_t layer3_input_h_size = layer2_output_h_size;
		size_t layer3_input_w_size = layer2_output_w_size;
		size_t layer3_output_c_size = 5;
		size_t layer3_filter_h_size = 2;
		size_t layer3_filter_w_size = 2;
		size_t layer3_y_step = 2;
		size_t layer3_x_step = 2;
		size_t layer3_output_h_size = ((layer3_input_h_size - layer3_filter_h_size + 1) + (layer3_y_step - 1)) / layer3_y_step;
		size_t layer3_output_w_size = ((layer3_input_w_size - layer3_filter_w_size + 1) + (layer3_x_step - 1)) / layer3_x_step;
		
		size_t layer3_filter_input_node_size = layer3_input_c_size * layer3_filter_h_size * layer3_filter_w_size;
		size_t layer3_filter_lut0_node_size = 30;
		size_t layer3_filter_lut1_node_size = layer3_output_c_size;
		

		// layer全体
		size_t input_node_size = layer0_input_h_size * layer0_input_w_size;
		size_t layer0_node_size = layer0_output_c_size * layer0_output_h_size * layer0_output_w_size;
		size_t layer1_node_size = layer1_output_c_size * layer1_output_h_size * layer1_output_w_size;
		size_t layer2_node_size = layer2_output_c_size * layer2_output_h_size * layer2_output_w_size;
		size_t layer3_node_size = layer3_output_c_size * layer3_output_h_size * layer3_output_w_size;
		size_t layer4_node_size = 60 * 5;
		size_t layer5_node_size = 10 * 5;
		size_t output_node_size = 10;

		// 入力層(バイナライズ)
		bb::NeuralNetBinarize<>   layer_binarize(input_node_size, input_node_size, train_mux_size);

		// layer0 畳み込み層作成
		bb::NeuralNetBinaryLut6<true> layer0_filter_lut0(layer0_filter_input_node_size, layer0_filter_lut0_node_size, train_mux_size);
		bb::NeuralNetBinaryLut6<true> layer0_filter_lut1(layer0_filter_lut0_node_size, layer0_filter_lut1_node_size, train_mux_size);
		bb::NeuralNetBinaryLut6<true> layer0_filter_lut2(layer0_filter_lut1_node_size, layer0_filter_lut2_node_size, train_mux_size);
		bb::NeuralNetGroup<> layer0_filter_net;
		layer0_filter_net.AddLayer(&layer0_filter_lut0);
		layer0_filter_net.AddLayer(&layer0_filter_lut1);
		layer0_filter_net.AddLayer(&layer0_filter_lut2);
		bb::NeuralNetBinaryFilter<> layer0_cnv(
			&layer0_filter_net,
			layer0_input_c_size,
			layer0_input_h_size,
			layer0_input_w_size,
			layer0_output_c_size,
			layer0_filter_h_size,
			layer0_filter_w_size,
			layer0_y_step,
			layer0_x_step);

		// layer1 プーリング層作成
		bb::NeuralNetBinaryLut6<true> layer1_filter_lut0(layer1_filter_input_node_size, layer1_filter_lut0_node_size, train_mux_size);
		bb::NeuralNetBinaryLut6<true> layer1_filter_lut1(layer1_filter_lut0_node_size, layer1_filter_lut1_node_size, train_mux_size);
		bb::NeuralNetGroup<> layer1_filter_net;
		layer1_filter_net.AddLayer(&layer1_filter_lut0);
		layer1_filter_net.AddLayer(&layer1_filter_lut1);
		bb::NeuralNetBinaryFilter<> layer1_pol(
			&layer1_filter_net,
			layer1_input_c_size,
			layer1_input_h_size,
			layer1_input_w_size,
			layer1_output_c_size,
			layer1_filter_h_size,
			layer1_filter_w_size,
			layer1_y_step,
			layer1_x_step);

		// layer2 畳み込み層作成
		bb::NeuralNetBinaryLut6<true> layer2_filter_lut0(layer2_filter_input_node_size, layer2_filter_lut0_node_size, train_mux_size);
		bb::NeuralNetBinaryLut6<true> layer2_filter_lut1(layer2_filter_lut0_node_size, layer2_filter_lut1_node_size, train_mux_size);
		bb::NeuralNetBinaryLut6<true> layer2_filter_lut2(layer2_filter_lut1_node_size, layer2_filter_lut2_node_size, train_mux_size);
		bb::NeuralNetGroup<> layer2_filter_net;
		layer2_filter_net.AddLayer(&layer2_filter_lut0);
		layer2_filter_net.AddLayer(&layer2_filter_lut1);
		layer2_filter_net.AddLayer(&layer2_filter_lut2);
		bb::NeuralNetBinaryFilter<> layer2_cnv(
			&layer2_filter_net,
			layer2_input_c_size,
			layer2_input_h_size,
			layer2_input_w_size,
			layer2_output_c_size,
			layer2_filter_h_size,
			layer2_filter_w_size,
			layer2_y_step,
			layer2_x_step);

		// layer3 プーリング層作成
		bb::NeuralNetBinaryLut6<true> layer3_filter_lut0(layer3_filter_input_node_size, layer3_filter_lut0_node_size, train_mux_size);
		bb::NeuralNetBinaryLut6<true> layer3_filter_lut1(layer3_filter_lut0_node_size, layer3_filter_lut1_node_size, train_mux_size);
		bb::NeuralNetGroup<> layer3_filter_net;
		layer3_filter_net.AddLayer(&layer3_filter_lut0);
		layer3_filter_net.AddLayer(&layer3_filter_lut1);
		bb::NeuralNetBinaryFilter<> layer3_pol(
			&layer3_filter_net,
			layer3_input_c_size,
			layer3_input_h_size,
			layer3_input_w_size,
			layer3_output_c_size,
			layer3_filter_h_size,
			layer3_filter_w_size,
			layer3_y_step,
			layer3_x_step);

		// Layer4-5 LUT層
		bb::NeuralNetBinaryLut6<> layer4_lut(layer3_node_size, layer4_node_size, train_mux_size);
		bb::NeuralNetBinaryLut6<> layer5_lut(layer4_node_size, layer5_node_size, train_mux_size);

		// Unbinarize
		bb::NeuralNetUnbinarize<> layer_unbinarize(layer5_node_size, output_node_size, train_mux_size);

		// バイナリの最終段
		bb::NeuralNetBinaryLut<>* lastBinLayer = &layer5_lut;

		// 学習用ネット
		bb::NeuralNet<> net;
		net.AddLayer(&layer_binarize);
		net.AddLayer(&layer0_cnv);
		net.AddLayer(&layer1_pol);
		net.AddLayer(&layer2_cnv);
		net.AddLayer(&layer3_pol);
		net.AddLayer(&layer4_lut);
		net.AddLayer(&layer5_lut);
		//	net.AddLayer(&layer_unbinarize);

		// 評価用ネット
		bb::NeuralNet<> net_eva;
		net_eva.AddLayer(&layer_binarize);
		net_eva.AddLayer(&layer0_cnv);
		net_eva.AddLayer(&layer1_pol);
		net_eva.AddLayer(&layer2_cnv);
		net_eva.AddLayer(&layer3_pol);
		net_eva.AddLayer(&layer4_lut);
		net_eva.AddLayer(&layer5_lut);
		net_eva.AddLayer(&layer_unbinarize);

		for (int epoc = 0; epoc < epoc_size; ++epoc) {
			// 学習状況評価
			if (epoc % test_rate == 0) {
				//			layer_binarize.SetMuxSize(test_mux_size);
				//			layer_lut0.SetMuxSize(test_mux_size);
				//			layer_lut1.SetMuxSize(test_mux_size);
				//			layer_lut2.SetMuxSize(test_mux_size);
				//			layer_unbinarize.SetMuxSize(test_mux_size);

				std::cout << "accuracy : " << CalcAccuracy(net_eva) << std::endl;
			}

			// 学習データセットシャッフル
			ShuffleTrainBatch(batch_size, mt());

			// データセット
			//		layer_binarize.SetMuxSize(train_mux_size);
			//		layer_lut0.SetMuxSize(train_mux_size);
			//		layer_lut1.SetMuxSize(train_mux_size);
			//		layer_lut2.SetMuxSize(train_mux_size);
			//		layer_unbinarize.SetMuxSize(train_mux_size);
			net.SetBatchSize(batch_size);
			for (size_t frame = 0; frame < batch_size; ++frame) {
				net.SetInputValue(frame, m_train_batch_images[frame]);
			}

			// 予測
			net.Forward();

			// バイナリ版フィードバック
			while (net.Feedback(lastBinLayer->GetOutputOnehotLoss<std::uint8_t, 10>(m_train_batch_labels)))
				;
		}
	}



protected:
	// バッチ数分のサンプルをランダム選択
	void ShuffleTrainBatch(size_t batch_size, std::uint64_t seed)
	{
		std::mt19937_64 mt(seed);

		// シャッフル
	//	std::shuffle(m_train_batch_index.begin(), m_train_batch_index.end(), mt);
		m_train_batch_index = m_shuffle_set.GetRandomSet(batch_size);

		m_train_batch_images.resize(batch_size);
		m_train_batch_labels.resize(batch_size);
		m_train_batch_onehot.resize(batch_size);
		for (size_t frame = 0; frame < batch_size; ++frame) {
			m_train_batch_images[frame] = m_train_images[m_train_batch_index[frame]];
			m_train_batch_labels[frame] = m_train_labels[m_train_batch_index[frame]];
			m_train_batch_onehot[frame] = m_train_onehot[m_train_batch_index[frame]];
		}
	}
#endif

	// ネットの正解率テスト
	float CalcAccuracy(bb::NeuralNet<>& net, std::vector< std::vector<float> >& images, std::vector<std::uint8_t>& labels, size_t start_layer = 0)
	{
		static bool first = true;
		static bb::NeuralNetBuffer<> test_buf;

		// 評価サイズ設定
		net.SetBatchSize(images.size());

		// 評価画像設定
		if (first) {
			for (size_t frame = 0; frame < images.size(); ++frame) {
				net.SetInputValue(frame, images[frame]);
			}
		}
		else {
			net.SetInputValueBuffer(start_layer, test_buf);
		}

		// 評価実施
		net.Forward(first ? 0 : start_layer);

		if (first) {
			test_buf = net.GetInputValueBuffer(start_layer).clone();
			first = false;
		}

		// 結果集計
		int ok_count = 0;
		for (size_t frame = 0; frame < images.size(); ++frame) {
			int max_idx = bb::argmax<float>(net.GetOutputValue(frame));
			ok_count += (max_idx == (int)labels[frame] ? 1 : 0);
		}

		//	std::cout << ok_count << " / " << images.size() << std::endl;

		return (float)ok_count / (float)images.size();
	}

	float CalcAccuracy(bb::NeuralNet<>& net, size_t start_layer=0)
	{
		return CalcAccuracy(net, m_test_images, m_test_labels, start_layer);
	}
};



int main()
{
	omp_set_num_threads(6);

#ifdef _DEBUG
	int train_max_size = 128;
	int test_max_size = 128;
	int epoc_size = 16;
#else
	int train_max_size = -1;
	int test_max_size = -1;
	int epoc_size = 10000;
#endif
	size_t max_batch_size = 8192;


//	train_max_size = 1024;
//	test_max_size = -1;
//	epoc_size = 10000;
//	batch_size = 1024;

	EvaluateMnist	eva_mnist(train_max_size, test_max_size);

	// 実験
//	eva_mnist.m_test_images = eva_mnist.m_train_images;
//	eva_mnist.m_test_labels = eva_mnist.m_train_labels;
//	eva_mnist.m_test_onehot = eva_mnist.m_train_onehot;

//	eva_mnist.RunFlatReal(epoc_size, max_batch_size, 1);
	eva_mnist.RunFlatIlReal(epoc_size, 256, 1);
//	eva_mnist.RunFlatBinary(epoc_size, max_batch_size, 1);
//	eva_mnist.RunFlatBinaryBackward(epoc_size, max_batch_size, 1);
//	eva_mnist.RunCnv1Binary(epoc_size, max_batch_size, 1);
//	eva_mnist.RunCnn1Binary(epoc_size, max_batch_size, 1);
//	eva_mnist.RunCnnBinary(epoc_size, max_batch_size, 1);

	return 0;
}


