/*-------------------------------------------------------------------------
 * Copyright (C) 2022, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#include "asm_memcpy.h"
#include <chrono>
#include <ctime>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string.h>
#include <thread>
#include <vector>

float calcBW(size_t size, const std::function<void(char* dst, char* in, size_t size)>& func) {
  using Clock = std::chrono::high_resolution_clock;
  std::vector<char> in(size);
  std::vector<char> out(size);
  auto start = Clock::now();
  func(out.data(), in.data(), size);
  auto et = Clock::now() - start;
  return ((size / 1e6) / (std::chrono::duration_cast<std::chrono::nanoseconds>(et).count() / 1e9f));
}

bool g_print_results = false;

void print_results(size_t size, float bw) {
  if (!g_print_results)
    return;
  if (size < (1 << 20)) {
    std::cout << "Size: " << size / (1 << 10) << "KB ";
  } else {
    std::cout << "Size: " << size / (1 << 20) << "MB ";
  }
  std::cout << " MBps: " << std::setprecision(2) << std::fixed << bw << std::endl;
}

float simple_memcpy(size_t size, int runs = 1) {
  if (g_print_results)
    std::cout << "Simple memcpy, no threads.\n";
  float bw = 0.0f;
  for (int i = 0; i < runs; ++i) {
    bw += calcBW(size, memcpy);
  }
  bw /= runs;
  print_results(size, bw);
  return bw;
}

float stdcopy(size_t size, int runs = 1) {
  if (g_print_results)
    std::cout << "Simple std::copy, no threads.\n";
  float bw = 0.0f;
  for (int i = 0; i < runs; ++i) {
    bw += calcBW(size, [](auto dst, auto src, size_t size) { std::copy(src, src + size, dst); });
  }
  bw /= runs;
  print_results(size, bw);
  return bw;
}

float omp_simple8(size_t size, int runs = 1) {
  if (g_print_results)
    std::cout << "Simple OMP 8 threads\n";
  float bw = 0.0f;
  auto memcpyfunc = [](char* dst, char* in, size_t size) {
    auto chunkSize = size / 8;
#pragma omp parallel for schedule(static) num_threads(8)
    for (int i = 0; i < 8; ++i) {
      memcpy(dst + i * chunkSize, in + i * chunkSize, chunkSize);
    }
    if (chunkSize * 8 < size) {
      memcpy(dst + 8 * chunkSize, in + 8 * chunkSize, size - chunkSize * 8);
    }
  };
  for (int i = 0; i < runs; ++i) {
    bw += calcBW(size, memcpyfunc);
  }
  bw /= runs;
  print_results(size, bw);
  return bw;
}

float omp_simple2(size_t size, int runs = 1) {
  if (g_print_results)
    std::cout << "Simple OMP 2 threads\n";
  float bw = 0.0f;
  auto memcpyfunc = [](char* dst, char* in, size_t size) {
    auto chunkSize = size / 2;
#pragma omp parallel for schedule(static) num_threads(2)
    for (int i = 0; i < 2; ++i) {
      memcpy(dst + i * chunkSize, in + i * chunkSize, chunkSize);
    }
    if (chunkSize * 2 < size) {
      memcpy(dst + 2 * chunkSize, in + 2 * chunkSize, size - chunkSize * 2);
    }
  };
  for (int i = 0; i < runs; ++i) {
    bw += calcBW(size, memcpyfunc);
  }
  bw /= runs;
  print_results(size, bw);
  return bw;
}

float threaded(size_t size, int th = 2, int runs = 1) {
  if (g_print_results)
    std::cout << "c++11 threads: " << th << " threads\n";
  float bw = 0.0f;
  auto chunkSize = size / th;
  auto memcpyfunc = [th, chunkSize](char* dst, char* in, size_t size) {
    std::vector<std::thread> threads;
    for (int i = 0; i < th; ++i) {
      threads.emplace_back([=] { memcpy(dst + i * chunkSize, in + i * chunkSize, chunkSize); });
    }
    if (chunkSize * th < size) {
      memcpy(dst + th * chunkSize, in + th * chunkSize, size - chunkSize * th);
    }
    for (auto& t : threads) {
      t.join();
    }
  };
  for (int i = 0; i < runs; ++i) {
    bw += calcBW(size, memcpyfunc);
  }
  bw /= runs;
  print_results(size, bw);
  return bw;
}

float asmmemcpy(size_t size, int runs = 1) {
  if (g_print_results)
    std::cout << "ASM memcpy, no threads.\n";
  float bw = 0.0f;
  for (int i = 0; i < runs; ++i) {
    bw += calcBW(size, asm_memcpy);
  }
  bw /= runs;
  print_results(size, bw);
  return bw;
}

int main() {
  std::cout << "size\tmemcpy\t\tstdcopy\t\tasm_memcpy\tomp_simple2\tomp_simple8\tc++11threads2\tc++11threads8\n"
            << std::setprecision(2) << std::fixed;
  for (auto size : {1 << 10, 1 << 15, 1 << 20, 1 << 25, 1 << 27}) {
    if (size < (1 << 20)) {
      std::cout << size / (1 << 10) << "KB\t";
    } else {
      std::cout << size / (1 << 20) << "MB\t";
    }
    int runs = (size > (1 << 20)) ? 5 : 500;
    std::cout << simple_memcpy(size, runs) << "MB/s\t";
    std::cout << stdcopy(size, runs) << "MB/s\t";
    std::cout << asmmemcpy(size, runs) << "MB/s\t";
    std::cout << omp_simple2(size, runs) << "MB/s\t";
    std::cout << omp_simple8(size, runs) << "MB/s\t";
    std::cout << threaded(size, 2, runs) << "MB/s\t";
    std::cout << threaded(size, 8, runs) << "MB/s\t";
    std::cout << std::endl;
  }
  return 0;
}