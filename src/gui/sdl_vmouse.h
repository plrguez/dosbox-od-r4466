
#ifndef SDL_VMOUSE_H
#define SDL_VMOUSE_H

typedef enum {
    VM_DPAD,
    VM_LEFT_STICK,
    VM_RIGHT_STICK,
    VM_CONTROL_LAST,
} vmouse_control_t;

typedef enum {
    VM_BL_ABY,
    VM_BL_R2L3L2,
    VM_BL_R2R3L2,
    VM_BL_AB,
    VM_BL_R2L2,
    VM_BL_R3L3,
    VM_BL_LAST,
} vmouse_button_layout_t;

void VMOUSE_Init(int bpp);
void VMOUSE_Deinit();
bool VMOUSE_IsEnabled(void);
vmouse_control_t VMOUSE_GetCurrentControl(void);
bool VMOUSE_UsingAxisControl(void);
void VMOUSE_ChangeCurrentControl(bool pressed);
void VMOUSE_ChangeNumberOfButtons(bool pressed);
void VMOUSE_SwapButtons(bool pressed);
bool VMOUSE_ButtonsSwapped(void);
void VMOUSE_IncreaseSpeed(bool pressed);
void VMOUSE_DecreaseSpeed(bool pressed);
double VMOUSE_GetSpeed(void);
const char* VMOUSE_GetNumberOfButtons(void);
void VMOUSE_SetEnabled(bool enabled);
bool VMOUSE_CheckAxisEvent(void);
bool VMOUSE_CheckEvent(SDL_Event *event);
void VMOUSE_BlitVMouse(SDL_Surface *surface);
void VMOUSE_CleanVMouse(SDL_Surface *surface);
void VMOUSE_Toggle(bool pressed);


#endif /* SDL_VMOUSE_H */

