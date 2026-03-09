-- premake5.lua
local fflMlcPath = os.getenv("FFL_MLC_PATH")
if not fflMlcPath then
    error("Environment variable FFL_MLC_PATH is not set")
end

workspace "FFL-Testing"
    language "C++"
    cppdialect "C++20"
    staticruntime "on"
    warnings "Extra"
    platforms { "x86", "x64", "ARM64" }
    configurations { "Debug", "Release", "ReleaseDev" }

    stl "libc++"
    linkgroups "On"

    targetdir "./bin"
    objdir "build/%{cfg.toolset}"
    targetname "%{prj.name}-%{cfg.buildcfg}-%{cfg.architecture}"
    debugdir "./bin"

    startproject "FFL-Testing"

    defines {
        "GLEW_STATIC"
    }

    includedirs {
        "include",

        "lib/glew/include",
        "lib/glfw/include",
        "lib/rio/include",
        "lib/ninTexUtils/include",
        "lib/CafeSDKWrappers/cafe_win",
        "lib/ffl-win/include"
    }

    disablewarnings {
        "unused-parameter",
        "missing-field-initializers"
    }

    filter { "not toolset:gcc", "platforms:x86" }
        stl "gnu"

    filter { "toolset:gcc", "files:**.cpp" }
        disablewarnings {
            "invalid-offsetof",
            "class-memaccess"
        }

    filter "not toolset:gcc"
        disablewarnings {
            "unused-private-field",
            "missing-braces",
            "invalid-offsetof"
        }

    filter { "system:linux", "not toolset:gcc" }
        disablewarnings {
            "nontrivial-memaccess"
        }

    filter { "system:windows", "not toolset:gcc" }
        disablewarnings {
            "missing-designated-field-initializers",
            "nontrivial-memcall"
        }

    filter "platforms:x86"
        architecture "x86"

    filter "platforms:x64"
        architecture "x64"
        vectorextensions "AVX2"

    filter "platforms:ARM64"
        architecture "ARM64"

    filter "configurations:Debug"
        optimize "debug"
        omitframepointer "off"
        symbols "on"
        defines {
            "RIO_DEBUG",
            "_DEBUG"
        }

    filter "configurations:Release*"
        optimize "speed"
        omitframepointer "on"
        symbols "off"
        defines {
            "RIO_RELEASE"
        }

    filter { "configurations:Release*", "not toolset:gcc" }
        linktimeoptimization "On"

    filter "configurations:ReleaseDev"
        omitframepointer "off"
        symbols "on"

    filter { "configurations:Debug", "platforms:x64", "not toolset:gcc" }
        sanitize { "Address", "UndefinedBehavior" }

    filter "system:linux"
        systemversion "latest"

    filter "system:windows"
        systemversion "latest"

        defines {
            "NOMINMAX",
            "WIN32_LEAN_AND_MEAN",
            "_CRT_SECURE_NO_WARNINGS"
        }

project "Lib_GLEW"
    kind "StaticLib"
    language "C"
    warnings "off"

    files {
        "lib/glew/src/glew.c"
    }

