//PSPHexEdit main.cpp
#include "main.h"

static unsigned int InitCPUSpeed = 100, InitBusSpeed = 50;
static bool CloseFile = false, AutoCPUSpeed = false;
static std::list<cFile*> File;
static std::list<cFile*>::iterator CurFileIter;
static cFile *CurFile = NULL;
static int CPUSpeeds[5] = {10, 50, 100, 200, 333}; //CPU speed for each battery status
static int AnalogDeadZone = 30, AnalogCursorDiv = 100;
static unsigned int EditAddr = 0;
static const char *Drives[] = {"ms0:/", "host0:/", NULL};

int main()
{
	SceCtrlData Pad, OldPad;
	char Filename[1024];
	bool UsingAnalog = false, IgnoreSelBtn = false, IgnoreLRBtn = false,
		IgnoreStartBtn = false, ForceRedraw = false;
	int StatusRedrawTimer = 0;
	u8 Byte;
	
	//Init
	fprintf(stderr, "PSPHexEdit v%u\n", BUILD_ID);
	memset(&OldPad, 0, sizeof(OldPad));
	InitCPUSpeed = scePowerGetCpuClockFrequency();
	InitBusSpeed = scePowerGetBusClockFrequency();
	pspDebugScreenInit();
	SetupCallbacks();
	chdir(Drives[0]);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	
	while(true) //Main loop
	{
		//Open file
		while(!CurFile)
		{
			memset(Filename, 0, sizeof(Filename));
			if(!ShowFileSelect(Filename, sizeof(Filename))) break;
			if(OpenFile(Filename)) break;
		}
		WaitForButtonRelease(&Pad);
		ForceRedraw = true;
		
		while(CurFile && !CloseFile)
		{
			//Sanity checking
			if(CurFile->StartAddr >= (CurFile->Size - (NUM_LINES * LINE_WIDTH)))
				CurFile->StartAddr = CurFile->Size - (NUM_LINES * LINE_WIDTH);
			if(CurFile->StartAddr >= CurFile->Size) CurFile->StartAddr = 0; //in case file is less than # bytes on screen
			
			EditAddr = CurFile->StartAddr + (CurFile->Y * LINE_WIDTH) + CurFile->X;
			if(EditAddr >= CurFile->Size) EditAddr = CurFile->Size - 1;
	
	
			//Redraw on any button press
			if(ForceRedraw || UsingAnalog || (Pad.Buttons & ALL_USER_BUTTONS))
				RedrawHex();
			
			//Redraw status every second
			StatusRedrawTimer--;
			if(ForceRedraw || UsingAnalog || (Pad.Buttons & ALL_USER_BUTTONS)
			|| (StatusRedrawTimer <= 0)) RedrawStatus();
			
			if(StatusRedrawTimer <= 0) StatusRedrawTimer = 12; //5-frame delay at end of loop, so 12*5 = 60
			
			ForceRedraw = false;
			
			
			//Read buttons
			memcpy(&OldPad, &Pad, sizeof(Pad));
			sceCtrlReadBufferPositive(&Pad, 1);
			
			if(Pad.Buttons & PSP_CTRL_LTRIGGER) //L: Prev page; L+Select: Prev file
			{
				if(Pad.Buttons & PSP_CTRL_SELECT)
				{
					if(!IgnoreLRBtn) //don't cycle repeatedly, it's annoying
					{
						ForceRedraw = true;
						IgnoreSelBtn = true; //don't show file list when released
						IgnoreLRBtn = true;
						if(CurFileIter == File.begin()) CurFileIter = File.end();
						CurFileIter--;
						CurFile = *CurFileIter;
					}
				}
				else if(CurFile->StartAddr >= (NUM_LINES * LINE_WIDTH))
					CurFile->StartAddr -= (NUM_LINES * LINE_WIDTH);
				else CurFile->StartAddr = 0; 
			}
			else if(Pad.Buttons & PSP_CTRL_RTRIGGER) //R: Next page; R+Select: Next file
			{
				if(Pad.Buttons & PSP_CTRL_SELECT)
				{
					if(!IgnoreLRBtn)
					{
						ForceRedraw = true;
						IgnoreSelBtn = true;
						IgnoreLRBtn = true;
						CurFileIter++;
						if(CurFileIter == File.end()) CurFileIter = File.begin();
						CurFile = *CurFileIter;
					}
				}
				else CurFile->StartAddr += (NUM_LINES * LINE_WIDTH);
			}
			else if(Pad.Buttons & PSP_CTRL_LEFT) //Left: Move left
			{
				if(CurFile->X) CurFile->X--;
				else CurFile->X = LINE_WIDTH - 1;
			}
			else if(Pad.Buttons & PSP_CTRL_RIGHT) //Right: Move right
			{
				if(CurFile->X < (int)(LINE_WIDTH - 1)) CurFile->X++;
				else CurFile->X = 0;
			}
			else if(Pad.Buttons & PSP_CTRL_UP) //Up: Move up
			{
				if(CurFile->Y) CurFile->Y--;
				else if(CurFile->StartAddr >= LINE_WIDTH)
					CurFile->StartAddr -= LINE_WIDTH; //scroll
				else CurFile->StartAddr = 0;
			}
			else if(Pad.Buttons & PSP_CTRL_DOWN) //Down: Move down
			{
				if(CurFile->Y < (int)(NUM_LINES - 1)) CurFile->Y++;
				else CurFile->StartAddr += LINE_WIDTH; //scroll
			}
			else if(Pad.Buttons & PSP_CTRL_CIRCLE) //O: Increment low nybble
			{
				Byte = CurFile->GetByteAt(EditAddr);
				CurFile->WriteByteAt(EditAddr,
					((Byte + 1) & 0xF) | (Byte & 0xF0));
			}
			else if(Pad.Buttons & PSP_CTRL_CROSS) //X: Decrement low nybble
			{
				Byte = CurFile->GetByteAt(EditAddr);
				CurFile->WriteByteAt(EditAddr,
					((Byte - 1) & 0xF) | (Byte & 0xF0));
			}
			else if(Pad.Buttons & PSP_CTRL_TRIANGLE) //^: Increment high nybble
			{
				Byte = CurFile->GetByteAt(EditAddr);
				CurFile->WriteByteAt(EditAddr,
					((Byte + 0x10) & 0xF0) | (Byte & 0xF));
			}
			else if(Pad.Buttons & PSP_CTRL_SQUARE) //[]: Decrement high nybble
			{
				Byte = CurFile->GetByteAt(EditAddr);
				CurFile->WriteByteAt(EditAddr,
					((Byte - 0x10) & 0xF0) | (Byte & 0xF));
			}
			else if(Pad.Buttons & PSP_CTRL_START) //Start: Menu; Start+Sel: Save
			{
				if(Pad.Buttons & PSP_CTRL_SELECT)
				{
					if(!IgnoreStartBtn) CurFile->Save();
					IgnoreStartBtn = true;
					IgnoreSelBtn = true;
				}
				else ShowMenu();
			}
			else if(!(Pad.Buttons & PSP_CTRL_SELECT)
			     &&(OldPad.Buttons & PSP_CTRL_SELECT)) //Sel release: File list
			{
				if(!IgnoreSelBtn) ShowFileList();
				IgnoreSelBtn = false;
				ForceRedraw = true;
				OldPad.Buttons &= ~PSP_CTRL_SELECT;
			}
			
			if(IgnoreLRBtn && !(Pad.Buttons & PSP_CTRL_LTRIGGER)
			&& !(Pad.Buttons & PSP_CTRL_RTRIGGER)) IgnoreLRBtn = false;
			
			if(IgnoreStartBtn && !(Pad.Buttons & PSP_CTRL_START))
				IgnoreStartBtn = false;
			
			
			//Analog stick
			UsingAnalog = false;
			if(Pad.Lx < (128 - AnalogDeadZone)) //Analog left: Move left
			{
				UsingAnalog = true;
				CurFile->X -= (128 - Pad.Lx) / AnalogCursorDiv;
				if(CurFile->X < 0) CurFile->X = LINE_WIDTH - 1;
			}
			else if(Pad.Lx > (128 + AnalogDeadZone)) //Analog right: Move right
			{
				UsingAnalog = true;
				CurFile->X += (128 + Pad.Lx) / AnalogCursorDiv;
				if(CurFile->X >= (int)LINE_WIDTH) CurFile->X = 0;
			}
			
			if(Pad.Ly < (128 - AnalogDeadZone)) //Analog up: Move up
			{
				UsingAnalog = true;
				CurFile->Y -= (128 - Pad.Ly) / AnalogCursorDiv;
				if(CurFile->Y < 0)
				{
					if(CurFile->StartAddr >= (uint)(-CurFile->Y * LINE_WIDTH))
						CurFile->StartAddr += CurFile->Y * LINE_WIDTH;
					else CurFile->StartAddr = 0;
					CurFile->Y = 0;
				}
			}
			else if(Pad.Ly > (128 + AnalogDeadZone)) //Analog down: Move down
			{
				UsingAnalog = true;
				CurFile->Y += (128 + Pad.Ly) / AnalogCursorDiv;
				if((uint)CurFile->Y >= NUM_LINES)
				{
					CurFile->StartAddr += (LINE_WIDTH * (CurFile->Y - NUM_LINES));
					CurFile->Y = NUM_LINES - 1;
				}
			}
			
			if(!UsingAnalog) Delay(5); //Slow down button response
		} //while(CurFile && !CloseFile)
		
		//Close file and go back to file select
		CloseFile = false;
		if(CurFileIter != File.end()) CurFileIter = File.erase(CurFileIter);
		if(CurFileIter == File.end()) CurFileIter = File.begin();
		if(CurFile) delete CurFile;
		
		if(CurFileIter != File.end()) CurFile = *CurFileIter;
		else CurFile = NULL;
	} //main loop
	
	//Should never reach this, but compilers tend to complain if it's not here
	return 0;
}

