#ifndef SDEES_H
#define SDEES_H

#if defined(SDGPOS)
#include <sdgpos.h>
#include <arch/sdgpos.h>
#endif
#if defined(SDTZ)
#error wut
#include <sdtz.h>
#include <arch/sdtz.h>
#endif
#if defined(SDTZM)
#include <sdtzm.h>
#include <arch/sdtzm.h>
#endif
#if defined(SDSGX)
#include <sdsgx.h>
#endif
#endif
