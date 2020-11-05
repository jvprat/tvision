/*------------------------------------------------------------*/
/* filename -       hardwrvr.cpp                              */
/*                                                            */
/* function(s)                                                */
/*                  Member variables for THardwareInfo class  */
/*                                                            */
/*------------------------------------------------------------*/
/*
 *      Turbo Vision - Version 2.0
 *
 *      Copyright (c) 1994 by Borland International
 *      All Rights Reserved.
 *
 */

#define Uses_TKeys
#define Uses_TEvent
#define Uses_TScreen
#define Uses_THardwareInfo
#define Uses_TSystemError
#include <tvision/tv.h>
#include <dos.h>

#if defined( __FLAT__ )

BOOL THardwareInfo::insertState = True;
THardwareInfo::PlatformType THardwareInfo::platform = THardwareInfo::plDPMI32;
HANDLE THardwareInfo::consoleHandle[3];
DWORD THardwareInfo::consoleMode;
DWORD THardwareInfo::pendingEvent;
INPUT_RECORD THardwareInfo::irBuffer;
CONSOLE_CURSOR_INFO THardwareInfo::crInfo;
CONSOLE_SCREEN_BUFFER_INFO THardwareInfo::sbInfo;
int THardwareInfo::eventTimeoutMs = 50; // 20 FPS

const ushort THardwareInfo::ShiftCvt[89] = {
         0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0, 0x0F00,
         0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0, 0x5400, 0x5500, 0x5600, 0x5700, 0x5800,
    0x5900, 0x5A00, 0x5B00, 0x5C00, 0x5D00,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,
         0,      0, 0x0500, 0x0700,      0,      0,      0, 0x8700,
    0x8800
};

const ushort THardwareInfo::CtrlCvt[89] = {
         0,      0,      0,      0,      0,      0,      0,      0,
         0,      0,      0,      0,      0,      0,      0,      0,
    0x0011, 0x0017, 0x0005, 0x0012, 0x0014, 0x0019, 0x0015, 0x0009,
    0x000F, 0x0010,      0,      0,      0,      0, 0x0001, 0x0013,
    0x0004, 0x0006, 0x0007, 0x0008, 0x000A, 0x000B, 0x000C,      0,
         0,      0,      0,      0, 0x001A, 0x0018, 0x0003, 0x0016,
    0x0002, 0x000E, 0x000D,      0,      0,      0,      0,      0,
         0,      0,      0, 0x5E00, 0x5F00, 0x6000, 0x6100, 0x6200,
    0x6300, 0x6400, 0x6500, 0x6600, 0x6700,      0,      0, 0x7700,
    0x8D00, 0x8400,      0, 0x7300,      0, 0x7400,      0, 0x7500,
    0x9100, 0x7600, 0x0400, 0x0600,      0,      0,      0, 0x8900,
    0x8A00
};

const ushort THardwareInfo::AltCvt[89] = {
         0,      0, 0x7800, 0x7900, 0x7A00, 0x7B00, 0x7C00, 0x7D00,
    0x7E00, 0x7F00, 0x8000, 0x8100, 0x8200, 0x8300, 0x0800,      0,
    0x1000, 0x1100, 0x1200, 0x1300, 0x1400, 0x1500, 0x1600, 0x1700,
    0x1800, 0x1900,      0,      0,      0,      0, 0x1E00, 0x1F00,
    0x2000, 0x2100, 0x2200, 0x2300, 0x2400, 0x2500, 0x2600,      0,
         0,      0,      0,      0, 0x2C00, 0x2D00, 0x2E00, 0x2F00,
    0x3000, 0x3100, 0x3200,      0,      0,      0,      0,      0,
         0, 0x0200,      0, 0x6800, 0x6900, 0x6A00, 0x6B00, 0x6C00,
    0x6D00, 0x6E00, 0x6F00, 0x7000, 0x7100,      0,      0, 0x9700,
    0x9800, 0x9900,      0, 0x9B00,      0, 0x9D00,      0, 0x9F00,
    0xA000, 0xA100, 0xA200, 0xA300,      0,      0,      0, 0x8B00,
    0x8C00
};

#else

Boolean THardwareInfo::dpmiFlag;
ushort THardwareInfo::colorSel;
ushort THardwareInfo::monoSel;
ushort THardwareInfo::biosSel;

#endif

#if defined( __FLAT__ )

#ifdef __BORLANDC__

#define INT10   { __emit__( 0xCD ); __emit__( 0x10 ); }

// Constructor for 16-bit version is in HARDWARE.ASM

