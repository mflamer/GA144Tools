#include "Serial.h"
#include "Compiler.h"
#include <iostream>
#include <fstream>

#include <map>
#include <string>


using namespace std;



void BuildASyncStream(char* data, int length)
{
	 for(int i = 0; i < length; i += 3)
	 {
		  int temp = *((int*)&data[i]) & 0x00FFFFFF;		
		  temp = (temp & 0x3FFFF) ^ 0x3FFFF; // invert
		  temp = (temp << 6) | 0x12; // shift, set calib bits			  
		  temp = temp & 0x00FFFFFF;
		  data[i]   = temp & 0x000000FF;
		  data[i+1] = (temp & 0x0000FF00) >> 8;
		  data[i+2] = (temp & 0x00FF0000) >> 16;
	 }
}

int main(int argc, char **argv)
{
    CCompiler comp(argv[1], "src\ga144code.bin");
	 fstream instrm("src\ga144code.bin", ios_base::in | ios_base::binary );
	 int length; 
	 if(instrm)
	 {
		  // get length of file:
		  instrm.seekg (0, instrm.end);
		  length = (streamoff)instrm.tellg();		  
		  instrm.seekg (0, instrm.beg);

		  if(length % 3 != 0)
				std::cout << "error: stream length is invalid, abort boot \n";
		  else
		  {				
				char * buffer = new char [length];

				std::cout << "Reading " << length << " characters... \n";
				// read data as a block:
				instrm.read (buffer, length);

				if (instrm)
				{
					 instrm.close();
					 std::cout << "all characters read successfully \n";

					 
					 int port = (int)strtol(argv[2], NULL, 0);
                int bps = (int)strtol(argv[3], NULL, 0);

					 BuildASyncStream(buffer, length);
					 
					 CSerial serial;
					 
					 serial.Open(port, bps);
					 if(serial.SendData(buffer, length) == length)
					 {
						  std::cout << "port opened, data sent \n";
						  //serial.Close();
					 }
					 else 
						  std::cout << "error: unable to open port \n";

					
						FILE* binout;
						fopen_s(&binout , "boot.bin", "wb");
						if(fwrite(buffer, 1, length, binout) == length)						 
							std::cout << "boot stream written to file \n";						 
						else
							std::cout << "could not write boot stream to file \n";
					 

                char* lpBuffer = new char[500];                
                int nBytesRead = 0;
                for(int i = 0; i < 500; i++)
                {
                   lpBuffer[i] = 0;
                }                
                for(int i = 0; i < 0xFFFFFF; i++)
                {
                   nBytesRead++;
                }

                nBytesRead = serial.ReadData(lpBuffer, 500);
                string strOut(lpBuffer, nBytesRead);
                if(nBytesRead)
                  std::cout << "cool we read some data back - " << strOut << "\n";

                //serial.Close();

                while(true){}


				}
				else
				{					 
					 std::cout << "error: only " << instrm.gcount() << " could be read \n";
					 instrm.close();
				}

				
		  }
	 }
	 else
		  std::cout << "error: file could not be opened for read \n";
    


	 
}








 