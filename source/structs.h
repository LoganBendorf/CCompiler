#pragma once

#include <stddef.h>

 typedef struct {
    const char*  data;
    const size_t size;
 } string;

 typedef struct {
   const string* strs;
   const size_t  size;
 } string_span;

