#pragma once

#include <firmngin.h>

// A normal component consumer: it gets Firmngin's public includes through
// REQUIRES, unlike IDF's special main component which can hide missing deps.
void run_device(Firmngin &device, const char *ssid, const char *password);
