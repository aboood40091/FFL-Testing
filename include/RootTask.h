#include <Shader.h>

#include <gfx/rio_Camera.h>
#include <task/rio_Task.h>

class Model;

class RootTask : public rio::ITask
{
public:
    RootTask();

private:
    void prepare_() override;
    void calc_() override;
    void exit_() override;

private:
    bool                mInitialized;
    Shader              mShader;
    rio::BaseMtx44f     mProjMtx;
    rio::LookAtCamera   mCamera;
    f32                 mCounter;
    Model*              mpModel;
};
