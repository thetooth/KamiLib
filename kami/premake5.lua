project "kami"
    kind "StaticLib"
    language "C++"
    includedirs { "./" }
    files { "**.h", "**.hpp", "**.cpp" }

    if os.is("windows") then
        excludes { "unix.*" }
    end
    
    platforms { "x64" }

    configuration "Debug"
        defines { "DEBUG" }
        flags { "Symbols" }

    configuration "Release"
        defines { "NDEBUG" }
        optimize "On"