/**
 * Chatroom Lab
 * CS 241 - Fall 2016
 */
#ifndef __CAMELCASER_TESTS_H
#define __CAMELCASER_TESTS_H

#include "camelCaser.h"

int test_camelCaser(char **(*camelCaser)(const char *));

void printOutput(char ** output);

void freeOutput(char ** output);

#endif
