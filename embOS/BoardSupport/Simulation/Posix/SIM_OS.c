/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 1995 - 2026 SEGGER Microcontroller GmbH                  *
*                                                                    *
*       Internet: segger.com  Support: support_embos@segger.com      *
*                                                                    *
**********************************************************************
*                                                                    *
*       embOS-Classic * Real time operating system                   *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product or a real-time            *
*       operating system for in-house use.                           *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       OS version: V5.22.0.0                                        *
*                                                                    *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
File    : SIM_OS.c
Purpose : Collection of functions to simulate the look & feel of the
          target device.
*/

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "SIM_OS.h"
#include "RTOS.h"
#include "BSP.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#define SIM_OS_TRANSPARENT_COLOR  (0xFF0000u)
#define SIM_OS_WINDOW_TITLE       "embOS Simulation"

#ifndef   SIM_OS_DEVICE_BITMAP_PATH
  #define SIM_OS_DEVICE_BITMAP_PATH  "SIM_OS_Device.bmp"
#endif

#ifndef   SIM_OS_RENDERER_FLAGS
  #define SIM_OS_RENDERER_FLAGS      SDL_RENDERER_SOFTWARE
#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static SDL_Window*   _pWindow;
static SDL_Renderer* _pRenderer;
static SDL_Texture*  _pDeviceTexture;
static SDL_atomic_t  _WindowActive;
static SDL_atomic_t  _RedrawPending;
static Uint32        _RedrawEventType;
static int           _WindowWidth;
static int           _WindowHeight;

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _FailSDL()
*/
static void _FailSDL(const char* sText) {
  const char* sError;

  sError = SDL_GetError();
  if ((sError == NULL) || (sError[0] == '\0')) {
    fprintf(stderr, "[ERROR] %s.\n", sText);
  } else {
    fprintf(stderr, "[ERROR] %s: %s\n", sText, sError);
  }
  exit(EXIT_FAILURE);
}

/*********************************************************************
*
*       _GetRed()
*/
static Uint8 _GetRed(OS_U32 Color) {
  return (Uint8)((Color >> 16) & 0xFFu);
}

/*********************************************************************
*
*       _GetGreen()
*/
static Uint8 _GetGreen(OS_U32 Color) {
  return (Uint8)((Color >> 8) & 0xFFu);
}

/*********************************************************************
*
*       _GetBlue()
*/
static Uint8 _GetBlue(OS_U32 Color) {
  return (Uint8)(Color & 0xFFu);
}

/*********************************************************************
*
*       _SetDrawColor()
*/
static void _SetDrawColor(SDL_Renderer* pRenderer, OS_U32 Color) {
  if (SDL_SetRenderDrawColor(pRenderer, _GetRed(Color), _GetGreen(Color), _GetBlue(Color), 0xFFu) != 0) {
    _FailSDL("Could not select draw color");
  }
}

/*********************************************************************
*
*       _DrawRect()
*/
static void _DrawRect(SDL_Renderer* pRenderer, int x, int y, int w, int h, OS_U32 Color) {
  SDL_Rect Rect;

  if ((w <= 0) || (h <= 0)) {
    return;
  }
  Rect.x = x;
  Rect.y = y;
  Rect.w = w;
  Rect.h = h;
  _SetDrawColor(pRenderer, Color);
  if (SDL_RenderFillRect(pRenderer, &Rect) != 0) {
    _FailSDL("Could not draw rectangle");
  }
}

/*********************************************************************
*
*       _MixColors()
*/
static OS_U32 _MixColors(OS_U32 Color, OS_U32 BkColor, unsigned Intens) {
  unsigned int R;
  unsigned int G;
  unsigned int B;

  //
  //  Calc Color separations for FgColor first
  //
  R = (Color & 0xff0000u) * Intens;
  G = (Color & 0x00ff00u) * Intens;
  B = (Color & 0x0000ffu) * Intens;
  //
  //  Add Color separations for BkColor
  //
  Intens = 256u - Intens;
  R     += (BkColor & 0xff0000u) * Intens;
  G     += (BkColor & 0x00ff00u) * Intens;
  B     += (BkColor & 0x0000ffu) * Intens;
  R      = (R >> 8) & 0xff0000u;
  G      = (G >> 8) & 0x00ff00u;
  B      = (B >> 8);
  return R + G + B;
}

