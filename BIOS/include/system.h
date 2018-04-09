#pragma once

#define frames * 1
#define seconds * 60 frames
#define minutes * 60 seconds
#define hours * 60 minutes

inline void halt() {
	__asm__("halt");
}

typedef void(*function)();

void jump(function f);
// Seriously, don't use if you don't know what you're doing.
void jumpWithoutStack(function f);

char getKeys();

void shutdown();
void reset();
