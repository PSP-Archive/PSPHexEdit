PSPHexEdit v78992 by HyperHacker
http://hyperhacker.no-ip.org
reverse("moc.liamg@rekcahrepyh");

Copyright (c) 2008, HyperHacker
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of HyperNova Software nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY HYPERHACKER ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL HYPERHACKER BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.



This is (if you hadn't guessed) a hex editor for PSP. It's a bit crude and ugly
at the moment but it gets the job done.

Installation:
Drop the PSPHexEdit folder into /PSP/GAME on your memory stick, or wherever else
you would normally put homebrew. You need to have a firmware capable of running
homebrew. (Look around at www.psp-hacks.com for info on this.)
The "source" folder contains source code, it's not necessary to run the
program.


Notes/features:
-As of version 71953 files are not edited directly on disk, and must be saved
 manually.
-Press Home any time to exit.
-Date is shown in D/M/Y format, 24-hour time. Battery status changes colour:
 -Red: Low battery
 -Yellow: Low, charging
 -Green: OK
 -Blue: OK, charging
 Temperature also changes colour if the reading is unusually high or low. I'm
 only guessing at what's "unusual" based on what I've seen from mine, so just
 because it changes colour doesn't necessarily mean it's going to blow up.
-The maximum number of open files at once seems to be about 10; the file list
 uses one to read the directory and one to check each file to see whether it's
 actually a directory. So if you have too many files open the file list will
 not work, or will show directories as files; it doesn't cause any harm, just
 close one or two and it'll work again. I decided not to implement a hard limit
 since the limit could change depending on firmware version and available
 memory.
-Ayumi Hamasaki is awesome.


Bugs/"features":
-If your memory stick has a volume label, the label shows up as a file.
-It's possible to move the cursor past the end of the file if it's less than the
 number of bytes displayed on the screen. This doesn't hurt anything; it will
 just edit the last byte.
-If you try to edit a zero-byte file, rather than editing, it will append a
 random byte every time you press O/X/Square/Triangle. You can do this until the
 file's size is what you want it to be, then close and open it again to edit.
 

Menu controls:
  D-pad: Select
  O or X: Accept
  Start: Cancel/close menu

Selecting a file:
  The program starts in a simple file selector. Same as menu controls, plus:
  Triangle: Up to parent directory
  Select: Toggle ms0:/ and host0:/ (PSPLink host access). Flash is not
   accessible since the program runs in user mode; probably a good thing anyway
   since editing flash can easily brick the PSP.
  Left/right: Skip several files
  L/R: Skip to first/last file

Editing:
  The layout is like any other hex editor: address, hex, ASCII. Unsaved bytes
  are shown in yellow; the filename is also shown in yellow if there are any
  unsaved changes.
  D-pad or analog nub: Select byte
  Triangle: Increment high nybble
  Square: Decrement high nybble
  O: Increment low nybble
  X: Decrement low nybble
  L, R: Skip pages
  Start: Show menu
  Select: Show open file list
   While holding Select:
   L, R: Cycle through open files
   Start: Save current file

Main Menu:
  Save: Save changes to current file
  Goto: Jump to an address
  Open File: Open another file
  Close File: Close current file
  Auto-adjust CPU Speed: When on will automatically reduce CPU speed when
   battery is low and increase it when running on AC power. Turning it back off
   reverts to the original speed.
  Exit Program: Return to XMB/shell
  Exit Menu: Return to hex editor

Goto:
  Left, Right: Select digit
  Up, Down: Modify digit
  O, X: Accept
  Start: Cancel


Version history:
v78992 (Oct 9 2009):
-Implemented search function.
-Added estimated battery runtime to status display.
-Fixed bug that could cause battery status not to display. Changed colours used
 for charging/low states.
-Updated web address (though ironically that address is also down right now, but
 will be back up someday)

v71953 (Dec 17 2008):
-Implemented file buffering instead of editing directly on disk.
-Change colour of battery temperature if it's unusually high or low.

v71952 (Dec 17 2008):
-Can now open multiple files.
-More code cleanup (now using OOP).
-Can toggle between ms0:/ and host0:/.
-Various cosmetic fixes.

v71921 (Dec 15 2008):
-Added automatic CPU speed adjustment.
-Added more help info to menus.
-Added clock, battery and CPU status display.
-Can now use analog nub to navigate hex quickly.
-Displays '.' instead of blank for ASCII beyond end of file.
-Cleaned up code a bit.
