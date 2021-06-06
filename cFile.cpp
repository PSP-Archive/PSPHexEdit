//PSPHexEdit cFile.cpp - File class
#include "cFile.h"

/* Default constructor
 */
cFile::cFile()
{
	this->File = NULL;
	this->Size = 0;
}

/* Constructor with file handle
 * Inputs:
 * -File: File handle to use
 */
cFile::cFile(FILE *File)
{
	this->Open(File);
}

/* Destructor
 */
cFile::~cFile()
{
	fclose(this->File);
}

/* Sets the file handle to use.
 * Inputs:
 * -File: File handle.
 * Returns: true on sucess, false on failure.
 * Notes:
 * -The file handle will be closed when the cFile is destroyed.
 */
bool cFile::Open(FILE *File)
{
	this->Saved = true; //we haven't changed it yet
	this->CurrentAddr = 0;
	this->File = File;
	fseek(this->File, 0, SEEK_END);
	this->Size = ftell(this->File);
	rewind(this->File);
	return true;
}

/* Seeks to specified address.
 * Inputs:
 * -Addr: Address to seek to.
 */
void cFile::SeekTo(u32 Addr)
{
	fseek(this->File, Addr, SEEK_SET);
	this->CurrentAddr = ftell(File);
}

/* Reads next byte.
 * Returns: Byte read, or -1 if read failed.
 */
int cFile::GetByte()
{
	u8 Byte;
	
	if(this->IsBuffered(this->CurrentAddr))
	{
		CurrentAddr++;
		fseek(this->File, 1, SEEK_CUR); //stay in sync
		return this->BufferData[this->CurrentAddr - 1];
	}
	
	CurrentAddr++;
	if(fread(&Byte, 1, 1, this->File) != 1) return -1;
	return Byte;
}

/* Reads byte at specified position.
 * Inputs:
 * -Addr: Address to read.
 * Returns: Byte read, or -1 if read failed.
 */
int cFile::GetByteAt(u32 Addr)
{
	this->SeekTo(Addr);
	return this->GetByte();
}

/* Reads multiple bytes.
 * Inputs:
 * -Buf: Buffer to read into.
 * -Count: Number of bytes to read.
 * Returns: Number of bytes read.
 */
int cFile::GetBytes(u8 *Buf, int Count)
{
	int Byte;
	for(int i=0; i<Count; i++)
	{
		Byte = this->GetByte();
		if(Byte == -1) return i;
		Buf[i] = Byte & 0xFF;
	}
	return Count;
	//return fread(Buf, 1, Count, this->File);
}

/* Reads multiple bytes at specified position.
 * Inputs:
 * -Addr: Address to read.
 * -Buf: Buffer to read into.
 * -Count: Number of bytes to read.
 * Returns: Number of bytes read.
 */
int cFile::GetBytesAt(u32 Addr, u8 *Buf, int Count)
{
	this->SeekTo(Addr);
	return this->GetBytes(Buf, Count);
}

/* Writes next byte.
 * Inputs:
 * -Byte: Byte to write.
 */
void cFile::WriteByte(u8 Byte)
{
	this->Saved = false;
	if(this->BufferData.size() < (this->CurrentAddr + 1))
		this->BufferData.resize(this->CurrentAddr + 1, 0);
		
	if(this->BufferMap.size() < ((this->CurrentAddr >> 3) + 1))
		this->BufferMap.resize((this->CurrentAddr >> 3) + 1, 0);
		
	this->BufferData[this->CurrentAddr] = Byte;
	this->BufferMap[this->CurrentAddr >> 3] |= (1 << (this->CurrentAddr & 7));
	CurrentAddr++;
	//fwrite(&Byte, 1, 1, this->File);
}

/* Writes byte at specified position.
 * Inputs:
 * -Addr: Address to write at.
 * -Byte: Byte to write.
 */
void cFile::WriteByteAt(u32 Addr, u8 Byte)
{
	this->SeekTo(Addr);
	this->WriteByte(Byte);
}

/* Saves buffers back to file.
 * Returns: Number of bytes written.
 */
uint cFile::Save()
{
	int Count = 0, s = this->BufferData.size();
	fseek(this->File, 0, SEEK_SET);
	
	for(int i=0; i<s; i++)
	{
		if(this->IsBuffered(i))
			Count += fwrite(&this->BufferData[i], 1, 1, this->File);
		else fseek(this->File, 1, SEEK_CUR); //stay in sync
	}
	
	//fprintf(stderr, "Saved %d/%d\n", Count, s);
	if(Count)
	{
		this->BufferData.clear();
		this->BufferMap.clear();
		this->Saved = true;
	}
	return Count;
}

/* Returns true if at end of file, false if not.
 */
bool cFile::IsEOF()
{
	return feof(this->File) ? true : false;
}

/* Returns true if the specified byte is buffered, false if not.
 * Inputs:
 * -Addr: Address to check.
 */
bool cFile::IsBuffered(u32 Addr)
{
	if((this->BufferMap.size() > (Addr >> 3))
	&&(this->BufferMap[Addr >> 3] & (1 << (Addr & 7))))
		return true;
	else return false;
}

/* Returns current read address.
 */
u32 cFile::GetCurrentAddr()
{
	return this->CurrentAddr;
}
