#include <Model.h>
#include <RootTask.h>

#include <filedevice/rio_FileDeviceMgr.h>
#include <gfx/rio_Projection.h>
#include <gfx/rio_Window.h>

#include <string>

RootTask::RootTask()
    : ITask("RootTask")
    , mInitialized(false)
{
}

void RootTask::prepare_()
{
    mInitialized = false;

    FFLInitDesc init_desc;
    init_desc.fontRegion = FFL_FONT_REGION_0;
    init_desc._c = false;
    init_desc._10 = true;

    FFLResourceDesc res_desc;
    rio::MemUtil::set(&res_desc, 0, sizeof(FFLResourceDesc));
    {
        std::string resPath;
        resPath.resize(256);
        // Middle
        {
            FFLGetResourcePath(resPath.data(), 256, FFL_RESOURCE_TYPE_MIDDLE, false);
            {
                rio::FileDevice::LoadArg arg;
                arg.path = resPath;
                arg.alignment = 0x2000;

                u8* buffer = rio::FileDeviceMgr::instance()->getNativeFileDevice()->tryLoad(arg);
                if (buffer == nullptr)
                {
                    RIO_LOG("NativeFileDevice failed to load: %s\n", resPath.c_str());
                    RIO_ASSERT(false);
                    return;
                }

                res_desc.pData[FFL_RESOURCE_TYPE_MIDDLE] = buffer;
                res_desc.size[FFL_RESOURCE_TYPE_MIDDLE] = arg.read_size;
            }
        }
        // High
        {
            FFLGetResourcePath(resPath.data(), 256, FFL_RESOURCE_TYPE_HIGH, false);
            {
                rio::FileDevice::LoadArg arg;
                arg.path = resPath;
                arg.alignment = 0x2000;

                u8* buffer = rio::FileDeviceMgr::instance()->getNativeFileDevice()->tryLoad(arg);
                if (buffer == nullptr)
                {
                    RIO_LOG("NativeFileDevice failed to load: %s\n", resPath.c_str());
                    RIO_ASSERT(false);
                    return;
                }

                res_desc.pData[FFL_RESOURCE_TYPE_HIGH] = buffer;
                res_desc.size[FFL_RESOURCE_TYPE_HIGH] = arg.read_size;
            }
        }
    }

    FFLResult result = FFLInitResEx(&init_desc, &res_desc);
    if (result != FFL_RESULT_OK)
    {
        RIO_LOG("FFLInitResEx() failed with result: %d\n", (s32)result);
        RIO_ASSERT(false);
        return;
    }

    FFLiEnableSpecialMii(333326543);

    RIO_ASSERT(FFLIsAvailable());

    FFLInitResGPUStep();

    mShader.initialize();

    // Set projection matrix
    {
        // Get window instance
        const rio::Window* const window = rio::Window::instance();

        // Create perspective projection instance
        rio::PerspectiveProjection proj(
            0.1f,
            100.0f,
            rio::Mathf::deg2rad(45),
            f32(window->getWidth()) / f32(window->getHeight())
        );

        // Calculate matrix
        mProjMtx = proj.getMatrix();
    }

    FFLStoreData store_data;

    result = FFLiGetStoreData(&store_data, FFL_DATA_SOURCE_DEFAULT, 0);
    RIO_ASSERT(result == FFL_RESULT_OK);

    Model::InitArgStoreData arg = {
        .desc = {
            .resolution = FFLResolution(128 | FFL_RESOLUTION_MIP_MAP_ENABLE_MASK),
            .expressionFlag = 8,
            .modelFlag = 1 << 0 | 1 << 1 | 1 << 2,
            .resourceType = FFL_RESOURCE_TYPE_MIDDLE,
        },
        .data = &store_data
    };

    mpModel = new Model();
    mpModel->initialize(arg, mShader);
    mpModel->setScale({ 1 / 16.f, 1 / 16.f, 1 / 16.f });

    mCounter = 0.0f;

    mInitialized = true;
}

void RootTask::calc_()
{
    if (!mInitialized)
        return;

    rio::Window::instance()->clearColor(0.2f, 0.3f, 0.3f, 1.0f);
    rio::Window::instance()->clearDepthStencil();

    static const rio::Vector3f CENTER_POS = { 0.0f, 2.0f, -0.25f };

    mCamera.at() = CENTER_POS;

    // Move camera
    mCamera.pos().set(
        CENTER_POS.x + std::sin(mCounter) * 10,
        CENTER_POS.y,
        CENTER_POS.z + std::cos(mCounter) * 10
    );
    mCounter += 1.f / 60;

    // Get view matrix
    rio::BaseMtx34f view_mtx;
    mCamera.getMatrix(&view_mtx);

  //mpModel->enableSpecialDraw();

    mpModel->drawOpa(view_mtx, mProjMtx);
    mpModel->drawXlu(view_mtx, mProjMtx);
}

void RootTask::exit_()
{
    if (!mInitialized)
        return;

    delete mpModel; // FFLCharModel destruction must happen before FFLExit
    mpModel = nullptr;

    FFLExit();

    mInitialized = false;
}
