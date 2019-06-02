﻿// --------------------------------------------------------------------------
//  Binary Brain  -- binary neural net framework
//
//                                Copyright (C) 2018-2019 by Ryuji Fuchikami
//                                https://github.com/ryuz
//                                ryuji.fuchikami@nifty.com
// --------------------------------------------------------------------------



#pragma once

#include <cstdint>
#include <random>

#include "bb/SparseLayer.h"
#include "bb/Tensor.h"
#include "bb/StochasticOperation.h"


namespace bb {


template <int N = 6, typename BinType = Bit, typename RealType = float>
class SparseLutN : public SparseLayer
{
    using _super = SparseLayer;
    static int const NN = (1 << N);

protected:
    bool                    m_host_only    = false;
    bool                    m_lut_binarize = false;
    bool                    m_binary_mode  = true;

    RealType                m_unbinarize_bias = (RealType)0.25;

    index_t                 m_max_tmp_mem_size = 256 * 1024 * 1024;

    std::string             m_connection;

    indices_t               m_input_shape;
    indices_t               m_output_shape;

    FrameBuffer             m_x_buf;

    Tensor_<std::int32_t>   m_input_index;
    Tensor_<std::int32_t>   m_reverse_index;
    bool                    m_reverse_index_dirty = true;

    std::shared_ptr<Tensor> m_W;
    std::shared_ptr<Tensor> m_dW;

    RealType                m_momentum;

    RealType                m_gamma;
    RealType                m_beta;
    
    Tensor_<RealType>       m_mean;     // 平均値
    Tensor_<RealType>       m_rstd;     // 標準偏差の逆数

    Tensor_<RealType>       m_running_mean;
    Tensor_<RealType>       m_running_var;
    
    std::mt19937_64         m_mt;

public:
    struct create_t
    {
        indices_t       output_shape;               //< 出力形状
        std::string     connection;                 //< 結線ルール
        RealType        momentum  = (RealType)0.0;
        RealType        gamma     = (RealType)0.3;
        RealType        beta      = (RealType)0.5;
        std::uint64_t   seed      = 1;              //< 乱数シード
    };

protected:
    SparseLutN(create_t const &create)
    {
        BB_ASSERT(!create.output_shape.empty());

        m_output_shape = create.output_shape;
        m_connection   = create.connection;
        m_momentum     = create.momentum;
        m_gamma        = create.gamma;
        m_beta         = create.beta;

        m_mt.seed(create.seed);

        m_W  = std::make_shared<Tensor>();
        m_dW = std::make_shared<Tensor>();

        if ( DataType<BinType>::type == BB_TYPE_BIT ) {
            m_binary_mode = true;
        }
    }

    void CommandProc(std::vector<std::string> args)
    {
        // バイナリモード設定
        if ( DataType<BinType>::type != BB_TYPE_BIT ) {
            if ( args.size() == 2 && args[0] == "binary" )
            {
                m_binary_mode = EvalBool(args[1]);
            }
        }

        // LUTバイナライズ設定
        if ( args.size() == 2 && args[0] == "lut_binarize" )
        {
            m_lut_binarize = EvalBool(args[1]);
        }

        // HostOnlyモード設定
        if (args.size() == 2 && args[0] == "host_only")
        {
            m_host_only = EvalBool(args[1]);
        }
    }

public:
    ~SparseLutN() {}


    static std::shared_ptr<SparseLutN> Create(create_t const &create)
    {
        return std::shared_ptr<SparseLutN>(new SparseLutN(create));
    }

    static std::shared_ptr<SparseLutN> Create(indices_t const &output_shape, std::string connection = "", std::uint64_t seed = 1)
    {
        create_t create;
        create.output_shape = output_shape;
        create.connection   = connection;
        create.seed         = seed;
        return Create(create);
    }

    static std::shared_ptr<SparseLutN> Create(index_t output_node_size, std::string connection = "", std::uint64_t seed = 1)
    {
        create_t create;
        create.output_shape.resize(1);
        create.output_shape[0] = output_node_size;
        create.connection      = connection;
        create.seed            = seed;
        return Create(create);
    }

    std::string GetClassName(void) const { return "SparseLutN"; }


public:
    // Serialize
    void Save(std::ostream &os) const 
    {
        SaveIndices(os, m_input_shape);
        SaveIndices(os, m_output_shape);
        m_input_index.Save(os);
        m_W->Save(os);
        bb::SaveValue(os, m_momentum);
        bb::SaveValue(os, m_gamma);
        bb::SaveValue(os, m_beta);
        m_running_mean.Save(os);
        m_running_var.Save(os);
    }

