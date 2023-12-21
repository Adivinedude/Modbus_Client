This is a demo program for PCM_Modbus arduino based controller

This program uses open source code and is based on the tui.c example from PDCurses library.
Original authors credits are still contained with in tui.h, tui.cpp.
Many features have been added to this library, but it is fundimently the same menu based program.
My thinks to the origional author.
Current Build Instructions
```
For a PC Running windows 11 ( as of 2023/11 )

Step 1)	install microsoft visual studio 2022 from https://visualstudio.microsoft.com/downloads/

Step 2) Create a folder in your documents called "Github"

Step 3) Open visual studio > Clone a repository
		Repository Location: https://github.com/wmcbrine/PDCurses.git
		Path: C:\Users\<USERNAME>\Documents\Github\PDCurses
		Click "Clone".

Step 4) Click "File" > "Clone Repository"
		Repository Location: https://github.com/stephane/libmodbus.git
		Path: C:\Users\<USERNAME>\Documents\Github\libmodbus
		Click "Clone"

Step 5) Click "File" > "Clone Repository"
		Repository Location: https://github.com/Adivinedude/Modbus_Client.git
		Path: C:\Users\<USERNAME>\Documents\Github\ModbusClient


Step 6)	Click "Tools" > "Command Line" > "Developer Command Prompt"
		In the command window
			cd ..\PDCurses\wincon
			nmake -f Makefile.vc WIDE=Y
			del *.obj

			cd ..\..\libmodbus\src\win32
			cscript configure.js
			exit

Step 7) Click "File" > "Open" > "Open Project/Solution" 
	Navigate to the "libmodbus\src\win32" folder and open "modbus-9.sln"
	Click "yes" to any import prompts until the solution has loaded.
	Click "Build" > "Build Solution"
	Click "File" > "Close Solution"

Step 8) Click "File" > "Open" > "Open Project/Solution"
	Navigate to the "ModbusClient" folder and open "ModbusCLient.sln"

Step 9) Press F5 to build and launch the application
```
Special Notes.
To run the program outside of visual studio, the modbus.dll needs to be moved into the same folder as the executable.