/* Redraws the hex display.
 */
void RedrawHex()
{
	unsigned int NumBytes = 0;
	unsigned char Byte[LINE_WIDTH];

	
	//Display header
	pspDebugScreenSetXY(0, 0); //instead of clearing screen - avoids flicker
	pspDebugScreenSetTextColor(GREEN);
	printf("%08X/%08X ", EditAddr, CurFile->Size - 1);
	
	if(!CurFile->Saved) pspDebugScreenSetTextColor(YELLOW);
	printf("%s\n", CurFile->DispName);
	
	
	//Read and display hex
	CurFile->SeekTo(CurFile->StartAddr);
	for(uint i=0; i<NUM_LINES; i++)
	{
		//Address
		pspDebugScreenSetTextColor(WHITE);
		NumBytes = CurFile->GetBytes(Byte, LINE_WIDTH);
		printf("%08X ", CurFile->StartAddr + (i * LINE_WIDTH));
		
		//Hex
		for(uint j=0; j<LINE_WIDTH; j++)
		{
			if((i == (uint)CurFile->Y) && (j == (uint)CurFile->X))
				pspDebugScreenSetTextColor(RED);
			else if(CurFile->IsBuffered(CurFile->StartAddr
				+ (i * LINE_WIDTH) + j)) pspDebugScreenSetTextColor(YELLOW);
			else pspDebugScreenSetTextColor(WHITE);	
			
			if(j < NumBytes) printf("%02X", Byte[j]);
			else printf("..");
			
			if((j & 3) == 3) printf(" ");
		}
		
		//ASCII
		for(uint j=0; j<LINE_WIDTH; j++)
		{
			if((i == (uint)CurFile->Y) && (j == (uint)CurFile->X))
				pspDebugScreenSetTextColor(RED);
			else pspDebugScreenSetTextColor(WHITE);
			
			if(j < NumBytes) 
			{
				printf("%c", ((Byte[j] >= 0x20) && (Byte[j] <= 0x7E))
					? Byte[j] : '.');
			}
			else printf(".");
		}
		
		printf("\n");
	}
}

