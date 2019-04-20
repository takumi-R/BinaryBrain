﻿// --------------------------------------------------------------------------
//  Binary Brain  -- binary neural net framework
//
//                                     Copyright (C) 2018 by Ryuji Fuchikami
//                                     https://github.com/ryuz
//                                     ryuji.fuchikami@nifty.com
// --------------------------------------------------------------------------



#pragma once

#include <algorithm>
#include <array>
#include <vector>
#include "bb/LutLayer.h"


namespace bb {


// テーブルサイズ固定LUT
template <typename T = float>
class StochasticLut6 : public SparseLayer<T, T>
{
    using super = SparseLayer<T, T>;

protected:
    bool                    m_lut_binarize = true;
    bool                    m_y_binarize = false;
    bool                    m_host_only = false;
    bool                    m_host_simd = true;

    std::string             m_connection;

    index_t                 m_input_node_size = 0;
    index_t                 m_output_node_size = 0;
    indices_t               m_input_shape;
    indices_t               m_output_shape;

    FrameBuffer             m_x_buf;
    FrameBuffer             m_y_buf;
    FrameBuffer             m_dx_buf;

//#ifdef BB_WITH_CUDA
    FrameBuffer             m_dx_tmp;
//#endif


    Tensor_<std::int32_t>   m_input_index;

    std::shared_ptr<Tensor> m_W;
    std::shared_ptr<Tensor> m_dW;

    std::mt19937_64         m_mt;

protected:
    StochasticLut6() {
        m_W  = std::make_shared<Tensor>();
        m_dW = std::make_shared<Tensor>();
    }

    void CommandProc(std::vector<std::string> args)
    {
        // LUTバイナライズ設定
        if ( args.size() == 2 && args[0] == "lut_binarize" )
        {
            m_lut_binarize = EvalBool(args[1]);
        }

        // Y出力バイナライズ設定
        if ( args.size() == 2 && args[0] == "y_binarize" )
        {
            m_y_binarize = EvalBool(args[1]);
        }

        // HostOnlyモード設定
        if (args.size() == 2 && args[0] == "host_only")
        {
            m_host_only = EvalBool(args[1]);
        }

        // Host SIMDモード設定
        if (args.size() == 2 && args[0] == "host_simd")
        {
            m_host_simd = EvalBool(args[1]);
        }
    }

public:
    ~StochasticLut6() {}

    struct create_t
    {
        indices_t       output_shape;   //< 出力形状
        std::string     connection;     //< 結線ルール
        std::uint64_t   seed = 1;       //< 乱数シード
    };

    static std::shared_ptr<StochasticLut6> Create(create_t const &create)
    {
        auto self = std::shared_ptr<StochasticLut6>(new StochasticLut6);
        BB_ASSERT(!create.output_shape.empty());
        self->m_output_shape     = create.output_shape;
        self->m_output_node_size = GetShapeSize(self->m_output_shape);
        self->m_connection       = create.connection;
        self->m_mt.seed(create.seed);
        return self;
    }

    static std::shared_ptr<StochasticLut6> Create(indices_t const &output_shape, std::string connection = "", std::uint64_t seed = 1)
    {
        create_t create;
        create.output_shape = output_shape;
        create.connection   = connection;
        create.seed         = seed;
        return Create(create);
    }

    static std::shared_ptr<StochasticLut6> Create(index_t output_node_size, std::string connection = "", std::uint64_t seed = 1)
    {
        create_t create;
        create.output_shape.resize(1);
        create.output_shape[0] = output_node_size;
        create.connection      = connection;
        create.seed            = seed;
        return Create(create);
    }

    std::string GetClassName(void) const { return "StochasticLut6"; }


public:
    // Serialize
    void Save(std::ostream &os) const 
    {
        SaveIndex(os, m_input_node_size);
        SaveIndex(os, m_output_node_size);
        SaveIndices(os, m_input_shape);
        SaveIndices(os, m_output_shape);
        m_input_index.Save(os);
        m_W->Save(os);
    }

    void Load(std::istream &is)
    {
        m_input_node_size  = LoadIndex(is); 
        m_output_node_size = LoadIndex(is);
        m_input_shape      = LoadIndices(is);
        m_output_shape     = LoadIndices(is);
        m_input_index.Load(is);
        m_W->Load(is);
    }


#ifdef BB_WITH_CEREAL
    template <class Archive>
    void save(Archive& archive, std::uint32_t const version) const
    {
        super::save(archive, version);
        archive(cereal::make_nvp("input_node_size",  m_input_node_size));
        archive(cereal::make_nvp("output_node_size", m_output_node_size));
        archive(cereal::make_nvp("input_shape",      m_input_shape));
        archive(cereal::make_nvp("output_shape",     m_output_shape));
        archive(cereal::make_nvp("input_index",      m_input_index));
        archive(cereal::make_nvp("W",                *m_W));
    }

    template <class Archive>
    void load(Archive& archive, std::uint32_t const version)
    {
        super::load(archive, version);
        archive(cereal::make_nvp("input_node_size",  m_input_node_size));
        archive(cereal::make_nvp("output_node_size", m_output_node_size));
        archive(cereal::make_nvp("input_shape",      m_input_shape));
        archive(cereal::make_nvp("output_shape",     m_output_shape));
        archive(cereal::make_nvp("input_index",      m_input_index));
        archive(cereal::make_nvp("W",                *m_W));
    }

    void Save(cereal::JSONOutputArchive& archive) const
    {
        archive(cereal::make_nvp("RealLut4", *this));
    }

    void Load(cereal::JSONInputArchive& archive)
    {
        archive(cereal::make_nvp("RealLut4", *this));
    }
#endif


    Tensor       &W(void)       { return *m_W; }
    Tensor const &W(void) const { return *m_W; }
    
    Tensor       &dW(void)       { return *m_dW; }
    Tensor const &dW(void) const { return *m_dW; }

    auto lock_InputIndex(void)             { return m_input_index.Lock(); }
    auto lock_InputIndex_const(void) const { return m_input_index.LockConst(); }

    auto lock_W(void)              { return m_W->Lock<T>(); }
    auto lock_W_const(void) const  { return m_W->LockConst<T>(); }
    auto lock_dW(void)             { return m_dW->Lock<T>(); }
    auto lock_dW_const(void) const { return m_dW->LockConst<T>(); }


    index_t GetNodeInputSize(index_t node) const
    {
        return 6;
    }

    void SetNodeInput(index_t node, index_t input_index, index_t input_node)
    {
        auto ptr = lock_InputIndex();
        ptr(node, input_index) = (std::int32_t)input_node;
    }

    index_t GetNodeInput(index_t node, index_t input_index) const
    {
        auto ptr = lock_InputIndex_const();
        return (index_t)ptr(node, input_index);
    }


   /**
     * @brief  入力のshape設定
     * @detail 入力のshape設定
     * @param shape 新しいshape
     * @return なし
     */
    indices_t SetInputShape(indices_t shape)
    {
        // 形状設定
        m_input_shape = shape;
        m_input_node_size = GetShapeSize(shape);
        
        // 接続初期化
        m_input_index.Resize(m_output_node_size, 6);
        this->InitializeNodeInput(m_mt(), m_connection);

        // パラメータ初期化(結局初期値は何が良いのかまだよくわからない)
//      m_W->Resize(DataType<T>::type, m_output_node_size, 64);  m_W->InitUniformDistribution(0.4, 0.6, m_mt());
//      m_W->Resize(DataType<T>::type, m_output_node_size, 64);  m_W->InitUniformDistribution(0.0, 1.0, m_mt());
//      m_W->Resize(DataType<T>::type, m_output_node_size, 64);  m_W->InitNormalDistribution(0.5, 0.001, m_mt());
        m_W->Resize(DataType<T>::type, m_output_node_size, 64);  m_W->InitNormalDistribution(0.5, 0.01, m_mt());

        m_dW->Resize(DataType<T>::type, m_output_node_size, 64); m_dW->FillZero();

        return m_output_shape;
    }

    /**
     * @brief  出力のshape設定
     * @detail 出力のshape設定
     *         出力ノード数が変わらない限りshpeは自由
     * @param shape 新しいshape
     * @return なし
     */
    void SetOutputShape(indices_t const &shape)
    {
        BB_ASSERT(GetShapeSize(shape) == m_output_node_size);
        m_output_shape = shape;
    }


    /**
     * @brief  入力形状取得
     * @detail 入力形状を取得する
     * @return 入力形状を返す
     */
    indices_t GetInputShape(void) const
    {
        return m_input_shape;
    }

    /**
     * @brief  出力形状取得
     * @detail 出力形状を取得する
     * @return 出力形状を返す
     */
    indices_t GetOutputShape(void) const
    {
        return m_output_shape;
    }
    
    
    
    Variables GetParameters(void)
    {
        Variables parameters;
        parameters.PushBack(m_W);
        return parameters;
    }

    Variables GetGradients(void)
    {
        Variables gradients;
        gradients.PushBack(m_dW);
        return gradients;
    }
    

