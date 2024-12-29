-- Premake5 file for zlib

project "zlibstatic"
    language    "C"
    kind        "StaticLib"
    defines     { "N_FSEEKO" }
    -- warnings    "off"
    staticruntime "off"

    targetdir ("%{wks.location}/Binaries/" .. OutputDir .. "/%{prj.name}")
    objdir ("%{wks.location}/Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

    files
    {
        "**.h",
        "adler32.c",
        "compress.c",
        "crc32.c",
        "deflate.c",
        "gzclose.c",
        "gzlib.c",
        "gzread.c",
        "gzwrite.c",
        "inflate.c",
        "infback.c",
        "inftrees.c",
        "inffast.c",
        "trees.c",
        "uncompr.c",
        "zutil.c"
    }
    
    includedirs
    {
        "./"
    }

    filter "system:windows"
        defines
        {
            "_WINDOWS",
            "_WIN32",
            "WIN32_LEAN_AND_MEAN",
            "_CRT_SECURE_NO_DEPRECATE",
            "_CRT_NONSTDC_NO_DEPRECATE"
        }

    filter "system:not windows"
        defines { 'HAVE_UNISTD_H' }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        runtime "Release"
        optimize "On"
        symbols "On"

    filter "configurations:Dist"
        runtime "Release"
        optimize "On"
        symbols "Off"