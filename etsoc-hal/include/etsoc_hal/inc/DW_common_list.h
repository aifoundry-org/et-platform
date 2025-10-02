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

#ifndef DW_COMMON_LIST_H
#define DW_COMMON_LIST_H

#ifdef __cplusplus
extern "C" {    // allow C++ to use these headers
#endif

// INLINE definition for the ARM C compiler
#define INLINE __inline

/****h* include.list
 * NAME
 *  dw_list.h -- simple double linked list implementation
 * DESCRIPTION
 *  Some of the internal functions ("__dw_xxx") are useful when
 *  manipulating whole lists rather than single entries, as
 *  sometimes we already know the next/prev entries and we can
 *  generate better code by using them directly rather than
 *  using the generic single-entry routines.
 ***/

/****is* include.dw_list_head
 * DESCRIPTION
 *  This is the structure used for managing linked lists.
 * SOURCE
 */
struct dw_list_head {
    struct dw_list_head *next, *prev;
};
/*****/

#define DW_LIST_HEAD_INIT(name) { &(name), &(name) }

#define DW_LIST_HEAD(name) \
    struct dw_list_head name = DW_LIST_HEAD_INIT(name)

#define DW_INIT_LIST_HEAD(ptr)                  \
do {                                            \
    (ptr)->next = (ptr); (ptr)->prev = (ptr);   \
} while (0)

/****if* include.list/__dw_list_add
 * DESCRIPTION
 *  Insert a new entry between two known consecutive entries. 
 *  This is only for internal list manipulation where we know
 *  the prev/next entries already!
 * ARGUMENTS
 *  new         element to insert
 *  prev        previous entry
 *  next        next entry
 * SOURCE
 */
static INLINE void __dw_list_add(struct dw_list_head * new, struct
        dw_list_head *prev, struct dw_list_head * next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}
/*****/

/****f* include.list/dw_list_add
 * DESCRIPTION
 *  Insert a new entry after the specified head.
 *  This is good for implementing stacks.
 * ARGUMENTS
 *  new         new entry to be added
 *  head        list head to add it after
 * SOURCE
 */
static INLINE void dw_list_add(struct dw_list_head *new, struct
        dw_list_head *head)
{
    __dw_list_add(new, head, head->next);
}
/*****/

/****f* include.list/dw_list_add_tail
 * DESCRIPTION
 *  Insert a new entry before the specified head.
 *  This is useful for implementing queues.
 * ARGUMENTS
 *  new         new entry to be added
 *  head        list head to add it before
 * SOURCE
 */
static INLINE void dw_list_add_tail(struct dw_list_head *new, struct
        dw_list_head *head)
{
    __dw_list_add(new, head->prev, head);
}
/*****/

/****if* include.list/__dw_list_del
 * DESCRIPTION
 *  Delete a list entry by making the prev/next entries point to each
 *  other.  This is only for internal list manipulation where we know
 *  the prev/next entries already!
 * ARGUMENTS
 *  prev        previous entry
 *  next        next entry
 * SOURCE
 */
static INLINE void __dw_list_del(struct dw_list_head * prev, struct
        dw_list_head * next)
{
    next->prev = prev;
    prev->next = next;
}
/*****/

/****f* include.list/dw_list_del
 * DESCRIPTION
 *  Deletes entry from list.
 * ARGUMENTS
 *  entry       the element to delete from the list
 * NOTES
 *  list_empty on entry does not return true after this, the entry
 *  is in an undefined state.
 * SOURCE
 */
static INLINE void dw_list_del(struct dw_list_head *entry)
{
    __dw_list_del(entry->prev, entry->next);
}
/*****/

/****f* include.list/dw_list_del_init
 * DESCRIPTION
 *  Deletes entry from list and reinitializes it.
 * ARGUMENTS
 *  entry       the element to delete from the list
 * SOURCE
 */
static INLINE void dw_list_del_init(struct dw_list_head *entry)
{
    __dw_list_del(entry->prev, entry->next);
    DW_INIT_LIST_HEAD(entry); 
}
/*****/

/****f* include.list/dw_list_empty
 * DESCRIPTION
 *  Tests whether a list is empty.
 * ARGUMENTS
 *  head        the list to test
 * SOURCE
 */
static INLINE int dw_list_empty(struct dw_list_head *head)
{
    return head->next == head;
}
/*****/

/****f* include.list/dw_list_splice
 * DESCRIPTION
 *  Join two lists.
 * ARGUMENTS
 *  list        the new list to add
 *  head        the place to add it in the first list
 * SOURCE
 */
static INLINE void dw_list_splice(struct dw_list_head *list, struct
        dw_list_head *head)
{
    struct dw_list_head *first = list->next;

    if (first != list) {
        struct dw_list_head *last = list->prev;
        struct dw_list_head *at = head->next;

        first->prev = head;
        head->next = first;

        last->next = at;
        at->prev = last;
    }
}
/*****/

/****d* include.list/dw_list_entry
 * DESCRIPTION
 *  Get the struct for this entry.
 * ARGUMENTS
 *  ptr         the &struct dw_list_head pointer
 *  type        the type of the struct this is embedded in
 *  member      the name of the list_struct within the struct
 * SOURCE
 */
#define DW_LIST_ENTRY(ptr, type, member) \
    ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))
/*****/

/****d* include.list/dw_list_for_each
 * DESCRIPTION
 *  Iterate over a list.
 * ARGUMENTS
 *  pos         the &struct dw_list_head to use as a loop counter
 *  head        the head for your list
 * SOURCE
 */
#define DW_LIST_FOR_EACH(pos, head)                             \
    for(pos = (head)->next; pos != (head); pos = pos->next)
/*****/

/****d* include.list/dw_list_for_each_safe
 * SYNOPSIS
 *  list_for_each_safe(pos, head)
 * DESCRIPTION
 *  Iterate over a list safe against removal of list entry.
 * ARGUMENTS
 *  pos         the &struct dw_list_head to use as a loop counter
 *  n           another &struct dw_list_head to use as temporary storage
 *  head        the head for your list
 * SOURCE
 */
#define DW_LIST_FOR_EACH_SAFE(pos, n, head)                     \
    for(pos = (head)->next, n = pos->next; pos != (head);       \
        pos = n, n = pos->next)
/*****/

/****d* include.list/dw_list_for_each_prev
 * DESCRIPTION
 *  Iterate over a list in reverse order.
 * ARGUMENTS
 *  pos         the &struct dw_list_head to use as a loop counter
 *  head        the head for your list
 * SOURCE
 */
#define DW_LIST_FOR_EACH_PREV(pos, head)                        \
    for(pos = (head)->prev; pos != (head); pos = pos->prev)
/*****/

#ifdef __cplusplus
}
#endif

#endif      // DW_COMMON_LIST_H

