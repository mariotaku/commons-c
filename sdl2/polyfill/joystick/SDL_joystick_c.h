#pragma


/* Useful functions and variables from SDL_joystick.c */
#include <SDL_gamecontroller.h>
#include <SDL_joystick.h>

/* Function to return the type of a controller */
extern SDL_GameControllerType SDL_GetJoystickGameControllerTypeFromVIDPID(Uint16 vendor, Uint16 product, const char *name, SDL_bool forUI);
extern SDL_GameControllerType SDL_GetJoystickGameControllerTypeFromGUID(SDL_JoystickGUID guid, const char *name);

/* Function to return whether a joystick guid comes from the XInput driver */
extern SDL_bool SDL_IsJoystickXInput(SDL_JoystickGUID guid);

/* Function to return whether a joystick guid comes from the HIDAPI driver */
extern SDL_bool SDL_IsJoystickHIDAPI(SDL_JoystickGUID guid);

/* Function to return whether a joystick guid comes from the Virtual driver */
extern SDL_bool SDL_IsJoystickVirtual(SDL_JoystickGUID guid);