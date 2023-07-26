/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../SDL_internal.h"

#include <SDL_joystick.h>
#include <SDL_gamecontroller.h>
#include <SDL_endian.h>
#include <SDL_version.h>

#include "SDL_joystick_c.h"

/* This is included in only one place because it has a large static list of controllers */
#include "controller_type.h"
#include "usb_ids.h"

#define SDL_HARDWARE_BUS_VIRTUAL   0xFF

#if NEED_POLYFILL(2, 26, 0)

#if !SDL_VERSION_ATLEAST(2, 26, 0)
void SDL_GetJoystickGUIDInfo(SDL_JoystickGUID guid, Uint16 *vendor, Uint16 *product, Uint16 *version, Uint16 *crc16);
#endif

SDL_GameControllerType SDL_GetJoystickGameControllerTypeFromVIDPID(Uint16 vendor, Uint16 product, const char *name, SDL_bool forUI)
{
    SDL_GameControllerType type = SDL_CONTROLLER_TYPE_UNKNOWN;

    if (vendor == 0x0000 && product == 0x0000) {
        /* Some devices are only identifiable by their name */
        if (name &&
            (SDL_strcmp(name, "Lic Pro Controller") == 0 ||
             SDL_strcmp(name, "Nintendo Wireless Gamepad") == 0 ||
             SDL_strcmp(name, "Wireless Gamepad") == 0)) {
            /* HORI or PowerA Switch Pro Controller clone */
            type = SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO;
        }

    } else if (vendor == 0x0001 && product == 0x0001) {
        type = SDL_CONTROLLER_TYPE_UNKNOWN;

    } else if (vendor == USB_VENDOR_MICROSOFT && product == USB_PRODUCT_XBOX_ONE_XINPUT_CONTROLLER) {
        type = SDL_CONTROLLER_TYPE_XBOXONE;

    } else if ((vendor == USB_VENDOR_AMAZON && product == USB_PRODUCT_AMAZON_LUNA_CONTROLLER) ||
               (vendor == BLUETOOTH_VENDOR_AMAZON && product == BLUETOOTH_PRODUCT_LUNA_CONTROLLER)) {
#if SDL_VERSION_ATLEAST(2, 0, 16)
        type = SDL_CONTROLLER_TYPE_AMAZON_LUNA;
#else
        type = SDL_CONTROLLER_TYPE_XBOXONE;
#endif
    } else if (vendor == USB_VENDOR_GOOGLE && product == USB_PRODUCT_GOOGLE_STADIA_CONTROLLER) {
#if SDL_VERSION_ATLEAST(2, 0, 16)
        type = SDL_CONTROLLER_TYPE_GOOGLE_STADIA;
#else
        type = SDL_CONTROLLER_TYPE_PS5;
#endif
    } else if (vendor == USB_VENDOR_NINTENDO && product == USB_PRODUCT_NINTENDO_SWITCH_JOYCON_LEFT) {
#if SDL_VERSION_ATLEAST(2, 24, 0)
        type = SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_LEFT;
#else
        type = SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO;
#endif
    } else if (vendor == USB_VENDOR_NINTENDO && product == USB_PRODUCT_NINTENDO_SWITCH_JOYCON_RIGHT) {
        if (name && SDL_strstr(name, "NES Controller") != NULL) {
            /* We don't have a type for the Nintendo Online NES Controller */
            type = SDL_CONTROLLER_TYPE_UNKNOWN;
        } else {
#if SDL_VERSION_ATLEAST(2, 24, 0)
            type = SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT;
#else
            type = SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO;
#endif
        }

    } else if (vendor == USB_VENDOR_NINTENDO && product == USB_PRODUCT_NINTENDO_SWITCH_JOYCON_GRIP) {
#if SDL_VERSION_ATLEAST(2, 24, 0)
        if (name && SDL_strstr(name, "(L)") != NULL) {
            type = SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_LEFT;
        } else {
            type = SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT;
        }
#else
        type = SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO;
#endif

    } else if (vendor == USB_VENDOR_NINTENDO && product == USB_PRODUCT_NINTENDO_SWITCH_JOYCON_PAIR) {
#if SDL_VERSION_ATLEAST(2, 24, 0)
        type = SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR;
#else
        type = SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO;
#endif
    } else if (vendor == USB_VENDOR_NVIDIA &&
               (product == USB_PRODUCT_NVIDIA_SHIELD_CONTROLLER_V103 ||
                product == USB_PRODUCT_NVIDIA_SHIELD_CONTROLLER_V104)) {
#if SDL_VERSION_ATLEAST(2,24,0)
        type = SDL_CONTROLLER_TYPE_NVIDIA_SHIELD;
#else
        type = SDL_CONTROLLER_TYPE_XBOXONE;
#endif
    } else {
        switch (GuessControllerType(vendor, product)) {
            case k_eControllerType_XBox360Controller:
                type = SDL_CONTROLLER_TYPE_XBOX360;
                break;
            case k_eControllerType_XBoxOneController:
                type = SDL_CONTROLLER_TYPE_XBOXONE;
                break;
            case k_eControllerType_PS3Controller:
                type = SDL_CONTROLLER_TYPE_PS3;
                break;
            case k_eControllerType_PS4Controller:
                type = SDL_CONTROLLER_TYPE_PS4;
                break;
            case k_eControllerType_PS5Controller:
                type = SDL_CONTROLLER_TYPE_PS5;
                break;
            case k_eControllerType_XInputPS4Controller:
                if (forUI) {
                    type = SDL_CONTROLLER_TYPE_PS4;
                } else {
                    type = SDL_CONTROLLER_TYPE_UNKNOWN;
                }
                break;
            case k_eControllerType_SwitchProController:
            case k_eControllerType_SwitchInputOnlyController:
                type = SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO;
                break;
            case k_eControllerType_XInputSwitchController:
                if (forUI) {
                    type = SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO;
                } else {
                    type = SDL_CONTROLLER_TYPE_UNKNOWN;
                }
                break;
            default:
                break;
        }
    }
    return type;
}