/*********************************************************************
*
*       _DrawLED()
*/
static void _DrawLED(SDL_Renderer* pRenderer, int x, int y, int w, int h, OS_U32 Color) {
  int Light;
  int R;
  int G;
  int B;

  R     = (Color & 0xFF0000u) >> 16;
  G     = (Color & 0x00FF00u) >>  8;
  B     = (Color & 0x0000FFu);
  Light = ((R + G + B) > 0xA0) ? 1 : 0;
  _DrawRect(pRenderer, x,     y,     w,     h,     0);
  _DrawRect(pRenderer, x + 1, y + 1, w - 2, h - 2, _MixColors(0x000000u, Color, 100));
  _DrawRect(pRenderer, x + 2, y + 2, w - 4, h - 4, _MixColors(0x000000u, Color,  50));
  _DrawRect(pRenderer, x + 2, y + 3, w - 4, h - 6, Color);
  _DrawRect(pRenderer, x + 2, y + 5, w - 4, h -10, _MixColors(0xFFFFFFu, Color, Light ?  90u : 40u));
  _DrawRect(pRenderer, x + 3, y + 7, w - 6, h -14, _MixColors(0xFFFFFFu, Color, Light ? 150u : 75u));
}

/*********************************************************************
*
*       _LoadDeviceTexture()
*
*  Function description
*    Loads the device bitmap and creates the SDL texture used by the
*    simulation window.
*/
static void _LoadDeviceTexture(void) {
  SDL_Surface* pSurface;
  Uint32       TransparentColor;

  pSurface = SDL_LoadBMP(SIM_OS_DEVICE_BITMAP_PATH);
  if (pSurface == NULL) {
    _FailSDL("Could not load device bitmap");
  }
  _WindowWidth      = pSurface->w;
  _WindowHeight     = pSurface->h;
  TransparentColor  = SDL_MapRGB(pSurface->format,
                                 _GetRed(SIM_OS_TRANSPARENT_COLOR),
                                 _GetGreen(SIM_OS_TRANSPARENT_COLOR),
                                 _GetBlue(SIM_OS_TRANSPARENT_COLOR));
  if (SDL_SetColorKey(pSurface, SDL_TRUE, TransparentColor) != 0) {
    SDL_FreeSurface(pSurface);
    _FailSDL("Could not select device bitmap transparency");
  }
  _pDeviceTexture = SDL_CreateTextureFromSurface(_pRenderer, pSurface);
  SDL_FreeSurface(pSurface);
  if (_pDeviceTexture == NULL) {
    _FailSDL("Could not create device texture");
  }
}

/*********************************************************************
*
*       _RenderWindow()
*
*  Function description
*    Redraws the device bitmap and lets the application update the
*    variable display elements.
*/
static void _RenderWindow(void) {
  //
  // Preparations.
  //
  if (SDL_SetRenderDrawColor(_pRenderer, 0, 0, 0, 0xFFu) != 0) {
    _FailSDL("Could not select background color");
  }
  if (SDL_RenderClear(_pRenderer) != 0) {
    _FailSDL("Could not clear renderer");
  }
  //
  // Copy device image into display.
  //
  if (SDL_RenderCopy(_pRenderer, _pDeviceTexture, NULL, NULL) != 0) {
    _FailSDL("Could not draw device bitmap");
  }
  //
  // Give application a chance to draw.
  //
  SIM_OS_PaintWindow(_pRenderer);
  SDL_RenderPresent(_pRenderer);
}