/* Redraws the time/battery/CPU speed display.
 */
void RedrawStatus()
{
	int BatteryStatus, BatteryPct = 0, BatteryTemp, BatteryTime;
	pspTime Time;
	
	pspDebugScreenSetXY(0, 33);
		
	//Read and display time
	pspDebugScreenSetTextColor(GREEN);
	sceRtcGetCurrentClockLocalTime(&Time);
	printf("%02d/%d/%04d %02d:%02d:%02d ", Time.day, Time.month,
		Time.year, Time.hour, Time.minutes, Time.seconds);
		
	//Read battery status
	if(scePowerIsBatteryExist())
	{
		BatteryPct = scePowerGetBatteryLifePercent();
		if(scePowerIsLowBattery()) BatteryStatus = BATTERY_LOW;
		else BatteryStatus = BATTERY_OK;
		if(scePowerIsBatteryCharging()) BatteryStatus++;
	}
	else BatteryStatus = BATTERY_NONE;
	
	//Set text colour
	switch(BatteryStatus)
	{
		case BATTERY_LOW: pspDebugScreenSetTextColor(RED); break;
		case BATTERY_LOW_CHARGING: pspDebugScreenSetTextColor(YELLOW); break;
		case BATTERY_OK_CHARGING: pspDebugScreenSetTextColor(AQUA); break;
		default: pspDebugScreenSetTextColor(GREEN);
	}
	
	//Display status and temperature
	if(BatteryStatus == BATTERY_NONE) printf("---%% ---min --C");
	else
	{
		printf("%d%% ", BatteryPct);
		
		BatteryTime = scePowerGetBatteryLifeTime();
		if(BatteryTime > 0) printf("%dmin ", BatteryTime);
		else printf("---min ");
		
		BatteryTemp = scePowerGetBatteryTemp();
		if(BatteryTemp > 50) pspDebugScreenSetTextColor(RED);
		else if(BatteryTemp > 40) pspDebugScreenSetTextColor(YELLOW);
		else if(BatteryTemp < 20) pspDebugScreenSetTextColor(BLUE);
		else pspDebugScreenSetTextColor(GREEN);
		printf("%dC", BatteryTemp);
	}
	
	
	//Adjust CPU speed depending on battery status
	if(AutoCPUSpeed)
	{
		scePowerSetCpuClockFrequency(CPUSpeeds[BatteryStatus]);
		scePowerSetBusClockFrequency(CPUSpeeds[BatteryStatus] / 2);
	}
	
	//Display CPU speed
	pspDebugScreenSetTextColor(GREEN);
	printf(" %dmhz     ", scePowerGetCpuClockFrequency());
	//a few spaces at the end of this string will wipe out any leftover characters
	//when the string gets shorter, e.g. the battery percentage dropping from 2 to
	//1 digit
}

/* Waits for a button to be pressed, updating the time/battery display and CPU
 * speed.
 * Inputs:
 * -pad: Button status to fill in.
 */
void Idle(SceCtrlData *pad)
{
	while(true)
	{
		RedrawStatus();
		if(WaitForButtonTimeout(pad, 60)) break; //Stop looping when a button is pressed
	}
}