THardwareInfo::THardwareInfo()
{
    HMODULE mod;

    if( (mod = GetModuleHandle( "KERNEL32" )) != 0 &&
         GetProcAddress( mod, "Borland32" ) != 0
      )
        platform = plDPMI32;
    else
        platform = plWinNT;

    consoleHandle[cnInput] = GetStdHandle( STD_INPUT_HANDLE );
    consoleHandle[cnOutput] = GetStdHandle( STD_OUTPUT_HANDLE );
    if (!GetConsoleMode( consoleHandle[cnInput], &consoleMode ))
        {
        cerr << "Error: standard input is being redirected or is not a "
                "Win32 console." << endl;
        ExitProcess(1);
        }
    GetConsoleCursorInfo( consoleHandle[cnOutput], &crInfo );
    GetConsoleScreenBufferInfo( consoleHandle[cnOutput], &sbInfo );

    if( platform == plWinNT )
        {
        consoleHandle[cnStartup] = consoleHandle[cnOutput];
        consoleHandle[cnOutput] = CreateConsoleScreenBuffer(
            GENERIC_READ | GENERIC_WRITE,
            0,
            0,
            CONSOLE_TEXTMODE_BUFFER,
            0);
        // Force the screen buffer size to match the window size.
        // The Console API guarantees this, but some implementations
        // are not compliant (e.g. Wine).
        sbInfo.dwSize.X = sbInfo.srWindow.Right - sbInfo.srWindow.Left + 1;
        sbInfo.dwSize.Y = sbInfo.srWindow.Bottom - sbInfo.srWindow.Top + 1;
        SetConsoleScreenBufferSize(consoleHandle[cnOutput], sbInfo.dwSize);
        }

    consoleMode |= ENABLE_WINDOW_INPUT; // Report changes in buffer size
    consoleMode &= ~ENABLE_PROCESSED_INPUT; // Report CTRL+C and SHIFT+Arrow events.
    SetConsoleMode( consoleHandle[cnInput], consoleMode );
}

void THardwareInfo::reloadScreenBufferInfo()
{
    // Update sbInfo with the current screen buffer info.
    GetConsoleScreenBufferInfo( consoleHandle[cnOutput], &sbInfo );
}

void THardwareInfo::setUpConsole()
{
    // SetConsoleActiveScreenBuffer depends on Kernel32.dll.
    // It can't be executed in DOS mode.
    if( platform == plWinNT )
        {
        SetConsoleActiveScreenBuffer( consoleHandle[cnOutput] );
        reloadScreenBufferInfo();
        }
}

void THardwareInfo::restoreConsole()
{
    if( platform == plWinNT )
        SetConsoleActiveScreenBuffer( consoleHandle[cnStartup] );
}

ushort THardwareInfo::getScreenMode()
{
    ushort mode;

    if( platform != plDPMI32 )      // B/W, mono not supported if running on
        mode = TDisplay::smCO80;    //   NT.  Would have to use the registry.
    else
        {
        _AH = 0x0F;
        INT10;                      // Emit CD, 10.  Supported by DPMI server.
        mode = _AL;
        }

    if( getScreenRows() > 25 )
        mode |= TDisplay::smFont8x8;
    return mode;
}

void THardwareInfo::setScreenMode( ushort mode )
{
    COORD newSize = { 80, 25 };
    SMALL_RECT rect = { 0, 0, 79, 24 };

    if( mode & TDisplay::smFont8x8 )
        {
        newSize.Y = 50;
        rect.Bottom = 49;
        }
    if( platform != plDPMI32 )
        {
        COORD maxSize = GetLargestConsoleWindowSize( consoleHandle[cnOutput] );
        if( newSize.Y > maxSize.Y )
            {
            newSize.Y = maxSize.Y;
            rect.Bottom = newSize.Y-1;
            }
        }

    if( mode & TDisplay::smFont8x8 )
        {
        SetConsoleScreenBufferSize( consoleHandle[cnOutput], newSize );
        SetConsoleWindowInfo( consoleHandle[cnOutput], True, &rect );
        }
    else
        {
        SetConsoleWindowInfo( consoleHandle[cnOutput], True, &rect );
        SetConsoleScreenBufferSize( consoleHandle[cnOutput], newSize );
        }

    reloadScreenBufferInfo();
}

void THardwareInfo::setCaretPosition( ushort x, ushort y )
{
    COORD coord = { x, y };
    SetConsoleCursorPosition( consoleHandle[cnOutput], coord );
}

void THardwareInfo::setCaretSize( ushort size )
{
    if( size == 0 )
    {
        crInfo.bVisible = False;
        crInfo.dwSize = 1;
    }
    else
    {
        crInfo.bVisible = True;
        crInfo.dwSize = size;
    }

    SetConsoleCursorInfo( consoleHandle[cnOutput], &crInfo );
}

void THardwareInfo::screenWrite( ushort x, ushort y, ushort *buf, DWORD len )
{
    COORD size = {len,1};
    COORD from = {0,0};
    SMALL_RECT to = {x,y,x+len,y+1};

    WriteConsoleOutput( consoleHandle[cnOutput], (CHAR_INFO *) buf, size, from, &to);
}

// Event functions.

