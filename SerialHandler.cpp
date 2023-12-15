#include "SerialHandler.h"

#ifdef WIN32

    #include <iostream>
    #include <Windows.h>

    SerialHandler::SerialHandler() {
        hSerial = INVALID_HANDLE_VALUE;
    }

    SerialHandler::~SerialHandler() {
        CloseComPort();
    }

	std::vector<std::string>* SerialHandler::GetAvailableComPorts() {
        rt.clear();
        
        std::string text;
        char lpTargetPath[1024]; // buffer to store the path of the COMPORTS
        bool gotPort = false; // in case the port is not found

        for (int i = 0; i < 255; i++) // checking ports from COM0 to COM255
        {
            std::string str = "COM" + std::to_string(i); // converting to COM0, COM1, COM2
            size_t test = QueryDosDeviceA(str.c_str(), lpTargetPath, 1024);

            // Test the return value and error if any
            if (test != 0) //QueryDosDevice returns zero if it didn't find an object
            {
                text.append(str.c_str());
                text.append(": ");
                text.append(lpTargetPath);
                rt.push_back(text);
                text.clear();
            }
        }
        
        return &rt;
	}
    
    
    bool SerialHandler::OpenSerialPort(const char* name) {

        hSerial = CreateFileA( name, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (hSerial == INVALID_HANDLE_VALUE) {
            return false;
        }
        DCB dcbSerialParams = { 0 };
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        if (!GetCommState(hSerial, &dcbSerialParams)) {
            return false;
        }
        dcbSerialParams.BaudRate    = CBR_9600;
        dcbSerialParams.ByteSize    = 8;
        dcbSerialParams.StopBits    = ONESTOPBIT;
        dcbSerialParams.Parity      = NOPARITY;

        if (!SetCommState(hSerial, &dcbSerialParams)) {
            return false;
        }

        COMMTIMEOUTS timeouts = { 0 };
        timeouts.ReadIntervalTimeout            = 50;
        timeouts.ReadTotalTimeoutConstant       = 50;
        timeouts.ReadTotalTimeoutMultiplier     = 10;
        timeouts.WriteTotalTimeoutConstant      = 50;
        timeouts.WriteTotalTimeoutMultiplier    = 10;

        if (!SetCommTimeouts(hSerial, &timeouts)) {
            return false;
        }

        return true;
    }

    DWORD total_read = 0;
    bool SerialHandler::ReadComPort(uint8_t* buffer, size_t size) {
        if (!ReadFile(hSerial, buffer, size, &total_read, NULL))
            return false;
        return true;
    }

    DWORD total_write = 0;
    bool SerialHandler::WriteComPort(uint8_t* buffer, size_t size) {
        if (!WriteFile(hSerial, buffer, size, &total_write, NULL))
            return false;
        return true;
    }

    void SerialHandler::CloseComPort() {
        if (hSerial != INVALID_HANDLE_VALUE)
            CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
    }


    const char* SerialHandler::ReadLastError() {
        char lastError[1024];
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            lastError,
            sizeof(lastError),
            NULL);
        LastError = lastError;
        return LastError.c_str();
    }
#endif //ifdef WIN32