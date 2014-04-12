#include "Compiler.h"



CCompiler::CCompiler(std::string filename)
{
	 binout = fopen((filename.append(".mfb")).c_str(), "wb");	 
	 h = 1;
}


CCompiler::~CCompiler(void)
{
	 if(fclose(binout))
		  std::cout << "error: could not close bin file \n";
}


bool CCompiler::NextTok(FILE* src, int c)
{
	 tok.clear();
	 // Skip white space
	 while(isspace(c) && c != EOF)
		  c = getc(src);	
	 if(c == EOF) return false;	 
	 // extract tok				
	 do
	 {
		  tok.push_back((char)c);
		  c = getc(src);					 
	 }while(!isspace(c) && c != EOF);
}


void CCompiler::CompileFile(std::string filename)
{
	 FILE* src = fopen((filename.append(".mfs")).c_str(), "r");
	 
	 bool needAddr;
	 if(src)
	 {
		  int c = getc(src);
		  while(c != EOF)
		  {
				if(!NextTok(src, c)) break;
				
				// Test if tok is a number
				if(isdigit(tok.at(0)) || (tok.size() > 1 && tok.at(0) == 45)) // "-"
				{
					 int lit32 = (int)strtol(tok.c_str(), NULL, 0);
					 AddLit(lit32);
				}

				// Test if tok is an instruction
				else if(tok.c_str == ";")			AddInst(0x00);
				else if(tok.c_str == "ex")		AddInst(0x01);
				else if(tok.c_str == "jump"){	
					 AddInst(0x02);
					 needAddr = true;
				}
				else if(tok.c_str == "call"){	
					 AddInst(0x03);
					 needAddr = true;
				}
				else if(tok.c_str == "unext")	AddInst(0x04);
				else if(tok.c_str == "next"){	
					 AddInst(0x05);
					 needAddr = true;
				}
				else if(tok.c_str == "if"){	
					 AddInst(0x06);
					 needAddr = true;
				}
				else if(tok.c_str == "-if"){	
					 AddInst(0x07);
					 needAddr = true;
				}
																		  	 
				else if(tok.c_str == "@p")		AddInst(0x08);
				else if(tok.c_str == "@+")	   AddInst(0x09);
				else if(tok.c_str == "@b")	   AddInst(0x0A);
				else if(tok.c_str == "@")			AddInst(0x0B);
				else if(tok.c_str == "!p")		AddInst(0x0C);
				else if(tok.c_str == "!+")	   AddInst(0x0D);
				else if(tok.c_str == "!b")		AddInst(0x0E);
				else if(tok.c_str == "!")	      AddInst(0x0F);
																		  
				else if(tok.c_str == "+*")		AddInst(0x10);
				else if(tok.c_str == "2*")		AddInst(0x11);
				else if(tok.c_str == "2/")		AddInst(0x12);
				else if(tok.c_str == "-")			AddInst(0x13);
				else if(tok.c_str == "+")			AddInst(0x14);
				else if(tok.c_str == "and")		AddInst(0x15);
				else if(tok.c_str == "or")		AddInst(0x16);
				else if(tok.c_str == "drop")		AddInst(0x17);
				else if(tok.c_str == "dup")		AddInst(0x18);
				else if(tok.c_str == "pop")		AddInst(0x19);
				else if(tok.c_str == "over")		AddInst(0x1A);
				else if(tok.c_str == "a")			AddInst(0x1B);
				else if(tok.c_str == ".")			AddInst(0x1C);
				else if(tok.c_str == "push")		AddInst(0x1D);
				else if(tok.c_str == "b!")		AddInst(0x1E);
				else if(tok.c_str == "a!")		AddInst(0x1F);

				// simple include file
				else if(tok.c_str == "include")		
				{
					 // extract next tok as filename						  
					 if(!NextTok(src, c)) break;
					 CompileFile(tok);
				}

				// new word
				else if(tok.c_str == ":")
				{
					 // extract next tok as word name						  
					 if(!NextTok(src, c)) break;
					 while(slot != 0)
						  AddInst(0x1C); // "."
					 nameMap[tok] = ic;
				}

				// call
				else
				{
					 std::map<std::string, unsigned short>::iterator itr = nameMap.find(tok);
					 if(itr == nameMap.end())
					 {
						  std::cout << "error: unknown word " << tok.c_str() <<  " \n";
						  break;
					 }
					 else
					 {
						  AddInst(0x03); // "call"
						  int addr = itr->second;
						  if(AddrToBig(addr))
								while(slot != 0)
									 AddInst(0x1C); // "."
						  AddAddress(addr);
					 }
				}

				// if necessary add address for instructions 
				if(needAddr)
				{
					 if(!NextTok(src, c)) break;
					 std::map<std::string, unsigned short>::iterator itr = nameMap.find(tok);
					 if(itr == nameMap.end())
					 {
						  std::cout << "error: unknown word " << tok.c_str() <<  " \n";
						  break;
					 }
					 else
					 {								
						  int addr = itr->second;
						  if(AddrToBig(addr))
								while(slot != 0)
									 AddInst(0x1C); // "."
						  AddAddress(addr);
					 }
				}
				
				c = getc(src);
		  }

		  if(fclose(src))
				std::cout << "error: could not close source file " << filename.c_str() <<  ".mfs \n";

	 }
	 else
		  std::cout << "error: could not open source file " << filename.c_str() <<  ".mfs \n";
	 
}


