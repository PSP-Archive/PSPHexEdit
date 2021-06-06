//PSPHexEdit cFile.h - File class
#include <stdio.h>
#include <vector>
#include "../uint.h"
#define SHOWLINE fprintf(stderr, "%s:%u\n", __FILE__, __LINE__)

class cFile {
	public:
		cFile();
		cFile(FILE *File);
		~cFile();
		bool Open(FILE *File);
		void SeekTo(u32 Addr);
		int GetByte();
		int GetByteAt(u32 Addr);
		int GetBytes(u8 *Buf, int Count);
		int GetBytesAt(u32 Addr, u8 *Buf, int Count);
		void WriteByte(u8 Byte);
		void WriteByteAt(u32 Addr, u8 Byte);
		uint Save();
		bool IsEOF();
		bool IsBuffered(u32 Addr);
		u32 GetCurrentAddr();
		
		u32 Size;
		bool Saved;
		
		//Following variables are NOT set by cFile itself
		char DispName[50];
		u32 StartAddr;
		int X, Y;
	
	protected:
		FILE *File;
		u32 CurrentAddr;
		std::vector<u8> BufferData, BufferMap;
	
	private:
};
