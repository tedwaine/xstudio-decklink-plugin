#pragma once
#ifdef __APPLE__
#include "mac/DeckLinkAPI.h"
// macOS MacTypes.h defines 'nil' as nullptr, which conflicts with
// CAF's caf::uuid::nil() method. Undefine it after including DeckLink SDK.
#undef nil
#else
#include "linux/DeckLinkAPI.h"
#endif