/* Displays the file selector. Does not return until it is closed.
 * Inputs:
 * -Path: Buffer to put file path into.
 * -Size: Size of buffer.
 * Returns: true if a file was selected, false if it was cancelled.
 */
bool ShowFileSelect(char *Path, unsigned int Size)
{
	static int DriveIdx = 0;
	DIR *dp;
    struct dirent *dirp;
    char Filename[1024], OpenName[1024], Disp[67];
    int i, Sel = 0, NumFiles = 0, Skip = 0, NumLines = 33;
    bool OpenMe = false, OpenDir = false;
    SceCtrlData pad;
	
	pspDebugScreenClear();
	while(true)
	{
		pspDebugScreenSetXY(0, 0);
		
		//Show current directory
		pspDebugScreenSetTextColor(GREEN);
		getcwd(Filename, sizeof(Filename));
		strncpy(Disp, Filename, 66); //truncate long names
		Disp[66] = '\0';
		printf("%s", Disp); //show name
		
		//Show files
		dp = opendir(".");
		if(!dp)
		{
			pspDebugScreenSetTextColor(RED);
			printf("\n * Cannot open directory");
		}
		else
		{
			NumFiles = 0;
			i = 0;
			//Read directory and print files.
			while((dirp = readdir(dp)) != NULL)
			{
				//Omit those "." and ".." pseudo-files, I don't know why it
				//even lists them
				if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
					continue;
					
				if((i >= Skip) && (i < (Skip + NumLines)))
				{
					pspDebugScreenSetTextColor(WHITE);
					printf("\n%c", (Sel == i) ? '>' : ' '); //show cursor
					//by printing the \n here, we avoid wiping out the first line
					//in a directory with 33+ files, as printing a \n at the very
					//bottom line wipes out the top.
					
					//Directories are in yellow with a slash in front
					if(IsDirectory(dirp->d_name))
					{
						pspDebugScreenSetTextColor(YELLOW);
						printf("/");
						if(OpenMe && (i == Sel)) //If this is the selected file and we're opening it
						{	//(yeah, bit of a hack, but oh well)
							OpenDir = true;
							strcpy(OpenName, dirp->d_name);
							break;
						}
					}
					else if(OpenMe && (i == Sel)) //If this is not a directory but it's selected and we're opening it
					{
						OpenDir = false;
						strncpy(OpenName, dirp->d_name, sizeof(OpenName));
					}
					
					//Display the filename
					memset(Disp, 0, sizeof(Disp));
					strncpy(Disp, dirp->d_name, 65); //truncate long names
					Disp[65] = '\0';
					printf("%s", Disp); //show name
				}
				i++;
				NumFiles++;
			}
			closedir(dp);
		}
		
		if(OpenMe) //If we're going to open this file
		{
			if(OpenDir) chdir(OpenName);
			else
			{
				pspDebugScreenClear();
				strncpy(Path, OpenName, Size);
				return true;
			}
			Sel = 0;
			WaitForButtonRelease(&pad);
			pspDebugScreenClear();
		}
		else WaitForButton(&pad);
		OpenMe = false;
		
		//Process buttons
		if(pad.Buttons & PSP_CTRL_UP) //Up: Move up
		{
			if(Sel) Sel--;
			else if(NumFiles) Sel = NumFiles - 1;
		}
		else if(pad.Buttons & PSP_CTRL_DOWN) //Down: Move down
		{
			Sel++;
			if(Sel >= NumFiles) Sel = 0;
		}
		else if(pad.Buttons & PSP_CTRL_LEFT) //Left: Move up 16
		{
			Sel -= 16;
			if(Sel < 0) Sel = 0;
		}
		else if(pad.Buttons & PSP_CTRL_RIGHT) //Right: Move down 16
		{
			Sel += 16;
			if(Sel >= NumFiles) Sel = NumFiles - 1;
			if(Sel < 0) Sel = 0; //In case there are no files here, in which
			//case Sel would be -1
		}
		else if(pad.Buttons & PSP_CTRL_LTRIGGER) //L: First file
			Sel = 0;
		else if(pad.Buttons & PSP_CTRL_RTRIGGER) //R: Last file
		{
			Sel = NumFiles - 1;
			if(Sel < 0) Sel = 0;
		}
		else if(pad.Buttons & (PSP_CTRL_CIRCLE | PSP_CTRL_CROSS)) //O or X: Open
		{
			WaitForButtonRelease(&pad);
			OpenMe = true;
			pspDebugScreenClear();
		}
		else if(pad.Buttons & PSP_CTRL_TRIANGLE) //^: Parent directory
		{
			WaitForButtonRelease(&pad);
			Sel = 0;
			chdir("..");
			pspDebugScreenClear();
		}
		else if(pad.Buttons & PSP_CTRL_START) //Start: Cancel
		{
			if(!File.empty()) //only cancel if there's at least one file already open
			{
				WaitForButtonRelease(&pad);
				pspDebugScreenClear();
				return false;
			}
		}
		else if(pad.Buttons & PSP_CTRL_SELECT) //Sel: Cycle drives
		{
			WaitForButtonRelease(&pad);
			DriveIdx++;
			if(!Drives[DriveIdx]) DriveIdx = 0;
			chdir(Drives[DriveIdx]);
			pspDebugScreenClear();
		}
		
		//Keep selection on screen
		if(Sel < Skip)
		{
			Skip = Sel;
			pspDebugScreenClear();
		}
		else if(Sel >= (Skip + NumLines))
		{
			Skip = (Sel - NumLines) + 1;
			pspDebugScreenClear();
		}
		
		Delay(5);
	}
	return false;
}

