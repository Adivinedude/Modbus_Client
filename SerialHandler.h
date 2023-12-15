#ifndef SERIALHANDLER_H
#define SERIALHANDLER_H
	#include <vector>
	#include <string>

	class SerialHandler {
	public:
		SerialHandler();
		~SerialHandler();
		std::vector<std::string>* GetAvailableComPorts();
		std::vector<std::string>* GetList() { return &rt; };
		void ClearList() { rt.clear(); }
		bool OpenSerialPort(const char* name);
		bool ReadComPort(uint8_t* buffer, size_t size);
		bool WriteComPort(uint8_t* buffer, size_t size);
		void CloseComPort();
		const char* ReadLastError();

	private:
		std::vector<std::string> rt;
		void* hSerial;// = INVALID_HANDLE_VALUE;
		unsigned long total_read;
		unsigned long total_write;
		std::string LastError;

		//
	};
#endif