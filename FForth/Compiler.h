#pragma once

#include <ctype.h>
#include <iostream>
#include <stdio.h>
#include <map>
#include "base.h"



class CCompiler
{
public:
	 CCompiler(std::string filename);
	 ~CCompiler(void);

	 void CompileFile(std::string filename);
	



private:
	 bool NextTok(FILE* src, int c);

	 void AddLit(int lit);
	 void AddInst(char inst);
	 void AddAddress(int address);
	 void WriteCell();

	 bool IsLittleInst(char inst);
	 bool AddrToBig(int addr);

	 FILE* binout;
	 int fCell;
	 int ic; // instruction counter
	 int h; // 
	 char slot; // slot counter
	 std::string tok;
	 

	 std::map<std::string, unsigned short> nameMap;
};