BOOL THardwareInfo::getMouseEvent( MouseEventType& event )
{
    if( !pendingEvent )
        {
        GetNumberOfConsoleInputEvents( consoleHandle[cnInput], &pendingEvent );
        if( pendingEvent )
            ReadConsoleInput( consoleHandle[cnInput], &irBuffer, 1, &pendingEvent );
        }

    if( pendingEvent && irBuffer.EventType == MOUSE_EVENT)
        {
        event.where.x = irBuffer.Event.MouseEvent.dwMousePosition.X;
        event.where.y = irBuffer.Event.MouseEvent.dwMousePosition.Y;
        event.buttons = irBuffer.Event.MouseEvent.dwButtonState;
        event.eventFlags = irBuffer.Event.MouseEvent.dwEventFlags;
        event.controlKeyState = irBuffer.Event.MouseEvent.dwControlKeyState;

        // Rotation sense is represented by the sign of dwButtonState's high word
        Boolean positive = !(irBuffer.Event.MouseEvent.dwButtonState & 0x80000000);
        if( irBuffer.Event.MouseEvent.dwEventFlags & MOUSE_WHEELED )
            event.wheel = positive ? mwUp : mwDown;
        else if( irBuffer.Event.MouseEvent.dwEventFlags & MOUSE_HWHEELED )
            event.wheel = positive ? mwRight : mwLeft;
        else
            event.wheel = 0;

        pendingEvent = 0;
        return True;
        }
    return False;
}

BOOL THardwareInfo::getKeyEvent( TEvent& event, Boolean blocking )
{
    if( !pendingEvent )
        {
        // Don't do busy polling. Wait for a timeout instead.
        pendingEvent = (WAIT_OBJECT_0 == WaitForSingleObject(consoleHandle[cnInput], blocking ? eventTimeoutMs : 0));
        if( pendingEvent )
            ReadConsoleInput( consoleHandle[cnInput], &irBuffer, 1, &pendingEvent );
        }

    if( pendingEvent )
        {
        if( irBuffer.EventType == KEY_EVENT && irBuffer.Event.KeyEvent.bKeyDown )
            {
            event.what = evKeyDown;
            event.keyDown.charScan.scanCode = irBuffer.Event.KeyEvent.wVirtualScanCode;
            event.keyDown.charScan.charCode = irBuffer.Event.KeyEvent.uChar.AsciiChar;
            event.keyDown.controlKeyState = irBuffer.Event.KeyEvent.dwControlKeyState;

            /* Convert NT style virtual scan codes to PC BIOS codes.
             */
            if( (event.keyDown.controlKeyState & (kbShift | kbAltShift | kbCtrlShift)) != 0 )
                {
                uchar index = irBuffer.Event.KeyEvent.wVirtualScanCode;

                if ((event.keyDown.controlKeyState & kbCtrlShift) &&
                    (event.keyDown.controlKeyState & kbAltShift)) // Ctrl+Alt is AltGr
                    {
                        // When AltGr+Key does not produce a character, a
                        // keyCode with unwanted effects may be read instead.
                        if (!event.keyDown.charScan.charCode)
                            event.keyDown.keyCode = kbNoKey;
                    }
                else if ((event.keyDown.controlKeyState & kbShift) && ShiftCvt[index] != 0)
                    event.keyDown.keyCode = ShiftCvt[index];
                else if ((event.keyDown.controlKeyState & kbCtrlShift) && CtrlCvt[index] != 0)
                    event.keyDown.keyCode = CtrlCvt[index];
                else if ((event.keyDown.controlKeyState & kbAltShift) && AltCvt[index] != 0)
                    event.keyDown.keyCode = AltCvt[index];
                }

            /* Set/Reset insert flag.
             */
            if( event.keyDown.keyCode == kbIns )
                insertState = !insertState;

            if( insertState )
                event.keyDown.controlKeyState |= kbInsState;

            pendingEvent = 0;
            return True;
            }
        // Ignore all events except mouse and buffer size events.
        // Pending mouse events will be read on the next polling loop.
        else if( irBuffer.EventType != MOUSE_EVENT )
            {
            pendingEvent = 0;
            if( irBuffer.EventType == WINDOW_BUFFER_SIZE_EVENT )
                {
                reloadScreenBufferInfo();
                event.what = evCommand;
                event.message.command = cmScreenChanged;
                return True;
                }
            }
        }

    return False;
}

#endif  // __BORLANDC__

ulong THardwareInfo::getTickCount()
{
    // To change units from ms to clock ticks.
    //   X ms * 1s/1000ms * 18.2ticks/s = X/55 ticks, roughly.
    return GetTickCount() / 55;
}

BOOL __stdcall THardwareInfo::ctrlBreakHandler( DWORD dwCtrlType )
{
    if( dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT )
        {
        TSystemError::ctrlBreakHit = True;
        return TRUE;
        }
    else
        return FALSE; // Don't handle 'CLOSE', 'LOGOFF' or 'SHUTDOWN' events.
}

#endif  // __FLAT__