/* Opens the specified file.
 * Inputs:
 * -Path: Path to file.
 * Returns: true on success, false on failure.
 */
bool OpenFile(char *Path)
{
	FILE *NewFile;
	SceCtrlData pad;
	
	NewFile = fopen(Path, "r+b");
	if(NewFile)
	{
		//Create a cFile, push it to the list, and make it the current file
		CurFile = new cFile(NewFile);
		if(CurFile)
		{
			CurFile->X = 0;
			CurFile->Y = 0;
			CurFile->StartAddr = 0;
			File.push_front(CurFile);
			CurFileIter = File.begin();
			memset(CurFile->DispName, 0, sizeof(CurFile->DispName));
			strncpy(CurFile->DispName, Path, sizeof(CurFile->DispName)); //truncate long names
			return true;
		}
		pspDebugScreenSetTextColor(RED);
		printf("\n * Out of memory");
	}
	else //fopen() failed
	{
		pspDebugScreenSetTextColor(RED);
		printf("\n * Cannot open file");
	}
	WaitForButton(&pad);
	WaitForButtonRelease(&pad);
	return false;
}

/* Displays the menu. Does not return until it is closed. */
void ShowMenu()
{
	char Version[64], Date[64], NewPath[1024];
	const char *Options[] = {"Save", "Goto", "Search", "Open File",
		"Close File", "Auto-adjust CPU Speed: ", "Exit Program", "Exit Menu",
		"", "O/X: Accept", "Start: Cancel", "", "", Version, Date,
		"http://hyperhacker.kicks-ass.net", "hyperhacker\x40gmail\x2E" "com",
		"", "MIPS FTW", NULL};
	SceCtrlData pad;
	int i, Opt = 0, NumOpts = 8;
	bool NoExit, Done = false;
	
	sprintf(Version, APP_NAME " v%u by HyperHacker", BUILD_ID);
	sprintf(Date, "Build #%u on %02u/%u/%04u", BUILD_INDEX + 1, BUILD_DAY,
		BUILD_MONTH, BUILD_YEAR + 2000);
	WaitForButtonRelease(&pad);
	pspDebugScreenClear();
	
	while(!Done)
	{
		//Display menu
		pspDebugScreenSetXY(0, 0);
		pspDebugScreenSetTextColor(GREEN);
		printf("%u file(s)  %s", File.size(), CurFile->DispName);
		
		pspDebugScreenSetXY(2, 4);
		pspDebugScreenSetTextColor(WHITE);
		printf("Main Menu\n");
		for(i = 0; Options[i] != NULL; i++)
		{
			if(i == 5) //auto CPU speed
				printf("%c%s%s\n", (Opt == i) ? '>' : ' ', Options[i],
					AutoCPUSpeed ? "Yes" : "No");
			else printf("%c%s\n", (Opt == i) ? '>' : ' ', Options[i]);
		}
		
		//Buttons
		Idle(&pad);
		if(pad.Buttons & PSP_CTRL_UP) //Up: Move up
		{
			Opt--;
			if(Opt < 0) Opt = NumOpts - 1;
		}
		else if(pad.Buttons & PSP_CTRL_DOWN) //Down: Move down
		{
			Opt++;
			if(Opt >= NumOpts) Opt = 0;
		}
		else if(pad.Buttons & (PSP_CTRL_CIRCLE | PSP_CTRL_CROSS)) //O or X: Accept
		{
			WaitForButtonRelease(&pad);
			switch(Opt)
			{
				case 0: //Save
					pspDebugScreenSetXY(7, 5);
					pspDebugScreenSetTextColor(GREEN);
					printf("Wrote %u bytes.", CurFile->Save());
					WaitForButton(&pad);
				break;
				
				case 1: //Goto
					ShowGoto();
					Done = true;
				break;
				
				case 2: //Search
					ShowSearch();
					Done = true;
				break;
				
				case 3: //Open file
					if(!ShowFileSelect(NewPath, sizeof(NewPath))) break;
					OpenFile(NewPath);
					Done = true;
				break;
				
				case 4: //Close file
					CloseFile = ShowSavePrompt(CurFile);
					Done = true;
				break;
				
				case 5: //Auto CPU speed
					AutoCPUSpeed = !AutoCPUSpeed;
					if(!AutoCPUSpeed) //restore original settings
					{
						scePowerSetCpuClockFrequency(InitCPUSpeed);
						scePowerSetBusClockFrequency(InitBusSpeed);
					}
				break;
				
				case 6: //Exit program
					NoExit = false;
					for(std::list<cFile*>::iterator i = File.begin();
						i != File.end(); i++)
					{
						if(!ShowSavePrompt(*i))
						{
							NoExit = true; //prompt was cancelled
							break;
						}
						delete *i;
						*i = NULL;
					}
					File.remove(NULL);
					
					CurFileIter = File.begin();
					if(CurFileIter != File.end()) CurFile = *CurFileIter;
					else CurFile = NULL;
					if(!NoExit) sceKernelExitGame();
				break;
				
				default:
					Done = true;
				break;
			}
		}
		else if(pad.Buttons & PSP_CTRL_START) //Start: Close menu
		{
			WaitForButtonRelease(&pad);
			Done = true;
		}
		Delay(5);
	}
	pspDebugScreenClear();
}

