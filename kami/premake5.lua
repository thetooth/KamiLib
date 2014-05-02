project "kami"
    kind "StaticLib"
    language "C++"
    includedirs { "./" }
    files { "**.h", "**.hpp", "**.c", "**.cpp" }

    if os.is("windows") then
        excludes { "unix.*" }
    else
        excludes { "win32.*" }
    end
    
    platforms { "x64" }

    configuration "Debug"
        defines { "DEBUG" }
        flags { "Symbols" }

    configuration "Release"
        defines { "NDEBUG" }
        optimize "On"