    // ノード単位でのForward計算
    std::vector<T> ForwardNode(index_t node, std::vector<T> input_value) const
    {
        BB_ASSERT(input_value.size() == 6);

        // パラメータクリップ
        m_W->Clamp((T)0.0, (T)1.0);

        auto W_ptr = lock_W_const();
        T W[64];
        for ( int i = 0; i < 64; ++i) {
            W[i] = W_ptr(node, i);
//          BB_ASSERT(W[i] >= 0 && W[i] <= 1.0f);
            if ( m_lut_binarize ) {
                W[i] = W[i] > (T)0.5 ? (T)1.0 : (T)0.0;
            }
        }

        T   xp[6], xn[6];
        for ( int i = 0; i < 6; ++i) {
            xp[i] = input_value[i];
            xp[i] = std::min((T)1.0, std::max((T)0.0, xp[i]));
//          BB_ASSERT(xp[i] >= 0 && xp[i] <= 1.0f);
            xn[i] = (T)1.0 - xp[i];
        }

        T x0_00 = xn[1] * xn[0];
        T x0_01 = xn[1] * xp[0];
        T x0_10 = xp[1] * xn[0];
        T x0_11 = xp[1] * xp[0];
        T x1_00 = xn[3] * xn[2];
        T x1_01 = xn[3] * xp[2];
        T x1_10 = xp[3] * xn[2];
        T x1_11 = xp[3] * xp[2];
        T x2_00 = xn[5] * xn[4];
        T x2_01 = xn[5] * xp[4];
        T x2_10 = xp[5] * xn[4];
        T x2_11 = xp[5] * xp[4];

        T xi[64];
        xi[0]  = x2_00 * x1_00 * x0_00;
        xi[1]  = x2_00 * x1_00 * x0_01;
        xi[2]  = x2_00 * x1_00 * x0_10;
        xi[3]  = x2_00 * x1_00 * x0_11;
        xi[4]  = x2_00 * x1_01 * x0_00;
        xi[5]  = x2_00 * x1_01 * x0_01;
        xi[6]  = x2_00 * x1_01 * x0_10;
        xi[7]  = x2_00 * x1_01 * x0_11;
        xi[8]  = x2_00 * x1_10 * x0_00;
        xi[9]  = x2_00 * x1_10 * x0_01;
        xi[10] = x2_00 * x1_10 * x0_10;
        xi[11] = x2_00 * x1_10 * x0_11;
        xi[12] = x2_00 * x1_11 * x0_00;
        xi[13] = x2_00 * x1_11 * x0_01;
        xi[14] = x2_00 * x1_11 * x0_10;
        xi[15] = x2_00 * x1_11 * x0_11;
        xi[16] = x2_01 * x1_00 * x0_00;
        xi[17] = x2_01 * x1_00 * x0_01;
        xi[18] = x2_01 * x1_00 * x0_10;
        xi[19] = x2_01 * x1_00 * x0_11;
        xi[20] = x2_01 * x1_01 * x0_00;
        xi[21] = x2_01 * x1_01 * x0_01;
        xi[22] = x2_01 * x1_01 * x0_10;
        xi[23] = x2_01 * x1_01 * x0_11;
        xi[24] = x2_01 * x1_10 * x0_00;
        xi[25] = x2_01 * x1_10 * x0_01;
        xi[26] = x2_01 * x1_10 * x0_10;
        xi[27] = x2_01 * x1_10 * x0_11;
        xi[28] = x2_01 * x1_11 * x0_00;
        xi[29] = x2_01 * x1_11 * x0_01;
        xi[30] = x2_01 * x1_11 * x0_10;
        xi[31] = x2_01 * x1_11 * x0_11;
        xi[32] = x2_10 * x1_00 * x0_00;
        xi[33] = x2_10 * x1_00 * x0_01;
        xi[34] = x2_10 * x1_00 * x0_10;
        xi[35] = x2_10 * x1_00 * x0_11;
        xi[36] = x2_10 * x1_01 * x0_00;
        xi[37] = x2_10 * x1_01 * x0_01;
        xi[38] = x2_10 * x1_01 * x0_10;
        xi[39] = x2_10 * x1_01 * x0_11;
        xi[40] = x2_10 * x1_10 * x0_00;
        xi[41] = x2_10 * x1_10 * x0_01;
        xi[42] = x2_10 * x1_10 * x0_10;
        xi[43] = x2_10 * x1_10 * x0_11;
        xi[44] = x2_10 * x1_11 * x0_00;
        xi[45] = x2_10 * x1_11 * x0_01;
        xi[46] = x2_10 * x1_11 * x0_10;
        xi[47] = x2_10 * x1_11 * x0_11;
        xi[48] = x2_11 * x1_00 * x0_00;
        xi[49] = x2_11 * x1_00 * x0_01;
        xi[50] = x2_11 * x1_00 * x0_10;
        xi[51] = x2_11 * x1_00 * x0_11;
        xi[52] = x2_11 * x1_01 * x0_00;
        xi[53] = x2_11 * x1_01 * x0_01;
        xi[54] = x2_11 * x1_01 * x0_10;
        xi[55] = x2_11 * x1_01 * x0_11;
        xi[56] = x2_11 * x1_10 * x0_00;
        xi[57] = x2_11 * x1_10 * x0_01;
        xi[58] = x2_11 * x1_10 * x0_10;
        xi[59] = x2_11 * x1_10 * x0_11;
        xi[60] = x2_11 * x1_11 * x0_00;
        xi[61] = x2_11 * x1_11 * x0_01;
        xi[62] = x2_11 * x1_11 * x0_10;
        xi[63] = x2_11 * x1_11 * x0_11;

        T sig = 0;
        for ( int i = 0; i < 64; ++i) {
            sig += W[i] * xi[i];
        }

        sig = std::max((T)0.0, sig);
        sig = std::min((T)1.0, sig);
        
        std::vector<T> result;
        result.push_back(sig);

        return result;
    }

