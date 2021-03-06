#include "Compiler.h"
#include <math.h>


void U32ToU18(int x, char* data)
{
	 data[0] = x & 0x000000FF;
	 data[1] = (x & 0x0000FF00) >> 8;
	 data[2] = (x & 0x00030000) >> 16;
    //data[2] |= (x & 0x80000000) >> 26;  
	 //if(x & 0x7FFC0000) 
		//  std::cout << "warning: loss of data while converting to 18 bit cell \n";
}

CCompiler::CCompiler(std::string SrcFilename, std::string BinFilename){
	 
	 fopen_s(&binout ,BinFilename.c_str(), "w+b");	
	 ic = 0;
	 h = 1;
	 slot = 0;
	 fCell = 0;
    InitWordMap();
    InitBootOrder();
    InitBootPath();

	 //fwrite(&ic, 1, 3, binout);// make sure there is room for first inst cell 
	 CompileFile(SrcFilename);

    WriteBootChipSeq();

    fclose(binout);
}


CCompiler::~CCompiler(void)
{
	 if(fclose(binout))
		  std::cout << "error: could not close bin file \n";
}


void CCompiler::EndFrame(int transferAddr, int completionAddr, int numCodeWords)
{
   //Add the BootFrame header first
   char cell[3];
   U32ToU18(completionAddr, cell);
   if(fwrite(cell, 1, 3, binout) != 3)
      std::cout << "error: could not write completionAddr to header \n";
   U32ToU18(transferAddr, cell);
   if(fwrite(cell, 1, 3, binout) != 3)
      std::cout << "error: could not write transferAddr to header \n";   
   if(!numCodeWords)
      U32ToU18(binBlk.size() / 3, cell);
   else
      U32ToU18(numCodeWords, cell);
   if(fwrite(cell, 1, 3, binout) != 3)
      std::cout << "error: could not write frame length to header \n";


   EndBlock();   
}


void CCompiler::EndBlock()
{
   // make sure we write the last inst cell
	if(fCell)
	{
		while(slot != 0)				
			AddInst(0x1C); // "."				
	}

	// resolve forward references
	std::vector<ForwardRef>::iterator itr = fRefs.begin();
	while(itr != fRefs.end())
	{
		std::map<std::string, int>::iterator ref = nameMap.find(itr->ident);
		if(ref == nameMap.end())
		{					 
				std::cout << "error: unable to resolve forward reference - " << tok.c_str() <<  " \n";
		}
		else
		{
				ResolveFRef(*itr, ref->second);
		}
		itr++;
	}

   if(fwrite(binBlk.data(), 1, binBlk.size(), binout) == binBlk.size())
   {
      Reset();
      // std::cout << "Sucessfully wrote a block to the file \n";
   }
   else
      std::cout << "error: failed to write block to file \n";
}

void CCompiler::Reset()
{
   binBlk.clear();
   fCell = 0;
   ic = 0;
   h = 1;
   slot = 0;
   nameMap.clear();
   fRefs.clear();
   thenStack.clear();
   forStack.clear();
   InitWordMap();
}

void CCompiler::StartNode(int nodeNum)
{
   fNode = nodeNum; 
   Reset();
}

void CCompiler::EndRam()
{
   // make sure we write the last inst cell
	if(fCell)
	{
		while(slot != 0)				
			AddInst(0x1C); // "."				
	}
   
   fGrid.at(fNode).fRamWords = ic;
}

void CCompiler::EndNode()
{
   // make sure we write the last inst cell

	while(slot != 0)				
		AddInst(0x1C); // "."				
	

	// resolve forward references
	std::vector<ForwardRef>::iterator itr = fRefs.begin();
	while(itr != fRefs.end())
	{
		std::map<std::string, int>::iterator ref = nameMap.find(itr->ident);
		if(ref == nameMap.end())
		{					 
				std::cout << "error: unable to resolve forward reference - " << tok.c_str() <<  " \n";
		}
		else
		{
				ResolveFRef(*itr, ref->second);
		}
		itr++;
	}

   if(fGrid[fNode].fRamWords > 144)
      std::cout << "Error: Binary too big to fit in node - " << binBlk.size() << " \n";
   else
   {
      fGrid.at(fNode).fCode = binBlk;
   }
   Reset();
}


