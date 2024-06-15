#include <misc/rio_Types.h>

#if RIO_IS_WIN

#include <MD5.h>
#include <ShaderUtil.h>

#include <filedevice/rio_FileDeviceMgr.h>

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
    #define NOMINMAX
#endif // NOMINMAX
#include <windows.h>

#include <array>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

#ifdef RIO_DEBUG

#define RIO_SHADER_DEBUG_LOG_ENABLE 0

#if RIO_SHADER_DEBUG_LOG_ENABLE
#define RIO_SHADER_DEBUG_LOG RIO_LOG
#else
#define RIO_SHADER_DEBUG_LOG(...)
#endif // RIO_SHADER_DEBUG_LOG_ENABLE

#else

#define RIO_SHADER_DEBUG_LOG(...)

#endif // RIO_DEBUG

namespace {

bool FileExists(const char* path)
{
    DWORD dwAttrib = GetFileAttributes(path);
    return dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}

bool FolderExists(const char* path)
{
    DWORD dwAttrib = GetFileAttributes(path);
    return dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}

void RunCommand(const char* cmd)
{
    STARTUPINFOA si = { sizeof(STARTUPINFOA), 0 };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = { 0 };

    if (CreateProcessA(NULL, const_cast<char*>(cmd), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}

bool WriteFile(const char* filename, const u8* data, u32 size)
{
    if (!data || !size)
        return false;

    std::ofstream outf(filename, std::ofstream::out | std::ofstream::binary);

    outf.write((char*)data, size);
    outf.close();

    return true;
}

bool ReadFile(const std::string& filename, std::string* out_str)
{
    if (!out_str)
        return false;

    std::ifstream inf(filename.c_str(), std::ifstream::in | std::ifstream::binary);

    inf.seekg(0, std::ios::end);
    const u32 inf_size = inf.tellg();
    inf.seekg(0, std::ios::beg);

    if (inf_size == 0xFFFFFFFF)
    {
        inf.close();
        return false;
    }

    u8* const inb = new u8[inf_size];
    if (!inb)
        return false;

    inf.read((char*)inb, inf_size);
    inf.close();

    *out_str = std::string((const char*)inb, inf_size);

    delete[] inb;

    return true;
}

bool WriteFile(const std::string& filename, const std::string& str)
{
    return WriteFile(filename.c_str(), (const u8*)str.c_str(), str.size() + 1);
}

size_t SaveGX2VertexShader(
    const GX2VertexShader* shader,
    void** pShaderBuf
)
{
    size_t shaderBufSize = sizeof(GX2VertexShader);

    const size_t uniformBlocksOffs = shaderBufSize;
    shaderBufSize += shader->numUniformBlocks * sizeof(GX2UniformBlock);

    const size_t uniformVarsOffs = shaderBufSize;
    shaderBufSize += shader->numUniforms * sizeof(GX2UniformVar);

    const size_t initialValuesOffs = shaderBufSize;
    shaderBufSize += shader->numInitialValues * sizeof(GX2UniformInitialValue);

    const size_t loopVarsOffs = shaderBufSize;
    shaderBufSize += shader->_numLoops * sizeof(u32) * 2;

    const size_t samplerVarsOffs = shaderBufSize;
    shaderBufSize += shader->numSamplers * sizeof(GX2SamplerVar);

    const size_t attribVarsOffs = shaderBufSize;
    shaderBufSize += shader->numAttribs * sizeof(GX2AttribVar);

    const size_t strBaseOffs = shaderBufSize;

    for (u32 i = 0; i < shader->numUniformBlocks; i++)
    {
        const char* const name = shader->uniformBlocks[i].name;
        if (name != NULL)
            shaderBufSize += std::strlen(name) + 1;
    }

    for (u32 i = 0; i < shader->numUniforms; i++)
    {
        const char* const name = shader->uniformVars[i].name;
        if (name != NULL)
            shaderBufSize += std::strlen(name) + 1;
    }

    for (u32 i = 0; i < shader->numSamplers; i++)
    {
        const char* const name = shader->samplerVars[i].name;
        if (name != NULL)
            shaderBufSize += std::strlen(name) + 1;
    }

    for (u32 i = 0; i < shader->numAttribs; i++)
    {
        const char* const name = shader->attribVars[i].name;
        if (name != NULL)
            shaderBufSize += std::strlen(name) + 1;
    }

    const size_t shaderDataOffs = shaderBufSize;
    shaderBufSize += shader->shaderSize;

    u8* shaderBuf = (u8*)rio::MemUtil::alloc(shaderBufSize, 4);

    std::memcpy(shaderBuf, shader, sizeof(GX2VertexShader));
    std::memcpy(shaderBuf + shaderDataOffs, shader->shaderPtr, shader->shaderSize);

    *(u32*)(shaderBuf + offsetof(GX2VertexShader, shaderPtr)) = shaderDataOffs;

    if (shader->numUniformBlocks != 0)
    {
        std::memcpy(shaderBuf + uniformBlocksOffs,
                    shader->uniformBlocks,
                    shader->numUniformBlocks * sizeof(GX2UniformBlock));
    }

    *(u32*)(shaderBuf + offsetof(GX2VertexShader, uniformBlocks)) = uniformBlocksOffs;

    if (shader->numUniforms != 0)
    {
        std::memcpy(shaderBuf + uniformVarsOffs,
                    shader->uniformVars,
                    shader->numUniforms * sizeof(GX2UniformVar));
    }

    *(u32*)(shaderBuf + offsetof(GX2VertexShader, uniformVars)) = uniformVarsOffs;

    if (shader->numInitialValues != 0)
    {
        std::memcpy(shaderBuf + initialValuesOffs,
                    shader->initialValues,
                    shader->numInitialValues * sizeof(GX2UniformInitialValue));
    }

    *(u32*)(shaderBuf + offsetof(GX2VertexShader, initialValues)) = initialValuesOffs;

    if (shader->_numLoops != 0)
    {
        std::memcpy(shaderBuf + loopVarsOffs,
                    shader->_loopVars,
                    shader->_numLoops * sizeof(u32) * 2);
    }

    *(u32*)(shaderBuf + offsetof(GX2VertexShader, _loopVars)) = loopVarsOffs;

    if (shader->numSamplers != 0)
    {
        std::memcpy(shaderBuf + samplerVarsOffs,
                    shader->samplerVars,
                    shader->numSamplers * sizeof(GX2SamplerVar));
    }

    *(u32*)(shaderBuf + offsetof(GX2VertexShader, samplerVars)) = samplerVarsOffs;

    if (shader->numAttribs != 0)
    {
        std::memcpy(shaderBuf + attribVarsOffs,
                    shader->attribVars,
                    shader->numAttribs * sizeof(GX2AttribVar));
    }

    *(u32*)(shaderBuf + offsetof(GX2VertexShader, attribVars)) = attribVarsOffs;

    size_t strOffs = strBaseOffs;

    for (u32 i = 0; i < shader->numUniformBlocks; i++)
    {
        const char* const name = shader->uniformBlocks[i].name;
        if (name != NULL)
        {
            const size_t nameLen = std::strlen(name) + 1;
            std::memcpy(shaderBuf + strOffs, name, nameLen);

            *(u32*)(shaderBuf +
                    uniformBlocksOffs
                    + i * sizeof(GX2UniformBlock)
                    + offsetof(GX2UniformBlock, name)) = strOffs;

            strOffs += nameLen;
        }
    }

    for (u32 i = 0; i < shader->numUniforms; i++)
    {
        const char* const name = shader->uniformVars[i].name;
        if (name != NULL)
        {
            const size_t nameLen = std::strlen(name) + 1;
            std::memcpy(shaderBuf + strOffs, name, nameLen);

            *(u32*)(shaderBuf +
                    uniformVarsOffs
                    + i * sizeof(GX2UniformVar)
                    + offsetof(GX2UniformVar, name)) = strOffs;

            strOffs += nameLen;
        }
    }

    for (u32 i = 0; i < shader->numSamplers; i++)
    {
        const char* const name = shader->samplerVars[i].name;
        if (name != NULL)
        {
            const size_t nameLen = std::strlen(name) + 1;
            std::memcpy(shaderBuf + strOffs, name, nameLen);

            *(u32*)(shaderBuf +
                    samplerVarsOffs
                    + i * sizeof(GX2SamplerVar)
                    + offsetof(GX2SamplerVar, name)) = strOffs;

            strOffs += nameLen;
        }
    }

    for (u32 i = 0; i < shader->numAttribs; i++)
    {
        const char* const name = shader->attribVars[i].name;
        if (name != NULL)
        {
            const size_t nameLen = std::strlen(name) + 1;
            std::memcpy(shaderBuf + strOffs, name, nameLen);

            *(u32*)(shaderBuf +
                    attribVarsOffs
                    + i * sizeof(GX2AttribVar)
                    + offsetof(GX2AttribVar, name)) = strOffs;

            strOffs += nameLen;
        }
    }

    *pShaderBuf = shaderBuf;
    return shaderBufSize;
}

size_t SaveGX2PixelShader(
    const GX2PixelShader* shader,
    void** pShaderBuf
)
{
    size_t shaderBufSize = sizeof(GX2PixelShader);

    const size_t uniformBlocksOffs = shaderBufSize;
    shaderBufSize += shader->numUniformBlocks * sizeof(GX2UniformBlock);

    const size_t uniformVarsOffs = shaderBufSize;
    shaderBufSize += shader->numUniforms * sizeof(GX2UniformVar);

    const size_t initialValuesOffs = shaderBufSize;
    shaderBufSize += shader->numInitialValues * sizeof(GX2UniformInitialValue);

    const size_t loopVarsOffs = shaderBufSize;
    shaderBufSize += shader->_numLoops * sizeof(u32) * 2;

    const size_t samplerVarsOffs = shaderBufSize;
    shaderBufSize += shader->numSamplers * sizeof(GX2SamplerVar);

    const size_t strBaseOffs = shaderBufSize;

    for (u32 i = 0; i < shader->numUniformBlocks; i++)
    {
        const char* const name = shader->uniformBlocks[i].name;
        if (name != NULL)
            shaderBufSize += std::strlen(name) + 1;
    }

    for (u32 i = 0; i < shader->numUniforms; i++)
    {
        const char* const name = shader->uniformVars[i].name;
        if (name != NULL)
            shaderBufSize += std::strlen(name) + 1;
    }

    for (u32 i = 0; i < shader->numSamplers; i++)
    {
        const char* const name = shader->samplerVars[i].name;
        if (name != NULL)
            shaderBufSize += std::strlen(name) + 1;
    }

    const size_t shaderDataOffs = shaderBufSize;
    shaderBufSize += shader->shaderSize;

    u8* shaderBuf = (u8*)rio::MemUtil::alloc(shaderBufSize, 4);

    std::memcpy(shaderBuf, shader, sizeof(GX2PixelShader));
    std::memcpy(shaderBuf + shaderDataOffs, shader->shaderPtr, shader->shaderSize);

    *(u32*)(shaderBuf + offsetof(GX2PixelShader, shaderPtr)) = shaderDataOffs;

    if (shader->numUniformBlocks != 0)
    {
        std::memcpy(shaderBuf + uniformBlocksOffs,
                    shader->uniformBlocks,
                    shader->numUniformBlocks * sizeof(GX2UniformBlock));
    }

    *(u32*)(shaderBuf + offsetof(GX2PixelShader, uniformBlocks)) = uniformBlocksOffs;

    if (shader->numUniforms != 0)
    {
        std::memcpy(shaderBuf + uniformVarsOffs,
                    shader->uniformVars,
                    shader->numUniforms * sizeof(GX2UniformVar));
    }

    *(u32*)(shaderBuf + offsetof(GX2PixelShader, uniformVars)) = uniformVarsOffs;

    if (shader->numInitialValues != 0)
    {
        std::memcpy(shaderBuf + initialValuesOffs,
                    shader->initialValues,
                    shader->numInitialValues * sizeof(GX2UniformInitialValue));
    }

    *(u32*)(shaderBuf + offsetof(GX2PixelShader, initialValues)) = initialValuesOffs;

    if (shader->_numLoops != 0)
    {
        std::memcpy(shaderBuf + loopVarsOffs,
                    shader->_loopVars,
                    shader->_numLoops * sizeof(u32) * 2);
    }

    *(u32*)(shaderBuf + offsetof(GX2PixelShader, _loopVars)) = loopVarsOffs;

    if (shader->numSamplers != 0)
    {
        std::memcpy(shaderBuf + samplerVarsOffs,
                    shader->samplerVars,
                    shader->numSamplers * sizeof(GX2SamplerVar));
    }

    *(u32*)(shaderBuf + offsetof(GX2PixelShader, samplerVars)) = samplerVarsOffs;

    size_t strOffs = strBaseOffs;

    for (u32 i = 0; i < shader->numUniformBlocks; i++)
    {
        const char* const name = shader->uniformBlocks[i].name;
        if (name != NULL)
        {
            const size_t nameLen = std::strlen(name) + 1;
            std::memcpy(shaderBuf + strOffs, name, nameLen);

            *(u32*)(shaderBuf +
                    uniformBlocksOffs
                    + i * sizeof(GX2UniformBlock)
                    + offsetof(GX2UniformBlock, name)) = strOffs;

            strOffs += nameLen;
        }
    }

    for (u32 i = 0; i < shader->numUniforms; i++)
    {
        const char* const name = shader->uniformVars[i].name;
        if (name != NULL)
        {
            const size_t nameLen = std::strlen(name) + 1;
            std::memcpy(shaderBuf + strOffs, name, nameLen);

            *(u32*)(shaderBuf +
                    uniformVarsOffs
                    + i * sizeof(GX2UniformVar)
                    + offsetof(GX2UniformVar, name)) = strOffs;

            strOffs += nameLen;
        }
    }

    for (u32 i = 0; i < shader->numSamplers; i++)
    {
        const char* const name = shader->samplerVars[i].name;
        if (name != NULL)
        {
            const size_t nameLen = std::strlen(name) + 1;
            std::memcpy(shaderBuf + strOffs, name, nameLen);

            *(u32*)(shaderBuf +
                    samplerVarsOffs
                    + i * sizeof(GX2SamplerVar)
                    + offsetof(GX2SamplerVar, name)) = strOffs;

            strOffs += nameLen;
        }
    }

    *pShaderBuf = shaderBuf;
    return shaderBufSize;
}

bool GX2UniformBlockComp(const GX2UniformBlock& a, const GX2UniformBlock& b)
{
    return a.location > b.location;
}

void ReplaceString(std::string& str, const std::string& a, const std::string& b)
{
    size_t pos = 0;
    while ((pos = str.find(a, pos)) != std::string::npos)
    {
         str.replace(pos, a.length(), b);
         pos += b.length();
    }
}

struct ShaderCache
{
    ShaderCache(const std::string& v, const std::string& f)
        : vertexShader(v)
        , fragmentShader(f)
    {
    }

    std::string vertexShader;
    std::string fragmentShader;
};
typedef std::unordered_map<std::string, const ShaderCache> ShaderCacheMap;
ShaderCacheMap sShaderCache;

bool DecompileProgram(
    const GX2VertexShader* vertexShader,
    const GX2PixelShader* pixelShader,
    std::string* vertexShaderSrc,
    std::string* fragmentShaderSrc
)
{
    void* vertexShaderBuf;
    size_t vertexShaderBufSize = SaveGX2VertexShader(vertexShader, &vertexShaderBuf);

    void* pixelShaderBuf;
    size_t pixelShaderBufSize = SaveGX2PixelShader(pixelShader, &pixelShaderBuf);

    u8* shaderBuf = new u8[vertexShaderBufSize + pixelShaderBufSize];
    std::memcpy(shaderBuf, vertexShaderBuf, vertexShaderBufSize);
    std::memcpy(shaderBuf + vertexShaderBufSize, pixelShaderBuf, pixelShaderBufSize);

    std::string key = md5(shaderBuf, vertexShaderBufSize + pixelShaderBufSize);

    std::string glVertexShader;
    std::string glFragmentShader;

    ShaderCacheMap::const_iterator it = sShaderCache.find(key);
    if (it != sShaderCache.end())
    {
        RIO_SHADER_DEBUG_LOG("  Loading cache from map\n");
        const ShaderCache& shaderCache = it->second;
        glVertexShader = shaderCache.vertexShader;
        glFragmentShader = shaderCache.fragmentShader;
    }
    else
    {
        const std::string& basePath = ShaderUtil::sTempPath + '/' + key;

        const std::string& vertexShaderPath = basePath + "VS";
        const std::string& fragmentShaderPath = basePath + "FS";

        const std::string& vertexShaderSrcPath = vertexShaderPath + ".vert";
        const std::string& fragmentShaderSrcPath = fragmentShaderPath + ".frag";

        if (FileExists(vertexShaderSrcPath.c_str()) && FileExists(fragmentShaderSrcPath.c_str()))
        {
            RIO_SHADER_DEBUG_LOG("  Loading cache from disk\n");
            {
                [[maybe_unused]] bool read = ReadFile(vertexShaderSrcPath, &glVertexShader);
                RIO_ASSERT(read);
            }
            {
                [[maybe_unused]] bool read = ReadFile(fragmentShaderSrcPath, &glFragmentShader);
                RIO_ASSERT(read);
            }
        }
        else
        {
            const std::string& vertexShaderSpirvPath = vertexShaderSrcPath + ".spv";
            const std::string& fragmentShaderSpirvPath = fragmentShaderSrcPath + ".spv";

            RIO_SHADER_DEBUG_LOG("\n");
            RIO_SHADER_DEBUG_LOG("%s\n", vertexShaderSpirvPath.c_str());
            RIO_SHADER_DEBUG_LOG("%s\n", fragmentShaderSpirvPath.c_str());

            WriteFile(vertexShaderPath.c_str(), (u8*)vertexShaderBuf, vertexShaderBufSize);
            WriteFile(fragmentShaderPath.c_str(), (u8*)pixelShaderBuf, pixelShaderBufSize);

            std::string cmd;
            {
                std::ostringstream cmdStrm;
                cmdStrm << "\"" << ShaderUtil::sGx2ShaderDecompilerPath << "\" -v \"" << vertexShaderPath << "\" -p \"" << fragmentShaderPath << "\"";
                cmd = cmdStrm.str();
            }
            RIO_SHADER_DEBUG_LOG("%s\n", cmd.c_str());
            RunCommand(cmd.c_str());

            DeleteFile(vertexShaderPath.c_str());
            DeleteFile(fragmentShaderPath.c_str());

            if (!FileExists(vertexShaderSpirvPath.c_str()))
                return false;

            if (!FileExists(fragmentShaderSpirvPath.c_str()))
                return false;

            {
                std::ostringstream cmdStrm;
                cmdStrm << "\"" << ShaderUtil::sSpirvCrossPath << "\" \"" << vertexShaderSpirvPath << "\" --no-es "  \
                    "--no-420pack-extension --no-support-nonzero-baseinstance " \
                    "--rename-interface-variable out 0 PARAM_0 " \
                    "--rename-interface-variable out 1 PARAM_1 " \
                    "--rename-interface-variable out 2 PARAM_2 " \
                    "--rename-interface-variable out 3 PARAM_3 " \
                    "--rename-interface-variable out 4 PARAM_4 " \
                    "--rename-interface-variable out 5 PARAM_5 " \
                    "--rename-interface-variable out 6 PARAM_6 " \
                    "--rename-interface-variable out 7 PARAM_7 " \
                    "--rename-interface-variable out 8 PARAM_8 " \
                    "--rename-interface-variable out 9 PARAM_9 " \
                    "--rename-interface-variable out 10 PARAM_10 " \
                    "--rename-interface-variable out 11 PARAM_11 " \
                    "--rename-interface-variable out 12 PARAM_12 " \
                    "--rename-interface-variable out 13 PARAM_13 " \
                    "--rename-interface-variable out 14 PARAM_14 " \
                    "--rename-interface-variable out 15 PARAM_15 " \
                    "--rename-interface-variable out 16 PARAM_16 " \
                    "--rename-interface-variable out 17 PARAM_17 " \
                    "--rename-interface-variable out 18 PARAM_18 " \
                    "--rename-interface-variable out 19 PARAM_19 " \
                    "--rename-interface-variable out 20 PARAM_20 " \
                    "--rename-interface-variable out 21 PARAM_21 " \
                    "--rename-interface-variable out 22 PARAM_22 " \
                    "--rename-interface-variable out 23 PARAM_23 " \
                    "--rename-interface-variable out 24 PARAM_24 " \
                    "--rename-interface-variable out 25 PARAM_25 " \
                    "--rename-interface-variable out 26 PARAM_26 " \
                    "--rename-interface-variable out 27 PARAM_27 " \
                    "--rename-interface-variable out 28 PARAM_28 " \
                    "--rename-interface-variable out 29 PARAM_29 " \
                    "--rename-interface-variable out 30 PARAM_30 " \
                    "--rename-interface-variable out 31 PARAM_31 " \
                    "--rename-interface-variable out 32 PARAM_32 " \
                    "--rename-interface-variable out 33 PARAM_33 " \
                    "--rename-interface-variable out 34 PARAM_34 " \
                    "--rename-interface-variable out 35 PARAM_35 " \
                    "--rename-interface-variable out 36 PARAM_36 " \
                    "--rename-interface-variable out 37 PARAM_37 " \
                    "--rename-interface-variable out 38 PARAM_38 " \
                    "--rename-interface-variable out 39 PARAM_39 " \
                    "--rename-interface-variable out 40 PARAM_40 " \
                    "--rename-interface-variable out 41 PARAM_41 " \
                    "--rename-interface-variable out 42 PARAM_42 " \
                    "--rename-interface-variable out 43 PARAM_43 " \
                    "--rename-interface-variable out 44 PARAM_44 " \
                    "--rename-interface-variable out 45 PARAM_45 " \
                    "--rename-interface-variable out 46 PARAM_46 " \
                    "--rename-interface-variable out 47 PARAM_47 " \
                    "--rename-interface-variable out 48 PARAM_48 " \
                    "--rename-interface-variable out 49 PARAM_49 " \
                    "--rename-interface-variable out 50 PARAM_50 " \
                    "--rename-interface-variable out 51 PARAM_51 " \
                    "--rename-interface-variable out 52 PARAM_52 " \
                    "--rename-interface-variable out 53 PARAM_53 " \
                    "--rename-interface-variable out 54 PARAM_54 " \
                    "--rename-interface-variable out 55 PARAM_55 " \
                    "--rename-interface-variable out 56 PARAM_56 " \
                    "--rename-interface-variable out 57 PARAM_57 " \
                    "--rename-interface-variable out 58 PARAM_58 " \
                    "--rename-interface-variable out 59 PARAM_59 " \
                    "--rename-interface-variable out 60 PARAM_60 " \
                    "--rename-interface-variable out 61 PARAM_61 " \
                    "--rename-interface-variable out 62 PARAM_62 " \
                    "--rename-interface-variable out 63 PARAM_63 " \
                    "--version 410 --output \"" << vertexShaderSrcPath << "\"";
                cmd = cmdStrm.str();
            }
            RIO_SHADER_DEBUG_LOG("%s\n", cmd.c_str());
            RunCommand(cmd.c_str());

            DeleteFile(vertexShaderSpirvPath.c_str());
            if (!FileExists(vertexShaderSrcPath.c_str()))
                return false;

            {
                std::ostringstream cmdStrm;
                cmdStrm << "\"" << ShaderUtil::sSpirvCrossPath << "\" \"" << fragmentShaderSpirvPath << "\" --no-es "  \
                    "--no-420pack-extension --no-support-nonzero-baseinstance " \
                    "--rename-interface-variable in 0 PARAM_0 " \
                    "--rename-interface-variable in 1 PARAM_1 " \
                    "--rename-interface-variable in 2 PARAM_2 " \
                    "--rename-interface-variable in 3 PARAM_3 " \
                    "--rename-interface-variable in 4 PARAM_4 " \
                    "--rename-interface-variable in 5 PARAM_5 " \
                    "--rename-interface-variable in 6 PARAM_6 " \
                    "--rename-interface-variable in 7 PARAM_7 " \
                    "--rename-interface-variable in 8 PARAM_8 " \
                    "--rename-interface-variable in 9 PARAM_9 " \
                    "--rename-interface-variable in 10 PARAM_10 " \
                    "--rename-interface-variable in 11 PARAM_11 " \
                    "--rename-interface-variable in 12 PARAM_12 " \
                    "--rename-interface-variable in 13 PARAM_13 " \
                    "--rename-interface-variable in 14 PARAM_14 " \
                    "--rename-interface-variable in 15 PARAM_15 " \
                    "--rename-interface-variable in 16 PARAM_16 " \
                    "--rename-interface-variable in 17 PARAM_17 " \
                    "--rename-interface-variable in 18 PARAM_18 " \
                    "--rename-interface-variable in 19 PARAM_19 " \
                    "--rename-interface-variable in 20 PARAM_20 " \
                    "--rename-interface-variable in 21 PARAM_21 " \
                    "--rename-interface-variable in 22 PARAM_22 " \
                    "--rename-interface-variable in 23 PARAM_23 " \
                    "--rename-interface-variable in 24 PARAM_24 " \
                    "--rename-interface-variable in 25 PARAM_25 " \
                    "--rename-interface-variable in 26 PARAM_26 " \
                    "--rename-interface-variable in 27 PARAM_27 " \
                    "--rename-interface-variable in 28 PARAM_28 " \
                    "--rename-interface-variable in 29 PARAM_29 " \
                    "--rename-interface-variable in 30 PARAM_30 " \
                    "--rename-interface-variable in 31 PARAM_31 " \
                    "--rename-interface-variable in 32 PARAM_32 " \
                    "--rename-interface-variable in 33 PARAM_33 " \
                    "--rename-interface-variable in 34 PARAM_34 " \
                    "--rename-interface-variable in 35 PARAM_35 " \
                    "--rename-interface-variable in 36 PARAM_36 " \
                    "--rename-interface-variable in 37 PARAM_37 " \
                    "--rename-interface-variable in 38 PARAM_38 " \
                    "--rename-interface-variable in 39 PARAM_39 " \
                    "--rename-interface-variable in 40 PARAM_40 " \
                    "--rename-interface-variable in 41 PARAM_41 " \
                    "--rename-interface-variable in 42 PARAM_42 " \
                    "--rename-interface-variable in 43 PARAM_43 " \
                    "--rename-interface-variable in 44 PARAM_44 " \
                    "--rename-interface-variable in 45 PARAM_45 " \
                    "--rename-interface-variable in 46 PARAM_46 " \
                    "--rename-interface-variable in 47 PARAM_47 " \
                    "--rename-interface-variable in 48 PARAM_48 " \
                    "--rename-interface-variable in 49 PARAM_49 " \
                    "--rename-interface-variable in 50 PARAM_50 " \
                    "--rename-interface-variable in 51 PARAM_51 " \
                    "--rename-interface-variable in 52 PARAM_52 " \
                    "--rename-interface-variable in 53 PARAM_53 " \
                    "--rename-interface-variable in 54 PARAM_54 " \
                    "--rename-interface-variable in 55 PARAM_55 " \
                    "--rename-interface-variable in 56 PARAM_56 " \
                    "--rename-interface-variable in 57 PARAM_57 " \
                    "--rename-interface-variable in 58 PARAM_58 " \
                    "--rename-interface-variable in 59 PARAM_59 " \
                    "--rename-interface-variable in 60 PARAM_60 " \
                    "--rename-interface-variable in 61 PARAM_61 " \
                    "--rename-interface-variable in 62 PARAM_62 " \
                    "--rename-interface-variable in 63 PARAM_63 " \
                    "--version 410 --output \"" << fragmentShaderSrcPath << "\"";
                cmd = cmdStrm.str();
            }
            RIO_SHADER_DEBUG_LOG("%s\n", cmd.c_str());
            RunCommand(cmd.c_str());

            DeleteFile(fragmentShaderSpirvPath.c_str());
            if (!FileExists(fragmentShaderSrcPath.c_str()))
                return false;

            {
                [[maybe_unused]] bool read = ReadFile(vertexShaderSrcPath, &glVertexShader);
                RIO_ASSERT(read);
            }
            DeleteFile(vertexShaderSrcPath.c_str());

            {
                [[maybe_unused]] bool read = ReadFile(fragmentShaderSrcPath, &glFragmentShader);
                RIO_ASSERT(read);
            }
            DeleteFile(fragmentShaderSrcPath.c_str());

            ReplaceString(glVertexShader, "\r\n", "\n");
            ReplaceString(glFragmentShader, "\r\n", "\n");

            RIO_ASSERT(vertexShader->shaderMode == GX2_SHADER_MODE_UNIFORM_REGISTERS || vertexShader->shaderMode == GX2_SHADER_MODE_UNIFORM_BLOCKS);

            RIO_SHADER_DEBUG_LOG("Vertex shader mode: %u\n", u32(vertexShader->shaderMode));

            if (vertexShader->shaderMode == GX2_SHADER_MODE_UNIFORM_REGISTERS)
            {
                const std::string& formatOldStr = "layout(std430) readonly buffer CFILE_DATA";
                const std::string& formatNewStr = "layout(std140) uniform VS_CFILE_DATA";

                ReplaceString(glVertexShader, formatOldStr, formatNewStr);
            }
            else
            {
                std::vector<GX2UniformBlock> vertexUBOs = std::vector<GX2UniformBlock>(vertexShader->uniformBlocks,
                                                                                       vertexShader->uniformBlocks + vertexShader->numUniformBlocks);

                std::sort(vertexUBOs.begin(), vertexUBOs.end(), GX2UniformBlockComp);

                for (u32 i = 0; i < vertexShader->numUniformBlocks; i++)
                {
                    std::ostringstream formatOldStrm;
                    formatOldStrm << "layout(std430) readonly buffer CBUFFER_DATA_" << vertexUBOs[i].location << std::endl
                                  << "{" << std::endl
                                  << "    vec4 values[];" << std::endl
                                  << "}";

                    std::ostringstream formatNewStrm;
                    formatNewStrm << "layout(std140) uniform " << vertexUBOs[i].name << std::endl
                                  << "{" << std::endl
                                  << "    vec4 values[" << ((vertexUBOs[i].size + 15) / 16) << "];" << std::endl
                                  << "}";

                    ReplaceString(glVertexShader, formatOldStrm.str(), formatNewStrm.str());
                }
            }

            RIO_ASSERT(pixelShader->shaderMode == GX2_SHADER_MODE_UNIFORM_REGISTERS || pixelShader->shaderMode == GX2_SHADER_MODE_UNIFORM_BLOCKS);

            RIO_SHADER_DEBUG_LOG("Pixel shader mode: %u\n", u32(pixelShader->shaderMode));

            if (pixelShader->shaderMode == GX2_SHADER_MODE_UNIFORM_REGISTERS)
            {
                const std::string& formatOldStr = "layout(std430) readonly buffer CFILE_DATA";
                const std::string& formatNewStr = "layout(std140) uniform PS_CFILE_DATA";

                ReplaceString(glFragmentShader, formatOldStr, formatNewStr);
            }
            else
            {
                std::vector<GX2UniformBlock> pixelUBOs = std::vector<GX2UniformBlock>(pixelShader->uniformBlocks,
                                                                                      pixelShader->uniformBlocks + pixelShader->numUniformBlocks);

                std::sort(pixelUBOs.begin(), pixelUBOs.end(), GX2UniformBlockComp);

                for (u32 i = 0; i < pixelShader->numUniformBlocks; i++)
                {
                    std::ostringstream formatOldStrm;
                    formatOldStrm << "layout(std430) readonly buffer CBUFFER_DATA_" << pixelUBOs[i].location << std::endl
                                  << "{" << std::endl
                                  << "    vec4 values[];" << std::endl
                                  << "}";

                    std::ostringstream formatNewStrm;
                    formatNewStrm << "layout(std140) uniform " << pixelUBOs[i].name << std::endl
                                  << "{" << std::endl
                                  << "    vec4 values[" << ((pixelUBOs[i].size + 15) / 16) << "];" << std::endl
                                  << "}";

                    ReplaceString(glFragmentShader, formatOldStrm.str(), formatNewStrm.str());
                }
            }

            for (u32 i = 0; i < vertexShader->numSamplers; i++)
            {
                const GX2SamplerVar& sampler = vertexShader->samplerVars[i];
                if (sampler.type != GX2_SAMPLER_TYPE_2D)
                    continue;

                // Remove dummy 2D samplers definitions
                {
                    std::ostringstream formatOldStrm;
                    formatOldStrm << "uniform sampler2D SPIRV_Cross_CombinedTEXTURE_"
                                  << sampler.location
                                  << "SPIRV_Cross_DummySampler;"
                                  << std::endl;

                    ReplaceString(glVertexShader, formatOldStrm.str(), "");
                }
                // Replace dummy 2D samplers
                {
                    std::ostringstream formatOldStrm;
                    formatOldStrm << "SPIRV_Cross_CombinedTEXTURE_"
                                  << sampler.location
                                  << "SPIRV_Cross_DummySampler";

                    ReplaceString(glVertexShader, formatOldStrm.str(), sampler.name);
                }
                // Replace 2D samplers
                {
                    std::ostringstream formatOldStrm;
                    formatOldStrm << "SPIRV_Cross_CombinedTEXTURE_"
                                  << sampler.location
                                  << "SAMPLER_"
                                  << sampler.location;

                    ReplaceString(glVertexShader, formatOldStrm.str(), sampler.name);
                }
            }

            for (u32 i = 0; i < pixelShader->numSamplers; i++)
            {
                const GX2SamplerVar& sampler = pixelShader->samplerVars[i];
                if (sampler.type != GX2_SAMPLER_TYPE_2D)
                    continue;

                // Remove dummy 2D samplers definitions
                {
                    std::ostringstream formatOldStrm;
                    formatOldStrm << "uniform sampler2D SPIRV_Cross_CombinedTEXTURE_"
                                  << sampler.location
                                  << "SPIRV_Cross_DummySampler;"
                                  << std::endl;

                    ReplaceString(glFragmentShader, formatOldStrm.str(), "");
                }
                // Replace dummy 2D samplers
                {
                    std::ostringstream formatOldStrm;
                    formatOldStrm << "SPIRV_Cross_CombinedTEXTURE_"
                                  << sampler.location
                                  << "SPIRV_Cross_DummySampler";

                    ReplaceString(glFragmentShader, formatOldStrm.str(), sampler.name);
                }
                // Replace 2D samplers
                {
                    std::ostringstream formatOldStrm;
                    formatOldStrm << "SPIRV_Cross_CombinedTEXTURE_"
                                  << sampler.location
                                  << "SAMPLER_"
                                  << sampler.location;

                    ReplaceString(glFragmentShader, formatOldStrm.str(), sampler.name);
                }
            }

            static const std::array<std::string, 5> qualifiers = {
                "noperspective centroid ",
                "noperspective ",
                "centroid ",
                "sample ",
                "flat "
            };

            for (u32 i = 0; i < 32; i++)
            {
                std::ostringstream layoutStrm;
                layoutStrm << "layout(location = " << i << ") ";
                const std::string& layoutStr = layoutStrm.str();

                std::ostringstream paramVsOldStrm;
                paramVsOldStrm << layoutStr << "out ";

                for (const std::string& qualifier : qualifiers)
                {
                    std::ostringstream paramFsStrm;
                    paramFsStrm << layoutStr << qualifier << "in ";

                    if (glFragmentShader.find(paramFsStrm.str()) != std::string::npos)
                    {
                        std::ostringstream paramVsNewStrm;
                        paramVsNewStrm << layoutStr << qualifier << "out ";

                        RIO_SHADER_DEBUG_LOG("Replacing \"%s\" with \"%s\"\n", paramVsOldStrm.str().c_str(), paramVsNewStrm.str().c_str());

                        ReplaceString(glVertexShader, paramVsOldStrm.str(), paramVsNewStrm.str());
                        break;
                    }
                }
            }

            ReplaceString(glFragmentShader, "#version 410", "#version 430");
            ReplaceString(glFragmentShader, "#extension GL_ARB_texture_query_levels : require\n", "");

            WriteFile(vertexShaderSrcPath, glVertexShader);
            WriteFile(fragmentShaderSrcPath, glFragmentShader);
        }

        sShaderCache.insert(ShaderCacheMap::value_type(key, ShaderCache(glVertexShader, glFragmentShader)));
    }

    rio::MemUtil::free(vertexShaderBuf);
    rio::MemUtil::free(pixelShaderBuf);
    delete[] shaderBuf;

    *vertexShaderSrc = glVertexShader;
    *fragmentShaderSrc = glFragmentShader;
    return true;
}

}

std::string ShaderUtil::sTempPath = "";
std::string ShaderUtil::sGx2ShaderDecompilerPath = "";
std::string ShaderUtil::sSpirvCrossPath = "";

bool ShaderUtil::decompileGsh(
    const GX2VertexShader& vertex_shader,
    const GX2PixelShader& pixel_shader,
    const std::string& out_vert_fname, const std::string& out_frag_fname
)
{
    if (sTempPath.empty() ||
        !FolderExists(sTempPath.c_str()))
    {
        return false;
    }

    if (sGx2ShaderDecompilerPath.empty() ||
        !FileExists(sGx2ShaderDecompilerPath.c_str()))
    {
        return false;
    }

    if (sSpirvCrossPath.empty() ||
        !FileExists(sSpirvCrossPath.c_str()))
    {
        return false;
    }

    if (out_vert_fname.empty() ||
        out_frag_fname.empty())
    {
        return false;
    }

    std::string out_vert_native_fname;
    std::string out_frag_native_fname;

    // Vertex
    {
        std::string out_vert_fname_no_drive;

        rio::FileDevice* device = rio::FileDeviceMgr::instance()->findDeviceFromPath(out_vert_fname, &out_vert_fname_no_drive);
        if (!device)
            return false;

        out_vert_native_fname = device->getNativePath(out_vert_fname_no_drive);
        RIO_SHADER_DEBUG_LOG("Vertex shader: %s\n", out_vert_native_fname.c_str());
        if (FileExists(out_vert_native_fname.c_str()))
            DeleteFile(out_vert_native_fname.c_str());
    }
    // Fragment
    {
        std::string out_frag_fname_no_drive;

        rio::FileDevice* device = rio::FileDeviceMgr::instance()->findDeviceFromPath(out_frag_fname, &out_frag_fname_no_drive);
        if (!device)
            return false;

        out_frag_native_fname = device->getNativePath(out_frag_fname_no_drive);
        RIO_SHADER_DEBUG_LOG("Fragment shader: %s\n", out_frag_native_fname.c_str());
        if (FileExists(out_frag_native_fname.c_str()))
            DeleteFile(out_frag_native_fname.c_str());
    }

    std::string vert_src;
    std::string frag_src;
    if (!DecompileProgram(
            &vertex_shader, &pixel_shader,
            &vert_src, &frag_src
    ))
        return false;

    if (!WriteFile(out_vert_native_fname, vert_src))
        return false;

    if (!WriteFile(out_frag_native_fname, frag_src))
        return false;

    return FileExists(out_vert_native_fname.c_str()) &&
           FileExists(out_frag_native_fname.c_str());
}

#endif // RIO_IS_WIN
