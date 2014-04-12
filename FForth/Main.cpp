#include "Serial.h"
#include <iostream>
#include <fstream>

#include <map>
#include <string>


using namespace std;

map<char, char> ASCIItoMFCharMap;






void U32ToU18(int x, char* data)
{
	 data[0] = x & 0x000000FF;
	 data[1] = (x & 0x0000FF00) >> 8;
	 data[2] = (x & 0x00030000) >> 16;
	 if(x & 0xFFFC0000) 
		  std::cout << "warning: loss of data while converting to 18 bit cell";
}

//void BuildCharStream(char* data, int length)
//{
//	  for(int i = 0; i < length; i += 3)
//	 {
//		  data[i] = ASCIItoMFCharMap.at(data[i]);
//		  data[i+1] = ASCIItoMFCharMap.at(data[i+1]);
//		  data[i+2] = ASCIItoMFCharMap.at(data[i+2]);
//		  int temp = data[i];	
//		  temp |= data[i+1] << 6;
//		  temp |= data[i+2] << 12;
//		  data[i]   = temp & 0x000000FF;
//		  data[i+1] = (temp & 0x0000FF00) >> 8;
//		  data[i+2] = (temp & 0x00FF0000) >> 16;
//		 
//	 }
//}




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
	 fstream instrm(argv[1], ios_base::in | ios_base::binary );
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

					 //Add frame header
					 int port = (int)strtol(argv[2], NULL, 0);
                int bps = (int)strtol(argv[3], NULL, 0);
					 //int compAddr = (int)strtol(argv[3], NULL, 0);
					 //int transAddr = (int)strtol(argv[4], NULL, 0);

					 //U32ToU18(compAddr, &buffer[0]); // Completion address
					 //U32ToU18(transAddr, &buffer[3]); // Transfer address
					 //U32ToU18(length / 3, &buffer[6]); // Transfer size

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

					 if(argc == 5)
					 {
						 FILE* binout;
						 fopen_s(&binout ,argv[4], "wb");
						 if(fwrite(buffer, 1, length, binout) == length)						 
							 std::cout << "boot stream written to file \n";						 
						 else
							 std::cout << "could not write boot stream to file \n";
					 }


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

                serial.Close();




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



