cmake -B ..\..\wordlebot_vs2022 -S . ^
	-G "Visual Studio 17 2022" ^
	-A "x64" ^
	-DCMAKE_CONFIGURATION_TYPES="Debug;Release"