    FrameBuffer Forward(FrameBuffer x_buf, bool train = true)
    {
        BB_ASSERT(x_buf.GetType() == DataType<T>::type);

        // backwardの為に保存
        m_x_buf = x_buf;

        // SetInputShpaeされていなければ初回に設定
        if (m_x_buf.GetNodeSize() != m_input_node_size) {
            SetInputShape(m_x_buf.GetShape());
        }

        // 出力を設定
        m_y_buf.Resize(DataType<T>::type, m_x_buf.GetFrameSize(), m_output_shape);

        // パラメータクリップ
        m_W->Clamp((T)0.0, (T)1.0);

#ifdef BB_WITH_CUDA
        if ( DataType<T>::type == BB_TYPE_FP32 && !m_host_only
                && m_x_buf.IsDeviceAvailable() && m_y_buf.IsDeviceAvailable() && Manager::IsDeviceAvailable()) {
            auto x_ptr           = x_buf.LockDeviceMemoryConst();
            auto y_ptr           = m_y_buf.LockDeviceMemory(true);
            auto input_index_ptr = m_input_index.LockDeviceMemoryConst();
            auto W_ptr           = m_W->LockDeviceMemoryConst();
               
            bbcu_fp32_StochasticLut6_Forward
                (
                    (const float *)x_ptr.GetAddr(),
                    (float       *)y_ptr.GetAddr(),
                    (int   const *)input_index_ptr.GetAddr(),
                    (float const *)W_ptr.GetAddr(),
                    (int          )m_y_buf.GetNodeSize(),
                    (int          )m_y_buf.GetFrameSize(),
                    (int          )(m_y_buf.GetFrameStride() / sizeof(float)),
                    (int          )(m_lut_binarize ? 1 : 0)
                );

            return m_y_buf;
        }
#endif

        if (DataType<T>::type == BB_TYPE_FP32 && m_host_simd) {
            auto x_ptr           = x_buf.LockConst<float>();
            auto y_ptr           = m_y_buf.Lock<float>(true);
            auto input_index_ptr = m_input_index.LockConst();
            auto W_ptr           = m_W->LockConst<float>();

            auto node_size  = m_y_buf.GetNodeSize();
            auto frame_size = m_y_buf.GetFrameStride() / (index_t)sizeof(float);

            #pragma omp parallel for
            for ( index_t node = 0; node < node_size; ++node ) {
                // read W
                __m256   W[64];
                for ( int i = 0; i < 64; ++i ) {
                    float W_val = W_ptr(node, i);
                    if ( m_lut_binarize ) {
                        W_val = W_val > 0.5f ? 1.0f : 0.0f;
                    }
                    W[i] = _mm256_set1_ps(W_val);
                }
                
                // read input index
                float const  *x_addr[6];
                for ( int i = 0; i < 6; ++i ) {
                    x_addr[i] = x_ptr.GetAddr(input_index_ptr(node, i));
                }
                float *y_addr = y_ptr.GetAddr(node);

                for ( index_t frame = 0; frame < frame_size; frame += 8) {
                    __m256   xp[6], xn[6];
                    for ( int i = 0; i < 6; ++i) {
                        xp[i] = _mm256_loadu_ps(&x_addr[i][frame]);
                        xp[i] = _mm256_min_ps(xp[i], _mm256_set1_ps(1.0));
                        xp[i] = _mm256_max_ps(xp[i], _mm256_set1_ps(0.0));
                        xn[i] = _mm256_sub_ps(_mm256_set1_ps(1.0f), xp[i]);
                    }

                    __m256 x0_00 = _mm256_mul_ps(xn[1], xn[0]);
                    __m256 x0_01 = _mm256_mul_ps(xn[1], xp[0]);
                    __m256 x0_10 = _mm256_mul_ps(xp[1], xn[0]);
                    __m256 x0_11 = _mm256_mul_ps(xp[1], xp[0]);
                    __m256 x1_00 = _mm256_mul_ps(xn[3], xn[2]);
                    __m256 x1_01 = _mm256_mul_ps(xn[3], xp[2]);
                    __m256 x1_10 = _mm256_mul_ps(xp[3], xn[2]);
                    __m256 x1_11 = _mm256_mul_ps(xp[3], xp[2]);
                    __m256 x2_00 = _mm256_mul_ps(xn[5], xn[4]);
                    __m256 x2_01 = _mm256_mul_ps(xn[5], xp[4]);
                    __m256 x2_10 = _mm256_mul_ps(xp[5], xn[4]);
                    __m256 x2_11 = _mm256_mul_ps(xp[5], xp[4]);

                    __m256  y;
                    y =   _mm256_mul_ps(W[0 ], _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_00, x0_00)));
                    y = _mm256_fmadd_ps(W[1 ], _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_00, x0_01)), y);
                    y = _mm256_fmadd_ps(W[2 ], _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_00, x0_10)), y);
                    y = _mm256_fmadd_ps(W[3 ], _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_00, x0_11)), y);
                    y = _mm256_fmadd_ps(W[4 ], _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_01, x0_00)), y);
                    y = _mm256_fmadd_ps(W[5 ], _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_01, x0_01)), y);
                    y = _mm256_fmadd_ps(W[6 ], _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_01, x0_10)), y);
                    y = _mm256_fmadd_ps(W[7 ], _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_01, x0_11)), y);
                    y = _mm256_fmadd_ps(W[8 ], _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_10, x0_00)), y);
                    y = _mm256_fmadd_ps(W[9 ], _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_10, x0_01)), y);
                    y = _mm256_fmadd_ps(W[10], _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_10, x0_10)), y);
                    y = _mm256_fmadd_ps(W[11], _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_10, x0_11)), y);
                    y = _mm256_fmadd_ps(W[12], _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_11, x0_00)), y);
                    y = _mm256_fmadd_ps(W[13], _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_11, x0_01)), y);
                    y = _mm256_fmadd_ps(W[14], _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_11, x0_10)), y);
                    y = _mm256_fmadd_ps(W[15], _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_11, x0_11)), y);
                    y = _mm256_fmadd_ps(W[16], _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_00, x0_00)), y);
                    y = _mm256_fmadd_ps(W[17], _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_00, x0_01)), y);
                    y = _mm256_fmadd_ps(W[18], _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_00, x0_10)), y);
                    y = _mm256_fmadd_ps(W[19], _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_00, x0_11)), y);
                    y = _mm256_fmadd_ps(W[20], _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_01, x0_00)), y);
                    y = _mm256_fmadd_ps(W[21], _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_01, x0_01)), y);
                    y = _mm256_fmadd_ps(W[22], _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_01, x0_10)), y);
                    y = _mm256_fmadd_ps(W[23], _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_01, x0_11)), y);
                    y = _mm256_fmadd_ps(W[24], _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_10, x0_00)), y);
                    y = _mm256_fmadd_ps(W[25], _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_10, x0_01)), y);
                    y = _mm256_fmadd_ps(W[26], _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_10, x0_10)), y);
                    y = _mm256_fmadd_ps(W[27], _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_10, x0_11)), y);
                    y = _mm256_fmadd_ps(W[28], _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_11, x0_00)), y);
                    y = _mm256_fmadd_ps(W[29], _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_11, x0_01)), y);
                    y = _mm256_fmadd_ps(W[30], _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_11, x0_10)), y);
                    y = _mm256_fmadd_ps(W[31], _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_11, x0_11)), y);
                    y = _mm256_fmadd_ps(W[32], _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_00, x0_00)), y);
                    y = _mm256_fmadd_ps(W[33], _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_00, x0_01)), y);
                    y = _mm256_fmadd_ps(W[34], _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_00, x0_10)), y);
                    y = _mm256_fmadd_ps(W[35], _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_00, x0_11)), y);
                    y = _mm256_fmadd_ps(W[36], _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_01, x0_00)), y);
                    y = _mm256_fmadd_ps(W[37], _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_01, x0_01)), y);
                    y = _mm256_fmadd_ps(W[38], _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_01, x0_10)), y);
                    y = _mm256_fmadd_ps(W[39], _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_01, x0_11)), y);
                    y = _mm256_fmadd_ps(W[40], _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_10, x0_00)), y);
                    y = _mm256_fmadd_ps(W[41], _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_10, x0_01)), y);
                    y = _mm256_fmadd_ps(W[42], _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_10, x0_10)), y);
                    y = _mm256_fmadd_ps(W[43], _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_10, x0_11)), y);
                    y = _mm256_fmadd_ps(W[44], _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_11, x0_00)), y);
                    y = _mm256_fmadd_ps(W[45], _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_11, x0_01)), y);
                    y = _mm256_fmadd_ps(W[46], _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_11, x0_10)), y);
                    y = _mm256_fmadd_ps(W[47], _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_11, x0_11)), y);
                    y = _mm256_fmadd_ps(W[48], _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_00, x0_00)), y);
                    y = _mm256_fmadd_ps(W[49], _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_00, x0_01)), y);
                    y = _mm256_fmadd_ps(W[50], _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_00, x0_10)), y);
                    y = _mm256_fmadd_ps(W[51], _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_00, x0_11)), y);
                    y = _mm256_fmadd_ps(W[52], _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_01, x0_00)), y);
                    y = _mm256_fmadd_ps(W[53], _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_01, x0_01)), y);
                    y = _mm256_fmadd_ps(W[54], _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_01, x0_10)), y);
                    y = _mm256_fmadd_ps(W[55], _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_01, x0_11)), y);
                    y = _mm256_fmadd_ps(W[56], _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_10, x0_00)), y);
                    y = _mm256_fmadd_ps(W[57], _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_10, x0_01)), y);
                    y = _mm256_fmadd_ps(W[58], _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_10, x0_10)), y);
                    y = _mm256_fmadd_ps(W[59], _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_10, x0_11)), y);
                    y = _mm256_fmadd_ps(W[60], _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_11, x0_00)), y);
                    y = _mm256_fmadd_ps(W[61], _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_11, x0_01)), y);
                    y = _mm256_fmadd_ps(W[62], _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_11, x0_10)), y);
                    y = _mm256_fmadd_ps(W[63], _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_11, x0_11)), y);
        
                    // clamp
                    y = _mm256_max_ps(y, _mm256_set1_ps(0.0f));
                    y = _mm256_min_ps(y, _mm256_set1_ps(1.0f));

                    _mm256_storeu_ps(&y_addr[frame], y);
                }
            }

            return m_y_buf;
        }

        {
            // 汎用版
            auto frame_size = m_x_buf.GetFrameSize();
            auto x_ptr = x_buf.LockConst<T>();
            auto y_ptr = m_y_buf.Lock<T>();
            auto input_index_ptr = m_input_index.LockConst();
            auto W_ptr = lock_W_const();

#pragma omp parallel for
            for ( index_t node = 0; node < m_output_node_size; ++node ) {
                index_t in_idx[6];
                for ( int i = 0; i < 6; ++i) {
                    in_idx[i] = input_index_ptr(node, i);
                }
                T W[64];
                for ( int i = 0; i < 64; ++i) {
                    W[i] = W_ptr(node, i);
                    BB_ASSERT(W[i] >= 0 && W[i] <= 1.0f);
                    if ( m_lut_binarize ) {
                        W[i] = W[i] > (T)0.5 ? (T)1.0 : (T)0.0;
                    }
                }

                for (index_t frame = 0; frame < frame_size; ++frame ) {
                    T   xp[6], xn[6];
                    for ( int i = 0; i < 6; ++i) {
                        xp[i] = x_ptr.Get(frame, in_idx[i]);
                        xp[i] = std::min((T)1.0, std::max((T)0.0, xp[i]));
//                      BB_ASSERT(xp[i] >= 0 && xp[i] <= 1.0f);
                        xn[i] = (T)1.0 - xp[i];
                    }

                    T x0_00 = xn[1] * xn[0];
                    T x0_01 = xn[1] * xp[0];
                    T x0_10 = xp[1] * xn[0];
                    T x0_11 = xp[1] * xp[0];
                    T x1_00 = xn[3] * xn[2];
                    T x1_01 = xn[3] * xp[2];
                    T x1_10 = xp[3] * xn[2];
                    T x1_11 = xp[3] * xp[2];
                    T x2_00 = xn[5] * xn[4];
                    T x2_01 = xn[5] * xp[4];
                    T x2_10 = xp[5] * xn[4];
                    T x2_11 = xp[5] * xp[4];

                    T xi[64];
                    xi[0]  = x2_00 * x1_00 * x0_00;
                    xi[1]  = x2_00 * x1_00 * x0_01;
                    xi[2]  = x2_00 * x1_00 * x0_10;
                    xi[3]  = x2_00 * x1_00 * x0_11;
                    xi[4]  = x2_00 * x1_01 * x0_00;
                    xi[5]  = x2_00 * x1_01 * x0_01;
                    xi[6]  = x2_00 * x1_01 * x0_10;
                    xi[7]  = x2_00 * x1_01 * x0_11;
                    xi[8]  = x2_00 * x1_10 * x0_00;
                    xi[9]  = x2_00 * x1_10 * x0_01;
                    xi[10] = x2_00 * x1_10 * x0_10;
                    xi[11] = x2_00 * x1_10 * x0_11;
                    xi[12] = x2_00 * x1_11 * x0_00;
                    xi[13] = x2_00 * x1_11 * x0_01;
                    xi[14] = x2_00 * x1_11 * x0_10;
                    xi[15] = x2_00 * x1_11 * x0_11;
                    xi[16] = x2_01 * x1_00 * x0_00;
                    xi[17] = x2_01 * x1_00 * x0_01;
                    xi[18] = x2_01 * x1_00 * x0_10;
                    xi[19] = x2_01 * x1_00 * x0_11;
                    xi[20] = x2_01 * x1_01 * x0_00;
                    xi[21] = x2_01 * x1_01 * x0_01;
                    xi[22] = x2_01 * x1_01 * x0_10;
                    xi[23] = x2_01 * x1_01 * x0_11;
                    xi[24] = x2_01 * x1_10 * x0_00;
                    xi[25] = x2_01 * x1_10 * x0_01;
                    xi[26] = x2_01 * x1_10 * x0_10;
                    xi[27] = x2_01 * x1_10 * x0_11;
                    xi[28] = x2_01 * x1_11 * x0_00;
                    xi[29] = x2_01 * x1_11 * x0_01;
                    xi[30] = x2_01 * x1_11 * x0_10;
                    xi[31] = x2_01 * x1_11 * x0_11;
                    xi[32] = x2_10 * x1_00 * x0_00;
                    xi[33] = x2_10 * x1_00 * x0_01;
                    xi[34] = x2_10 * x1_00 * x0_10;
                    xi[35] = x2_10 * x1_00 * x0_11;
                    xi[36] = x2_10 * x1_01 * x0_00;
                    xi[37] = x2_10 * x1_01 * x0_01;
                    xi[38] = x2_10 * x1_01 * x0_10;
                    xi[39] = x2_10 * x1_01 * x0_11;
                    xi[40] = x2_10 * x1_10 * x0_00;
                    xi[41] = x2_10 * x1_10 * x0_01;
                    xi[42] = x2_10 * x1_10 * x0_10;
                    xi[43] = x2_10 * x1_10 * x0_11;
                    xi[44] = x2_10 * x1_11 * x0_00;
                    xi[45] = x2_10 * x1_11 * x0_01;
                    xi[46] = x2_10 * x1_11 * x0_10;
                    xi[47] = x2_10 * x1_11 * x0_11;
                    xi[48] = x2_11 * x1_00 * x0_00;
                    xi[49] = x2_11 * x1_00 * x0_01;
                    xi[50] = x2_11 * x1_00 * x0_10;
                    xi[51] = x2_11 * x1_00 * x0_11;
                    xi[52] = x2_11 * x1_01 * x0_00;
                    xi[53] = x2_11 * x1_01 * x0_01;
                    xi[54] = x2_11 * x1_01 * x0_10;
                    xi[55] = x2_11 * x1_01 * x0_11;
                    xi[56] = x2_11 * x1_10 * x0_00;
                    xi[57] = x2_11 * x1_10 * x0_01;
                    xi[58] = x2_11 * x1_10 * x0_10;
                    xi[59] = x2_11 * x1_10 * x0_11;
                    xi[60] = x2_11 * x1_11 * x0_00;
                    xi[61] = x2_11 * x1_11 * x0_01;
                    xi[62] = x2_11 * x1_11 * x0_10;
                    xi[63] = x2_11 * x1_11 * x0_11;

                    T sig = 0;
                    for ( int i = 0; i < 64; ++i) {
                        sig += W[i] * xi[i];
                    }

                    sig = std::max((T)0.0, sig);
                    sig = std::min((T)1.0, sig);

                    BB_ASSERT(sig >= 0 && sig <= 1.0f);
                    y_ptr.Set(frame, node, sig);
                }
            }

            return m_y_buf;
        }
    }


    FrameBuffer Backward(FrameBuffer dy_buf)
    {
        BB_ASSERT(dy_buf.GetType() == DataType<T>::type);

        m_dx_buf.Resize(DataType<T>::type, dy_buf.GetFrameSize(), m_input_node_size);
        
#ifdef BB_WITH_CUDA
        if (DataType<T>::type == BB_TYPE_FP32 && !m_host_only
                && dy_buf.IsDeviceAvailable() && m_y_buf.IsDeviceAvailable() && m_dx_buf.IsDeviceAvailable() && Manager::IsDeviceAvailable()) {
            auto x_ptr           = m_x_buf.LockDeviceMemoryConst();
            auto dy_ptr          = dy_buf.LockDeviceMemoryConst();
            auto dx_ptr          = m_dx_buf.LockDeviceMemory(true);
            auto input_index_ptr = m_input_index.LockDeviceMemoryConst();
            auto W_ptr           = m_W->LockDeviceMemoryConst();
            auto dW_ptr          = m_dW->LockDeviceMemory();

            m_dx_tmp.Resize(BB_TYPE_FP32, dy_buf.GetFrameSize(), m_output_node_size * 6);
            auto dx_tmp_ptr = m_dx_tmp.LockDeviceMemory();
            
            bbcu_fp32_StochasticLut6_Backward(
                    (float const *)x_ptr.GetAddr(),
                    (float const *)dy_ptr.GetAddr(),
                    (float       *)dx_ptr.GetAddr(),
                    (float       *)dx_tmp_ptr.GetAddr(),
                    (int   const *)input_index_ptr.GetAddr(),
                    (float const *)W_ptr.GetAddr(),
                    (float       *)dW_ptr.GetAddr(),
                    (int          )m_dx_buf.GetNodeSize(),
                    (int          )m_y_buf.GetNodeSize(),
                    (int          )m_dx_buf.GetFrameSize(),
                    (int          )(m_dx_buf.GetFrameStride() / sizeof(float)),
                    (int          )(m_lut_binarize ? 1 : 0)
                );
       
            return m_dx_buf;
        }
#endif

        if ( DataType<T>::type == BB_TYPE_FP32 && m_host_simd && m_y_buf.GetFrameSize() % 8 == 0 ) {
     //     m_dW->FillZero();
            m_dx_buf.FillZero();

            auto node_size  = m_y_buf.GetNodeSize();
            auto frame_size = m_y_buf.GetFrameStride() / (index_t)sizeof(float);

            auto x_ptr           = m_x_buf.LockConst<float>();
            auto dy_ptr          = dy_buf.LockConst<float>();
            auto dx_ptr          = m_dx_buf.Lock<float>(true);
            auto input_index_ptr = m_input_index.LockConst();
            auto W_ptr           = m_W->LockConst<float>();
            auto dW_ptr          = m_dW->Lock<float>();

            // 並列化用tmpバッファ確保
            m_dx_tmp.Resize(BB_TYPE_FP32, dy_buf.GetFrameSize(), m_output_node_size * 6);
            auto dx_tmp_ptr = m_dx_tmp.Lock<float>();

            #pragma omp parallel for
            for ( index_t node = 0; node < m_output_node_size; ++node ) {           // initialize dW
                __m256  dW[64];
                for ( int i = 0; i < 64; ++i) {
                    dW[i] = _mm256_set1_ps(0.0f);
                }

                // read W
                __m256   W[64];
                for ( int i = 0; i < 64; ++i ) {
                    float W_val = W_ptr(node, i);
                    if ( m_lut_binarize ) {
                        W_val = W_val > 0.5f ? 1.0f : 0.0f;
                    }
                    W[i] = _mm256_set1_ps(W_val);
                }
    
                // read input index
                float const  *x_addr[6];
                for ( int i = 0; i < 6; ++i ) {
                    x_addr[i] = x_ptr.GetAddr(input_index_ptr(node, i));
                }
                
                float const *dy_addr = dy_ptr.GetAddr(node);

                float   *dx_addr[6];
                for ( int i = 0; i < 6; ++i ) {
                    dx_addr[i] = dx_tmp_ptr.GetAddr(node*6 + i);
                }
                
                for ( index_t frame = 0; frame < frame_size; frame += 8 ) {
                    __m256   xp[6], xn[6];
                    for ( int i = 0; i < 6; ++i) {
                        xp[i] = _mm256_loadu_ps(&x_addr[i][frame]);
                        xp[i] = _mm256_min_ps(xp[i], _mm256_set1_ps(1.0));
                        xp[i] = _mm256_max_ps(xp[i], _mm256_set1_ps(0.0));
                        xn[i] = _mm256_sub_ps(_mm256_set1_ps(1.0f), xp[i]);
                    }

                    __m256 x0_00 = _mm256_mul_ps(xn[1], xn[0]);
                    __m256 x0_01 = _mm256_mul_ps(xn[1], xp[0]);
                    __m256 x0_10 = _mm256_mul_ps(xp[1], xn[0]);
                    __m256 x0_11 = _mm256_mul_ps(xp[1], xp[0]);
                    __m256 x1_00 = _mm256_mul_ps(xn[3], xn[2]);
                    __m256 x1_01 = _mm256_mul_ps(xn[3], xp[2]);
                    __m256 x1_10 = _mm256_mul_ps(xp[3], xn[2]);
                    __m256 x1_11 = _mm256_mul_ps(xp[3], xp[2]);
                    __m256 x2_00 = _mm256_mul_ps(xn[5], xn[4]);
                    __m256 x2_01 = _mm256_mul_ps(xn[5], xp[4]);
                    __m256 x2_10 = _mm256_mul_ps(xp[5], xn[4]);
                    __m256 x2_11 = _mm256_mul_ps(xp[5], xp[4]);

                    __m256 grad = _mm256_load_ps(&dy_addr[frame]);
                    dW[ 0] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_00, x0_00)), dW[ 0]);  
                    dW[ 1] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_00, x0_01)), dW[ 1]);
                    dW[ 2] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_00, x0_10)), dW[ 2]);
                    dW[ 3] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_00, x0_11)), dW[ 3]);
                    dW[ 4] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_01, x0_00)), dW[ 4]);
                    dW[ 5] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_01, x0_01)), dW[ 5]);
                    dW[ 6] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_01, x0_10)), dW[ 6]);
                    dW[ 7] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_01, x0_11)), dW[ 7]);
                    dW[ 8] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_10, x0_00)), dW[ 8]);
                    dW[ 9] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_10, x0_01)), dW[ 9]);
                    dW[10] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_10, x0_10)), dW[10]);
                    dW[11] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_10, x0_11)), dW[11]);
                    dW[12] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_11, x0_00)), dW[12]);
                    dW[13] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_11, x0_01)), dW[13]);
                    dW[14] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_11, x0_10)), dW[14]);
                    dW[15] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_00, _mm256_mul_ps(x1_11, x0_11)), dW[15]);
                    dW[16] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_00, x0_00)), dW[16]);
                    dW[17] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_00, x0_01)), dW[17]);
                    dW[18] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_00, x0_10)), dW[18]);
                    dW[19] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_00, x0_11)), dW[19]);
                    dW[20] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_01, x0_00)), dW[20]);
                    dW[21] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_01, x0_01)), dW[21]);
                    dW[22] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_01, x0_10)), dW[22]);
                    dW[23] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_01, x0_11)), dW[23]);
                    dW[24] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_10, x0_00)), dW[24]);
                    dW[25] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_10, x0_01)), dW[25]);
                    dW[26] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_10, x0_10)), dW[26]);
                    dW[27] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_10, x0_11)), dW[27]);
                    dW[28] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_11, x0_00)), dW[28]);
                    dW[29] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_11, x0_01)), dW[29]);
                    dW[30] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_11, x0_10)), dW[30]);
                    dW[31] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_01, _mm256_mul_ps(x1_11, x0_11)), dW[31]);
                    dW[32] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_00, x0_00)), dW[32]);
                    dW[33] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_00, x0_01)), dW[33]);
                    dW[34] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_00, x0_10)), dW[34]);
                    dW[35] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_00, x0_11)), dW[35]);
                    dW[36] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_01, x0_00)), dW[36]);
                    dW[37] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_01, x0_01)), dW[37]);
                    dW[38] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_01, x0_10)), dW[38]);
                    dW[39] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_01, x0_11)), dW[39]);
                    dW[40] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_10, x0_00)), dW[40]);
                    dW[41] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_10, x0_01)), dW[41]);
                    dW[42] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_10, x0_10)), dW[42]);
                    dW[43] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_10, x0_11)), dW[43]);
                    dW[44] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_11, x0_00)), dW[44]);
                    dW[45] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_11, x0_01)), dW[45]);
                    dW[46] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_11, x0_10)), dW[46]);
                    dW[47] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_10, _mm256_mul_ps(x1_11, x0_11)), dW[47]);
                    dW[48] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_00, x0_00)), dW[48]);
                    dW[49] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_00, x0_01)), dW[49]);
                    dW[50] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_00, x0_10)), dW[50]);
                    dW[51] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_00, x0_11)), dW[51]);
                    dW[52] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_01, x0_00)), dW[52]);
                    dW[53] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_01, x0_01)), dW[53]);
                    dW[54] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_01, x0_10)), dW[54]);
                    dW[55] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_01, x0_11)), dW[55]);
                    dW[56] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_10, x0_00)), dW[56]);
                    dW[57] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_10, x0_01)), dW[57]);
                    dW[58] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_10, x0_10)), dW[58]);
                    dW[59] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_10, x0_11)), dW[59]);
                    dW[60] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_11, x0_00)), dW[60]);
                    dW[61] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_11, x0_01)), dW[61]);
                    dW[62] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_11, x0_10)), dW[62]);
                    dW[63] = _mm256_fmadd_ps(grad, _mm256_mul_ps(x2_11, _mm256_mul_ps(x1_11, x0_11)), dW[63]);

                    __m256 dxi;
                    __m256 dx0_00 = _mm256_set1_ps(0.0f);
                    __m256 dx0_01 = _mm256_set1_ps(0.0f);
                    __m256 dx0_10 = _mm256_set1_ps(0.0f);
                    __m256 dx0_11 = _mm256_set1_ps(0.0f);
                    __m256 dx1_00 = _mm256_set1_ps(0.0f);
                    __m256 dx1_01 = _mm256_set1_ps(0.0f);
                    __m256 dx1_10 = _mm256_set1_ps(0.0f);
                    __m256 dx1_11 = _mm256_set1_ps(0.0f);
                    __m256 dx2_00 = _mm256_set1_ps(0.0f);
                    __m256 dx2_01 = _mm256_set1_ps(0.0f);
                    __m256 dx2_10 = _mm256_set1_ps(0.0f);
                    __m256 dx2_11 = _mm256_set1_ps(0.0f);
                    dxi = _mm256_mul_ps(W[ 0], grad);  dx0_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x1_00), dx0_00);  dx1_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x0_00), dx1_00);  dx2_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_00, x0_00), dx2_00);
                    dxi = _mm256_mul_ps(W[ 1], grad);  dx0_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x1_00), dx0_01);  dx1_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x0_01), dx1_00);  dx2_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_00, x0_01), dx2_00);
                    dxi = _mm256_mul_ps(W[ 2], grad);  dx0_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x1_00), dx0_10);  dx1_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x0_10), dx1_00);  dx2_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_00, x0_10), dx2_00);
                    dxi = _mm256_mul_ps(W[ 3], grad);  dx0_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x1_00), dx0_11);  dx1_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x0_11), dx1_00);  dx2_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_00, x0_11), dx2_00);
                    dxi = _mm256_mul_ps(W[ 4], grad);  dx0_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x1_01), dx0_00);  dx1_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x0_00), dx1_01);  dx2_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_01, x0_00), dx2_00);
                    dxi = _mm256_mul_ps(W[ 5], grad);  dx0_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x1_01), dx0_01);  dx1_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x0_01), dx1_01);  dx2_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_01, x0_01), dx2_00);
                    dxi = _mm256_mul_ps(W[ 6], grad);  dx0_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x1_01), dx0_10);  dx1_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x0_10), dx1_01);  dx2_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_01, x0_10), dx2_00);
                    dxi = _mm256_mul_ps(W[ 7], grad);  dx0_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x1_01), dx0_11);  dx1_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x0_11), dx1_01);  dx2_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_01, x0_11), dx2_00);
                    dxi = _mm256_mul_ps(W[ 8], grad);  dx0_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x1_10), dx0_00);  dx1_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x0_00), dx1_10);  dx2_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_10, x0_00), dx2_00);
                    dxi = _mm256_mul_ps(W[ 9], grad);  dx0_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x1_10), dx0_01);  dx1_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x0_01), dx1_10);  dx2_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_10, x0_01), dx2_00);
                    dxi = _mm256_mul_ps(W[10], grad);  dx0_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x1_10), dx0_10);  dx1_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x0_10), dx1_10);  dx2_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_10, x0_10), dx2_00);
                    dxi = _mm256_mul_ps(W[11], grad);  dx0_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x1_10), dx0_11);  dx1_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x0_11), dx1_10);  dx2_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_10, x0_11), dx2_00);
                    dxi = _mm256_mul_ps(W[12], grad);  dx0_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x1_11), dx0_00);  dx1_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x0_00), dx1_11);  dx2_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_11, x0_00), dx2_00);
                    dxi = _mm256_mul_ps(W[13], grad);  dx0_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x1_11), dx0_01);  dx1_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x0_01), dx1_11);  dx2_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_11, x0_01), dx2_00);
                    dxi = _mm256_mul_ps(W[14], grad);  dx0_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x1_11), dx0_10);  dx1_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x0_10), dx1_11);  dx2_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_11, x0_10), dx2_00);
                    dxi = _mm256_mul_ps(W[15], grad);  dx0_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x1_11), dx0_11);  dx1_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_00, x0_11), dx1_11);  dx2_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_11, x0_11), dx2_00);
                    dxi = _mm256_mul_ps(W[16], grad);  dx0_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x1_00), dx0_00);  dx1_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x0_00), dx1_00);  dx2_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_00, x0_00), dx2_01);
                    dxi = _mm256_mul_ps(W[17], grad);  dx0_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x1_00), dx0_01);  dx1_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x0_01), dx1_00);  dx2_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_00, x0_01), dx2_01);
                    dxi = _mm256_mul_ps(W[18], grad);  dx0_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x1_00), dx0_10);  dx1_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x0_10), dx1_00);  dx2_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_00, x0_10), dx2_01);
                    dxi = _mm256_mul_ps(W[19], grad);  dx0_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x1_00), dx0_11);  dx1_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x0_11), dx1_00);  dx2_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_00, x0_11), dx2_01);
                    dxi = _mm256_mul_ps(W[20], grad);  dx0_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x1_01), dx0_00);  dx1_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x0_00), dx1_01);  dx2_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_01, x0_00), dx2_01);
                    dxi = _mm256_mul_ps(W[21], grad);  dx0_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x1_01), dx0_01);  dx1_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x0_01), dx1_01);  dx2_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_01, x0_01), dx2_01);
                    dxi = _mm256_mul_ps(W[22], grad);  dx0_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x1_01), dx0_10);  dx1_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x0_10), dx1_01);  dx2_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_01, x0_10), dx2_01);
                    dxi = _mm256_mul_ps(W[23], grad);  dx0_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x1_01), dx0_11);  dx1_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x0_11), dx1_01);  dx2_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_01, x0_11), dx2_01);
                    dxi = _mm256_mul_ps(W[24], grad);  dx0_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x1_10), dx0_00);  dx1_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x0_00), dx1_10);  dx2_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_10, x0_00), dx2_01);
                    dxi = _mm256_mul_ps(W[25], grad);  dx0_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x1_10), dx0_01);  dx1_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x0_01), dx1_10);  dx2_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_10, x0_01), dx2_01);
                    dxi = _mm256_mul_ps(W[26], grad);  dx0_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x1_10), dx0_10);  dx1_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x0_10), dx1_10);  dx2_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_10, x0_10), dx2_01);
                    dxi = _mm256_mul_ps(W[27], grad);  dx0_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x1_10), dx0_11);  dx1_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x0_11), dx1_10);  dx2_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_10, x0_11), dx2_01);
                    dxi = _mm256_mul_ps(W[28], grad);  dx0_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x1_11), dx0_00);  dx1_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x0_00), dx1_11);  dx2_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_11, x0_00), dx2_01);
                    dxi = _mm256_mul_ps(W[29], grad);  dx0_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x1_11), dx0_01);  dx1_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x0_01), dx1_11);  dx2_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_11, x0_01), dx2_01);
                    dxi = _mm256_mul_ps(W[30], grad);  dx0_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x1_11), dx0_10);  dx1_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x0_10), dx1_11);  dx2_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_11, x0_10), dx2_01);
                    dxi = _mm256_mul_ps(W[31], grad);  dx0_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x1_11), dx0_11);  dx1_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_01, x0_11), dx1_11);  dx2_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_11, x0_11), dx2_01);
                    dxi = _mm256_mul_ps(W[32], grad);  dx0_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x1_00), dx0_00);  dx1_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x0_00), dx1_00);  dx2_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_00, x0_00), dx2_10);
                    dxi = _mm256_mul_ps(W[33], grad);  dx0_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x1_00), dx0_01);  dx1_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x0_01), dx1_00);  dx2_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_00, x0_01), dx2_10);
                    dxi = _mm256_mul_ps(W[34], grad);  dx0_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x1_00), dx0_10);  dx1_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x0_10), dx1_00);  dx2_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_00, x0_10), dx2_10);
                    dxi = _mm256_mul_ps(W[35], grad);  dx0_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x1_00), dx0_11);  dx1_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x0_11), dx1_00);  dx2_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_00, x0_11), dx2_10);
                    dxi = _mm256_mul_ps(W[36], grad);  dx0_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x1_01), dx0_00);  dx1_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x0_00), dx1_01);  dx2_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_01, x0_00), dx2_10);
                    dxi = _mm256_mul_ps(W[37], grad);  dx0_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x1_01), dx0_01);  dx1_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x0_01), dx1_01);  dx2_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_01, x0_01), dx2_10);
                    dxi = _mm256_mul_ps(W[38], grad);  dx0_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x1_01), dx0_10);  dx1_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x0_10), dx1_01);  dx2_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_01, x0_10), dx2_10);
                    dxi = _mm256_mul_ps(W[39], grad);  dx0_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x1_01), dx0_11);  dx1_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x0_11), dx1_01);  dx2_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_01, x0_11), dx2_10);
                    dxi = _mm256_mul_ps(W[40], grad);  dx0_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x1_10), dx0_00);  dx1_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x0_00), dx1_10);  dx2_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_10, x0_00), dx2_10);
                    dxi = _mm256_mul_ps(W[41], grad);  dx0_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x1_10), dx0_01);  dx1_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x0_01), dx1_10);  dx2_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_10, x0_01), dx2_10);
                    dxi = _mm256_mul_ps(W[42], grad);  dx0_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x1_10), dx0_10);  dx1_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x0_10), dx1_10);  dx2_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_10, x0_10), dx2_10);
                    dxi = _mm256_mul_ps(W[43], grad);  dx0_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x1_10), dx0_11);  dx1_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x0_11), dx1_10);  dx2_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_10, x0_11), dx2_10);
                    dxi = _mm256_mul_ps(W[44], grad);  dx0_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x1_11), dx0_00);  dx1_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x0_00), dx1_11);  dx2_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_11, x0_00), dx2_10);
                    dxi = _mm256_mul_ps(W[45], grad);  dx0_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x1_11), dx0_01);  dx1_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x0_01), dx1_11);  dx2_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_11, x0_01), dx2_10);
                    dxi = _mm256_mul_ps(W[46], grad);  dx0_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x1_11), dx0_10);  dx1_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x0_10), dx1_11);  dx2_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_11, x0_10), dx2_10);
                    dxi = _mm256_mul_ps(W[47], grad);  dx0_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x1_11), dx0_11);  dx1_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_10, x0_11), dx1_11);  dx2_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_11, x0_11), dx2_10);
                    dxi = _mm256_mul_ps(W[48], grad);  dx0_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x1_00), dx0_00);  dx1_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x0_00), dx1_00);  dx2_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_00, x0_00), dx2_11);
                    dxi = _mm256_mul_ps(W[49], grad);  dx0_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x1_00), dx0_01);  dx1_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x0_01), dx1_00);  dx2_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_00, x0_01), dx2_11);
                    dxi = _mm256_mul_ps(W[50], grad);  dx0_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x1_00), dx0_10);  dx1_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x0_10), dx1_00);  dx2_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_00, x0_10), dx2_11);
                    dxi = _mm256_mul_ps(W[51], grad);  dx0_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x1_00), dx0_11);  dx1_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x0_11), dx1_00);  dx2_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_00, x0_11), dx2_11);
                    dxi = _mm256_mul_ps(W[52], grad);  dx0_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x1_01), dx0_00);  dx1_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x0_00), dx1_01);  dx2_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_01, x0_00), dx2_11);
                    dxi = _mm256_mul_ps(W[53], grad);  dx0_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x1_01), dx0_01);  dx1_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x0_01), dx1_01);  dx2_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_01, x0_01), dx2_11);
                    dxi = _mm256_mul_ps(W[54], grad);  dx0_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x1_01), dx0_10);  dx1_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x0_10), dx1_01);  dx2_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_01, x0_10), dx2_11);
                    dxi = _mm256_mul_ps(W[55], grad);  dx0_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x1_01), dx0_11);  dx1_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x0_11), dx1_01);  dx2_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_01, x0_11), dx2_11);
                    dxi = _mm256_mul_ps(W[56], grad);  dx0_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x1_10), dx0_00);  dx1_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x0_00), dx1_10);  dx2_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_10, x0_00), dx2_11);
                    dxi = _mm256_mul_ps(W[57], grad);  dx0_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x1_10), dx0_01);  dx1_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x0_01), dx1_10);  dx2_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_10, x0_01), dx2_11);
                    dxi = _mm256_mul_ps(W[58], grad);  dx0_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x1_10), dx0_10);  dx1_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x0_10), dx1_10);  dx2_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_10, x0_10), dx2_11);
                    dxi = _mm256_mul_ps(W[59], grad);  dx0_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x1_10), dx0_11);  dx1_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x0_11), dx1_10);  dx2_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_10, x0_11), dx2_11);
                    dxi = _mm256_mul_ps(W[60], grad);  dx0_00 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x1_11), dx0_00);  dx1_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x0_00), dx1_11);  dx2_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_11, x0_00), dx2_11);
                    dxi = _mm256_mul_ps(W[61], grad);  dx0_01 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x1_11), dx0_01);  dx1_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x0_01), dx1_11);  dx2_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_11, x0_01), dx2_11);
                    dxi = _mm256_mul_ps(W[62], grad);  dx0_10 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x1_11), dx0_10);  dx1_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x0_10), dx1_11);  dx2_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_11, x0_10), dx2_11);
                    dxi = _mm256_mul_ps(W[63], grad);  dx0_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x1_11), dx0_11);  dx1_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x2_11, x0_11), dx1_11);  dx2_11 = _mm256_fmadd_ps(dxi, _mm256_mul_ps(x1_11, x0_11), dx2_11);
                    
                    __m256  dxn;
                    __m256  dxp;
                    dxn = _mm256_mul_ps  (dx0_00, xn[1]);
                    dxn = _mm256_fmadd_ps(dx0_10, xp[1], dxn);
                    dxp = _mm256_mul_ps  (dx0_01, xn[1]);
                    dxp = _mm256_fmadd_ps(dx0_11, xp[1], dxp);
                    _mm256_store_ps(&dx_addr[0][frame], _mm256_sub_ps(dxp, dxn));

                    dxn = _mm256_mul_ps  (dx0_00, xn[0]);
                    dxn = _mm256_fmadd_ps(dx0_01, xp[0], dxn);
                    dxp = _mm256_mul_ps  (dx0_10, xn[0]);
                    dxp = _mm256_fmadd_ps(dx0_11, xp[0], dxp);
                    _mm256_store_ps(&dx_addr[1][frame], _mm256_sub_ps(dxp, dxn));

                    dxn = _mm256_mul_ps  (dx1_00, xn[3]);
                    dxp = _mm256_mul_ps  (dx1_01, xn[3]);
                    dxn = _mm256_fmadd_ps(dx1_10, xp[3], dxn);
                    dxp = _mm256_fmadd_ps(dx1_11, xp[3], dxp);  
                    _mm256_store_ps(&dx_addr[2][frame], _mm256_sub_ps(dxp, dxn));

                    dxn = _mm256_mul_ps  (dx1_00, xn[2]);
                    dxn = _mm256_fmadd_ps(dx1_01, xp[2], dxn);
                    dxp = _mm256_mul_ps  (dx1_10, xn[2]);
                    dxp = _mm256_fmadd_ps(dx1_11, xp[2], dxp);
                    _mm256_store_ps(&dx_addr[3][frame], _mm256_sub_ps(dxp, dxn));

                    dxn = _mm256_mul_ps  (dx2_00, xn[5]);     
                    dxp = _mm256_mul_ps  (dx2_01, xn[5]);     
                    dxn = _mm256_fmadd_ps(dx2_10, xp[5], dxn); 
                    dxp = _mm256_fmadd_ps(dx2_11, xp[5], dxp); 
                    _mm256_store_ps(&dx_addr[4][frame], _mm256_sub_ps(dxp, dxn));

                    dxn = _mm256_mul_ps  (dx2_00, xn[4]);
                    dxn = _mm256_fmadd_ps(dx2_01, xp[4], dxn);
                    dxp = _mm256_mul_ps  (dx2_10, xn[4]);
                    dxp = _mm256_fmadd_ps(dx2_11, xp[4], dxp);
                    _mm256_store_ps(&dx_addr[5][frame], _mm256_sub_ps(dxp, dxn));
                }
                
                // dW水平加算
                for ( int i = 0; i < 64; ++i) {
                    dW_ptr(node, i) += bb_mm256_cvtss_f32(bb_mm256_hsum_ps(dW[i]));
                }
            }

            #pragma omp parallel for
            for ( index_t frame = 0; frame < frame_size; frame += 8 ) {
                for ( index_t node = 0; node < node_size; ++node ) {
                    for ( int i = 0; i < 6; ++i) {
                        float       *dx_addr     = dx_ptr.GetAddr(input_index_ptr(node, i));
                        __m256 dx  = _mm256_load_ps(&dx_addr[frame]);
                        float const *dx_tmp_addr = dx_tmp_ptr.GetAddr(node*6 + i);
                        __m256 tmp = _mm256_load_ps(&dx_tmp_addr[frame]);
                        dx = _mm256_add_ps(dx, tmp);
                        _mm256_store_ps(&dx_addr[frame], dx);
                    }
                }
            }

            return m_dx_buf;
        }


        {
//          m_dW->FillZero();
            m_dx_buf.FillZero();

            auto frame_size = m_x_buf.GetFrameSize();
            auto x_ptr = m_x_buf.LockConst<T>();
            auto dy_ptr = dy_buf.LockConst<T>();
            auto dx_ptr = m_dx_buf.Lock<T>();
            auto input_index_ptr = m_input_index.LockConst();
            auto W_ptr  = lock_W_const();
            auto dW_ptr = lock_dW();
        
            for ( index_t node = 0; node < m_output_node_size; ++node ) {
                index_t in_idx[6];
                for ( int i = 0; i < 6; ++i) {
                    in_idx[i] = input_index_ptr(node, i);
                }
                T W[64];
                for ( int i = 0; i < 64; ++i) {
                    W[i] = W_ptr(node, i);
                    if ( m_lut_binarize ) {
                        W[i] = W[i] > (T)0.5 ? (T)1.0 : (T)0.0;
                    }
                }

                T dW[64]  = {0};
                for (index_t frame = 0; frame < frame_size; ++frame ) {
                    T   xp[6], xn[6];
                    for ( int i = 0; i < 6; ++i) {
                        xp[i] = x_ptr.Get(frame, in_idx[i]);
                        xp[i] = std::min((T)1.0, std::max((T)0.0, xp[i]));
//                      BB_ASSERT(xp[i] >= 0 && xp[i] <= 1.0f);
                        xn[i] = (T)1.0 - xp[i];
                    }

                    T x0_00 = xn[1] * xn[0];
                    T x0_01 = xn[1] * xp[0];
                    T x0_10 = xp[1] * xn[0];
                    T x0_11 = xp[1] * xp[0];
                    T x1_00 = xn[3] * xn[2];
                    T x1_01 = xn[3] * xp[2];
                    T x1_10 = xp[3] * xn[2];
                    T x1_11 = xp[3] * xp[2];
                    T x2_00 = xn[5] * xn[4];
                    T x2_01 = xn[5] * xp[4];
                    T x2_10 = xp[5] * xn[4];
                    T x2_11 = xp[5] * xp[4];

                    T xi[64];
                    xi[0]  = x2_00 * x1_00 * x0_00;
                    xi[1]  = x2_00 * x1_00 * x0_01;
                    xi[2]  = x2_00 * x1_00 * x0_10;
                    xi[3]  = x2_00 * x1_00 * x0_11;
                    xi[4]  = x2_00 * x1_01 * x0_00;
                    xi[5]  = x2_00 * x1_01 * x0_01;
                    xi[6]  = x2_00 * x1_01 * x0_10;
                    xi[7]  = x2_00 * x1_01 * x0_11;
                    xi[8]  = x2_00 * x1_10 * x0_00;
                    xi[9]  = x2_00 * x1_10 * x0_01;
                    xi[10] = x2_00 * x1_10 * x0_10;
                    xi[11] = x2_00 * x1_10 * x0_11;
                    xi[12] = x2_00 * x1_11 * x0_00;
                    xi[13] = x2_00 * x1_11 * x0_01;
                    xi[14] = x2_00 * x1_11 * x0_10;
                    xi[15] = x2_00 * x1_11 * x0_11;
                    xi[16] = x2_01 * x1_00 * x0_00;
                    xi[17] = x2_01 * x1_00 * x0_01;
                    xi[18] = x2_01 * x1_00 * x0_10;
                    xi[19] = x2_01 * x1_00 * x0_11;
                    xi[20] = x2_01 * x1_01 * x0_00;
                    xi[21] = x2_01 * x1_01 * x0_01;
                    xi[22] = x2_01 * x1_01 * x0_10;
                    xi[23] = x2_01 * x1_01 * x0_11;
                    xi[24] = x2_01 * x1_10 * x0_00;
                    xi[25] = x2_01 * x1_10 * x0_01;
                    xi[26] = x2_01 * x1_10 * x0_10;
                    xi[27] = x2_01 * x1_10 * x0_11;
                    xi[28] = x2_01 * x1_11 * x0_00;
                    xi[29] = x2_01 * x1_11 * x0_01;
                    xi[30] = x2_01 * x1_11 * x0_10;
                    xi[31] = x2_01 * x1_11 * x0_11;
                    xi[32] = x2_10 * x1_00 * x0_00;
                    xi[33] = x2_10 * x1_00 * x0_01;
                    xi[34] = x2_10 * x1_00 * x0_10;
                    xi[35] = x2_10 * x1_00 * x0_11;
                    xi[36] = x2_10 * x1_01 * x0_00;
                    xi[37] = x2_10 * x1_01 * x0_01;
                    xi[38] = x2_10 * x1_01 * x0_10;
                    xi[39] = x2_10 * x1_01 * x0_11;
                    xi[40] = x2_10 * x1_10 * x0_00;
                    xi[41] = x2_10 * x1_10 * x0_01;
                    xi[42] = x2_10 * x1_10 * x0_10;
                    xi[43] = x2_10 * x1_10 * x0_11;
                    xi[44] = x2_10 * x1_11 * x0_00;
                    xi[45] = x2_10 * x1_11 * x0_01;
                    xi[46] = x2_10 * x1_11 * x0_10;
                    xi[47] = x2_10 * x1_11 * x0_11;
                    xi[48] = x2_11 * x1_00 * x0_00;
                    xi[49] = x2_11 * x1_00 * x0_01;
                    xi[50] = x2_11 * x1_00 * x0_10;
                    xi[51] = x2_11 * x1_00 * x0_11;
                    xi[52] = x2_11 * x1_01 * x0_00;
                    xi[53] = x2_11 * x1_01 * x0_01;
                    xi[54] = x2_11 * x1_01 * x0_10;
                    xi[55] = x2_11 * x1_01 * x0_11;
                    xi[56] = x2_11 * x1_10 * x0_00;
                    xi[57] = x2_11 * x1_10 * x0_01;
                    xi[58] = x2_11 * x1_10 * x0_10;
                    xi[59] = x2_11 * x1_10 * x0_11;
                    xi[60] = x2_11 * x1_11 * x0_00;
                    xi[61] = x2_11 * x1_11 * x0_01;
                    xi[62] = x2_11 * x1_11 * x0_10;
                    xi[63] = x2_11 * x1_11 * x0_11;

                    T grad = dy_ptr.Get(frame, node);

                    T dxi[64];
                    for ( int i = 0; i < 64; ++i) {
                        dW[i]  += xi[i] * grad;
                        dxi[i]  = W[i]  * grad;
                    }

                    T dx0_00 = 0;
                    T dx0_01 = 0;
                    T dx0_10 = 0;
                    T dx0_11 = 0;
                    T dx1_00 = 0;
                    T dx1_01 = 0;
                    T dx1_10 = 0;
                    T dx1_11 = 0;
                    T dx2_00 = 0;
                    T dx2_01 = 0;
                    T dx2_10 = 0;
                    T dx2_11 = 0;
                    dx0_00 += dxi[0]  * x2_00 * x1_00;  dx1_00 += dxi[0]  * x2_00 * x0_00;  dx2_00 += dxi[0]  * x1_00 * x0_00;
                    dx0_01 += dxi[1]  * x2_00 * x1_00;  dx1_00 += dxi[1]  * x2_00 * x0_01;  dx2_00 += dxi[1]  * x1_00 * x0_01;
                    dx0_10 += dxi[2]  * x2_00 * x1_00;  dx1_00 += dxi[2]  * x2_00 * x0_10;  dx2_00 += dxi[2]  * x1_00 * x0_10;
                    dx0_11 += dxi[3]  * x2_00 * x1_00;  dx1_00 += dxi[3]  * x2_00 * x0_11;  dx2_00 += dxi[3]  * x1_00 * x0_11;
                    dx0_00 += dxi[4]  * x2_00 * x1_01;  dx1_01 += dxi[4]  * x2_00 * x0_00;  dx2_00 += dxi[4]  * x1_01 * x0_00;
                    dx0_01 += dxi[5]  * x2_00 * x1_01;  dx1_01 += dxi[5]  * x2_00 * x0_01;  dx2_00 += dxi[5]  * x1_01 * x0_01;
                    dx0_10 += dxi[6]  * x2_00 * x1_01;  dx1_01 += dxi[6]  * x2_00 * x0_10;  dx2_00 += dxi[6]  * x1_01 * x0_10;
                    dx0_11 += dxi[7]  * x2_00 * x1_01;  dx1_01 += dxi[7]  * x2_00 * x0_11;  dx2_00 += dxi[7]  * x1_01 * x0_11;
                    dx0_00 += dxi[8]  * x2_00 * x1_10;  dx1_10 += dxi[8]  * x2_00 * x0_00;  dx2_00 += dxi[8]  * x1_10 * x0_00;
                    dx0_01 += dxi[9]  * x2_00 * x1_10;  dx1_10 += dxi[9]  * x2_00 * x0_01;  dx2_00 += dxi[9]  * x1_10 * x0_01;
                    dx0_10 += dxi[10] * x2_00 * x1_10;  dx1_10 += dxi[10] * x2_00 * x0_10;  dx2_00 += dxi[10] * x1_10 * x0_10;
                    dx0_11 += dxi[11] * x2_00 * x1_10;  dx1_10 += dxi[11] * x2_00 * x0_11;  dx2_00 += dxi[11] * x1_10 * x0_11;
                    dx0_00 += dxi[12] * x2_00 * x1_11;  dx1_11 += dxi[12] * x2_00 * x0_00;  dx2_00 += dxi[12] * x1_11 * x0_00;
                    dx0_01 += dxi[13] * x2_00 * x1_11;  dx1_11 += dxi[13] * x2_00 * x0_01;  dx2_00 += dxi[13] * x1_11 * x0_01;
                    dx0_10 += dxi[14] * x2_00 * x1_11;  dx1_11 += dxi[14] * x2_00 * x0_10;  dx2_00 += dxi[14] * x1_11 * x0_10;
                    dx0_11 += dxi[15] * x2_00 * x1_11;  dx1_11 += dxi[15] * x2_00 * x0_11;  dx2_00 += dxi[15] * x1_11 * x0_11;
                    dx0_00 += dxi[16] * x2_01 * x1_00;  dx1_00 += dxi[16] * x2_01 * x0_00;  dx2_01 += dxi[16] * x1_00 * x0_00;
                    dx0_01 += dxi[17] * x2_01 * x1_00;  dx1_00 += dxi[17] * x2_01 * x0_01;  dx2_01 += dxi[17] * x1_00 * x0_01;
                    dx0_10 += dxi[18] * x2_01 * x1_00;  dx1_00 += dxi[18] * x2_01 * x0_10;  dx2_01 += dxi[18] * x1_00 * x0_10;
                    dx0_11 += dxi[19] * x2_01 * x1_00;  dx1_00 += dxi[19] * x2_01 * x0_11;  dx2_01 += dxi[19] * x1_00 * x0_11;
                    dx0_00 += dxi[20] * x2_01 * x1_01;  dx1_01 += dxi[20] * x2_01 * x0_00;  dx2_01 += dxi[20] * x1_01 * x0_00;
                    dx0_01 += dxi[21] * x2_01 * x1_01;  dx1_01 += dxi[21] * x2_01 * x0_01;  dx2_01 += dxi[21] * x1_01 * x0_01;
                    dx0_10 += dxi[22] * x2_01 * x1_01;  dx1_01 += dxi[22] * x2_01 * x0_10;  dx2_01 += dxi[22] * x1_01 * x0_10;
                    dx0_11 += dxi[23] * x2_01 * x1_01;  dx1_01 += dxi[23] * x2_01 * x0_11;  dx2_01 += dxi[23] * x1_01 * x0_11;
                    dx0_00 += dxi[24] * x2_01 * x1_10;  dx1_10 += dxi[24] * x2_01 * x0_00;  dx2_01 += dxi[24] * x1_10 * x0_00;
                    dx0_01 += dxi[25] * x2_01 * x1_10;  dx1_10 += dxi[25] * x2_01 * x0_01;  dx2_01 += dxi[25] * x1_10 * x0_01;
                    dx0_10 += dxi[26] * x2_01 * x1_10;  dx1_10 += dxi[26] * x2_01 * x0_10;  dx2_01 += dxi[26] * x1_10 * x0_10;
                    dx0_11 += dxi[27] * x2_01 * x1_10;  dx1_10 += dxi[27] * x2_01 * x0_11;  dx2_01 += dxi[27] * x1_10 * x0_11;
                    dx0_00 += dxi[28] * x2_01 * x1_11;  dx1_11 += dxi[28] * x2_01 * x0_00;  dx2_01 += dxi[28] * x1_11 * x0_00;
                    dx0_01 += dxi[29] * x2_01 * x1_11;  dx1_11 += dxi[29] * x2_01 * x0_01;  dx2_01 += dxi[29] * x1_11 * x0_01;
                    dx0_10 += dxi[30] * x2_01 * x1_11;  dx1_11 += dxi[30] * x2_01 * x0_10;  dx2_01 += dxi[30] * x1_11 * x0_10;
                    dx0_11 += dxi[31] * x2_01 * x1_11;  dx1_11 += dxi[31] * x2_01 * x0_11;  dx2_01 += dxi[31] * x1_11 * x0_11;
                    dx0_00 += dxi[32] * x2_10 * x1_00;  dx1_00 += dxi[32] * x2_10 * x0_00;  dx2_10 += dxi[32] * x1_00 * x0_00;
                    dx0_01 += dxi[33] * x2_10 * x1_00;  dx1_00 += dxi[33] * x2_10 * x0_01;  dx2_10 += dxi[33] * x1_00 * x0_01;
                    dx0_10 += dxi[34] * x2_10 * x1_00;  dx1_00 += dxi[34] * x2_10 * x0_10;  dx2_10 += dxi[34] * x1_00 * x0_10;
                    dx0_11 += dxi[35] * x2_10 * x1_00;  dx1_00 += dxi[35] * x2_10 * x0_11;  dx2_10 += dxi[35] * x1_00 * x0_11;
                    dx0_00 += dxi[36] * x2_10 * x1_01;  dx1_01 += dxi[36] * x2_10 * x0_00;  dx2_10 += dxi[36] * x1_01 * x0_00;
                    dx0_01 += dxi[37] * x2_10 * x1_01;  dx1_01 += dxi[37] * x2_10 * x0_01;  dx2_10 += dxi[37] * x1_01 * x0_01;
                    dx0_10 += dxi[38] * x2_10 * x1_01;  dx1_01 += dxi[38] * x2_10 * x0_10;  dx2_10 += dxi[38] * x1_01 * x0_10;
                    dx0_11 += dxi[39] * x2_10 * x1_01;  dx1_01 += dxi[39] * x2_10 * x0_11;  dx2_10 += dxi[39] * x1_01 * x0_11;
                    dx0_00 += dxi[40] * x2_10 * x1_10;  dx1_10 += dxi[40] * x2_10 * x0_00;  dx2_10 += dxi[40] * x1_10 * x0_00;
                    dx0_01 += dxi[41] * x2_10 * x1_10;  dx1_10 += dxi[41] * x2_10 * x0_01;  dx2_10 += dxi[41] * x1_10 * x0_01;
                    dx0_10 += dxi[42] * x2_10 * x1_10;  dx1_10 += dxi[42] * x2_10 * x0_10;  dx2_10 += dxi[42] * x1_10 * x0_10;
                    dx0_11 += dxi[43] * x2_10 * x1_10;  dx1_10 += dxi[43] * x2_10 * x0_11;  dx2_10 += dxi[43] * x1_10 * x0_11;
                    dx0_00 += dxi[44] * x2_10 * x1_11;  dx1_11 += dxi[44] * x2_10 * x0_00;  dx2_10 += dxi[44] * x1_11 * x0_00;
                    dx0_01 += dxi[45] * x2_10 * x1_11;  dx1_11 += dxi[45] * x2_10 * x0_01;  dx2_10 += dxi[45] * x1_11 * x0_01;
                    dx0_10 += dxi[46] * x2_10 * x1_11;  dx1_11 += dxi[46] * x2_10 * x0_10;  dx2_10 += dxi[46] * x1_11 * x0_10;
                    dx0_11 += dxi[47] * x2_10 * x1_11;  dx1_11 += dxi[47] * x2_10 * x0_11;  dx2_10 += dxi[47] * x1_11 * x0_11;
                    dx0_00 += dxi[48] * x2_11 * x1_00;  dx1_00 += dxi[48] * x2_11 * x0_00;  dx2_11 += dxi[48] * x1_00 * x0_00;
                    dx0_01 += dxi[49] * x2_11 * x1_00;  dx1_00 += dxi[49] * x2_11 * x0_01;  dx2_11 += dxi[49] * x1_00 * x0_01;
                    dx0_10 += dxi[50] * x2_11 * x1_00;  dx1_00 += dxi[50] * x2_11 * x0_10;  dx2_11 += dxi[50] * x1_00 * x0_10;
                    dx0_11 += dxi[51] * x2_11 * x1_00;  dx1_00 += dxi[51] * x2_11 * x0_11;  dx2_11 += dxi[51] * x1_00 * x0_11;
                    dx0_00 += dxi[52] * x2_11 * x1_01;  dx1_01 += dxi[52] * x2_11 * x0_00;  dx2_11 += dxi[52] * x1_01 * x0_00;
                    dx0_01 += dxi[53] * x2_11 * x1_01;  dx1_01 += dxi[53] * x2_11 * x0_01;  dx2_11 += dxi[53] * x1_01 * x0_01;
                    dx0_10 += dxi[54] * x2_11 * x1_01;  dx1_01 += dxi[54] * x2_11 * x0_10;  dx2_11 += dxi[54] * x1_01 * x0_10;
                    dx0_11 += dxi[55] * x2_11 * x1_01;  dx1_01 += dxi[55] * x2_11 * x0_11;  dx2_11 += dxi[55] * x1_01 * x0_11;
                    dx0_00 += dxi[56] * x2_11 * x1_10;  dx1_10 += dxi[56] * x2_11 * x0_00;  dx2_11 += dxi[56] * x1_10 * x0_00;
                    dx0_01 += dxi[57] * x2_11 * x1_10;  dx1_10 += dxi[57] * x2_11 * x0_01;  dx2_11 += dxi[57] * x1_10 * x0_01;
                    dx0_10 += dxi[58] * x2_11 * x1_10;  dx1_10 += dxi[58] * x2_11 * x0_10;  dx2_11 += dxi[58] * x1_10 * x0_10;
                    dx0_11 += dxi[59] * x2_11 * x1_10;  dx1_10 += dxi[59] * x2_11 * x0_11;  dx2_11 += dxi[59] * x1_10 * x0_11;
                    dx0_00 += dxi[60] * x2_11 * x1_11;  dx1_11 += dxi[60] * x2_11 * x0_00;  dx2_11 += dxi[60] * x1_11 * x0_00;
                    dx0_01 += dxi[61] * x2_11 * x1_11;  dx1_11 += dxi[61] * x2_11 * x0_01;  dx2_11 += dxi[61] * x1_11 * x0_01;
                    dx0_10 += dxi[62] * x2_11 * x1_11;  dx1_11 += dxi[62] * x2_11 * x0_10;  dx2_11 += dxi[62] * x1_11 * x0_10;
                    dx0_11 += dxi[63] * x2_11 * x1_11;  dx1_11 += dxi[63] * x2_11 * x0_11;  dx2_11 += dxi[63] * x1_11 * x0_11;


                    T dxn[6] = {0};
                    T dxp[6] = {0};
                    dxn[0] += dx0_00 * xn[1];     dxn[1] += dx0_00 * xn[0];
                    dxp[0] += dx0_01 * xn[1];     dxn[1] += dx0_01 * xp[0];
                    dxn[0] += dx0_10 * xp[1];     dxp[1] += dx0_10 * xn[0];
                    dxp[0] += dx0_11 * xp[1];     dxp[1] += dx0_11 * xp[0];
                    dxn[2] += dx1_00 * xn[3];     dxn[3] += dx1_00 * xn[2];
                    dxp[2] += dx1_01 * xn[3];     dxn[3] += dx1_01 * xp[2];
                    dxn[2] += dx1_10 * xp[3];     dxp[3] += dx1_10 * xn[2];
                    dxp[2] += dx1_11 * xp[3];     dxp[3] += dx1_11 * xp[2];
                    dxn[4] += dx2_00 * xn[5];     dxn[5] += dx2_00 * xn[4];
                    dxp[4] += dx2_01 * xn[5];     dxn[5] += dx2_01 * xp[4];
                    dxn[4] += dx2_10 * xp[5];     dxp[5] += dx2_10 * xn[4];
                    dxp[4] += dx2_11 * xp[5];     dxp[5] += dx2_11 * xp[4];

                    T dx_grad[6];
                    dx_grad[0] = (dxp[0] - dxn[0]);
                    dx_grad[1] = (dxp[1] - dxn[1]);
                    dx_grad[2] = (dxp[2] - dxn[2]);
                    dx_grad[3] = (dxp[3] - dxn[3]);
                    dx_grad[4] = (dxp[4] - dxn[4]);
                    dx_grad[5] = (dxp[5] - dxn[5]);
                    for ( int i = 0; i < 6; ++i) {
                        dx_ptr.Add(frame, in_idx[i], dx_grad[i]);
                    }
                }

                for ( int i = 0; i < 64; ++i) {
                    dW_ptr(node, i) = dW[i];
                }
            }

            return m_dx_buf;
        }
    }
};


}
