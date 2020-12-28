

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "dosbox.h"
#include "mouse.h"
#include "sdl_vmouse_image.h"
#include "sdl_vmouse.h"

#define MOUSE_SENSIVITY 4
#define MAX_MOUSE_ACCELERATION_X (MOUSE_SENSIVITY * 2)
#define MAX_MOUSE_ACCELERATION_Y MOUSE_SENSIVITY

typedef struct vmouse_buttons {
    vmouse_button_layout_t layout;
    int buttons;
    SDLKey left;
    SDLKey mid;
    SDLKey right;
    const char * info;
    const char * info_swap;
} vmouse_buttons_t ;

static vmouse_buttons_t vm_buttons[] = {
    { VM_BL_ABY,    3, SDLK_LCTRL,     SDLK_LALT,      SDLK_LSHIFT,    "l:A m:B r:Y",    "l:Y m:B r:A" },
    { VM_BL_R2L3L2, 3, SDLK_BACKSPACE, SDLK_KP_DIVIDE, SDLK_TAB,       "l:R2 m:L3 r:L2", "l:L2 m:L3 r:R2" },
    { VM_BL_R2R3L2, 3, SDLK_BACKSPACE, SDLK_KP_PERIOD, SDLK_TAB,       "l:R2 m:R3 r:L2", "l:L2 m:R3 r:R2" },
    { VM_BL_AB,     2, SDLK_LCTRL,     SDLK_FIRST,     SDLK_LALT,      "l:A r:B",        "l:B r:A" },
    { VM_BL_R2L2,   2, SDLK_BACKSPACE, SDLK_FIRST,     SDLK_TAB,       "l:R2 r:L2",      "l:L2 r:R2" },
    { VM_BL_R3L3,   2, SDLK_KP_PERIOD, SDLK_FIRST,     SDLK_KP_DIVIDE, "l:R3 r:L3",      "l:L3 r:R3" },
};

static struct {
    double x, y;
    double accelx, accely;
    double prev_accelx, prev_accely;
    
    bool left, right, up, down;
    bool enabled;
    vmouse_control_t current_control;
    bool old_enabled;
    
    SDL_Surface *cursor;
    SDL_Surface *icon;
    SDL_Surface *icon2x;
    SDL_Surface *back;
    SDL_Joystick *sdl_joystick;
    int num_axes;
    vmouse_button_layout_t buttons;
    bool swap_buttons;
    double speed;
} vmouse;

extern bool vkeyb_active;
extern bool vkeyb_disabled;

void GFX_ForceUpdate(); // in sdlmain.cpp

static void CalculateIconPosition(SDL_Surface *surface, SDL_Rect &position)
{
        position.x = surface->w - vmouse.icon->w;
        position.y = surface->h - ((surface->h > surface->w) ? vmouse.icon->h * 2 : vmouse.icon->h) ;
        position.w = vmouse.icon->w;
        position.h = (surface->h > surface->w) ? vmouse.icon->h * 2 : vmouse.icon->h;
}

void VMOUSE_Toggle(bool pressed)
{
    if (!pressed) return;

    if(vkeyb_active) return;

    VMOUSE_SetEnabled(!VMOUSE_IsEnabled());
}

void VMOUSE_Init(int bpp)
{
    vmouse.x = 0.0;
    vmouse.y = 0.0;
    vmouse.accelx = 0.0;
    vmouse.accely = 0.0;
    vmouse.prev_accelx = 0.0;
    vmouse.prev_accely = 0.0;
    vmouse.speed = 1;
    
    vmouse.left = false;
    vmouse.right = false;
    vmouse.up = false;
    vmouse.down = false;

    vmouse.buttons = VM_BL_ABY;
    vmouse.swap_buttons = false;

    vmouse.enabled = false;
    vmouse.old_enabled = false;
    
    vmouse.cursor = IMG_LoadPNG_RW(SDL_RWFromMem(pointer_image, sizeof(pointer_image)));
    vmouse.icon = IMG_LoadPNG_RW(SDL_RWFromMem(mouse_image, sizeof(mouse_image)));
    vmouse.icon2x = 0;
    vmouse.back = SDL_CreateRGBSurface(SDL_SWSURFACE,vmouse.icon->w*2,vmouse.icon->h*2,bpp,0,0,0,0);

    // Need to explicitly set width and height as they're not read from the file
    vmouse.cursor->w = 24;
    vmouse.cursor->h = 24;
    vmouse.icon->w = 16;
    vmouse.icon->h = 16;
    vmouse.current_control = VM_DPAD;
    
    vmouse.sdl_joystick = 0;
    vmouse.num_axes = 0;
    if (SDL_NumJoysticks>0) vmouse.sdl_joystick = SDL_JoystickOpen(0);
    if (vmouse.sdl_joystick) vmouse.num_axes = SDL_JoystickNumAxes(vmouse.sdl_joystick);
}

