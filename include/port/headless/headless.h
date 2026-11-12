#ifndef HEADLESS_H
#define HEADLESS_H

// The headless port runs the game without a window, sound or frame pacing, so that
// tests can drive it: the harness (port/headless/harness.c) is called around every
// frame, sets the pads and reads the game state.

#include "port/sdl/sdl_pad.h"

// Called by the headless port. frame counts from 1.
void harness_init(void);
void harness_begin_frame(unsigned long frame); // before the game reads the pads
void harness_end_frame(unsigned long frame);   // after the game frame has run
int harness_should_quit(void);                 // nonzero ends the run
int harness_exit_code(void);

// Called by the harness: the pad state the game reads on this frame.
void headless_set_pad(int id, const SDLPad_ButtonState* state);

// Called by SYS_sub.c:Convert_User_Setting() in the headless build: nonzero when the
// harness supplies player's input word for this frame (stored in *word).
int harness_input(int player, unsigned short* word);

#endif