bool CCompiler::NextTok(FILE* src)
{
	 tok.clear();
	 int c = getc(src);
	 if(c == EOF) return false;	

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
	 return true;
}


const int LE = 0x175;
const int RI = 0x1D5;
const int UP = 0x145;
const int DN = 0x115;


bool GetPortAddress( std::string tok, int& addr)
{
   addr = 0;
        if(tok == "io"  )	addr = 0x15D;
	else if(tok == "data")  addr = 0x141;
	else if(tok == "---u")  addr = 0x145;
	else if(tok == "--l-")  addr = 0x175;
	else if(tok == "--lu")  addr = 0x165;
	else if(tok == "-d--")  addr = 0x115;
	else if(tok == "-d-u")  addr = 0x105;
	else if(tok == "-dl-")  addr = 0x135;
	else if(tok == "-dlu")  addr = 0x125;
	else if(tok == "r---")  addr = 0x1D5;
	else if(tok == "r--u")  addr = 0x1C5;
	else if(tok == "r-l-")  addr = 0x1F5;
	else if(tok == "r-lu")  addr = 0x1E5;
	else if(tok == "rd--")  addr = 0x195;
	else if(tok == "rd-u")  addr = 0x185;
	else if(tok == "rdl-")  addr = 0x1B5;
	else if(tok == "rdlu")  addr = 0x1A5;
   return addr;
}

bool CCompiler::GetWordAddress( std::string tok, int& addr )
{
   std::map<std::string, int>::iterator itr = nameMap.find(tok);
	if(itr == nameMap.end())
	{
		return false;
	}
	else
	{						  
		addr = itr->second;
      return true;
	}	
}