/* Displays the "goto" prompt. Does not return until it is closed. */
void ShowGoto()
{
	SceCtrlData pad;
	int Pos = 7;
	unsigned int Mask, Addr, OldAddr;
	
	OldAddr = CurFile->StartAddr;
	pspDebugScreenClear();
	while(true)
	{
		//Display cursor
		pspDebugScreenSetTextColor(WHITE);
		pspDebugScreenSetXY(7, 4);
		printf("        ");
		pspDebugScreenSetXY(7 + Pos, 4);
		printf("V\n");
		
		//Display address
		printf("  Goto %08X\n  Size %08X\n\n  O/X: Accept\n  Start: Cancel\n"
			"  Left/Right: Select\n  Up/Down: Adjust",
			CurFile->StartAddr, CurFile->Size);
		if(CurFile->StartAddr >= CurFile->Size)
		{
			pspDebugScreenSetXY(16, 5);
			pspDebugScreenSetTextColor(RED);
			printf("* Past EOF");
		}
		
		//Process buttons
		Idle(&pad);
		if(pad.Buttons & PSP_CTRL_LEFT) //Left: Move left
		{
			Pos--;
			if(Pos < 0) Pos = 7;
		}
		else if(pad.Buttons & PSP_CTRL_RIGHT) //Right: Move right
		{
			Pos++;
			if(Pos > 7) Pos = 0;
		}
		else if(pad.Buttons & PSP_CTRL_UP) //Up: Increment
		{
			Mask = 0xF0000000 >> (Pos * 4);
			Addr = (CurFile->StartAddr & Mask) >> ((7 - Pos) * 4);
			Addr = (Addr + 1) & 0xF;
			CurFile->StartAddr = (CurFile->StartAddr & ~Mask)
				| (Addr << ((7 - Pos) * 4));
		}
		else if(pad.Buttons & PSP_CTRL_DOWN) //Down: Decrement
		{
			Mask = 0xF0000000 >> (Pos * 4);
			Addr = (CurFile->StartAddr & Mask) >> ((7 - Pos) * 4);
			Addr = (Addr - 1) & 0xF;
			CurFile->StartAddr = (CurFile->StartAddr & ~Mask)
				| (Addr << ((7 - Pos) * 4));
		}
		else if(pad.Buttons & (PSP_CTRL_CIRCLE | PSP_CTRL_CROSS)) //O or X: Accept
		{
			if(CurFile->StartAddr < CurFile->Size)
			{
				WaitForButtonRelease(&pad);
				break;
			}
		}
		else if(pad.Buttons & PSP_CTRL_START) //Start: cancel
		{
			WaitForButtonRelease(&pad);
			CurFile->StartAddr = OldAddr;
			break;
		}
		
		Delay(5);
	}
	
	pspDebugScreenClear();
}

