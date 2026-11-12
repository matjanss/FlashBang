// FlashBang V3.0 - flash schedule
#pragma once
#include "config.h"
#include "state.h"

void buildSchedule();
int seqSort(Setup& s, int track);
int seqCount(const Setup& s, int ch);
void allOff();
void fireSchedule(uint32_t t0);