void CCompiler::CompileFile(std::string SrcFileName)
{
	 FILE* src;
	 errno_t err = fopen_s(&src, SrcFileName.c_str(), "r");	 
	 
	 if(src)
	 {		  
		  while(NextTok(src))
		  {
				bool needAddr = false;
            int addr = 0;

				// Comments
				if(tok == "(")
				{
					 int c;
					 do
					 {						  
						  c = getc(src);					 
					 }while(c != 41 && c != EOF); // ")"
					 if(c == EOF) break;
				}

            // Strings
            else if(tok.size() > 2 && tok.at(0) == *"\"" && tok.at(tok.size()-1) == *"\"")
            {
               tok.erase(tok.begin());
               for(int i = 0; i < tok.size() - 1; i++)
               {
                  AddLit(tok.at(i), false);
               }
            }
            else if(tok == "----NODE----")
            {
               NextTok(src);
               int nodeNum = (int)strtol(tok.c_str(), NULL, 0);
               StartNode(nodeNum);
            }
             else if(tok == "----ENDRAM")
            {               
               EndRam();
            }
            else if(tok == "----ENDNODE")
            {
               EndNode();
            }
            else if(tok == "----ENDFRAME")
            {
               // Make sure to finish last cell
               while(slot != 0)
						  AddInst(0x1C);

               int transferAddr;
               NextTok(src);
               if(GetPortAddress(tok, transferAddr));
               else if(GetWordAddress(tok, transferAddr));
               else transferAddr = (int)strtol(tok.c_str(), NULL, 0);

               int completionAddr;
               NextTok(src);
               if(GetPortAddress(tok, completionAddr));
               else if(GetWordAddress(tok, completionAddr));
               else completionAddr = (int)strtol(tok.c_str(), NULL, 0);

               EndFrame(transferAddr, completionAddr);
            }

            else if(tok == "--ENDBLOCK")
            {
               EndBlock();
            }

				// Constant Lits           
            //else if(GetPortAddress(tok, addr))
            //{
            //   AddLit(addr);
            //}
							

				// Test if tok is an instruction
				else if(tok == ";")		
               AddInst(0x00);
				else if(tok == "ex")		AddInst(0x01);
				else if(tok == "jump"){	
					 AddInst(0x02);
					 needAddr = true;
				}
				else if(tok == "call"){
					 AddInst(0x03);
					 needAddr = true;
				}
				else if(tok == "unext")	AddInst(0x04);
				else if(tok == "next"){	
					 AddInst(0x05);
					 needAddr = true;
				}
				else if(tok == "if"){	
					 AddInst(0x06);	
					 needAddr = true;
				}
				else if(tok == "-if"){	
					 AddInst(0x07);
					 needAddr = true;
				}
																		  	 
				else if(tok == "@p")		AddInst(0x08);
				else if(tok == "@+")	   AddInst(0x09);
				else if(tok == "@b")	   AddInst(0x0A);
				else if(tok == "@")		AddInst(0x0B);
				else if(tok == "!p")		AddInst(0x0C);
				else if(tok == "!+")	   AddInst(0x0D);
				else if(tok == "!b")		AddInst(0x0E);
				else if(tok == "!")	   AddInst(0x0F);
																		  
				else if(tok == "+*")		AddInst(0x10);
				else if(tok == "2*")		AddInst(0x11);
				else if(tok == "2/")		AddInst(0x12);
				else if(tok == "-")		AddInst(0x13);
				else if(tok == "+")		AddInst(0x14);
				else if(tok == "and")	AddInst(0x15);
				else if(tok == "or")		AddInst(0x16);
				else if(tok == "drop")	AddInst(0x17);
				else if(tok == "dup")	AddInst(0x18);
				else if(tok == "pop")	AddInst(0x19);
				else if(tok == "over")	AddInst(0x1A);
				else if(tok == "a")		AddInst(0x1B);
				else if(tok == ".")		AddInst(0x1C);
				else if(tok == "push")	AddInst(0x1D);
				else if(tok == "b!")		AddInst(0x1E);
				else if(tok == "a!")		AddInst(0x1F);


            	// Test if tok is a number
				else if(isdigit(tok.at(0)) 
               || (tok.size() > 1 && tok.at(0) == 45)  // "-"
               || (tok.size() > 1 && tok.at(0) == 35)  // "#"
               || (tok.size() > 1 && tok.at(0) == 38)) // "&"
				{
					bool call = !(tok.at(0) == 35);
					if(tok.at(0) == 35)                
					   tok.erase(tok.begin());

					int lit32 = 0;
					if(tok.at(0) == 38)
					{
					   tok.erase(tok.begin());
					   GetWordAddress(tok, lit32);
					}
					else
					   lit32 = (int)strtol(tok.c_str(), NULL, 0);               					  					 
					 
					AddLit(lit32, call);
				}


				// pop a then addr off stack
				else if(tok == "then")
				{
					 while(slot != 0)
						  AddInst(0x1C); // "."
					 
					 ForwardRef ref = thenStack.back();
					 thenStack.pop_back();
					 ResolveFRef(ref, ic);
				}

				else if(tok == "for")
				{
					 AddInst(0x1D);//push
					  while(slot != 0) // cell align
						  AddInst(0x1C); // "."
					  forStack.push_back(ic);
				}

				// simple include file
				else if(tok == "include")		
				{
					 // extract next tok as filename						  
					 if(!NextTok(src)) break;
					 CompileFile(tok);
				}

				// new word
				else if(tok == ":")
				{
					 //extract next tok as word name					 
					 if(!NextTok(src)) break;
					 while(slot != 0)
						  AddInst(0x1C); // "."
					 nameMap[tok] = ic;
                std::cout << "word addr | " << tok << " = " << ic <<  " \n";
				}

				// call
				else
				{		 

					 // if this call is preceded by a ";" we can "jump" instead (TCO)
					 fpos_t pos;
					 std::string thisTok = tok;
					 fgetpos(src, &pos);
					 NextTok(src);

					 slot++;// temporarily inc slot to account for call
					 bool noRoom = AddrToBig(addr); 
					 slot--;
					 if(noRoom)
						  while(slot != 0)
								AddInst(0x1C); // "."

					 if(tok ==";")
						  AddInst(0x02); // tail call, so jump instead
					 else
					 {                    
						  fsetpos(src, &pos);
						  AddInst(0x03); // "call"
					 }                
					 
					if(!GetWordAddress(thisTok, addr))
					{
						ForwardRef ref;
						ref.ident = thisTok;
						ref.ic = ic;
						ref.slot = slot;
						fRefs.push_back(ref);
						std::cout << "forward reference - " << thisTok.c_str() <<  " \n";
					}									 

					 AddAddress(addr);					 
				}

				// if necessary add address for instructions 
				if(needAddr)				{
					 
					 if(tok == "if" || tok == "-if")
					 {
						  ForwardRef loc;
						  loc.ic = ic;
						  loc.slot = slot;
						  thenStack.push_back(loc);
					 }
					 else if(tok == "next" )
					 {
						  if(forStack.size() == 0)
						  {
								std::cout << "no for to match next \n";
						  }
						  else
						  {
								addr = forStack.back();
								forStack.pop_back();
						  }
					 }
					 else
					 {
						  if(!NextTok(src)) break;						  
						 
							std::map<std::string, int>::iterator itr = nameMap.find(tok);
							if(itr == nameMap.end())
							{
                        // Test if it's a port address
                        if(!GetPortAddress(tok, addr))
                        {
                           // Test if tok is a number
                           if(isdigit(tok.at(0)) || (tok.size() > 1 && tok.at(0) == 45)) // "-"
						         {							 
								      addr = (int)strtol(tok.c_str(), NULL, 0);						  
						         }
                           else // Forward ref
                           {
									   ForwardRef ref;
									   ref.ident = tok;
									   ref.ic = ic;
									   ref.slot = slot;
									   fRefs.push_back(ref);
									   std::cout << "forward reference - " << tok.c_str() <<  " \n";
                           }
                        }
							}
							else
							{
									addr = itr->second;
							}						  												
					
						  if(AddrToBig(addr))
								while(slot != 0)
									 AddInst(0x1C); // "."
					 }

                 if(AddrToBig(addr))
                     std::cout << "address too big for slot - " << slot << " \n";
					 AddAddress(addr);					 
				}				
		  }

      

		  if(fclose(src))
				std::cout << "error: could not close source file " << SrcFileName.c_str() <<  " \n";

	 }
	 else
		  std::cout << "error: could not open source file " << SrcFileName.c_str() <<  " \n";


	 std::cout << "compilation sucessful \n";	 


	
}