/* Displays the "search" prompt. Does not return until it is closed. */
void ShowSearch()
{
	SceCtrlData pad;
	static u8 SearchByte[256];
	static u32 Pos = 0, NumBytes = 1;
	static bool Init = false;
	
	if(!Init)
	{
		memset(SearchByte, 0, sizeof(SearchByte));
		Init = true;
	}
	
	pspDebugScreenClear();
	while(true)
	{
		//Display cursor
		pspDebugScreenSetTextColor(WHITE);
		pspDebugScreenSetXY(7, 4);
		for(int i=0; i<25; i++) printf(" ");
		pspDebugScreenSetXY(7 + Pos, 4);
		printf("V\n");
		
		//Display string
		printf("  Find ");
		for(u32 i=0; i<NumBytes; i++)
			printf("%02X", SearchByte[i]);
		
		printf("\n  Text ");
		for(u32 i=0; i<NumBytes; i++)
			printf(" %c", ((SearchByte[i] >= 0x20) && (SearchByte[i] < 0x7F))
				? SearchByte[i] : '.');
		
		printf("\n\n  O/X: Accept\n  Start: Cancel\n"
			"  Left/Right: Select\n  Up/Down: Adjust\n"
			"  L/R: String length\n\n"
			"  Search range is current position (%08X) to EOF", EditAddr);
		
		//Process buttons
		Idle(&pad);
		
		if(pad.Buttons & PSP_CTRL_LEFT) //Left: Move left
		{
			Pos--;
			if((int)Pos < 0) Pos = (NumBytes * 2) - 1;
		}
		else if(pad.Buttons & PSP_CTRL_RIGHT) //Right: Move right
		{
			Pos++;
			if(Pos > ((NumBytes * 2) - 1)) Pos = 0;
		}
		else if(pad.Buttons & PSP_CTRL_UP) //Up: Increment
		{
			int i = Pos >> 1;
			u8 Mask = (Pos & 1) ? 0x0F : 0xF0, n = SearchByte[i] & Mask;
			n += (Pos & 1) ? 0x01 : 0x10;
			SearchByte[i] = (SearchByte[i] & ~Mask) | (n & Mask);
		}
		else if(pad.Buttons & PSP_CTRL_DOWN) //Down: Decrement
		{
			int i = Pos >> 1;
			u8 Mask = (Pos & 1) ? 0x0F : 0xF0, n = SearchByte[i] & Mask;
			n -= (Pos & 1) ? 0x01 : 0x10;
			SearchByte[i] = (SearchByte[i] & ~Mask) | (n & Mask);
		}
		else if(pad.Buttons & PSP_CTRL_LTRIGGER) //L: Reduce string length
		{
			if(NumBytes > 1) NumBytes--;
			if(Pos > ((NumBytes * 2) - 1)) Pos = (NumBytes * 2) - 1;
		}
		else if(pad.Buttons & PSP_CTRL_RTRIGGER) //R: Increase string length
		{
			if(NumBytes < sizeof(SearchByte)) NumBytes++;
		}
		else if(pad.Buttons & (PSP_CTRL_CIRCLE | PSP_CTRL_CROSS)) //O or X: Accept
		{
			WaitForButtonRelease(&pad);
			
			//Perform search
			u32 NumMatched = 0;
			u32 CurAddr = EditAddr;
			u32 MatchStart = 0;
			u32 BestMatch = 0, BestSize = 0;
			u32 PadReadCounter = 0;
			bool Cancelled = false;
			
			pspDebugScreenSetXY(2, 16);
			pspDebugScreenSetTextColor(GREEN);
			printf("Searching... (X: Cancel)");
			
			CurFile->SeekTo(CurAddr);
			while((NumMatched < NumBytes) && (CurAddr < CurFile->Size))
			{
				u8 Byte = CurFile->GetByte();
				
				if(Byte == SearchByte[NumMatched])
				{
					if(!NumMatched) MatchStart = CurAddr;
					NumMatched++;
					
					//Keep track of the best match so far
					if(NumMatched > BestSize)
					{
						BestMatch = MatchStart;
						BestSize = NumMatched;
					}
				}
				else NumMatched = 0;
				
				CurAddr++;
				
				//Read the buttons periodically to check for cancel
				PadReadCounter++;
				if(PadReadCounter == 32768)
				{
					PadReadCounter = 0;
					sceCtrlReadBufferPositive(&pad, 1);
					if(pad.Buttons & PSP_CTRL_CROSS)
					{
						Cancelled = true;
						break;
					}
				}
			}
			
			if(NumMatched == NumBytes) //got exact match
			{
				CurFile->StartAddr = MatchStart;
				EditAddr = MatchStart;
				break; //exit loop to return from function
			}
			else
			{
				pspDebugScreenSetXY(2, 16);
				pspDebugScreenSetTextColor(RED);
				if(Cancelled) printf("Search cancelled        \n");
				else printf("String not found        \n");
				pspDebugScreenSetTextColor(WHITE);
				printf("  Closest match: %u bytes at %08X",
					BestSize, BestMatch);
			}
			
			WaitForButtonRelease(&pad);
		}
		else if(pad.Buttons & PSP_CTRL_START) //Start: cancel
		{
			WaitForButtonRelease(&pad);
			Delay(5);
			pspDebugScreenClear();
			return;
		}
		
		Delay(5);
	}
	
	pspDebugScreenClear();
}

/* Displays list of open files. Does not return until it is closed. */
void ShowFileList()
{
	SceCtrlData pad;
	std::list<cFile*>::iterator OldCurFileIter = CurFileIter;
	
	WaitForButtonRelease(&pad);
	pspDebugScreenClear();
	
	while(true)
	{
		pspDebugScreenSetXY(0, 0);
		pspDebugScreenSetTextColor(GREEN);
		printf("Open files (%u):\n", File.size());
		
		pspDebugScreenSetTextColor(WHITE);
		for(std::list<cFile*>::iterator i=File.begin(); i != File.end(); i++)
			printf("%c%s\n", (i == CurFileIter) ? '>' : ' ', (*i)->DispName);
			
		Idle(&pad);
		
		if(pad.Buttons & PSP_CTRL_UP) //Up: Move up
		{
			if(CurFileIter == File.begin()) CurFileIter = File.end();
			CurFileIter--;
		}
		else if(pad.Buttons & PSP_CTRL_DOWN) //Down: Move down
		{
			CurFileIter++;
			if(CurFileIter == File.end()) CurFileIter = File.begin();
		}
		else if(pad.Buttons & (PSP_CTRL_CIRCLE | PSP_CTRL_CROSS | PSP_CTRL_START)) //O, X, or Start: Accept
			break;
		else if(pad.Buttons & PSP_CTRL_SELECT) //Sel: Cancel
		{
			CurFileIter = OldCurFileIter;
			break;
		}
		Delay(10);
	}
	WaitForButtonRelease(&pad);
	CurFile = *CurFileIter;
}

