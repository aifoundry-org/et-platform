#ifndef _CVT_H
#define _CVT_H

float32 float16tofloat32(uint32 val);
float32 float11tofloat32(uint32 val);
float32 float10tofloat32(uint32 val);
float32 unorm24tofloat32(uint32 val);
float32 unorm16tofloat32(uint32 val);
float32 unorm10tofloat32(uint32 val);
float32 unorm8tofloat32(uint32 val);
float32 unorm2tofloat32(uint32 val);
float32 snorm24tofloat32(uint32 val);
float32 snorm16tofloat32(uint32 val);
float32 snorm10tofloat32(uint32 val);
float32 snorm8tofloat32(uint32 val);
float32 snorm2tofloat32(uint32 val);
uint32 float32tofloat16(float32 val);
uint32 float32tofloat11(float32 val);
uint32 float32tofloat10(float32 val);
uint32 float32tounorm24(float32 val);
uint32 float32tounorm16(float32 val);
uint32 float32tounorm10(float32 val);
uint32 float32tounorm8(float32 val);
uint32 float32tounorm2(float32 val);
uint32 float32tosnorm24(float32 val);
uint32 float32tosnorm16(float32 val);
uint32 float32tosnorm8(float32 val);

#endif
