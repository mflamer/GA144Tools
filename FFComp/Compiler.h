#pragma once

#include <ctype.h>
#include <iostream>
#include <stdio.h>
#include <map>
#include <string>
#include <vector>


struct ForwardRef
{
	 std::string ident;
	 int ic;
	 char slot;
};



class CCompiler
{
public:
	 CCompiler(std::string SrcFilename, std::string BinFilename);
	 ~CCompiler(void);



private:
	 void CompileFile(std::string SrcFileName);
	 bool NextTok(FILE* src);

	 void AddLit(int lit, bool call = true);
	 bool AddInst(int inst);
	 void AddAddress(int address);
	 void ResolveFRef(ForwardRef ref, int addr);
	 void WriteCell();
    bool GetWordAddress( std::string tok, int& addr );

	 bool IsLittleInst(char inst);
	 bool AddrToBig(int addr);

    void EndBlock();
    void EndFrame(int transferAddr, int completionAddr);

	 void InitMap();

	 FILE* binout;
	 int fCell;
	 int ic; // instruction counter
	 int h; // 
	 char slot; // slot counter	 

	 std::string tok;
	 std::vector<unsigned char> binBlk;

	 std::map<std::string, int> nameMap;

	 std::vector<ForwardRef> fRefs;
	 std::vector<ForwardRef> thenStack;
	 std::vector<int> forStack;
};