    void Load(std::istream &is)
    {
        m_input_shape  = LoadIndices(is);
        m_output_shape = LoadIndices(is);
        m_input_index.Load(is);
        m_W->Load(is);
        bb::LoadValue(is, m_momentum);
        bb::LoadValue(is, m_gamma);
        bb::LoadValue(is, m_beta);
        m_running_mean.Load(is);
        m_running_var.Load(is);
    }


#ifdef BB_WITH_CEREAL
    template <class Archive>
    void save(Archive& archive, std::uint32_t const version) const
    {
        _super::save(archive, version);
        archive(cereal::make_nvp("input_shape",  m_input_shape));
        archive(cereal::make_nvp("output_shape", m_output_shape));
        archive(cereal::make_nvp("input_index",  m_input_index));
        archive(cereal::make_nvp("W",            *m_W));
        archive(cereal::make_nvp("gamma",        m_gamma));
        archive(cereal::make_nvp("beta",         m_beta));
        archive(cereal::make_nvp("running_mean", m_running_mean));
        archive(cereal::make_nvp("running_var",  m_running_var));
    }

    template <class Archive>
    void load(Archive& archive, std::uint32_t const version)
    {
        _super::load(archive, version);
        archive(cereal::make_nvp("input_shape",  m_input_shape));
        archive(cereal::make_nvp("output_shape", m_output_shape));
        archive(cereal::make_nvp("input_index",  m_input_index));
        archive(cereal::make_nvp("W",            *m_W));
        archive(cereal::make_nvp("gamma",        m_gamma));
        archive(cereal::make_nvp("beta",         m_beta));
        archive(cereal::make_nvp("running_mean", m_running_mean));
        archive(cereal::make_nvp("running_var",  m_running_var));
    }

    void Save(cereal::JSONOutputArchive& archive) const
    {
        archive(cereal::make_nvp("SparseLutN", *this));
    }

    void Load(cereal::JSONInputArchive& archive)
    {
        archive(cereal::make_nvp("SparseLutN", *this));
    }
#endif


    Tensor       &W(void)       { return *m_W; }
    Tensor const &W(void) const { return *m_W; }
    
    Tensor       &dW(void)       { return *m_dW; }
    Tensor const &dW(void) const { return *m_dW; }

    auto lock_InputIndex(void)             { return m_input_index.Lock(); }
    auto lock_InputIndex_const(void) const { return m_input_index.LockConst(); }

    auto lock_W(void)              { return m_W->Lock<RealType>(); }
    auto lock_W_const(void) const  { return m_W->LockConst<RealType>(); }
    auto lock_dW(void)             { return m_dW->Lock<RealType>(); }
    auto lock_dW_const(void) const { return m_dW->LockConst<RealType>(); }

    auto lock_mean(void)               { return m_running_mean.Lock(); }
    auto lock_mean_const(void)   const { return m_running_mean.LockConst(); }
    auto lock_var(void)                { return m_running_var.Lock(); }
    auto lock_var_const(void)    const { return m_running_var.LockConst(); }
    
    // debug
    auto lock_tmp_mean_const(void)   const { return m_mean.LockConst(); }
    auto lock_tmp_rstd_const(void)   const { return m_rstd.LockConst(); }

    index_t GetNodeInputSize(index_t node) const
    {
        return N;
    }

