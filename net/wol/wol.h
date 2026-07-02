#pragma once

int wol_broadcast(const char *mac);

int wol_unicast(const char *mac, const char *address);