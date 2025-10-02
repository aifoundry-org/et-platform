/* --------------------------------------------------------------------
** 
** Synopsys DesignWare AMBA Software Driver Kit and
** documentation (hereinafter, "Software") is an Unsupported
** proprietary work of Synopsys, Inc. unless otherwise expressly
** agreed to in writing between Synopsys and you.
** 
** The Software IS NOT an item of Licensed Software or Licensed
** Product under any End User Software License Agreement or Agreement
** for Licensed Product with Synopsys or any supplement thereto. You
** are permitted to use and redistribute this Software in source and
** binary forms, with or without modification, provided that
** redistributions of source code must retain this notice. You may not
** view, use, disclose, copy or distribute this file or any information
** contained herein except pursuant to this license grant from Synopsys.
** If you do not agree with this notice, including the disclaimer
** below, then you are not authorized to use the Software.
** 
** THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS"
** BASIS AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
** FOR A PARTICULAR PURPOSE ARE HEREBY DISCLAIMED. IN NO EVENT SHALL
** SYNOPSYS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
** OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
** USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
** DAMAGE.
** 
** --------------------------------------------------------------------
*/

#ifndef DW_COMMON_DBC_H
#define DW_COMMON_DBC_H

#ifdef __cplusplus
extern "C" {    // allow C++ to use these headers
#endif

// DW_NASSERT macro disables all contract validations
// (assertions, preconditions, postconditions, and invariants).
#ifndef DW_NASSERT 

// callback invoked in case of assertion failure
extern void onAssert__(char const *file, unsigned line);

#define DW_DEFINE_THIS_FILE static const char THIS_FILE__[] = __FILE__

#define DW_ASSERT(test_)   \
        if (test_) {    \
        }               \
        else onAssert__(THIS_FILE__, __LINE__)

#define DW_REQUIRE(test_)           DW_ASSERT(test_)
#define DW_ENSURE(test_)            DW_ASSERT(test_)
#define DW_INVARIANT(test_)         DW_ASSERT(test_)
#define DW_ALLEGE(test_)            DW_ASSERT(test_)

#else   // DW_NASSERT

#define DW_DEFINE_THIS_FILE extern const char THIS_FILE__[]
#define DW_ASSERT(test_)
#define DW_REQUIRE(test_)
#define DW_ENSURE(test_)
#define DW_INVARIANT(test_)
#define DW_ALLEGE(test_)   \
   if(test_) {          \
   }                    \
   else

#endif  // DW_NASSERT


#ifdef __cplusplus
}
#endif

#endif  // DW_COMMON_DBC_H

