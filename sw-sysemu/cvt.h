#ifndef _CVT_H
#define _CVT_H

float32_t float16tofloat32(uint16_t val);
float32_t float11tofloat32(uint16_t val);
float32_t float10tofloat32(uint16_t val);

uint16_t float32tofloat16(float32_t val);
uint16_t float32tofloat11(float32_t val);
uint16_t float32tofloat10(float32_t val);

float32_t unorm24tofloat32(uint32_t val);
float32_t unorm16tofloat32(uint16_t val);
float32_t unorm10tofloat32(uint16_t val);
float32_t unorm8tofloat32(uint8_t val);
float32_t unorm2tofloat32(uint8_t val);

uint32_t float32tounorm24(float32_t val);
uint16_t float32tounorm16(float32_t val);
uint16_t float32tounorm10(float32_t val);
uint8_t  float32tounorm8(float32_t val);
uint8_t  float32tounorm2(float32_t val);

float32_t snorm24tofloat32(uint32_t val);
float32_t snorm16tofloat32(uint16_t val);
float32_t snorm10tofloat32(uint16_t val);
float32_t snorm8tofloat32(uint8_t val);
float32_t snorm2tofloat32(uint8_t val);

uint32_t float32tosnorm24(float32_t val);
uint16_t float32tosnorm16(float32_t val);
uint8_t  float32tosnorm8(float32_t val);

#endif
