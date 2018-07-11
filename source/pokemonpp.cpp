#include <windows.h>
#include <stdint.h>

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#define local_persist static
#define global_variable static
#define internal static

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t   int8;
typedef int16_t  int16;
typedef int32_t  int32;
typedef int64_t  int64;

const int WINDOW_WIDTH  = 200;
const int WINDOW_HEIGHT = 200;

struct win_offscreen_buffer
{
    //NOTE(Josh): Pixels are always 32-bits wide. (BB GG RR xx)
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
};

struct win_window_dimensions
{
    int Width;
    int Height;
};

//TODO(Josh): Move out of global, maybe.
global_variable bool isRunning = 1;
global_variable struct win_offscreen_buffer OffscreenBuffer;

win_window_dimensions WIN_GetWindowDimensions(HWND Window)
{
    RECT ClientRect;
    win_window_dimensions Result;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;
    return(Result);
}

internal void RenderSoftware (win_offscreen_buffer *BufferBitmap, int OffsetX, int OffsetY)
{
    int Width = BufferBitmap->Width;
    int Height = BufferBitmap->Height;
    int BytesPerPixel = 4;

    int Pitch = Width*BytesPerPixel;
    uint8 *Row = (uint8 *)BufferBitmap->Memory;
    for(int Y = 0; Y < Height; ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for(int X = 0; X < Width; ++X)
        {
            uint8 r = (X + OffsetX);
            uint8 g = (Y + OffsetY);
            uint8 b = 127;

            *Pixel++ = (uint32)(r << 16 | g << 8 | b | 0xFF000000); //
            //++Pixel;++Pixel;++Pixel;
        }
        Row += Pitch;
    }
}

internal void WIN_ResizeDIBSection(win_offscreen_buffer *Buffer, int Width, int Height)
{
    if(Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;
    int BytesPerPixel = 4;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Width;
    Buffer->Info.bmiHeader.biHeight = -Height; //the 0 point is at the bottom of the bitmap when this value is positive and at the top when this is negative
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = BytesPerPixel*(Width*Height);
    Buffer->Memory = VirtualAlloc(NULL, BitmapMemorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
//    RenderSoftware(0, 0);
}

internal void WIN_DisplayBufferToWindow(HDC DeviceContext, win_offscreen_buffer *Buffer, int Width, int Height)
{
    StretchDIBits(DeviceContext,
//                  X, Y, Width, Height,
//                  X, Y, Width, Height,
                  0, 0, Width, Height,
                  0, 0, Buffer->Width, Buffer->Height,
                  Buffer->Memory, 
                  &Buffer->Info,
                  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK WIN_MainWindowCallBack(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;

    switch(Message)
    {
        case WM_SIZE:
        {
            OutputDebugStringA("WM_SIZE\n");
            #if 1 //resizing window resizes rendering backbuffer
            win_window_dimensions WinDem = WIN_GetWindowDimensions(Window);
            WIN_ResizeDIBSection(&OffscreenBuffer, WinDem.Width, WinDem.Height);
            #endif
        } break;
        
        case WM_DESTROY:
        {
            OutputDebugStringA("WM_DESTROY\n");
            isRunning = 0;
        } break;

        case WM_CLOSE:
        {
            OutputDebugStringA("WM_CLOSE\n");
            isRunning = 0;
        } break;

        case WM_QUIT:
        {
            OutputDebugStringA("WM_CLOSE\n");
            isRunning = 0;
        } break;
        
        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            win_window_dimensions WinDem = WIN_GetWindowDimensions(Window);
            WIN_DisplayBufferToWindow(DeviceContext, &OffscreenBuffer, WinDem.Width, WinDem.Height);
//            PatBlt(DeviceContext, X, Y, Width, Height, WHITENESS);
            EndPaint(Window, &Paint);

        } break;

        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return (Result);
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{
    WNDCLASSEXA WindowClass = {};


    WindowClass.cbSize = sizeof(WNDCLASSEX);
    //WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    WindowClass.lpfnWndProc = WIN_MainWindowCallBack;
    WindowClass.hInstance = Instance;
//    WindowClass.hCursor = ;
    WindowClass.hIcon = LoadIconA(0, MAKEINTRESOURCEA(32515));
    WindowClass.lpszClassName = "Pokemon++WindowClass";

    if(RegisterClassExA(&WindowClass))
    {
        RECT DesiredClientSize = {};
        DesiredClientSize.right = WINDOW_WIDTH;
        DesiredClientSize.bottom = WINDOW_HEIGHT;
        AdjustWindowRectEx(&DesiredClientSize, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, WS_OVERLAPPEDWINDOW);
        //TODO(Josh): (+ 2) is to fix off by 2 issue. Known to work on Windows 10,
        //check if this is a general issue or if a better solution needs to be found
        //2018/07/09
        #if 0 //add 2 to each dimension to compensate for windows fuckery
        int WindowWidth = DesiredClientSize.right - DesiredClientSize.left + 2;
        int WindowHeight = DesiredClientSize.bottom - DesiredClientSize.top + 2;
        #else
        int WindowWidth = DesiredClientSize.right - DesiredClientSize.left;
        int WindowHeight = DesiredClientSize.bottom - DesiredClientSize.top;
        #endif
        WIN_ResizeDIBSection(&OffscreenBuffer, DesiredClientSize.right, DesiredClientSize.bottom);
        
        HWND Window = CreateWindowExA(0,
                                           WindowClass.lpszClassName,
                                           "Mauz = sexybeast",
                                           WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            /*posX, posY, width, height*/  CW_USEDEFAULT, CW_USEDEFAULT, WindowWidth, WindowHeight,
                                           0, 0,
                                           Instance,
                                           0);

        LPDIRECTINPUT8A DInputInterface = 0;
        HRESULT DInputResult;
        DInputResult = DirectInput8Create(Instance, DIRECTINPUT_VERSION, IID_IDirectInput8A, ( VOID** )DInputInterface, 0);
       
        if( &DInputResult > 0)
        {
            OutputDebugStringA("DInput successful?\n");
        }
        else
        {
            OutputDebugStringA("DInput failed?\n");
        }
        
        //IDirectInput8A dinput;
        class DInputThing : public IDirectInput8A {
            void ConfigureDevices() = 0;
            void CreateDevice() = 0;
            void EnumDevices() = 0;
            void EnumDevicesBySemantics() = 0;
            void FindDevice() = 0;
            void GetDeviceStatus() = 0;
            void Initialize() = 0;
            void RunControlPanel() = 0;
        }

        di::Initialize(Instance, DIRECTINPUT_VERSION);

        if(Window != NULL)
        {
            int OffsetX = 0;
            int OffsetY = 0;

            isRunning = true;
            while(isRunning)
            {
                MSG Message;
                while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }
            RenderSoftware(&OffscreenBuffer, OffsetX, OffsetY);
            
            HDC DeviceContext = GetDC(Window);
            win_window_dimensions WinDem = WIN_GetWindowDimensions(Window);
            WIN_DisplayBufferToWindow(DeviceContext, &OffscreenBuffer, WinDem.Width, WinDem.Height);
            ReleaseDC(Window, DeviceContext);
            
            ++OffsetX;
            }

        }
        else
        {
            OutputDebugStringA("CreateWindowEX did not create HWND WindowHandle successfully");
        }
    }
    else
    {
        // TODO(Josh): Log error properly
        OutputDebugStringA("Could not Register WindowClass.");
    }
    return(0);
}