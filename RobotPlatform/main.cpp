#include <iostream>
#include <string>
#include <algorithm>
#include <windows.h>
#include <ctime>

HANDLE ComPort;

int RS232Get(char* buf, int bufSize)
{
	int n;
	ReadFile(ComPort, buf, bufSize, (LPDWORD)((void *)&n), NULL);
	return n;
}

int RS232Send(unsigned char byte)
{
	int n;
	WriteFile(ComPort, &byte, 1, (LPDWORD)((void *)&n), NULL);
	if (n < 0)
		return 1;
	return 0;
}

int RS232SendString(const char* str)
{
	for (int i = 0; i < strlen(str); i++)
	{
		RS232Send(str[i]);
	}
	RS232Send('\r');
	return 0;
}

bool StrEqual(char* buf, const char* str)
{
	for (int i = 0; i < strlen(str); i++)
	{
		if (buf[i] != str[i])
			return false;
	}
	return true;
}

void InitRS232()
{
	char mode_str[128];
	strcpy_s(mode_str, "baud=115200");
	//const char* mode = "8N1";
	strcat_s(mode_str, " data=8");
	strcat_s(mode_str, " parity=n");
	strcat_s(mode_str, " stop=1");
	strcat_s(mode_str, " dtr=on rts=on");
	ComPort = CreateFileA("\\\\.\\COM5",
		GENERIC_READ | GENERIC_WRITE,
		0,                          /* no share  */
		NULL,                       /* no security */
		OPEN_EXISTING,
		0,                          /* no threads */
		NULL);                      /* no templates */

	if (ComPort == INVALID_HANDLE_VALUE)
	{
		std::cout << "ComPort Error" << std::endl;
		return;
	}

	DCB port_settings;
	memset(&port_settings, 0, sizeof(port_settings));
	port_settings.DCBlength = sizeof(port_settings);

	if (!BuildCommDCBA(mode_str, &port_settings))
	{
		std::cout << "BuildCommDCBA Error" << std::endl;
		CloseHandle(ComPort);
		return;
	}

	if (!SetCommState(ComPort, &port_settings))
	{
		std::cout << "SetCommState Error" << std::endl;
		CloseHandle(ComPort);
		return;
	}

	COMMTIMEOUTS Cptimeouts;

	Cptimeouts.ReadIntervalTimeout = MAXDWORD;
	Cptimeouts.ReadTotalTimeoutMultiplier = 0;
	Cptimeouts.ReadTotalTimeoutConstant = 0;
	Cptimeouts.WriteTotalTimeoutMultiplier = 1;
	Cptimeouts.WriteTotalTimeoutConstant = 1;

	if (!SetCommTimeouts(ComPort, &Cptimeouts))
	{
		std::cout << "SetCommTimeouts" << std::endl;
		CloseHandle(ComPort);
		return;
	}
}

char buffer[50];
int bufCount = 0;
DWORD WINAPI ReadThread()
{
	char buf[128];
	while (true)
	{
		int x = RS232Get(buf, 128);
		if (x > 0)
		{
			if (buf[0] == '\r')
			{
				std::cout << "\nReceived Signal: ";
				for (int i = 0; i < bufCount; i++)
				{
					std::cout << buffer[i];
				}
				std::cout << std::endl << "Input>>: ";
				bufCount = 0;
			}
			else
			{
				buffer[bufCount] = buf[0];
				bufCount++;
			}
		}
	}
}

int main(int argc, char** argv)
{
	InitRS232();
	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ReadThread, 0, 0, 0);
	std::cout << "Input>>: ";
	while (true)
	{
		std::string str;
		std::cin.clear();
		std::getline(std::cin, str);
		std::transform(str.begin(), str.end(), str.begin(), tolower);
		RS232SendString(str.c_str());
	}

	return 0;
}