#pragma once

// Fixups for the port builds.
//
// WebAssembly keeps one signature per function in the function table, and a
// call that does not match the signature of the function is an error there.
// A few functions of the game and of the CRI middleware are called with
// arguments that their implementation ignores, while the implementation does
// not take them - the arguments are dropped here, so that the call and the
// definition agree.
//
// On the desktop the call is a direct one and the extra argument is simply
// ignored, which is why the retail build never noticed.
//
// The header is only part of the port builds: the retail build is compiled
// without it and stays exactly as it is.

#if !defined(TARGET_PS2)

// The purpara of the game: the implementations do nothing on the port.
#define pp_pulpara_remake_at_init(...) pp_pulpara_remake_at_init()
#define pp_pulpara_remake_at_init2(...) pp_pulpara_remake_at_init2()
#define pp_pulpara_remake_at_hit(...) pp_pulpara_remake_at_hit()
#define pp_pulpara_remake_at(...) pp_pulpara_remake_at()

// The effect of the entry: the argument selects the entry and is ignored.
#define effect_A2_init(...) effect_A2_init()

// The volume of the sound effects.
#define setSeVolume(...) setSeVolume()

// The lock of the resources of the CRI middleware.
#define SVM_LockRsc(...) SVM_LockRsc()
#define SVM_UnlockRsc(...) SVM_UnlockRsc()

// The cache of the file list of the host file system.
#define htCiLoadFcache(...) htCiLoadFcache()

#endif // !TARGET_PS2