/* Displays a save prompt for the specified file if it has unsaved changes.
 * Does not return until it is closed.
 * Returns: true if OK, false if cancelled.
 */
bool ShowSavePrompt(cFile *File)
{
	SceCtrlData Pad;
	bool Ret = true;
	
	if(File->Saved) return true;
	WaitForButtonRelease(&Pad);
	pspDebugScreenClear();
	pspDebugScreenSetXY(0, 0);
	pspDebugScreenSetTextColor(YELLOW);
	printf("Save changes to\n%s?\nO or X: Yes\nTriangle: No\nSquare: Cancel\n",
		File->DispName);
	
	while(!(Pad.Buttons &
		(PSP_CTRL_CROSS | PSP_CTRL_CIRCLE | PSP_CTRL_TRIANGLE | PSP_CTRL_SQUARE)))
		Idle(&Pad);
	
	if(Pad.Buttons & (PSP_CTRL_CROSS | PSP_CTRL_CIRCLE))
	{
		pspDebugScreenSetXY(0, 6);
		pspDebugScreenSetTextColor(GREEN);
		printf("Wrote %u bytes.\n", File->Save());
	}
	else if(Pad.Buttons & PSP_CTRL_SQUARE) Ret = false;
	
	WaitForButtonRelease(&Pad);
	Delay(30);
	pspDebugScreenClear();
	return Ret;
}

/* Waits for a button to be pressed.
 * Inputs:
 * -pad: Button status to fill in when pressed.
 */
void WaitForButton(SceCtrlData *pad)
{
	do {
		sceDisplayWaitVblankStart();
		sceCtrlReadBufferPositive(pad, 1);
	} while(!(pad->Buttons & ALL_USER_BUTTONS)
		&& (pad->Lx > (128 - AnalogDeadZone))
		&& (pad->Lx < (128 + AnalogDeadZone))
		&& (pad->Ly > (128 - AnalogDeadZone))
		&& (pad->Ly < (128 + AnalogDeadZone)));
}

/* Waits for all buttons to be released.
 * Inputs:
 * -pad: Button status to fill in when released.
 */
void WaitForButtonRelease(SceCtrlData *pad)
{
	do {
		sceDisplayWaitVblankStart();
		sceCtrlReadBufferPositive(pad, 1);
	} while((pad->Buttons & ALL_USER_BUTTONS)
		|| (pad->Lx < (128 - AnalogDeadZone))
		|| (pad->Lx > (128 + AnalogDeadZone))
		|| (pad->Ly < (128 - AnalogDeadZone))
		|| (pad->Ly > (128 + AnalogDeadZone)));
}

/* Waits for a button to be pressed, up to the specified number of frames.
 * Inputs:
 * -pad: Button status to fill in when pressed.
 * -Timeout: Number of frames to wait.
 * Returns: true if a button was pressed, false if timeout expired.
 */
bool WaitForButtonTimeout(SceCtrlData *pad, int Timeout)
{
	for(; Timeout > 0; Timeout--)
	{
		sceDisplayWaitVblankStart();
		sceCtrlReadBufferPositive(pad, 1);
		if((pad->Buttons & ALL_USER_BUTTONS)
		|| (pad->Lx < (128 - AnalogDeadZone))
		|| (pad->Lx > (128 + AnalogDeadZone))
		|| (pad->Ly < (128 - AnalogDeadZone))
		|| (pad->Ly > (128 + AnalogDeadZone))) return true;
	}
	return false;
}

/* Delays for the specified number of frames.
 */
void Delay(int Frames)
{
	int i;
	for(i=0; i<Frames; i++) sceDisplayWaitVblankStart();
}

/* Checks whether the specified file is a directory.
 * Inputs:
 * -File: Filename to check.
 * Returns: true if directory, false if not.
 */
bool IsDirectory(char *File)
{
	struct stat st;
	if(stat(File, &st) == 0 && S_ISDIR(st.st_mode)) return true;
	else if(!strcmp(File, "..")) return true; //for some reason this isn't listed as a directory O_o
	else return false;
}

/* Exit callback
 */
int exit_callback(int arg1, int arg2, void *common)
{
	for(std::list<cFile*>::iterator i = File.begin(); i != File.end(); i++)
		delete *i;
	File.clear();
	sceKernelExitGame();
	return 0;
}

/* Callback thread
 */
int CallbackThread(SceSize args, void *argp)
{
	int cbid;

	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}

/* Sets up the callback thread and returns its thread id
 */
int SetupCallbacks(void)
{
	int thid = 0;

	thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0) sceKernelStartThread(thid, 0, 0);
	return thid;
}