#include <Model.h>
#include <RootTask.h>

#include <filedevice/rio_FileDeviceMgr.h>
#include <gfx/rio_Projection.h>
#include <gfx/rio_Window.h>

#include <string>

//#define DECOMPILE_SHADER

#if RIO_IS_WIN && defined(DECOMPILE_SHADER)
#include <ShaderUtil.h>
#include <ninTexUtils/gfd/gfdStruct.h>
#endif // RIO_IS_WIN

RootTask::RootTask()
    : ITask("RootTask")
    , mInitialized(false)
{
}

void RootTask::prepare_()
{
    mInitialized = false;

#if RIO_IS_WIN && defined(DECOMPILE_SHADER)
    ShaderUtil::sTempPath                   = rio::FileDeviceMgr::instance()->getNativeFileDevice()->getCWD() + "/fs/content/shaders/cache";
    ShaderUtil::sGx2ShaderDecompilerPath    = rio::FileDeviceMgr::instance()->getNativeFileDevice()->getCWD() + "/fs/content/gx2shader-decompiler.exe";
    ShaderUtil::sSpirvCrossPath             = rio::FileDeviceMgr::instance()->getNativeFileDevice()->getCWD() + "/fs/content/spirv-cross.exe";

    GFDFile gfd;
    {
        rio::FileDevice::LoadArg arg;
        arg.path = "shaders/FFLShader.gsh";
        u8* bruh = rio::FileDeviceMgr::instance()->load(arg);

        [[maybe_unused]] size_t size = gfd.load(bruh);
        RIO_ASSERT(size == arg.read_size);

        rio::FileDeviceMgr::unload(bruh);
    }

    RIO_LOG("Vertex shader:\n");
    {
        const GX2VertexShader& shader = gfd.mVertexShaders[0];
        RIO_LOG("  - Shader Mode: %d\n", s32(shader.shaderMode));
        if (shader.numUniformBlocks)
        {
            RIO_LOG("  - Uniform blocks:\n");
            for (u32 i = 0; i < shader.numUniformBlocks; i++)
            {
                const GX2UniformBlock& ub = shader.uniformBlocks[i];
                RIO_LOG("    - Name: %s\n", ub.name);
                RIO_LOG("      Location: %u\n", ub.location);
                RIO_LOG("      Size: %u\n", ub.size);
            }
        }
        if (shader.numUniforms)
        {
            RIO_LOG("  - Uniforms:\n");
            for (u32 i = 0; i < shader.numUniforms; i++)
            {
                const GX2UniformVar& ur = shader.uniformVars[i];
                RIO_LOG("    - Name: %s\n", ur.name);
                RIO_LOG("      Uniform Type: %d\n", s32(ur.type));
                RIO_LOG("      Array count: %u\n", ur.arrayCount);
                RIO_LOG("      Offset: %u\n", ur.offset);
                RIO_LOG("      Block index: %d\n", s32(ur.blockIndex));
            }
        }
        if (shader.numSamplers)
        {
            RIO_LOG("  - Samplers:\n");
            for (u32 i = 0; i < shader.numSamplers; i++)
            {
                const GX2SamplerVar& sampler = shader.samplerVars[i];
                RIO_LOG("    - Name: %s\n", sampler.name);
                RIO_LOG("      Sampler Type: %d\n", s32(sampler.type));
                RIO_LOG("      Location: %u\n", sampler.location);
            }
        }
        if (shader.numAttribs)
        {
            RIO_LOG("  - Attributes:\n");
            for (u32 i = 0; i < shader.numAttribs; i++)
            {
                const GX2AttribVar& attrib = shader.attribVars[i];
                RIO_LOG("    - Name: %s\n", attrib.name);
                RIO_LOG("      Attrib Type: %d\n", s32(attrib.type));
                RIO_LOG("      Array count: %u\n", attrib.arrayCount);
                RIO_LOG("      Location: %u\n", attrib.location);
            }
        }
    }

    RIO_LOG("Pixel shader:\n");
    {
        const GX2PixelShader& shader = gfd.mPixelShaders[0];
        RIO_LOG("  - Shader Mode: %d\n", s32(shader.shaderMode));
        if (shader.numUniformBlocks)
        {
            RIO_LOG("  - Uniform blocks:\n");
            for (u32 i = 0; i < shader.numUniformBlocks; i++)
            {
                const GX2UniformBlock& ub = shader.uniformBlocks[i];
                RIO_LOG("    - Name: %s\n", ub.name);
                RIO_LOG("      Location: %u\n", ub.location);
                RIO_LOG("      Size: %u\n", ub.size);
            }
        }
        if (shader.numUniforms)
        {
            RIO_LOG("  - Uniforms:\n");
            for (u32 i = 0; i < shader.numUniforms; i++)
            {
                const GX2UniformVar& ur = shader.uniformVars[i];
                RIO_LOG("    - Name: %s\n", ur.name);
                RIO_LOG("      Uniform Type: %d\n", s32(ur.type));
                RIO_LOG("      Array count: %u\n", ur.arrayCount);
                RIO_LOG("      Offset: %u\n", ur.offset);
                RIO_LOG("      Block index: %d\n", s32(ur.blockIndex));
            }
        }
        if (shader.numSamplers)
        {
            RIO_LOG("  - Samplers:\n");
            for (u32 i = 0; i < shader.numSamplers; i++)
            {
                const GX2SamplerVar& sampler = shader.samplerVars[i];
                RIO_LOG("    - Name: %s\n", sampler.name);
                RIO_LOG("      Sampler Type: %d\n", s32(sampler.type));
                RIO_LOG("      Location: %u\n", sampler.location);
            }
        }
    }

    {
        [[maybe_unused]] bool success = ShaderUtil::decompileGsh(gfd.mVertexShaders[0], gfd.mPixelShaders[0], "shaders/FFLShader.vert", "shaders/FFLShader.frag");
        RIO_ASSERT(success);
    }

    RIO_LOG("RootTask::prepare_(): FFLShader decompiled\n");
#endif // RIO_IS_WIN

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
            .resolution = FFLResolution(/* 128 | FFL_RESOLUTION_MIP_MAP_ENABLE_MASK */ 2048),
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
