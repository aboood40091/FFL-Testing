#pragma once

#include <math/rio_MathTypes.h>

#include <nn/ffl.h>

class Shader;

class Model
{
public:
    template <typename T>
    struct InitArg
    {
        FFLCharModelDesc    desc;
        const T*            data;
        u16                 index;
    };

    typedef InitArg<FFLStoreData>   InitArgStoreData;
    typedef InitArg<FFLMiddleDB>    InitArgMiddleDB;

public:
    Model();
    ~Model();

    template <typename T>
    bool initialize(const InitArg<T>& arg, const Shader& shader);

    void enableSpecialDraw();

    void drawOpa(const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx);
    void drawXlu(const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx);

    const rio::BaseMtx34f& getMtxRT() const
    {
        return mMtxRT;
    }

    void setMtxRT(const rio::BaseMtx34f& mtx)
    {
        mMtxRT = mtx;
        updateMtxSRT_();
    }

    const rio::BaseVec3f& getScale() const
    {
        return mScale;
    }

    void setScale(const rio::BaseVec3f& scale)
    {
        mScale = scale;
        updateMtxSRT_();
    }

private:
    void updateMtxSRT_();

    void setViewUniform_(const rio::BaseMtx34f& model_mtx, const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx);

    void drawOpaNormal_();
    void drawOpaSpecial_();

    void drawXluNormal_();
    void drawXluSpecial_();

    void initialize_(const FFLCharModelDesc* p_desc);
    bool initializeCpu_();
    void initializeGpu_(const Shader& shader);

    bool setCharModelSource_(const FFLStoreData* p_store_data, u16);
    bool setCharModelSource_(const FFLMiddleDB* p_middle_db, u16 index);

private:
    FFLCharModel        mCharModel;
    FFLCharModelDesc    mCharModelDesc;
    FFLCharModelSource  mCharModelSource;
    rio::BaseMtx34f     mMtxRT;
    rio::BaseVec3f      mScale;
    rio::BaseMtx34f     mMtxSRT;
    const Shader*       mpShader;
    bool                mIsEnableSpecialDraw;
    bool                mIsInitialized;
};

template <typename T>
bool Model::initialize(const InitArg<T>& arg, const Shader& shader)
{
    RIO_ASSERT(mIsInitialized == false);

    initialize_(&arg.desc);

    if (!setCharModelSource_(arg.data, arg.index))
        return false;

    initializeGpu_(shader);

    mIsInitialized = true;
    return true;
}