    void SetNodeInput(index_t node, index_t input_index, index_t input_node)
    {
        auto ptr = lock_InputIndex();
        ptr(node, input_index) = (std::int32_t)input_node;
        m_reverse_index_dirty = true;
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
        
        // 接続初期化
        auto output_node_size = GetShapeSize(m_output_shape);
        m_input_index.Resize(output_node_size, N);
        this->InitializeNodeInput(m_mt(), m_connection);
        m_reverse_index_dirty = true;

        // パラメータ初期化(結局初期値は何が良いのかまだよくわからない)
        m_W->Resize(DataType<RealType>::type, GetShapeSize(m_output_shape), NN);  m_W->InitNormalDistribution(0.5, 0.01, m_mt());
        m_dW->Resize(DataType<RealType>::type, GetShapeSize(m_output_shape), NN); m_dW->FillZero();

        m_mean.Resize(m_output_shape);
        m_rstd.Resize(m_output_shape);

        m_running_mean.Resize(m_output_shape); m_running_mean = (RealType)0.0;
        m_running_var.Resize(m_output_shape);  m_running_var  = (RealType)1.0;

        return m_output_shape;
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
    
    void        SetFrameBufferX(FrameBuffer x) { m_x_buf = x; }
    FrameBuffer GetFrameBufferX(void)          { return m_x_buf; }

    // ノード単位でのForward計算
    std::vector<double> ForwardNode(index_t node, std::vector<double> input_value) const
    {
        BB_ASSERT(input_value.size() == N);

        // パラメータクリップ
        m_W->Clamp((RealType)0.0, (RealType)1.0);

        auto W_ptr            = lock_W_const();
        auto running_mean_ptr = m_running_mean.LockConst();
        auto running_var_ptr  = m_running_var.LockConst();

        RealType W[(1 << N)];
        for ( int i = 0; i < (1 << N); ++i) {
            W[i] = W_ptr(node, i);
            if ( m_lut_binarize ) {
                W[i] = ((W[i] > (RealType)0.5) ? (RealType)1.0 : (RealType)0.0);
            }
        }

        RealType mean = running_mean_ptr[node];
        RealType var  = running_var_ptr[node];
        RealType rstd = (RealType)1.0 / std::sqrt(var);

        RealType x[N];
        for ( int i = 0; i < N; ++i) {
            x[i] = (RealType)input_value[i];
            if ( m_binary_mode ) {
                x[i] = (RealType)0.5 + ((x[i] > (RealType)0.5) ? +m_unbinarize_bias : -m_unbinarize_bias);
            }
            else {
                x[i] = std::min((RealType)1.0, std::max((RealType)0.0, x[i]));
            }
        }

        RealType y;
        StochasticOperation_Lut_Forward<RealType>(x, &y, W, N);

        y = (y - mean) * rstd;
        y = y * m_gamma + m_beta;

        if ( m_binary_mode ) {
            // binarize
            y = ((y > (RealType)0.5) ? (RealType)1.0 : (RealType)0.0);
        }
        else {
            // hard-tanh
            y = std::min(y, (RealType)1.0);
            y = std::max(y, (RealType)0.0);
        }

        std::vector<double> result;
        result.push_back((double)y);

        return result;
    }


    FrameBuffer Forward(FrameBuffer x_buf, bool train = true)
    {
        BB_ASSERT(x_buf.GetType() == DataType<BinType>::type);

        // SetInputShpaeされていなければ初回に設定
        if (x_buf.GetShape() != m_input_shape) {
            SetInputShape(x_buf.GetShape());
        }

        // 出力を設定
        FrameBuffer y_buf(DataType<BinType>::type, x_buf.GetFrameSize(), m_output_shape);

        // backwardの為に保存
        if ( train ) {
            m_x_buf = x_buf;
        }

        // パラメータクリップ
        m_W->Clamp((RealType)0.0, (RealType)1.0);


#ifdef BB_WITH_CUDA
        // CUDA float
        if ( N == 6 && DataType<BinType>::type == BB_TYPE_FP32 && DataType<RealType>::type == BB_TYPE_FP32 && !m_host_only
                && x_buf.IsDeviceAvailable() && y_buf.IsDeviceAvailable() && Manager::IsDeviceAvailable()) {
            if ( train ) {
                auto x_ptr            = x_buf.LockDeviceMemoryConst();
                auto y_ptr            = y_buf.LockDeviceMemory(true);
                auto input_index_ptr  = m_input_index.LockDeviceMemoryConst();
                auto W_ptr            = m_W->LockDeviceMemoryConst();
                auto mean_ptr         = m_mean.LockDeviceMemory(true);
                auto rstd_ptr         = m_rstd.LockDeviceMemory(true);
                auto running_mean_ptr = m_running_mean.LockDeviceMemory();
                auto running_var_ptr  = m_running_var.LockDeviceMemory();

                bbcu_fp32_SparseLut6_ForwardTraining
                    (
                        (float const *)x_ptr.GetAddr(),
                        (float       *)y_ptr.GetAddr(),
                        (int   const *)input_index_ptr.GetAddr(),
                        (float const *)W_ptr.GetAddr(),
                        (float       *)mean_ptr.GetAddr(),
                        (float       *)rstd_ptr.GetAddr(),
                        (float       *)running_mean_ptr.GetAddr(),
                        (float       *)running_var_ptr.GetAddr(),
                        (float        )m_gamma,
                        (float        )m_beta,
                        (float        )m_momentum,
                        (float        )m_unbinarize_bias, 
                        (int          )y_buf.GetNodeSize(),
                        (int          )y_buf.GetFrameSize(),
                        (int          )(y_buf.GetFrameStride() / sizeof(float)),
                        (int          )(m_lut_binarize ? 1 : 0),
                        (int          )(m_binary_mode ? 1 : 0)
                    );
            }
            else {
                auto x_ptr            = x_buf.LockDeviceMemoryConst();
                auto y_ptr            = y_buf.LockDeviceMemory(true);
                auto input_index_ptr  = m_input_index.LockDeviceMemoryConst();
                auto W_ptr            = m_W->LockDeviceMemoryConst();
                auto running_mean_ptr = m_running_mean.LockDeviceMemory();
                auto running_var_ptr  = m_running_var.LockDeviceMemory();

                bbcu_fp32_SparseLut6_ForwardInference
                    (
                        (float const *)x_ptr.GetAddr(),
                        (float       *)y_ptr.GetAddr(),
                        (int   const *)input_index_ptr.GetAddr(),
                        (float const *)W_ptr.GetAddr(),
                        (float       *)running_mean_ptr.GetAddr(),
                        (float       *)running_var_ptr.GetAddr(),
                        (float        )m_gamma,
                        (float        )m_beta,
                        (float        )m_unbinarize_bias,
                        (int          )y_buf.GetNodeSize(),
                        (int          )y_buf.GetFrameSize(),
                        (int          )(y_buf.GetFrameStride() / sizeof(float)),
                        (int          )(m_lut_binarize ? 1 : 0),
                        (int          )(m_binary_mode  ? 1 : 0)
                    );
            }

            return y_buf;
        }

        // CUDA Bit
        if ( N == 6 && DataType<BinType>::type == BB_TYPE_BIT && DataType<RealType>::type == BB_TYPE_FP32 && !m_host_only
                && x_buf.IsDeviceAvailable() && y_buf.IsDeviceAvailable() && Manager::IsDeviceAvailable()) {
            if ( train ) {
                auto x_ptr            = x_buf.LockDeviceMemoryConst();
                auto y_ptr            = y_buf.LockDeviceMemory(true);
                auto input_index_ptr  = m_input_index.LockDeviceMemoryConst();
                auto W_ptr            = m_W->LockDeviceMemoryConst();
                auto mean_ptr         = m_mean.LockDeviceMemory(true);
                auto rstd_ptr         = m_rstd.LockDeviceMemory(true);
                auto running_mean_ptr = m_running_mean.LockDeviceMemory();
                auto running_var_ptr  = m_running_var.LockDeviceMemory();

                bbcu_bit_fp32_SparseLut6_ForwardTraining
                    (
                        (int   const *)x_ptr.GetAddr(),
                        (int         *)y_ptr.GetAddr(),
                        (int   const *)input_index_ptr.GetAddr(),
                        (float const *)W_ptr.GetAddr(),
                        (float       *)mean_ptr.GetAddr(),
                        (float       *)rstd_ptr.GetAddr(),
                        (float       *)running_mean_ptr.GetAddr(),
                        (float       *)running_var_ptr.GetAddr(),
                        (float        )m_gamma,
                        (float        )m_beta,
                        (float        )m_momentum,
                        (float        )m_unbinarize_bias, 
                        (int          )y_buf.GetNodeSize(),
                        (int          )y_buf.GetFrameSize(),
                        (int          )(y_buf.GetFrameStride() / sizeof(int)),
                        (int          )(m_lut_binarize ? 1 : 0)
                    );
            }
            else {
                auto x_ptr            = x_buf.LockDeviceMemoryConst();
                auto y_ptr            = y_buf.LockDeviceMemory(true);
                auto input_index_ptr  = m_input_index.LockDeviceMemoryConst();
                auto W_ptr            = m_W->LockDeviceMemoryConst();
                auto running_mean_ptr = m_running_mean.LockDeviceMemoryConst();
                auto running_var_ptr  = m_running_var.LockDeviceMemoryConst();

                bbcu_bit_fp32_SparseLut6_ForwardInference
                    (
                        (int   const *)x_ptr.GetAddr(),
                        (int         *)y_ptr.GetAddr(),
                        (int   const *)input_index_ptr.GetAddr(),
                        (float const *)W_ptr.GetAddr(),
                        (float const *)running_mean_ptr.GetAddr(),
                        (float const *)running_var_ptr.GetAddr(),
                        (float        )m_gamma,
                        (float        )m_beta,
                        (float        )m_unbinarize_bias,
                        (int          )y_buf.GetNodeSize(),
                        (int          )y_buf.GetFrameSize(),
                        (int          )(y_buf.GetFrameStride() / sizeof(int)),
                        (int          )(m_lut_binarize ? 1 : 0)
                    );
            }

            return y_buf;
        }
#endif

        {
            // Generic
            auto node_size  = y_buf.GetNodeSize();
            auto frame_size = y_buf.GetFrameSize();

            RealType reciprocal_frame_size = (RealType)1.0 / frame_size;

            if ( train ) {
                auto x_ptr            = x_buf.LockConst<BinType>();
                auto y_ptr            = y_buf.Lock<BinType>();
                auto input_index_ptr  = m_input_index.LockConst();
                auto W_ptr            = lock_W_const();
                auto mean_ptr         = m_mean.Lock(true);
                auto rstd_ptr         = m_rstd.Lock(true);
                auto running_mean_ptr = m_running_mean.Lock();
                auto running_var_ptr  = m_running_var.Lock();

                #pragma omp parallel for
                for ( index_t node = 0; node < node_size; ++node ) {
                    RealType W[(1 << N)];
                    for ( int i = 0; i < (1 << N); ++i) {
                        W[i] = W_ptr(node, i);
                        if ( m_lut_binarize ) {
                            W[i] = ((W[i] > (RealType)0.5) ? (RealType)1.0 : (RealType)0.0);
                        }
                    }
                    
                    // 平均と分散計測
                    RealType s1 = 0, c1 = 0, y1, t1;
                    RealType s2 = 0, c2 = 0, y2, t2;
                    for ( index_t frame = 0; frame < frame_size; ++frame ) {
                        RealType   x[N];
                        for ( int i = 0; i < N; ++i) {
                            x[i] = (RealType)x_ptr.Get(frame, input_index_ptr(node, i));
                            if ( m_binary_mode ) {
                                x[i] = (RealType)0.5 + ((x[i] > (RealType)0.5) ? +m_unbinarize_bias : -m_unbinarize_bias);
                            }
                            else {
                                x[i] = std::min((RealType)1.0, std::max((RealType)0.0, x[i]));
                            }
                        }

                        RealType y;
                        StochasticOperation_Lut_Forward<RealType>(x, &y, W, N);

                        // 集計
                        y1 = y - c1;
                        t1 = s1 + y1;
                        c1 = (t1 - s1) - y1;
                        s1 = t1;

                        y2 = (y * y) - c2;
                        t2 = s2 + y2;
                        c2 = (t2 - s2) - y2;
                        s2 = t2;
                    }

                    RealType mean = s1 * reciprocal_frame_size;
                    RealType var  = std::max(1.0e-5f, (s2 * reciprocal_frame_size) - (mean * mean));
                    RealType rstd = (RealType)1.0 / std::sqrt(var);

                    // 書き込み
                    running_mean_ptr[node] = running_mean_ptr[node] * m_momentum + mean * ((RealType)1.0 - m_momentum);
                    running_var_ptr[node]  = running_var_ptr[node]  * m_momentum + var  * ((RealType)1.0 - m_momentum);
                    mean_ptr[node] = mean;
                    rstd_ptr[node] = rstd;

                    // 正規化
                    for ( index_t frame = 0; frame < frame_size; ++frame ) {
                        // Forward計算
                        RealType x[N];
                        for ( int i = 0; i < N; ++i) {
                            x[i] = (RealType)x_ptr.Get(frame, input_index_ptr(node, i));
                            if ( m_binary_mode ) {
                                x[i] = (RealType)0.5 + ((x[i] > (RealType)0.5) ? +m_unbinarize_bias : -m_unbinarize_bias);
                            }
                            else {
                                x[i] = std::min((RealType)1.0, std::max((RealType)0.0, x[i]));
                            }
                        }

                        RealType y;
                        StochasticOperation_Lut_Forward<RealType>(x, &y, W, N);

                        y = (y - mean) * rstd;
                        y = y * m_gamma + m_beta;

                        if ( m_binary_mode ) {
                            // binarize
                            y = ((y > (RealType)0.5) ? (RealType)1.0 : (RealType)0.0);
                        }
                        else {
                            // hard-tanh
                            y = std::min(y, (RealType)1.0);
                            y = std::max(y, (RealType)0.0);
                        }

                        y_ptr.Set(frame, node, y);
                    }
                }
            }
            else {
                auto x_ptr            = x_buf.LockConst<BinType>();
                auto y_ptr            = y_buf.Lock<BinType>();
                auto input_index_ptr  = m_input_index.LockConst();
                auto W_ptr            = lock_W_const();
                auto running_mean_ptr = m_running_mean.LockConst();
                auto running_var_ptr  = m_running_var.LockConst();

                #pragma omp parallel for
                for ( index_t node = 0; node < node_size; ++node ) {
                    RealType W[(1 << N)];
                    for ( int i = 0; i < (1 << N); ++i) {
                        W[i] = W_ptr(node, i);
                        if ( m_lut_binarize ) {
                            W[i] = ((W[i] > (RealType)0.5) ? (RealType)1.0 : (RealType)0.0);
                        }
                    }
                    
                    RealType mean = running_mean_ptr[node];
                    RealType var  = running_var_ptr[node];
                    RealType rstd = (RealType)1.0 / std::sqrt(var);

                    // Forward計算
                    for ( index_t frame = 0; frame < frame_size; ++frame ) {
                        RealType x[N];
                        for ( int i = 0; i < N; ++i) {
                            x[i] = (RealType)x_ptr.Get(frame, input_index_ptr(node, i));
                            if ( m_binary_mode ) {
                                x[i] = (RealType)0.5 + ((x[i] > (RealType)0.5) ? +m_unbinarize_bias : -m_unbinarize_bias);
                            }
                            else {
                                x[i] = std::min((RealType)1.0, std::max((RealType)0.0, x[i]));
                            }
                        }

                        RealType y;
                        StochasticOperation_Lut_Forward<RealType>(x, &y, W, N);

                        y = (y - mean) * rstd;
                        y = y * m_gamma + m_beta;

                        if ( m_binary_mode ) {
                            // binarize
                            y = ((y > (RealType)0.5) ? (RealType)1.0 : (RealType)0.0);
                        }
                        else {
                            // hard-tanh
                            y = std::min(y, (RealType)1.0);
                            y = std::max(y, (RealType)0.0);
                        }

                        y_ptr.Set(frame, node, y);
                    }
                }
            }

            return y_buf;
        }
    }


    FrameBuffer Backward(FrameBuffer dy_buf)
    {
        BB_ASSERT(dy_buf.GetType() == DataType<RealType>::type);

        FrameBuffer x_buf = m_x_buf;
        m_x_buf = FrameBuffer();

        FrameBuffer dx_buf(DataType<RealType>::type, dy_buf.GetFrameSize(), m_input_shape);

        // make reverse index table
        if ( m_reverse_index_dirty ) {
            m_reverse_index = this->MakeReverseIndexTable(m_input_index, GetShapeSize(m_input_shape));
            m_reverse_index_dirty = false;
        }

        // tmp buffer
        index_t tmp_frame_size = m_max_tmp_mem_size / (sizeof(float) * GetShapeSize(m_output_shape)*N);
        tmp_frame_size = ((tmp_frame_size + 31) & ~0x1f);
        tmp_frame_size = std::min(tmp_frame_size, dy_buf.GetFrameSize());
        FrameBuffer tmp_buf(DataType<RealType>::type, tmp_frame_size, GetShapeSize(m_output_shape)*N);


#ifdef BB_WITH_CUDA
        // CUDA float
        if ( N == 6, DataType<BinType>::type == BB_TYPE_FP32 && DataType<RealType>::type == BB_TYPE_FP32 && !m_host_only
                && x_buf.IsDeviceAvailable() && dy_buf.IsDeviceAvailable() && tmp_buf.IsDeviceAvailable() && dx_buf.IsDeviceAvailable() && Manager::IsDeviceAvailable()) {

            Tensor_<RealType>   dmean(m_output_shape);
            Tensor_<RealType>   dvar(m_output_shape);

            auto x_ptr             = x_buf.LockDeviceMemoryConst();
            auto dy_ptr            = dy_buf.LockDeviceMemoryConst();
            auto dx_ptr            = dx_buf.LockDeviceMemory(true);
            auto tmp_ptr           = tmp_buf.LockDeviceMemory(true);
            auto input_index_ptr   = m_input_index.LockDeviceMemoryConst();
            auto reverse_index_ptr = m_reverse_index.LockDeviceMemoryConst();
            auto W_ptr             = m_W->LockDeviceMemoryConst();
            auto dW_ptr            = m_dW->LockDeviceMemory();
            auto mean_ptr          = m_mean.LockDeviceMemoryConst();
            auto rstd_ptr          = m_rstd.LockDeviceMemoryConst();
            auto dmean_ptr         = dmean.LockDeviceMemory(true);
            auto dvar_ptr          = dvar.LockDeviceMemory(true);
            
            bbcu_fp32_SparseLut6_Backward
                (
                    (float const *)x_ptr.GetAddr(),
                    (float const *)dy_ptr.GetAddr(),
                    (float       *)dx_ptr.GetAddr(),
                    (float       *)tmp_ptr.GetAddr(),
                    (int   const *)input_index_ptr.GetAddr(),
                    (int   const *)reverse_index_ptr.GetAddr(),
                    (float const *)W_ptr.GetAddr(),
                    (float       *)dW_ptr.GetAddr(),
                    (float const *)mean_ptr.GetAddr(),
                    (float const *)rstd_ptr.GetAddr(),
                    (float       *)dmean_ptr.GetAddr(),
                    (float       *)dvar_ptr.GetAddr(),
                    (float        )m_gamma,
                    (float        )m_beta,
                    (float        )m_unbinarize_bias,
                    (int          )m_reverse_index.GetShape()[0],
                    (int          )dx_buf.GetNodeSize(),
                    (int          )dy_buf.GetNodeSize(),
                    (int          )dy_buf.GetFrameSize(),
                    (int          )(dy_buf.GetFrameStride() / sizeof(float)),
                    (int          )tmp_buf.GetFrameSize(),
                    (int          )(tmp_buf.GetFrameStride() / sizeof(float)),
                    (int          )(m_lut_binarize ? 1 : 0),
                    (int          )(m_binary_mode  ? 1 : 0)
                );
            
            return dx_buf;
        }

        // CUDA bit
        if ( N == 6, DataType<BinType>::type == BB_TYPE_BIT && DataType<RealType>::type == BB_TYPE_FP32 && !m_host_only
                && x_buf.IsDeviceAvailable() && dy_buf.IsDeviceAvailable() && tmp_buf.IsDeviceAvailable() && dx_buf.IsDeviceAvailable() && Manager::IsDeviceAvailable()) {

            Tensor_<RealType>   dmean(m_output_shape);
            Tensor_<RealType>   dvar(m_output_shape);

            auto x_ptr             = x_buf.LockDeviceMemoryConst();
            auto dy_ptr            = dy_buf.LockDeviceMemoryConst();
            auto dx_ptr            = dx_buf.LockDeviceMemory(true);
            auto tmp_ptr           = tmp_buf.LockDeviceMemory(true);
            auto input_index_ptr   = m_input_index.LockDeviceMemoryConst();
            auto reverse_index_ptr = m_reverse_index.LockDeviceMemoryConst();
            auto W_ptr             = m_W->LockDeviceMemoryConst();
            auto dW_ptr            = m_dW->LockDeviceMemory();
            auto mean_ptr          = m_mean.LockDeviceMemoryConst();
            auto rstd_ptr          = m_rstd.LockDeviceMemoryConst();
            auto dmean_ptr         = dmean.LockDeviceMemory(true);
            auto dvar_ptr          = dvar.LockDeviceMemory(true);
            
            bbcu_bit_fp32_SparseLut6_Backward
                (
                    (int   const *)x_ptr.GetAddr(),
                    (float const *)dy_ptr.GetAddr(),
                    (float       *)dx_ptr.GetAddr(),
                    (float       *)tmp_ptr.GetAddr(),
                    (int   const *)input_index_ptr.GetAddr(),
                    (int   const *)reverse_index_ptr.GetAddr(),
                    (float const *)W_ptr.GetAddr(),
                    (float       *)dW_ptr.GetAddr(),
                    (float const *)mean_ptr.GetAddr(),
                    (float const *)rstd_ptr.GetAddr(),
                    (float       *)dmean_ptr.GetAddr(),
                    (float       *)dvar_ptr.GetAddr(),
                    (float        )m_gamma,
                    (float        )m_beta,
                    (float        )m_unbinarize_bias,
                    (int          )m_reverse_index.GetShape()[0],
                    (int          )dx_buf.GetNodeSize(),
                    (int          )dy_buf.GetNodeSize(),
                    (int          )dy_buf.GetFrameSize(),
                    (int          )(dy_buf.GetFrameStride() / sizeof(float)),
                    (int          )(x_buf.GetFrameStride() / sizeof(int)),
                    (int          )tmp_buf.GetFrameSize(),
                    (int          )(tmp_buf.GetFrameStride() / sizeof(int)),
                    (int          )m_lut_binarize
                );
            
            return dx_buf;
        }
#endif
        {
            dx_buf.FillZero();

            auto node_size  = dy_buf.GetNodeSize();
            auto frame_size = dy_buf.GetFrameSize();
            auto reciprocal_frame_size = (RealType)1.0 / (RealType)frame_size;

            auto x_ptr             = x_buf.LockConst<BinType>();
            auto dy_ptr            = dy_buf.LockConst<RealType>();
            auto dx_ptr            = dx_buf.Lock<RealType>(true);
            auto input_index_ptr   = m_input_index.LockConst();
            auto W_ptr             = lock_W_const();
            auto dW_ptr            = lock_dW();
            auto mean_ptr          = m_mean.LockConst();
            auto rstd_ptr          = m_rstd.LockConst();

            for ( index_t node = 0; node < node_size; ++node ) {
                RealType W[(1 << N)];
                for ( int i = 0; i < (1 << N); ++i) {
                    W[i] = W_ptr(node, i);
                    if ( m_lut_binarize ) {
                        W[i] = ((W[i] > (RealType)0.5) ? (RealType)1.0 : (RealType)0.0);
                    }
                }
                RealType dW[(1 << N)] = {0};
                
                // 平均分散の勾配計算
                RealType    mean   = mean_ptr[node];
                RealType    rstd   = rstd_ptr[node];
                RealType    rstd2  = rstd * rstd;
                RealType    dmeanx = 0;
                RealType    dstd   = 0;
                for ( index_t frame = 0; frame < frame_size; ++frame ) {
                    // x を再計算
                    RealType   x_vec[N];
                    for ( int i = 0; i < N; ++i) {
                        x_vec[i] = (RealType)x_ptr.Get(frame, input_index_ptr(node, i));
                        if ( m_binary_mode ) {
                            x_vec[i] = (RealType)0.5 + ((x_vec[i] > (RealType)0.5) ? +m_unbinarize_bias : -m_unbinarize_bias);
                        }
                        else {
                            x_vec[i] = std::min((RealType)1.0, std::max((RealType)0.0, x_vec[i]));
                        }
                    }
                    RealType x;
                    StochasticOperation_Lut_Forward<RealType>(x_vec, &x, W, N);

                    // hard-tanh の入力 x を求める
                    RealType tanh_x = ((x - mean) * rstd) * m_gamma + m_beta;

                    // hard-tanh
                    RealType   dy = dy_ptr.Get(frame, node);
                    if (tanh_x <= 0.0) { dy = 0.0; }
                    if (tanh_x >= 1.0) { dy = 0.0; }

                    // BatchNorm
                    RealType   xc = x - mean;
            //      RealType   xn = xc * rstd;
                    RealType   dxn = m_gamma * dy;

                    dstd   += -(dxn * xc * rstd2);
                    dmeanx += -(dxn * rstd);
                }
                RealType    dvar  = dstd * rstd;
                RealType    dmean = (dmeanx - (mean * dvar)) * reciprocal_frame_size;

                // 入力の勾配 dx を求める
                for ( index_t frame = 0; frame < frame_size; ++frame ) {
                    // x を再計算
                    RealType   x_vec[N];
                    for ( int i = 0; i < N; ++i) {
                        x_vec[i] = (RealType)x_ptr.Get(frame, input_index_ptr(node, i));
                        if ( m_binary_mode ) {
                            x_vec[i] = (RealType)0.5 + ((x_vec[i] > (RealType)0.5) ? +m_unbinarize_bias : -m_unbinarize_bias);
                        }
                        else {
                            x_vec[i] = std::min((RealType)1.0, std::max((RealType)0.0, x_vec[i]));
                        }
                    }
                    RealType x;
                    StochasticOperation_Lut_Forward<RealType>(x_vec, &x, W, N);

                    // hard-tanh の入力 x を求める
                    RealType tanh_x = ((x - mean) * rstd) * m_gamma + m_beta;

                    // hard-tanh
                    RealType   dy = dy_ptr.Get(frame, node);
                    if (tanh_x <= 0.0) { dy = 0.0; }
                    if (tanh_x >= 1.0) { dy = 0.0; }

                    RealType   dxn = dy * m_gamma;
                    RealType   dxc = dxn * rstd;
                    RealType   dx  = dxc + dmean + (x * dvar * reciprocal_frame_size);

                    RealType   dx_vec[N];
                    StochasticOperation_Lut_Backward<RealType>(x_vec, dx_vec, &dx, W, dW, N);

                    for ( int i = 0; i < N; ++i) {
                        dx_ptr.Add(frame, input_index_ptr(node, i), dx_vec[i]);
                    }
                }

                for ( int i = 0; i < (1 << N); ++i ) {
                    dW_ptr(node, i) += dW[i];
                }
            }

            return dx_buf;
        }

        BB_ASSERT(0);

        return dx_buf;
    }
};


}


// end of file