void CCompiler::AddLit(int lit, bool call)
{
	 char cell[3];
	 U32ToU18(lit, cell);
	 int pos;
	 if(call)
		  pos = h * 3;
	 else
    {
        while(slot != 0)
				AddInst(0x1C); // "."
		  pos = ic * 3;
    }

    
    while(pos > binBlk.size())
    {
       binBlk.push_back(0);
    }
    if(pos == binBlk.size())
    {
       binBlk.push_back(cell[0]);
       binBlk.push_back(cell[1]);
       binBlk.push_back(cell[2]);
    }
    else
    {
       binBlk[pos] = cell[0];
       binBlk[pos+1] = cell[1];
       binBlk[pos+2] = cell[2];
    }
       

   if(call)
	{
		AddInst(0x08); // @p
		h++;
	}
	else
	{
		ic = h;
		h++;
	}

}

bool CCompiler::AddInst(int inst)
{	
    bool result = false;
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
					 int temp = inst << 13;
					 int code = 0x15555 & 0x3E000;
					 temp = temp ^ code;
					 fCell |= temp;
					 break;
				}
				case 1:
				{
					 int temp = inst << 8;
					 int code = 0x15555 & 0x1F00;
					 temp = temp ^ code;
					 fCell |= temp;
					 break;
				}
				case 2:
				{
					 int temp = inst << 3; 
					 int code = 0x15555 & 0xF8;
					 temp = temp ^ code;
					 fCell |= temp;
					 break;
				}
				case 3:
				{
					 int temp = inst >> 2;
					 int code = 0x15555 & 0x7;
					 temp = temp ^ code;
					 fCell |= temp;
					 break;
				}
		  }
		  
		  slot++;
		  if(slot == 4) 
		  {
				WriteCell();
            result = true; //we inc'ed h already
		  }
	 }	
    return result;
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
				{
					if(addr & 0xFFFFFE00) return true;				 
					break;
				}
				case 2:
				{
					if(addr & 0xFFFFFF00) return true;	
					break;
				}
				case 3:
				{
					if(addr & 0xFFFFFFF8) return true;	
					break;
				}
				default:
					 return true;
		  }
	 return false;
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
					 if(address & 0x3E000)				
						  std::cout << " slot 1 too small for address \n";	
					 fCell |= (address & 0x3FF);
					 break;
				}
				case 2:
				{
					 if(address & 0x3FF00)			
						  std::cout << " slot 2 too small for address \n";
					 fCell |= (address & 0xFF);
					 break;
				}
				case 3:
				{
					 if(address & 0x3FFFC)
						  std::cout << " slot 3 too small for address \n";
					 fCell |= (address & 0x7);
					 break;
				}
		  }

	  WriteCell();
}

