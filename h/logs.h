#ifndef LOGS_H
#define LOGS_H

#include <stdio.h>

#include <my-error.h>

extern FILE* fdump;
extern const char log_path[];

ErrEnum openDumpFile();
void closeDumpFile();

#endif // LOGS_H