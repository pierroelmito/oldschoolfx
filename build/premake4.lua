
solution "oldschoolfx"

configurations { "debug", "release" }

configuration "debug"
	flags { "Symbols" };
configuration "release"
	flags { "OptimizeSpeed" };
configuration "*"

buildoptions { "-std=c++11", "-Wall", "-pedantic" }
links { "sfml-window", "sfml-system", "sfml-graphics" }

project "oldschoolfx"

language "C++"
kind "windowedapp"
files "../src/**.cpp"

