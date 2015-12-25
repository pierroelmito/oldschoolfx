
solution "oldschoolfx"

configurations { "debug", "release" }
buildoptions "-std=c++11"
links { "sfml-window", "sfml-system", "sfml-graphics" }

project "oldschoolfx"

language "C++"
kind "windowedapp"
files "../src/**.cpp"

