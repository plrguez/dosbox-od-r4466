/*
 *  Copyright (C) 2002-2013  The DOSBox Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <cstring>
 
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_gfxPrimitives_font.h>

#include "dosbox.h"
#include "video.h"
#include "render.h"
#include "cpu.h"
#include "sdl_vmouse.h"

extern Bitu CPU_extflags_toggle;

bool dynamic_available = false;
bool menu_active = false;
bool menu_last = false;
bool keystates[1024];

typedef enum {
    MENU_RESUME,
    MENU_FRAMESKIP,
    MENU_CYCLES,
    MENU_CPU_CORE,
    MENU_CPU_TYPE,
    MENU_DOUBLE_BUFFER,
    MENU_ASPECT,
    MENU_RESOLUTION,
    MENU_SCALER,
    MENU_VMOUSE_CONTROL,
    MENU_VMOUSE_BUTTONS,
    MENU_EXIT,
} menu_items_t;

#define MENU_ITEMS 12

struct MENU_Block 
{
    SDL_Surface *surface;
    menu_items_t selected;
    char *frameskip;
    char *cycles;
    char *core;
    char *cpuType;
    bool doublebuf;
    bool aspect;
    char *fullresolution;
    char *scaler;
    char *vmouse;
    char *vmouse_buttons;
};

static MENU_Block menu;

const char *menuoptions[MENU_ITEMS] = {
    "Resume",
    "Frameskip: ",
    "Cycles: ",
    "CPU Core: ",
    "CPU Type: ",
    "Triple Buffer: ",
    "Aspect: ",
    "Resolution: ",
    "Scaler: ",
    "VMouse control: ",
    "VMouse buttons: ",
    "Exit"
};


void MENU_Init(int bpp)
{
    if(!menu.surface) 
    {
	int width, height;
	GFX_GetSupportedSize(width, height);
        menu.surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, bpp, 0, 0, 0, 0);
    }
    
    menu.selected = MENU_RESUME;
    menu.frameskip = (char*)malloc(16);
    menu.cycles = (char*)malloc(16);
    menu.core = (char*)malloc(16);
    menu.cpuType = (char*)malloc(16);
    menu.doublebuf = GFX_IsDoubleBuffering();
    menu.aspect = render.aspect;
    menu.fullresolution = (char*)malloc(17);
    menu.scaler = (char*)malloc(16);
    menu.vmouse = (char*)malloc(16);
    menu.vmouse_buttons = (char*)malloc(16);
    
#if (C_DYNREC)
    dynamic_available = true;
#endif 
}

void MENU_Deinit()
{
    free(menu.frameskip);
    free(menu.cycles);
    free(menu.core);
    free(menu.cpuType);
    free(menu.fullresolution);
    free(menu.scaler);
    free(menu.vmouse);
    free(menu.vmouse_buttons);
    SDL_FreeSurface(menu.surface);
}

void MENU_UpdateMenu()
{
    // Frameskip
    sprintf(menu.frameskip, "%i", render.frameskip.max);
    
    // CPU Cycles
    if(CPU_AutoDetermineMode & CPU_AUTODETERMINE_CYCLES) strcpy(menu.cycles, "auto");
    else if(CPU_CycleAutoAdjust) strcpy(menu.cycles, "max");
    else sprintf(menu.cycles, "%i", CPU_CycleMax);
    
    // CPU Core
    if(CPU_AutoDetermineMode & CPU_AUTODETERMINE_CORE) strcpy(menu.core, "auto");
    else if(cpudecoder == &CPU_Core_Normal_Run) strcpy(menu.core, "normal");
    else if(cpudecoder == &CPU_Core_Simple_Run) strcpy(menu.core, "simple");
    else if(cpudecoder == &CPU_Core_Full_Run) strcpy(menu.core, "full");
#if (C_DYNREC)
    else if(cpudecoder == &CPU_Core_Dynrec_Run) strcpy(menu.core, "dynamic");
#endif
    else if(cpudecoder ==  &CPU_Core_Prefetch_Run) strcpy(menu.core, "prefetch");
    else strcpy(menu.core, "unknown");
    
    if(CPU_AutoDetermineMode & CPU_AUTODETERMINE_CORE) strcpy(menu.core, "auto");
    
    // CPU Type
    if(CPU_ArchitectureType == CPU_ARCHTYPE_MIXED) strcpy(menu.cpuType, "auto");
    else if(CPU_ArchitectureType == CPU_ARCHTYPE_386FAST && cpudecoder == &CPU_Core_Prefetch_Run) strcpy(menu.cpuType, "386 (Prefetch)");
    else if(CPU_ArchitectureType == CPU_ARCHTYPE_386FAST) strcpy(menu.cpuType, "386");
    else if(CPU_ArchitectureType == CPU_ARCHTYPE_386SLOW) strcpy(menu.cpuType, "386 (Slow)");
    else if(CPU_ArchitectureType == CPU_ARCHTYPE_486NEWSLOW && cpudecoder == &CPU_Core_Prefetch_Run) strcpy(menu.cpuType, "486 (Prefetch)");
    else if(CPU_ArchitectureType == CPU_ARCHTYPE_486NEWSLOW) strcpy(menu.cpuType, "486 (Slow)");
    else if(CPU_ArchitectureType == CPU_ARCHTYPE_PENTIUMSLOW) strcpy(menu.cpuType, "Pentium (Slow)");
    else strcpy(menu.cpuType, "Unknown");

    if (GFX_IsDesktopScreenResolution()) {
	int width, height;
	GFX_GetSupportedSize(width, height);
	sprintf(menu.fullresolution,"Desktop(%dx%d)", width, height);
    } else {
	strcpy(menu.fullresolution, "Original");
    }

    // Scaler
    switch(render.scale.op ) {
	case scalerOpNormal:
	    if (render.scale.size==1)
		strcpy(menu.scaler, "none");
	    else
		strcpy(menu.scaler, "normal2x");
	    break;
#if RENDER_USE_ADVANCED_SCALERS>2
	case scalerOpAdvMame:
	    strcpy(menu.scaler, "advmame2x");
	    break;
	case scalerOpAdvInterp:
	    strcpy(menu.scaler, "advinterp2x");
	    break;
	case scalerOpHQ:
	    strcpy(menu.scaler, "hq2x");
	    break;
	case scalerOpSaI:
	    strcpy(menu.scaler, "2xsai");
	    break;
	case scalerOpSuperSaI:
	    strcpy(menu.scaler, "super2xsai");
	    break;
	case scalerOpSuperEagle:
	    strcpy(menu.scaler, "supereagle");
	    break;
#endif
#if RENDER_USE_ADVANCED_SCALERS>0
	case scalerOpTV:
	    strcpy(menu.scaler, "tv2x");
	    break;
	case scalerOpRGB:
	    strcpy(menu.scaler, "rgb2x");
	    break;
	case scalerOpScan:
	    strcpy(menu.scaler, "scan2x");
	    break;
	default:
	    break;
#endif
    }

    switch (VMOUSE_GetCurrentControl()) {
	case LEFT_STICK:
	    strcpy(menu.vmouse, "Left Stick");
	    break;
	case RIGHT_STICK:
	    strcpy(menu.vmouse, "Right Stick");
	    break;
	case DPAD:
	default:
	    strcpy(menu.vmouse, "DPad");
	    break;
    }

    // VMOUSE Number of buttons
    sprintf(menu.vmouse_buttons, "%i", VMOUSE_GetNumberOfButtons());
}

void MENU_Toggle()
{
    menu_active = !menu_active;
    
    if(!menu_active)
    {
        if(GFX_IsDoubleBuffering() != menu.doublebuf) GFX_SwitchDoubleBuffering();
	render.aspect = menu.aspect;

	if (menu.scaler == "none") { render.scale.op = scalerOpNormal;render.scale.size = 1; }
	else if (menu.scaler == "normal2x") { render.scale.op = scalerOpNormal;render.scale.size = 2; }
#if RENDER_USE_ADVANCED_SCALERS>2
	else if (menu.scaler == "advmame2x") { render.scale.op = scalerOpAdvMame;render.scale.size = 2; }
	else if (menu.scaler == "advinterp2x") { render.scale.op = scalerOpAdvInterp;render.scale.size = 2; }
	else if (menu.scaler == "hq2x") { render.scale.op = scalerOpHQ;render.scale.size = 2; }
	else if (menu.scaler == "2xsai") { render.scale.op = scalerOpSaI;render.scale.size = 2; }
	else if (menu.scaler == "super2xsai") { render.scale.op = scalerOpSuperSaI;render.scale.size = 2; }
	else if (menu.scaler == "supereagle") { render.scale.op = scalerOpSuperEagle;render.scale.size = 2; }
#endif
#if RENDER_USE_ADVANCED_SCALERS>0
	else if (menu.scaler == "tv2x") { render.scale.op = scalerOpTV;render.scale.size = 2; }
	else if (menu.scaler == "rgb2x"){ render.scale.op = scalerOpRGB;render.scale.size = 2; }
	else if (menu.scaler == "scan2x"){ render.scale.op = scalerOpScan;render.scale.size = 2; }
#endif
    }
    else
    {
        menu.doublebuf = GFX_IsDoubleBuffering();
	menu.aspect = render.aspect;
    }
    
    for(int i=0; i<1024; i++) keystates[i] = false;
    
    MENU_UpdateMenu();
}

void MENU_MoveCursor(int direction)
{
    if (direction < 0 && menu.selected == MENU_RESUME) {
	menu.selected = MENU_EXIT;
    } else if (direction > 0 && menu.selected == MENU_EXIT) {
	menu.selected = MENU_RESUME;
    } else {
	int selected = menu.selected + direction;
	menu.selected = (menu_items_t)selected;
    }
}

void MENU_Activate()
{
    switch(menu.selected)
    {
        case MENU_RESUME: // Resume
            MENU_Toggle();
            break;
            
        case MENU_CYCLES: // Cycles
            
            if(CPU_AutoDetermineMode & CPU_AUTODETERMINE_CYCLES)
            { // Max
                CPU_CycleAutoAdjust = 1;
                CPU_AutoDetermineMode ^= CPU_AUTODETERMINE_CYCLES;
            }
            else if(CPU_CycleAutoAdjust)
            { // Fixed
                CPU_CycleAutoAdjust = 0;
            }
            else
            { // Auto
                CPU_AutoDetermineMode |= CPU_AUTODETERMINE_CYCLES;
            }
            
            break;
            
        case MENU_CPU_CORE: // CPU core
            
            if(CPU_AutoDetermineMode & CPU_AUTODETERMINE_CORE) 
            {
                CPU_AutoDetermineMode ^= CPU_AUTODETERMINE_CORE;
            }
            else if(cpudecoder == &CPU_Core_Normal_Run) 
            {
                cpudecoder = &CPU_Core_Simple_Run;
            }
            else if(cpudecoder == &CPU_Core_Simple_Run) 
            {
                cpudecoder = &CPU_Core_Full_Run;
            }
#if (C_DYNREC)
            else if(cpudecoder == &CPU_Core_Full_Run && dynamic_available) 
            {
                cpudecoder = &CPU_Core_Dynrec_Run;
            }
            else if(cpudecoder == &CPU_Core_Dynrec_Run || (cpudecoder == &CPU_Core_Full_Run && !dynamic_available)) 
            {
                cpudecoder = &CPU_Core_Normal_Run;
            }
#else
            else if(cpudecoder == &CPU_Core_Full_Run) 
            {
                cpudecoder = &CPU_Core_Normal_Run;
                CPU_AutoDetermineMode |= CPU_AUTODETERMINE_CORE;
            }
#endif
            else 
            {
                cpudecoder = &CPU_Core_Normal_Run;
            }
            
            break;
            
        case MENU_CPU_TYPE: // CPU Type
            
            if(CPU_ArchitectureType == CPU_ARCHTYPE_MIXED)
            {
                CPU_ArchitectureType = CPU_ARCHTYPE_386FAST;
                CPU_PrefetchQueueSize = 16;
                cpudecoder = &CPU_Core_Prefetch_Run;
            }
            else if(CPU_ArchitectureType == CPU_ARCHTYPE_386FAST && cpudecoder == &CPU_Core_Prefetch_Run) 
            {
                cpudecoder = &CPU_Core_Normal_Run;
            }
            else if(CPU_ArchitectureType == CPU_ARCHTYPE_386FAST) 
            {
                CPU_ArchitectureType = CPU_ARCHTYPE_386SLOW;
            }
            else if(CPU_ArchitectureType == CPU_ARCHTYPE_386SLOW)
            {
                CPU_ArchitectureType = CPU_ARCHTYPE_486NEWSLOW;
                CPU_PrefetchQueueSize = 32;
                cpudecoder = &CPU_Core_Prefetch_Run;
            }
            else if(CPU_ArchitectureType == CPU_ARCHTYPE_486NEWSLOW && cpudecoder == &CPU_Core_Prefetch_Run) 
            {
                cpudecoder = &CPU_Core_Normal_Run;
            }
            else if(CPU_ArchitectureType == CPU_ARCHTYPE_486NEWSLOW) 
            {
                CPU_ArchitectureType = CPU_ARCHTYPE_PENTIUMSLOW;
            }
            else if(CPU_ArchitectureType == CPU_ARCHTYPE_PENTIUMSLOW) 
            {
                CPU_ArchitectureType = CPU_ARCHTYPE_MIXED;
            }
            
            if(CPU_ArchitectureType >= CPU_ARCHTYPE_486NEWSLOW) CPU_extflags_toggle = (FLAG_ID | FLAG_AC);
            else if(CPU_ArchitectureType >= CPU_ARCHTYPE_486OLDSLOW) CPU_extflags_toggle = (FLAG_AC);
            else CPU_extflags_toggle = 0;
            
            break;
            
        case MENU_DOUBLE_BUFFER: // Double Buffering
            menu.doublebuf = !menu.doublebuf;
            break;

        case MENU_ASPECT: // Aspect
            menu.aspect = !menu.aspect;
            break;

	case MENU_RESOLUTION: // FullScreenResolution
	    GFX_SwitchDesktopScreenResolution();
            break;

	case MENU_SCALER: // Scaler
	    ChangeScalerSize(true);
            break;

	case MENU_VMOUSE_CONTROL: // Virtual Mouse
	    VMOUSE_ChangeCurrentControl();
            break;

	case MENU_VMOUSE_BUTTONS: // Virtual Mouse
	    VMOUSE_ChangeNumberOfButtons();
            break;
            
        case MENU_EXIT: // Exit
            throw(0);
            break;
    }
    
    MENU_UpdateMenu();
}

void MENU_Increase()
{
    switch(menu.selected)
    {
        case MENU_FRAMESKIP: // Frameskip
            IncreaseFrameSkip(true);
            break;
            
        case MENU_CYCLES: // CPU cycles
            CPU_CycleIncrease(true);
            break;
    }
    
    MENU_UpdateMenu();
}

void MENU_Decrease()
{
    switch(menu.selected)
    {
        case MENU_FRAMESKIP: // Frameskip
            DecreaseFrameSkip(true);
            break;
            
        case MENU_CYCLES: // CPU cycles
            CPU_CycleDecrease(true);
            break;
    }
    
    MENU_UpdateMenu();
}

void GFX_ForceUpdate(); // in sdlmain.cpp

int MENU_CheckEvent(SDL_Event *event)
{
    bool keystate = (event->type == SDL_KEYDOWN) ? true : false;
    int sym = event->key.keysym.sym;
    
    if(!menu_active) return 0;
    
    if(keystate && !keystates[sym])
    {
        if(sym == SDLK_UP) MENU_MoveCursor(-1);
        if(sym == SDLK_DOWN) MENU_MoveCursor(1);
        if(sym == SDLK_LEFT) MENU_Decrease();
        if(sym == SDLK_RIGHT) MENU_Increase();
        if(sym == SDLK_LCTRL) MENU_Activate(); // A - normal keypress
        if(sym == SDLK_LALT) MENU_Toggle();    // B - exit
    }
    
    if(sym >= 0 && sym < 1024) keystates[sym] = keystate;
    
    return 1;
}

void MENU_BlitDoubledSurface(SDL_Surface *source, int left, int top, SDL_Surface *destination)
{
    int x, y;
    int w = source->pitch;
    int h = source->h;
    
    int bytes = source->format->BytesPerPixel;
    int offset = left * bytes;
    int trailing = destination->pitch - source->pitch - offset;
    
    uint8_t* s8 = (uint8_t*)source->pixels;
    uint64_t* s64 = (uint64_t*)source->pixels;
    
    uint8_t* d8 = (uint8_t*)destination->pixels;
    uint64_t* d64 = (uint64_t*)destination->pixels;

    // Move down the buffer to where we want to start rendering
    d8 += (destination->pitch * top);

    for(y=0; y<h; y++)
    {
        d8 += offset;
        
        for(x=0; x<w; )
        {
            if(w-x >= 64)
            {
                d64 = (uint64_t*)((void*)d8);
                s64 = (uint64_t*)((void*)s8);
                
                *d64++ = *s64++;
                *d64++ = *s64++;
                *d64++ = *s64++;
                *d64++ = *s64++;
                *d64++ = *s64++;
                *d64++ = *s64++;
                *d64++ = *s64++;
                *d64++ = *s64++;
                
                x += 64;
                d8 = (uint8_t*)((void*)d64);
                s8 = (uint8_t*)((void*)s64);
            }
            else
            {
                *d8++ = *s8++;
                x++;
            }
        }
        
        d8 += trailing;
        d8 += offset;
        s8 -= w;
        
        for(x=0; x<w; )
        {
            if(w-x >= 64)
            {
                d64 = (uint64_t*)((void*)d8);
                s64 = (uint64_t*)((void*)s8);
                
                *d64++ = *s64++;
                *d64++ = *s64++;
                *d64++ = *s64++;
                *d64++ = *s64++;
                *d64++ = *s64++;
                *d64++ = *s64++;
                *d64++ = *s64++;
                *d64++ = *s64++;
                
                x += 64;
                d8 = (uint8_t*)((void*)d64);
                s8 = (uint8_t*)((void*)s64);
            }
            else
            {
                *d8++ = *s8++;
                x++;
            }
        }

        d8 += trailing;
    }
}

void MENU_Draw(SDL_Surface *surface)
{
    int y = (surface->h - (MENU_ITEMS * 18)) / 2 + 12;
    int color = 0xFF;
    SDL_Rect dest;
    
    if(!menu_active) return;
 
    SDL_FillRect(menu.surface, NULL, SDL_MapRGB(menu.surface->format, 0x00, 0x00, 0xAA));

    for(int i=0; i<MENU_ITEMS; i++)
    {
        color = 0xFF;

	menu_items_t selected = (menu_items_t)i;
        
        if(menu.selected == selected)
        {
            dest.x = 20;
            dest.y = y-5;
            dest.w = 280;
            dest.h = 18;
            
            color = 0x00;

            SDL_FillRect(menu.surface, &dest, SDL_MapRGB(menu.surface->format, 0xFF, 0xFF, 0xFF));
        }
        
        stringRGBA(menu.surface, 40, y, menuoptions[i], color, color, color, 0xFF);
       
        if(selected == MENU_FRAMESKIP) stringRGBA(menu.surface, 165, y, menu.frameskip, color, color, color, 0xFF);
        if(selected == MENU_CYCLES) stringRGBA(menu.surface, 165, y, menu.cycles, color, color, color, 0xFF);
        if(selected == MENU_CPU_CORE) stringRGBA(menu.surface, 165, y, menu.core, color, color, color, 0xFF);
        if(selected == MENU_CPU_TYPE) stringRGBA(menu.surface, 165, y, menu.cpuType, color, color, color, 0xFF);
        if(selected == MENU_DOUBLE_BUFFER) stringRGBA(menu.surface, 165, y, menu.doublebuf ? "On" : "Off", color, color, color, 0xFF);
	if(selected == MENU_ASPECT) stringRGBA(menu.surface, 165, y, menu.aspect ? "On" : "Off", color, color, color, 0xFF);
	if(selected == MENU_RESOLUTION) stringRGBA(menu.surface, 165, y, menu.fullresolution, color, color, color, 0xFF);
	if(selected == MENU_SCALER) stringRGBA(menu.surface, 165, y, menu.scaler, color, color, color, 0xFF);
	if(selected == MENU_VMOUSE_CONTROL) stringRGBA(menu.surface, 165, y, menu.vmouse, color, color, color, 0xFF);
	if(selected == MENU_VMOUSE_BUTTONS) stringRGBA(menu.surface, 165, y, menu.vmouse_buttons, color, color, color, 0xFF);
        
        y += 17;
    }

    if(surface->h <= 240) SDL_BlitSurface(menu.surface, NULL, surface, NULL);
    else MENU_BlitDoubledSurface(menu.surface, 0, 0, surface);
    
    dest.x = 0;
    dest.y = 200;
    dest.w = 320;
    dest.h = 40;
    
    // I'm not fully sure why, but this fixes a flicker from left-over blue at the 
    // bottom of the screen. This *should* actually draw red down there. But it
    // doesn't. Best guess, this wipes out alpha information. I'll return and 
    // figure this out later.
    SDL_FillRect(menu.surface, &dest, SDL_MapRGBA(surface->format, 0x00, 0x00, 0xAA, 0xFF));
}

void MENU_CleanScreen(SDL_Surface *surface)
{
    SDL_FillRect(surface, NULL, 0);
    menu_last = false;
}

