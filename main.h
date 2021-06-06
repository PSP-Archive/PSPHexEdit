/* PSPHexEdit main.h
 * Created: December 15th 2008 by HyperHacker
 * Purpose: Simple hex editor.
 * Runs on: PSP.
 * License: BSD (see readme.txt).
 * 
 * Todo:
 * -Less flickering
 * -Don't show volume label as a file
 * -XMB icon
 * 
 * Possible future features:
 * -Use OSK to enter directory name
 * -If possible, list all disks (ms0, flash0, etc) in file select
 * -Bit shift and other fun things, maybe with select+buttons
 * -Copy/paste, search, resize file, all that nice stuff
 * -Sorting in file list
 * -Search enhancements:
 *  -Search all open files
 *  -Wildcard/relative/case-insensitive search
 *  -Allow best match to start beyond first byte of search string (e.g.
 *   "pudding" may be the best match for "oudding")
 * -Bookmarks
 * -Recent/favourite files list
 * -Support for character tables for displaying non-ASCII text
 * -Better rendering engine
 * -Translation into other languages
 *  -Asian characters will probably require better rendering engine
 * -View file as ASCII, MIPS assembly, etc
 * -A RAM editor plugin based on this program
 */
#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <psprtc.h>
#include <psppower.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <list>
#include "../uint.h"
#include "cFile.h"

PSP_MODULE_INFO(PSPHexEdit, 0, 1, 3);
#define BUILD_ID ((BUILD_YEAR << 13) | (BUILD_MONTH << 9) | (BUILD_DAY << 4) | BUILD_INDEX)

#define printf pspDebugScreenPrintf
#define SHOWLINE fprintf(stderr, "%s:%u\n", __FILE__, __LINE__)
#define RGB(r, g, b) ((r) | ((g) << 8) | ((b) << 16))
#define WHITE RGB(255, 255, 255)
#define RED RGB(255, 0, 0)
#define GREEN RGB(0, 255, 0)
#define BLUE RGB(0, 0, 255)
#define AQUA RGB(0, 255, 255)
#define YELLOW RGB(255, 255, 0)

//all buttons that can be read in user mode that are actually buttons
#define ALL_USER_BUTTONS (PSP_CTRL_SELECT | PSP_CTRL_START | PSP_CTRL_UP | PSP_CTRL_RIGHT | PSP_CTRL_DOWN | PSP_CTRL_LEFT | PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER | PSP_CTRL_TRIANGLE | PSP_CTRL_CIRCLE | PSP_CTRL_CROSS | PSP_CTRL_SQUARE)

#define LINE_WIDTH 16 //# bytes per line
#define NUM_LINES 32 //# lines of hex

//battery status
enum {
	BATTERY_LOW = 0,
	BATTERY_LOW_CHARGING,
	BATTERY_OK,
	BATTERY_OK_CHARGING,
	BATTERY_NONE //no battery
};

int main();
void RedrawHex();
void RedrawStatus();
void Idle(SceCtrlData *pad);
bool ShowFileSelect(char *Path, unsigned int Size);
bool OpenFile(char *Path);
void ShowMenu();
void ShowGoto();
void ShowSearch();
void ShowFileList();
bool ShowSavePrompt(cFile *File);
void WaitForButton(SceCtrlData *pad);
void WaitForButtonRelease(SceCtrlData *pad);
bool WaitForButtonTimeout(SceCtrlData *pad, int Timeout);
void Delay(int Frames);
bool IsDirectory(char *File);
int SetupCallbacks(void);
int CallbackThread(SceSize args, void *argp);
int exit_callback(int arg1, int arg2, void *common);