void InitASCIItoMFCharMap()
{
	 ASCIItoMFCharMap[ 32] =  0; // (sp) 0x20
	 ASCIItoMFCharMap[ 9 ] =  0; // (ht) 0x09
	 ASCIItoMFCharMap[ 13] =  0; // (cr) 0x0d
	 ASCIItoMFCharMap[ 97] =  1; // a    0x61
	 ASCIItoMFCharMap[ 98] =  2; // b    0x62
	 ASCIItoMFCharMap[ 99] =  3; // c    0x63
	 ASCIItoMFCharMap[100] =  4; // d    0x64
	 ASCIItoMFCharMap[101] =  5; // e    0x65
	 ASCIItoMFCharMap[102] =  6; // f    0x66
	 ASCIItoMFCharMap[103] =  7; // g    0x67
	 ASCIItoMFCharMap[104] =  8; // h    0x68
	 ASCIItoMFCharMap[105] =  9; // i    0x69
	 ASCIItoMFCharMap[106] = 10; // j    0x6a
	 ASCIItoMFCharMap[107] = 11; // k    0x6b
	 ASCIItoMFCharMap[108] = 12; // l    0x6c
	 ASCIItoMFCharMap[109] = 13; // m    0x6d
	 ASCIItoMFCharMap[110] = 14; // n    0x6e
	 ASCIItoMFCharMap[111] = 15; // o    0x6f
	 ASCIItoMFCharMap[112] = 16; // p    0x70
	 ASCIItoMFCharMap[113] = 17; // q    0x71
	 ASCIItoMFCharMap[114] = 18; // r    0x72
	 ASCIItoMFCharMap[115] = 19; // s    0x73
	 ASCIItoMFCharMap[116] = 20; // t    0x74
	 ASCIItoMFCharMap[117] = 21; // u    0x75
	 ASCIItoMFCharMap[118] = 22; // v    0x76
	 ASCIItoMFCharMap[119] = 23; // w    0x77
	 ASCIItoMFCharMap[120] = 24; // x    0x78
	 ASCIItoMFCharMap[121] = 25; // y    0x79
	 ASCIItoMFCharMap[122] = 26; // z    0x7a
	 ASCIItoMFCharMap[ 65] =  1; // A    0x41
	 ASCIItoMFCharMap[ 66] =  2; // B    0x42
	 ASCIItoMFCharMap[ 67] =  3; // C    0x43
	 ASCIItoMFCharMap[ 68] =  4; // D    0x44
	 ASCIItoMFCharMap[ 69] =  5; // E    0x45
	 ASCIItoMFCharMap[ 70] =  6; // F    0x46
	 ASCIItoMFCharMap[ 71] =  7; // G    0x47
	 ASCIItoMFCharMap[ 72] =  8; // H    0x48
	 ASCIItoMFCharMap[ 73] =  9; // I    0x49
	 ASCIItoMFCharMap[ 74] = 10; // J    0x4a
	 ASCIItoMFCharMap[ 75] = 11; // K    0x4b
	 ASCIItoMFCharMap[ 76] = 12; // L    0x4c
	 ASCIItoMFCharMap[ 77] = 13; // M    0x4d
	 ASCIItoMFCharMap[ 78] = 14; // N    0x4e
	 ASCIItoMFCharMap[ 79] = 15; // O    0x4f
	 ASCIItoMFCharMap[ 80] = 16; // P    0x50
	 ASCIItoMFCharMap[ 81] = 17; // Q    0x51
	 ASCIItoMFCharMap[ 82] = 18; // R    0x52
	 ASCIItoMFCharMap[ 83] = 19; // S    0x53
	 ASCIItoMFCharMap[ 84] = 20; // T    0x54
	 ASCIItoMFCharMap[ 85] = 21; // U    0x55
	 ASCIItoMFCharMap[ 86] = 22; // V    0x56
	 ASCIItoMFCharMap[ 87] = 23; // W    0x57
	 ASCIItoMFCharMap[ 88] = 24; // X    0x58
	 ASCIItoMFCharMap[ 89] = 25; // Y    0x59
	 ASCIItoMFCharMap[ 90] = 26; // Z    0x5a
	 ASCIItoMFCharMap[ 48] = 27; // 0    0x30
	 ASCIItoMFCharMap[ 49] = 28; // 1    0x31
	 ASCIItoMFCharMap[ 50] = 29; // 2    0x32
	 ASCIItoMFCharMap[ 51] = 30; // 3    0x33
	 ASCIItoMFCharMap[ 52] = 31; // 4    0x34
	 ASCIItoMFCharMap[ 53] = 32; // 5    0x35
	 ASCIItoMFCharMap[ 54] = 33; // 6    0x36
	 ASCIItoMFCharMap[ 55] = 34; // 7    0x37
	 ASCIItoMFCharMap[ 56] = 35; // 8    0x38
	 ASCIItoMFCharMap[ 57] = 36; // 9    0x39
	 ASCIItoMFCharMap[ 33] = 37; // !    0x21
	 ASCIItoMFCharMap[ 34] = 38; // "    0x22
	 ASCIItoMFCharMap[ 35] = 39; // #    0x23
	 ASCIItoMFCharMap[ 36] = 40; // $    0x24
	 ASCIItoMFCharMap[ 37] = 41; // %    0x25
	 ASCIItoMFCharMap[ 38] = 42; // &    0x26
	 ASCIItoMFCharMap[ 39] = 43; // '    0x27
	 ASCIItoMFCharMap[ 40] = 44; // (    0x28
	 ASCIItoMFCharMap[ 41] = 45; // )    0x29
	 ASCIItoMFCharMap[ 42] = 46; // *    0x2a
	 ASCIItoMFCharMap[ 43] = 47; // +    0x2b
	 ASCIItoMFCharMap[ 44] = 48; // ,    0x2c
	 ASCIItoMFCharMap[ 45] = 49; // -    0x2d
	 ASCIItoMFCharMap[ 46] = 50; // .    0x2e
	 ASCIItoMFCharMap[ 47] = 51; // /    0x2f
	 ASCIItoMFCharMap[ 58] = 52; // :    0x3a
	 ASCIItoMFCharMap[ 59] = 53; // ;    0x3b
	 ASCIItoMFCharMap[ 60] = 54; // <    0x3c
	 ASCIItoMFCharMap[ 61] = 55; // =    0x3d
	 ASCIItoMFCharMap[ 62] = 56; // >    0x3e
	 ASCIItoMFCharMap[ 63] = 57; // ?    0x3f
	 ASCIItoMFCharMap[ 64] = 58; // @    0x40
	 ASCIItoMFCharMap[ 91] = 59; // [    0x5b
	 ASCIItoMFCharMap[ 93] = 60; // ]    0x5d
	 ASCIItoMFCharMap[ 94] = 61; // ^    0x5e
	 ASCIItoMFCharMap[124] = 62; // |    0x7c
	 ASCIItoMFCharMap[126] = 63; // ~    0x7e
}




 