void VMOUSE_Deinit()
{
    if(vmouse.cursor) SDL_FreeSurface(vmouse.cursor);
    if(vmouse.icon) SDL_FreeSurface(vmouse.icon);
    if(vmouse.icon2x) SDL_FreeSurface(vmouse.icon2x);
    if(vmouse.sdl_joystick) SDL_JoystickClose(vmouse.sdl_joystick);
}

bool VMOUSE_IsEnabled(void)
{
    return vmouse.enabled;
}

vmouse_control_t VMOUSE_GetCurrentControl(void)
{
    return vmouse.current_control;
}

bool VMOUSE_UsingAxisControl(void)
{
    return (vmouse.current_control == VM_LEFT_STICK || vmouse.current_control == VM_RIGHT_STICK);
}

void VMOUSE_ChangeCurrentControl(bool pressed)
{
    if (!pressed) return;
    switch (vmouse.current_control) {
	case VM_DPAD:
	    if (vmouse.num_axes) {
		vmouse.current_control = VM_LEFT_STICK;
		vmouse.buttons = VM_BL_R2L3L2;
	    }
	    break;

	case VM_LEFT_STICK:
	    if (vmouse.num_axes > 2) {
		vmouse.current_control = VM_RIGHT_STICK;
		vmouse.buttons = VM_BL_R2R3L2;
	    } else {
		vmouse.current_control = VM_DPAD;
		vmouse.buttons = VM_BL_ABY;
	    }
	    break;

	case VM_RIGHT_STICK:
	default:
	    vmouse.current_control = VM_DPAD;
	    vmouse.buttons = VM_BL_ABY;
	    break;
    }
}

void VMOUSE_ChangeNumberOfButtons(bool pressed)
{
    if (!pressed) return;
    vmouse.buttons = (vmouse_button_layout_t)((int)vmouse.buttons + 1);
    if (vmouse.buttons == VM_BL_LAST)
	vmouse.buttons = VM_BL_ABY;
}

void VMOUSE_SwapButtons(bool pressed)
{
    if (!pressed) return;
    vmouse.swap_buttons = !vmouse.swap_buttons;
}

bool VMOUSE_ButtonsSwapped(void)
{
    return vmouse.swap_buttons;
}

void VMOUSE_IncreaseSpeed(bool pressed)
{
    if (!pressed) return;
    if (vmouse.speed < 5.0) vmouse.speed+=0.5;
}

void VMOUSE_DecreaseSpeed(bool pressed)
{
    if (!pressed) return;
    if (vmouse.speed > 0.5) vmouse.speed-=0.5;
}

double VMOUSE_GetSpeed(void)
{
    return vmouse.speed;
}

const char* VMOUSE_GetNumberOfButtons(void)
{
    return vmouse.swap_buttons ? vm_buttons[(int)vmouse.buttons].info_swap : vm_buttons[(int)vmouse.buttons].info;
}

void VMOUSE_SetEnabled(bool enabled)
{
    vmouse.enabled = enabled;
    
    if(!enabled)
    {
        vmouse.accelx = 0.0;
        vmouse.accely = 0.0;
        vmouse.prev_accelx = 0.0;
        vmouse.prev_accely = 0.0;

        vmouse.left = false;
        vmouse.right = false;
        vmouse.up = false;
        vmouse.down = false;
    }
    
    GFX_ForceUpdate();
}

bool VMOUSE_CheckAxisEvent(void) {
    if (!vmouse.sdl_joystick) return false;
    if(vkeyb_active) return false;
    if(!vmouse.enabled || !VMOUSE_UsingAxisControl()) return false;

    int axis = vmouse.current_control == VM_LEFT_STICK ? 0 : 2;
    Sint16 xaxis_pos = SDL_JoystickGetAxis(vmouse.sdl_joystick,axis++);
    Sint16 yaxis_pos = SDL_JoystickGetAxis(vmouse.sdl_joystick,axis);

    vmouse.accelx = (xaxis_pos/32768.0)*MAX_MOUSE_ACCELERATION_X*vmouse.speed;
    vmouse.accely = (yaxis_pos/32768.0)*MAX_MOUSE_ACCELERATION_Y*vmouse.speed;

    if (vmouse.prev_accelx != vmouse.accelx || vmouse.prev_accely != vmouse.accely) {
	vmouse.prev_accelx = vmouse.accelx;
	vmouse.prev_accely = vmouse.accely;
	GFX_ForceUpdate();
    }

    return true;
}

