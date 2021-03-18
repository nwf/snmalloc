#pragma once

#if defined(__FreeBSD__) && !defined(_KERNEL)
#  include "pal_bsd_aligned.h"

#  if defined(__CHERI_PURE_CAPABILITY__)
#    include <cheri/cheric.h>
#  endif

namespace snmalloc
{
  /**
   * FreeBSD-specific platform abstraction layer.
   *
   * This adds FreeBSD-specific aligned allocation to the generic BSD
   * implementation.
   */
  class PALFreeBSD : public PALBSD_Aligned<PALFreeBSD>
  {
  public:
    /**
     * Bitmap of PalFeatures flags indicating the optional features that this
     * PAL supports.
     *
     * The FreeBSD PAL does not currently add any features beyond those of a
     * generic BSD with support for arbitrary alignment from `mmap`.  This
     * field is declared explicitly to remind anyone modifying this class to
     * add new features that they should add any required feature flags.
     */
    static constexpr uint64_t pal_features = PALBSD_Aligned::pal_features;

#  if defined(__CHERI_PURE_CAPABILITY__)
    static_assert(
      aal_supports<StrictProvenance>,
      "CHERI purecap support requires StrictProvenance AAL");

    /**
     * On CheriBSD, exporting a pointer means stripping it of the authority to
     * manage the address space it references by clearing the CHERIABI_VMMAP
     * permission bit.
     */
    template<typename T, SNMALLOC_CONCEPT(capptr_bounds::c) B>
    static SNMALLOC_FAST_PATH CapPtr<T, capptr_export_type<B>>
    capptr_export(CapPtr<T, B> p)
    {
      return CapPtr<T, capptr_export_type<B>>(__builtin_cheri_perms_and(
        p.unsafe_capptr,
        ~static_cast<unsigned int>(CHERI_PERM_CHERIABI_VMMAP)));
    }
#  endif
  };
} // namespace snmalloc
#endif