project "Lib_GLFW"
    kind "StaticLib"
    language "C"
    warnings "off"

    files {
        "lib/glfw/include/GLFW/glfw3.h",
        "lib/glfw/include/GLFW/glfw3native.h",
        "lib/glfw/src/glfw_config.h",
        "lib/glfw/src/context.c",
        "lib/glfw/src/init.c",
        "lib/glfw/src/input.c",
        "lib/glfw/src/monitor.c",
        "lib/glfw/src/null_init.c",
        "lib/glfw/src/null_joystick.c",
        "lib/glfw/src/null_monitor.c",
        "lib/glfw/src/null_window.c",
        "lib/glfw/src/platform.c",
        "lib/glfw/src/vulkan.c",
        "lib/glfw/src/window.c"
    }

    filter "system:linux"
        files {
            "lib/glfw/src/x11_init.c",
            "lib/glfw/src/x11_monitor.c",
            "lib/glfw/src/x11_window.c",
            "lib/glfw/src/xkb_unicode.c",
            "lib/glfw/src/posix_module.c",
            "lib/glfw/src/posix_time.c",
            "lib/glfw/src/posix_thread.c",
            "lib/glfw/src/posix_module.c",
            "lib/glfw/src/posix_poll.c",
            "lib/glfw/src/glx_context.c",
            "lib/glfw/src/egl_context.c",
            "lib/glfw/src/osmesa_context.c",
            "lib/glfw/src/linux_joystick.c"
        }

        defines {
            "_GLFW_X11"
        }

    filter "system:windows"
        files {
            "lib/glfw/src/win32_init.c",
            "lib/glfw/src/win32_joystick.c",
            "lib/glfw/src/win32_module.c",
            "lib/glfw/src/win32_monitor.c",
            "lib/glfw/src/win32_time.c",
            "lib/glfw/src/win32_thread.c",
            "lib/glfw/src/win32_window.c",
            "lib/glfw/src/wgl_context.c",
            "lib/glfw/src/egl_context.c",
            "lib/glfw/src/osmesa_context.c"
        }

        defines {
            "_GLFW_WIN32"
        }

    filter "system:macosx"
        files {
            "lib/glfw/src/cocoa_init.m",
            "lib/glfw/src/cocoa_monitor.m",
            "lib/glfw/src/cocoa_window.m",
            "lib/glfw/src/cocoa_joystick.m",
            "lib/glfw/src/cocoa_time.c",
            "lib/glfw/src/nsgl_context.m",
            "lib/glfw/src/posix_thread.c",
            "lib/glfw/src/posix_module.c",
            "lib/glfw/src/osmesa_context.c",
            "lib/glfw/src/egl_context.c"
        }

        defines {
            "_GLFW_COCOA"
        }

project "Lib_BackwardCpp"
    kind "StaticLib"
    warnings "off"

    filter "system:linux"
        includedirs {
            "lib/backward-cpp"
        }

        files {
            "lib/backward-cpp/backward.cpp"
        }

project "Lib_RIO"
    kind "StaticLib"

    files {
        "lib/rio/src/**"
    }

project "Lib_ninTexUtils"
    kind "StaticLib"

    files {
        "lib/ninTexUtils/**"
    }

project "Lib_FFL"
    kind "StaticLib"

    files {
        "lib/ffl-win/src/**"
    }

    defines {
        "FFL_MLC_PATH=\"" .. fflMlcPath .. "\""
    }

project "FFL-Testing"
    files {
        "src/**"
    }

    links {
        "Lib_RIO",
        "Lib_ninTexUtils",
        "Lib_FFL",

        "Lib_GLEW",
        "Lib_GLFW"
    }

    filter "not toolset:gcc"
        disablewarnings {
            "invalid-source-encoding"
        }

    filter "system:linux"
        links {
            "Lib_BackwardCpp",
            "GL"
        }

        buildoptions { "`pkg-config --cflags dbus-1`" }
        linkoptions  { "`pkg-config --libs dbus-1`" }

    filter "system:windows"
        links {
            "opengl32",
            "gdi32",
            "z"  -- zlib
        }

    filter { "system:windows", "not action:vs*" }
        linkoptions { "-static" }

    filter "system:macosx"
        links {
            "Cocoa.framework",
            "IOKit.framework",
            "CoreVideo.framework",
            "OpenGL.framework",
            "QuartzCore.framework",
            "UniformTypeIdentifiers.framework"
        }

    filter "configurations:Debug"
        kind "ConsoleApp"

    filter "configurations:Release*"
        kind "WindowedApp"

    filter { "system:windows", "configurations:Release*" }
        entrypoint "mainCRTStartup"

    filter "action:gmake*"
        postbuildcommands {
            'cd "%{cfg.targetdir}" && rm -f *.a *.lib'
        }

    filter "action:vs*"
        postbuildcommands {
            'cd /d "%{cfg.targetdir}"',
            'del /q *.lib 2>nul',
            'del /q *.a 2>nul'
        }
