# -------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
# -------------------------------------------------------------------------

fpu_hdrs := \
	fpu/debug.h \
	fpu/fpu.h \
	fpu/fpu_casts.h \
	fpu/fpu_types.h \
	fpu/texp.h \
	fpu/tlog.h \
	fpu/trcp.h \
	fpu/trsqrt.h \
	fpu/tsin.h \

fpu_cpp_srcs := \
	fpu/cvt.cpp \
	fpu/f10_to_f32.cpp \
	fpu/f11_to_f32.cpp \
	fpu/f32_cubeFaceIdx.cpp \
	fpu/f32_cubeFaceSignS.cpp \
	fpu/f32_cubeFaceSignT.cpp \
	fpu/f32_frac.cpp \
	fpu/f32_to_f10.cpp \
	fpu/f32_to_f11.cpp \
	fpu/f32_to_fxp1714.cpp \
	fpu/fxp1516_to_f32.cpp \
	fpu/fxp1714_rcpStep.cpp \
	fpu/tensors.cpp \
	fpu/ttrans.cpp

fpu_c_srcs := \
	fpu/f32_copySign.c \
	fpu/f32_copySignNot.c \
	fpu/f32_copySignXor.c \
	fpu/f32_log2.c \
	fpu/f32_maxNum.c \
	fpu/f32_maximumNumber.c \
	fpu/f32_minNum.c \
	fpu/f32_minimumNumber.c \
	fpu/f32_mulSub.c \
	fpu/f32_subMulAdd.c \
	fpu/f32_subMulSub.c