/*********************************************************************
*
*       _HandleWindowEvent()
*
*  Function description
*    Handles SDL window events received by the main event loop.
*
*  Parameters
*    pEvent: A pointer to the SDL window event.
*
*  Return value
*    == 0: Window was closed.
*    == 1: Event was handled or ignored.
*/
static int _HandleWindowEvent(const SDL_WindowEvent* pEvent) {
  switch (pEvent->event) {
  case SDL_WINDOWEVENT_CLOSE:
    return 0;
  case SDL_WINDOWEVENT_EXPOSED:
  case SDL_WINDOWEVENT_SHOWN:
  case SDL_WINDOWEVENT_RESTORED:
    _RenderWindow();
    break;
  default:
    break;
  }
  return 1;
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       SIM_OS_InitWindow()
*
*  Function description
*    Initializes the POSIX device window.
*
*  Return value
*    == 1: No problem.
*/
int SIM_OS_InitWindow(void) {
  if (SDL_AtomicGet(&_WindowActive) != 0) {
    return 1;
  }
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    _FailSDL("Could not initialize SDL");
  }
  _RedrawEventType = SDL_RegisterEvents(1);
  if (_RedrawEventType == ((Uint32)-1)) {
    _FailSDL("Could not register redraw event");
  }
  //
  // Create main window.
  //
  _pWindow = SDL_CreateWindow(SIM_OS_WINDOW_TITLE,
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              1,
                              1,
                              SDL_WINDOW_SHOWN);
  if (_pWindow == NULL) {
    _FailSDL("Could not create window");
  }
  _pRenderer = SDL_CreateRenderer(_pWindow, -1, SIM_OS_RENDERER_FLAGS);
  if (_pRenderer == NULL) {
    _FailSDL("Could not create renderer");
  }
  //
  // Load device bitmap and show main window.
  //
  _LoadDeviceTexture();
  SDL_SetWindowSize(_pWindow, _WindowWidth, _WindowHeight);
  SDL_SetWindowPosition(_pWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
  SDL_AtomicSet(&_RedrawPending, 0);
  SDL_AtomicSet(&_WindowActive, 1);
  _RenderWindow();
  return 1;
}

/*********************************************************************
*
*       SIM_OS_RunWindow()
*
*  Function description
*    Handles the POSIX GUI event loop.
*
*  Return value
*    == 0: Window was closed.
*    == 1: Event loop failed.
*/
int SIM_OS_RunWindow(void) {
  SDL_Event Event;

  while (SDL_WaitEvent(&Event) != 0) {
    if (Event.type == SDL_QUIT) {
      return 0;
    }
    if (Event.type == _RedrawEventType) {
      SDL_AtomicSet(&_RedrawPending, 0);
      _RenderWindow();
      continue;
    }
    if (Event.type == SDL_WINDOWEVENT) {
      if (_HandleWindowEvent(&Event.window) == 0) {
        return 0;
      }
    }
  }
  _FailSDL("Could not wait for SDL event");
  return 1;
}

/*********************************************************************
*
*       SIM_OS_RequestQuit()
*
*  Function description
*    Requests the POSIX GUI event loop to terminate.
*/
void SIM_OS_RequestQuit(void) {
  SDL_Event Event;

  if (SDL_AtomicGet(&_WindowActive) == 0) {
    return;
  }
  SDL_zero(Event);
  Event.type = SDL_QUIT;
  if (SDL_PushEvent(&Event) != 1) {
    _FailSDL("Could not post quit event");
  }
}

/*********************************************************************
*
*       SIM_OS_ShutdownWindow()
*
*  Function description
*    Releases the POSIX device window resources.
*/
void SIM_OS_ShutdownWindow(void) {
  SDL_AtomicSet(&_WindowActive, 0);
  if (_pDeviceTexture != NULL) {
    SDL_DestroyTexture(_pDeviceTexture);
    _pDeviceTexture = NULL;
  }
  if (_pRenderer != NULL) {
    SDL_DestroyRenderer(_pRenderer);
    _pRenderer = NULL;
  }
  if (_pWindow != NULL) {
    SDL_DestroyWindow(_pWindow);
    _pWindow = NULL;
  }
  SDL_Quit();
}

/*********************************************************************
*
*       SIM_OS_PaintWindow()
*
*  Function description
*    This function is called from the callback of the main window when
*    the SDL renderer needs to be updated.
*    It gives the application a chance to draw the variable elements.
*    Per default, these are 8 LEDs on the PCB board.
*
*  Parameters
*    pContext: A pointer to the SDL renderer.
*/
void SIM_OS_PaintWindow(void* pContext) {
  SDL_Renderer* pRenderer;
  OS_U32        Color;
  double        dx;
  int           i;
  int           x;
  int           y;
  int           w;
  int           h;

  pRenderer = (SDL_Renderer*)pContext;
  //
  // Position parameters for LEDs. These depend on the image used.
  //
  x  = 213;
  y  = 46;
  w  = 10;
  h  = 20;
  dx = 17.3;
  //
  // Draw LEDs
  //
  for (i = 0; i < 8; i++) {
    Color = BSP_GetLEDState(i) ? 0x00FF80u : 0x005000u;    // Select color depending on state of LED
    _DrawLED(pRenderer, (int)(x + i * dx), y, w, h, Color);
  }
}

/*********************************************************************
*
*       SIM_OS_UpdateWindow()
*
*  Function description
*    Requests an update of the entire device on the display.
*/
void SIM_OS_UpdateWindow(void) {
  SDL_Event Event;

  if (SDL_AtomicGet(&_WindowActive) == 0) {
    return;
  }
  if (SDL_AtomicCAS(&_RedrawPending, 0, 1) == SDL_FALSE) {
    return;
  }
  SDL_zero(Event);
  Event.type = _RedrawEventType;
  if (SDL_PushEvent(&Event) != 1) {
    SDL_AtomicSet(&_RedrawPending, 0);
    _FailSDL("Could not post redraw event");
  }
}

/*************************** End of file ****************************/
