
#ifndef SDL_VMOUSE_H
#define SDL_VMOUSE_H

typedef enum {
    DPAD,
    LEFT_STICK,
    RIGHT_STICK,
    LAST,
} vmouse_control_t;
void VMOUSE_Init(int bpp);
void VMOUSE_Deinit();
bool VMOUSE_IsEnabled(void);
vmouse_control_t VMOUSE_GetCurrentControl(void);
bool VMOUSE_UsingAxisControl(void);
void VMOUSE_ChangeCurrentControl(void);
void VMOUSE_ChangeNumberOfButtons(void);
int VMOUSE_GetNumberOfButtons(void);
void VMOUSE_SetEnabled(bool enabled);
bool VMOUSE_CheckAxisEvent(void);
bool VMOUSE_CheckEvent(SDL_Event *event);
void VMOUSE_BlitVMouse(SDL_Surface *surface);
void VMOUSE_CleanVMouse(SDL_Surface *surface);
void VMOUSE_Toggle(bool pressed);


#endif /* SDL_VMOUSE_H */

