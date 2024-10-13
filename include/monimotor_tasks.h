#pragma once

#include <stdlib.h>

#ifndef MONIMOTOR_TASKS_H
#define MONIMOTOR_TASKS_H

void* sound_capture_task(void* args);

void* preprocessing_task(void* args);

#endif