void CCompiler::WriteCell()
{
	 int pos = ic * 3;

    char* cell = (char*)&fCell;
    while(pos > binBlk.size())
    {
       binBlk.push_back(0);
    }
    if(pos >= binBlk.size())
    {
       binBlk.push_back(cell[0]);
       binBlk.push_back(cell[1]);
       binBlk.push_back(cell[2]);
    }
    else
    {
       binBlk[pos] = cell[0];
       binBlk[pos+1] = cell[1];
       binBlk[pos+2] = cell[2];
    } 
    

    slot = 0;
	 fCell = 0;
	 ic = h;		  	  
	 h++;

}

void CCompiler::ResolveFRef(ForwardRef ref, int addr)
{
	 int cell = 0;

    int pos = ref.ic * 3;
    cell = binBlk.at(pos);
    cell |= binBlk.at(pos+1) << 8;
    cell |= binBlk.at(pos+2) << 16;

	 switch(ref.slot)
	 {
		  case 0:
		  {
				std::cout << "trying to add a then address at slot 0 \n";
				break;
		  }
		  case 1:
		  {
				if(addr & 0x3E000)				
					 std::cout << " slot 1 too small for address at fRef \n";				
				cell |= (addr & 0x3FF);
				break;
		  }
		  case 2:
		  {
				if(addr & 0x3FF00)			
					 std::cout << " slot 2 too small for address at fRef \n";
				cell |= (addr & 0xFF);
				break;
		  }
		  case 3:
		  {
				if(addr & 0x3FFFC)
					 std::cout << " slot 3 too small for address at fRef \n";
				cell |= (addr & 0x7);
				break;
		  }
	 }

    
    char* tCell = (char*)&cell;
    binBlk.at(pos) = tCell[0];
    binBlk.at(pos+1) = tCell[1];
    binBlk.at(pos+2) = tCell[2];
   

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


void CCompiler::InitWordMap()
{
	 nameMap["warm"]     = 0xA9;
	 nameMap["relay"]    = 0xA1;
	 nameMap["*.17"]     = 0xB0;
	 nameMap["*."]       = 0xB7;
	 nameMap["triangle"] = 0xCE;
	 nameMap["clc"]      = 0x2D3;
	 nameMap["--u/mod"]  = 0x2D5;
	 nameMap["-u/mod"]   = 0x2D6;
	 nameMap["interp"]   = 0xC4;
	 nameMap["taps"]     = 0xBC;
	 nameMap["poly"]     = 0xAA;
	 nameMap["lsh"]      = 0xD9;
	 nameMap["rsh"]      = 0xDB;
	 nameMap["-dac"]     = 0xBC;
	 nameMap["boot"]     = 0xAE;
    nameMap["io"]       = 0x15D;
    nameMap["data"]     = 0x141;
    nameMap["---u"]     = 0x145;
    nameMap["--l-"]     = 0x175;
    nameMap["--lu"]     = 0x165;
    nameMap["-d--"]     = 0x115;
    nameMap["-d-u"]     = 0x105;
    nameMap["-dl-"]     = 0x135;
    nameMap["-dlu"]     = 0x125;
    nameMap["r---"]     = 0x1D5;
    nameMap["r--u"]     = 0x1C5;
    nameMap["r-l-"]     = 0x1F5;
    nameMap["r-lu"]     = 0x1E5;
    nameMap["rd--"]     = 0x195;
    nameMap["rd-u"]     = 0x185;
    nameMap["rdl-"]     = 0x1B5;
    nameMap["rdlu"]     = 0x1A5;
}


void CCompiler::InitBootOrder()
{
   int node = 708;
   size_t index = 0;
   for(size_t i = 0; i < 9; i++)
   {
      node++;
      fNodeBootOrder[index] = node;     
      index++;
   }
   for(size_t i = 0; i < 3; i++)
   {      
      node -= 99;
      for(size_t j = 0; j < 17; j++)
      {
         node--;
         fNodeBootOrder[index] = node;         
         index++;
      }      
      node -= 101;
      for(size_t j = 0; j < 17; j++)
      {
         node++;
         fNodeBootOrder[index] = node;         
         index++;
      }
   }
   node -= 99;
   for(size_t j = 0; j < 18; j++)
   {
      node--;
      fNodeBootOrder[index] = node;         
      index++;
   } 
   for(size_t j = 0; j < 7; j++)
   {
      node += 100;
      fNodeBootOrder[index] = node;         
      index++;
   }
   for(size_t j = 0; j < 7; j++)
   {
      node++;
      fNodeBootOrder[index] = node;         
      index++;
   }

   //Init fGrid
   for(size_t j = 0; j < fNodeBootOrder.size(); j++)
   {
      TNode n;
      n.fRamWords = 0;
      fGrid[fNodeBootOrder.at(j)] = n;
   }

   //Boot node
   TNode n;
   n.fRamWords = 0;
   fGrid[708] = n;
}

void CCompiler::InitBootPath()
{
   size_t index = 0;
   for(size_t i = 0; i < 4; i++)
   {      
      fNodeBootPath[index] = LE;
      index++;
      fNodeBootPath[index] = RI;   
      index++;
   }

   for(size_t i = 0; i < 3; i++)
   {
      fNodeBootPath[index] = DN;
      index++;
      for(size_t j = 0; j < 8; j++)
      {
         fNodeBootPath[index] = RI;   
         index++;
         fNodeBootPath[index] = LE;
         index++;
      }   
      fNodeBootPath[index] = UP;
      index++;
      for(size_t j = 0; j < 8; j++)
      {
         fNodeBootPath[index] = LE;   
         index++;
         fNodeBootPath[index] = RI;
         index++;
      }  
   }

   fNodeBootPath[index] = DN;
   index++;
   for(size_t j = 0; j < 8; j++)
   {
      fNodeBootPath[index] = RI;   
      index++;
      fNodeBootPath[index] = LE;
      index++;
   }
   fNodeBootPath[index] = RI;   
   index++;

   for(size_t j = 0; j < 3; j++)
   {
      fNodeBootPath[index] = DN;   
      index++;
      fNodeBootPath[index] = UP;
      index++;
   }
   fNodeBootPath[index] = DN;   
   index++;

   for(size_t j = 0; j < 3; j++)
   {
      fNodeBootPath[index] = RI;   
      index++;
      fNodeBootPath[index] = LE;
      index++;
   }
   fNodeBootPath[index] = RI;   
   index++;

}


void CCompiler::PortPump( int port, int wrdCnt )// 5 words
{
	AddInst(0x08); // @p
	AddInst(0x18); // dup
	AddInst(0x1F); // a!
	AddInst(0x08); // @p

	AddInst(0x03);    // call
	AddAddress(port); // addr

	AddLit(wrdCnt - 1, false); // lit

	AddInst(0x1D); // push
	AddInst(0x0F); // !
	AddInst(0x1C); // .
	AddInst(0x1C); // .

	AddInst(0x08); // @p
	AddInst(0x0F); // !
	AddInst(0x04); // unext
	AddInst(0x1C); // .

	EndBlock();
	Reset();

}

void CCompiler::LoadPump( int port, int wrdCnt )// 5 words
{
	AddInst(0x08); // @p	
	AddInst(0x1F); // a!
	AddInst(0x08); // @p
	AddInst(0x1C); // .
	
	AddLit(port, false); // addr

	AddLit(wrdCnt - 1, false); // lit

	AddInst(0x1D); // push	
	AddInst(0x1C); // .
	AddInst(0x1C); // .
	AddInst(0x1C); // .

	AddInst(0x08); // @p
	AddInst(0x0D); // !+
	AddInst(0x04); // unext
	AddInst(0x1C); // .

	EndBlock();
}

void CCompiler::DefaultInit()// 1 words
{
   AddInst(0x02);    // call
	AddAddress(0xA9); // warm

   EndBlock();	
}



void CCompiler::WriteBootChipSeq()
{
	int pumpTotal = 0;

   //Calc total words of data we are going to port pump
	for(size_t i = 0; i < 143; i++)
   {
      //If there is code going into ram we add it and a pump
      int code = fGrid[fNodeBootOrder[i]].fCode.size() / 3;
      pumpTotal += code;
      if(fGrid[fNodeBootOrder[i]].fRamWords > 0) 
         pumpTotal += 5;
      if(code == 0)
         pumpTotal += 1;//DefaultInit
   }
   //Each node except the last and boot node gets a port pump
   pumpTotal += 142 * 5;

   EndFrame(0x1D5, 0xAE, pumpTotal); //We are ending an empty frame, this is just to set the header

   //Now we start adding the code///////
   //First the port pumps for all the nodes except the last and boot node
   for(size_t i = 0; i < 142; i++)
   {
      //Calc the total code that gets pumped through this node
      pumpTotal -= 5;// This portPump
      int code = fGrid[fNodeBootOrder[i]].fCode.size() / 3;
      pumpTotal -= code;//This nodes code
      if(fGrid[fNodeBootOrder[i]].fRamWords > 0) 
         pumpTotal -= 5;//This loadPump       
      if(code == 0)
         pumpTotal -= 1;//DefaultInit

      PortPump(fNodeBootPath[i], pumpTotal);
   }

   //Add the code for each node starting from the end of the path
    for(int i = 142; i >= 0; i--)
    {
       //If we have ram code add a loadPump
       if(fGrid[fNodeBootOrder[i]].fRamWords > 0) 
          LoadPump(0, fGrid[fNodeBootOrder[i]].fRamWords); 
       //If we have no code add defaultInit
       if(fGrid[fNodeBootOrder[i]].fCode.size() == 0)
          DefaultInit();
       else// Or write the nodes code
       if(fwrite(fGrid[fNodeBootOrder[i]].fCode.data(), 1, fGrid[fNodeBootOrder[i]].fCode.size(), binout) != fGrid[fNodeBootOrder[i]].fCode.size())		 
            std::cout << "error: failed to write block to file \n";
    }

    Reset();

	//Now add last boot frame to take care of the boot node (708)
	if(fGrid.find(708) != fGrid.end())
	{
		EndFrame(0, 0, fGrid[708].fRamWords); //We are ending an empty frame, this is just to set the header
		if(fwrite(fGrid[708].fCode.data(), 1, fGrid[708].fRamWords * 3, binout) != fGrid[708].fRamWords * 3)		 
            std::cout << "error: failed to write block to file \n";
	}

	



}