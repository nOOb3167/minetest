#ifndef _VSERV_MISC_H_
#define _VSERV_MISC_H_

#include <cassert>

#include <memory>

#define GS_LOG(LEVEL, TT, ...) do { } while(0)

/* nulling destruction - delete [] P */
#define GS_DELETE_ARRAY(PTR_PTR_ALLOCATED_WITH_NEW_BRACKET, TYPE_ELT) \
  do {                                                          \
    TYPE_ELT **ptr_ptr = (PTR_PTR_ALLOCATED_WITH_NEW_BRACKET);  \
	if (*ptr_ptr) {                                             \
	  delete [] *ptr_ptr;                                       \
	  *ptr_ptr = NULL;                                          \
	}                                                           \
  } while (0)

/* nulling destruction - delete P */
#define GS_DELETE(PTR_PTR_ALLOCATED_WITH_NEW, TYPE) \
  do {                                              \
    TYPE **ptr_ptr = (PTR_PTR_ALLOCATED_WITH_NEW);  \
	if (*ptr_ptr) {                                 \
	  delete *ptr_ptr;                              \
	  *ptr_ptr = NULL;                              \
	}                                               \
  } while (0)

/* nulling destruction - DELETER(P) */
#define GS_DELETE_F(PTR_PTR_VARNAME, FNAME)                \
  do {                                                     \
    decltype(PTR_PTR_VARNAME) ptr_ptr = (PTR_PTR_VARNAME); \
	if (*ptr_ptr) {                                        \
      if (!!((FNAME)(*ptr_ptr))) GS_ASSERT(0);             \
      *ptr_ptr = NULL;                                     \
	}                                                      \
  } while (0)

#define GS_ARGOWN(PTR_PTR_VARNAME) ( (std::remove_reference<decltype(*(PTR_PTR_VARNAME))>::type)(gs_aux_argown((void **)(PTR_PTR_VARNAME))) )

#define GS_ASSERT(x) \
	do { bool the_x = (x); if (! the_x) { assert(0); } } while (0)

#define GS_DBG_LOG() ((void)0) // FIXME:

#define GS_ERR_NO_CLEAN(THE_R) do { r = (THE_R); goto noclean; } while(0)
#define GS_ERR_CLEAN(THE_R)    do { r = (THE_R); GS_DBG_LOG(); goto clean; } while(0)
#define GS_GOTO_CLEAN()        do { GS_DBG_LOG(); goto clean; } while(0)
#define GS_ERR_NO_CLEAN_J(JUMPLABEL, THE_R) do { r = (THE_R); goto noclean_ ## JUMPLABEL; } while(0)
#define GS_ERR_CLEAN_J(JUMPLABEL, THE_R) do { r = (THE_R); GS_DBG_LOG(); goto clean_ ## JUMPLABEL; } while(0)

/* WARNING: evaluates arguments multiple times. rework using block with decltype assignment. */
#define GS_MAX(x, y) (((x) > (y)) ? (x) : (y))
#define GS_MIN(x, y) (((x) < (y)) ? (x) : (y))

template<typename T>
using sp = ::std::shared_ptr<T>;

void * gs_aux_argown(void **ptr);

#endif /* _VSERV_MISC_H_ */