SDL_GameControllerType SDL_GetJoystickGameControllerTypeFromGUID(SDL_JoystickGUID guid, const char *name)
{
    SDL_GameControllerType type;
    Uint16 vendor, product;

    SDL_GetJoystickGUIDInfo(guid, &vendor, &product, NULL, NULL);
    type = SDL_GetJoystickGameControllerTypeFromVIDPID(vendor, product, name, SDL_TRUE);
    if (type == SDL_CONTROLLER_TYPE_UNKNOWN) {
        if (SDL_IsJoystickXInput(guid)) {
            /* This is probably an Xbox One controller */
            return SDL_CONTROLLER_TYPE_XBOXONE;
        }
        if (SDL_IsJoystickVirtual(guid)) {
            return SDL_CONTROLLER_TYPE_VIRTUAL;
        }
#ifdef SDL_JOYSTICK_HIDAPI
        if (SDL_IsJoystickHIDAPI(guid)) {
            return SDL_CONTROLLER_TYPE_UNKNOWN;
        }
#endif /* SDL_JOYSTICK_HIDAPI */
    }
    return type;
}

POLYFILL void SDL_GetJoystickGUIDInfo(SDL_JoystickGUID guid, Uint16 *vendor, Uint16 *product, Uint16 *version, Uint16 *crc16)
{
    Uint16 *guid16 = (Uint16 *)guid.data;
    Uint16 bus = SDL_SwapLE16(guid16[0]);

    if ((bus < ' ' || bus == SDL_HARDWARE_BUS_VIRTUAL) && guid16[3] == 0x0000 && guid16[5] == 0x0000) {
        /* This GUID fits the standard form:
         * 16-bit bus
         * 16-bit CRC16 of the joystick name (can be zero)
         * 16-bit vendor ID
         * 16-bit zero
         * 16-bit product ID
         * 16-bit zero
         * 16-bit version
         * 8-bit driver identifier ('h' for HIDAPI, 'x' for XInput, etc.)
         * 8-bit driver-dependent type info
         */
        if (vendor) {
            *vendor = SDL_SwapLE16(guid16[2]);
        }
        if (product) {
            *product = SDL_SwapLE16(guid16[4]);
        }
        if (version) {
            *version = SDL_SwapLE16(guid16[6]);
        }
        if (crc16) {
            *crc16 = SDL_SwapLE16(guid16[1]);
        }
    } else if (bus < ' ') {
        /* This GUID fits the unknown VID/PID form:
         * 16-bit bus
         * 16-bit CRC16 of the joystick name (can be zero)
         * 11 characters of the joystick name, null terminated
         */
        if (vendor) {
            *vendor = 0;
        }
        if (product) {
            *product = 0;
        }
        if (version) {
            *version = 0;
        }
        if (crc16) {
            *crc16 = SDL_SwapLE16(guid16[1]);
        }
    } else {
        if (vendor) {
            *vendor = 0;
        }
        if (product) {
            *product = 0;
        }
        if (version) {
            *version = 0;
        }
        if (crc16) {
            *crc16 = 0;
        }
    }
}

SDL_bool SDL_IsJoystickXInput(SDL_JoystickGUID guid)
{
    return (guid.data[14] == 'x') ? SDL_TRUE : SDL_FALSE;
}

SDL_bool SDL_IsJoystickWGI(SDL_JoystickGUID guid)
{
    return (guid.data[14] == 'w') ? SDL_TRUE : SDL_FALSE;
}

SDL_bool SDL_IsJoystickHIDAPI(SDL_JoystickGUID guid)
{
    return (guid.data[14] == 'h') ? SDL_TRUE : SDL_FALSE;
}

SDL_bool SDL_IsJoystickRAWINPUT(SDL_JoystickGUID guid)
{
    return (guid.data[14] == 'r') ? SDL_TRUE : SDL_FALSE;
}

SDL_bool SDL_IsJoystickVirtual(SDL_JoystickGUID guid)
{
    return (guid.data[14] == 'v') ? SDL_TRUE : SDL_FALSE;
}

#endif