void CCompiler::AddLit(int lit)
{
	 char cell[3];
	 i32Toi18(lit, cell);
	 char temp = 0;
	 // add empty space for current instruction cell then add lit
	 if(fwrite(&temp, 1, 3, binout) == 3 && fwrite(cell, 1, 3, binout) == 3) 	 
	 {	
		  AddInst(0x08); // @p
		  h++;
	 }
	 else
		  std::cout << "error: unable to write cell \n";
}

void CCompiler::AddInst(char inst)
{	 
	 if(slot == 3 && !IsLittleInst(inst))
	 {
		  AddInst(0x1C);// no op
		  AddInst(inst);		  
	 }
	 else
	 {
		  switch(slot)
		  {
				case 0:
				{
					 int temp = (inst << 13) ^ 0x15555;
					 fCell |= temp;
					 break;
				}
				case 1:
				{
					 int temp = (inst << 8) ^ 0x15555;
					 fCell |= temp;
					 break;
				}
				case 2:
				{
					 int temp = (inst << 3) ^ 0x15555;
					 fCell |= temp;
					 break;
				}
				case 3:
				{
					 int temp = (inst & 0x7) ^ 0x15555;
					 fCell |= temp;
					 break;
				}
		  }
		  
		  slot++;
		  if(slot == 4) 
		  {
				WriteCell();
		  }
	 }	 
}

bool CCompiler::AddrToBig(int addr)
{	 
	 switch(slot)
		  {
				case 0:
				{
					 std::cout << "trying to add an address at slot 0 \n";
					 return false;
				}
				case 1:									 
					 if(addr & 0xFFFFFE00) return true;				 
				
				case 2:
					 if(addr & 0xFFFFFFF0) return true;	
				case 3:
					 if(addr & 0xFFFFFFF8) return true;	
		  }
}


void CCompiler::AddAddress(int address)
{
	  switch(slot)
		  {
				case 0:
				{
					 std::cout << "trying to add an address at slot 0 \n";
					 break;
				}
				case 1:
				{					 
					 fCell |= (address & 0x3FF);
					 break;
				}
				case 2:
				{
					 fCell |= (address & 0xFF);
					 break;
				}
				case 3:
				{
					 fCell |= (address & 0x7);
					 break;
				}
		  }

	  WriteCell();
}

void CCompiler::WriteCell()
{
	 int pos = ic * 3;
	 fsetpos(binout, (fpos_t*)&pos);
	 if(fwrite((const void*)&fCell, 1, 3, binout) == 3)
	 { 		  
		  slot = 0;
		  fCell = 0;
		  ic = h;
		  pos = h * 3;
		  fsetpos(binout, (fpos_t*)&pos);
		  h++;		  
	 }
	 else
		  std::cout << "error: unable to write cell \n";
}


bool CCompiler::IsLittleInst(char inst)
{
	 switch(inst)
	 {
		  case 0x00:
		  case 0x04:
		  case 0x08:
		  case 0x12:
		  case 0x10:
		  case 0x14:
		  case 0x18:
		  case 0x1C:
				return true;
		  default:
				return false;
	 }
}