bool VMOUSE_CheckEvent(SDL_Event *event)
{
    if(vkeyb_active) 
    {
	if (!vmouse.old_enabled) vmouse.old_enabled = vmouse.enabled;
        vmouse.enabled = false;
        return false;
    }

    // Virtual mouse enabled previously?
    if (!vmouse.enabled && vmouse.old_enabled)
    {
	vmouse.old_enabled = false;
	VMOUSE_SetEnabled(true);
	return true;
    }

    // Enable/Disbale Virtual Mouse
    if(event->key.keysym.sym == SDLK_PAGEDOWN)
    {
        if(event->type == SDL_KEYDOWN) VMOUSE_SetEnabled(!VMOUSE_IsEnabled());
        return true;
    }

    if(!vmouse.enabled) return false;

    // Mouse emulation with Sticks don't block D-PAD Keys
    if (VMOUSE_UsingAxisControl()) {
	// With Axis control free use of DPAD keys
	if (event->key.keysym.sym == SDLK_LEFT || event->key.keysym.sym == SDLK_RIGHT ||
	    event->key.keysym.sym == SDLK_UP || event->key.keysym.sym == SDLK_DOWN )
	    return false;
    } else {
        // D-Pad Keys only when we don't use Stick
	if(event->key.keysym.sym == SDLK_LEFT) vmouse.left = (event->type != SDL_KEYUP);
	if(event->key.keysym.sym == SDLK_RIGHT) vmouse.right = (event->type != SDL_KEYUP);
	if(event->key.keysym.sym == SDLK_UP) vmouse.up = (event->type != SDL_KEYUP);
	if(event->key.keysym.sym == SDLK_DOWN) vmouse.down = (event->type != SDL_KEYUP);

	if(!vmouse.left && !vmouse.right) vmouse.accelx = 0.0;
	if(!vmouse.up && !vmouse.down) vmouse.accely = 0.0;
	if(vmouse.left) vmouse.accelx = -MOUSE_SENSIVITY*vmouse.speed;
	if(vmouse.right) vmouse.accelx = MOUSE_SENSIVITY*vmouse.speed;
	if(vmouse.up) vmouse.accely = -MOUSE_SENSIVITY*vmouse.speed;
	if(vmouse.down) vmouse.accely = MOUSE_SENSIVITY*vmouse.speed;
    }

    //Left
    int current_layout = (int)vmouse.buttons;
    int left = vmouse.swap_buttons ? 1 : 0;
    int right = vmouse.swap_buttons ? 0 : 1;
    if (vm_buttons[current_layout].left != SDLK_FIRST && vm_buttons[current_layout].left == event->key.keysym.sym)
	(event->type == SDL_KEYUP) ? Mouse_ButtonReleased(left) : Mouse_ButtonPressed(left);

    //Mid
    else if (vm_buttons[current_layout].mid != SDLK_FIRST && vm_buttons[current_layout].mid == event->key.keysym.sym)
	(event->type == SDL_KEYUP) ? Mouse_ButtonReleased(2) : Mouse_ButtonPressed(2);

    //Right
    else if (vm_buttons[current_layout].right != SDLK_FIRST && vm_buttons[current_layout].right == event->key.keysym.sym)
	(event->type == SDL_KEYUP) ? Mouse_ButtonReleased(right) : Mouse_ButtonPressed(right);

    //D-Pad Keys
    else if (event->key.keysym.sym == SDLK_LEFT || event->key.keysym.sym == SDLK_RIGHT ||
	    event->key.keysym.sym == SDLK_UP || event->key.keysym.sym == SDLK_DOWN )
	;

    //Don't block not used buttons
    else
	return false;

    GFX_ForceUpdate();
    
    return true;
}

void VMOUSE_BlitDoubledSurface(SDL_Surface *source, SDL_Surface *destination)
{
    int x, y;
    int w = source->pitch;
    int h = source->h;
    int left = 0;
    int top = 0;
    
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

SDL_Surface* load_icon2x()
{
    if(vmouse.icon2x) return vmouse.icon2x;

    vmouse.icon2x = SDL_CreateRGBSurface(SDL_SWSURFACE, vmouse.icon->w, vmouse.icon->h * 2, vmouse.icon->format->BitsPerPixel, 0, 0, 0, 0);
    
    VMOUSE_BlitDoubledSurface(vmouse.icon, vmouse.icon2x);
    
    return vmouse.icon2x;
}

void VMOUSE_BlitVMouse(SDL_Surface *surface)
{
    SDL_Rect position;
    
    if(!vmouse.enabled) return;

    vmouse.x += vmouse.accelx;
    vmouse.y += vmouse.accely;
    
    Mouse_CursorMoved(vmouse.accelx, vmouse.accely, vmouse.x, vmouse.y, true);
    
    if(!Mouse_IsHidden())
    {
        position.x = vmouse.x;
        position.y = vmouse.y;
        position.w = vmouse.cursor->w;
        position.h = vmouse.cursor->h;

        SDL_BlitSurface(vmouse.cursor, NULL, surface, &position);
    }
    else
    {
	CalculateIconPosition(surface, position);
	SDL_BlitSurface(surface, &position, vmouse.back, NULL);
        SDL_BlitSurface((surface->h > surface->w) ? load_icon2x() : vmouse.icon, NULL, surface, &position);
    }
}

void VMOUSE_CleanVMouse(SDL_Surface *surface)
{
    SDL_Rect position;

    if(!vmouse.enabled) return;
    CalculateIconPosition(surface, position);
    SDL_BlitSurface(vmouse.back, NULL, surface, &position);
//    SDL_FillRect(surface, &position, 0);
}
