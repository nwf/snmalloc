#pragma once

#include "../ds/defines.h"

#include <cheri.h>

namespace snmalloc
{
  template<typename Base>
  class AAL_CHERI : public Base
  {
  public:
    /**
     * CHERI pointers are not integers and come with strict provenance
     * requirements.
     */
    static constexpr uint64_t aal_features =
      (Base::aal_features & ~IntegerPointers) | StrictProvenance;

    /**
     * On CHERI-aware compilers, ptraddr_t is an integral type that is wide
     * enough to hold any address that may be contained within a memory
     * capability.  It does not carry provenance: it is not a capability, but
     * merely an address.
     */
    typedef ptraddr_t address_t;

    template<
      typename T,
      SNMALLOC_CONCEPT(capptr_bounds::c) BOut,
      SNMALLOC_CONCEPT(capptr_bounds::c) BIn,
      typename U = T>
    static SNMALLOC_FAST_PATH CapPtr<T, BOut>
    capptr_bound(CapPtr<U, BIn> a, size_t size) noexcept
    {
      // Impose constraints on bounds annotations.
      static_assert(BIn::spatial >= capptr_bounds::spatial::Chunk);
      static_assert(capptr_is_spatial_refinement<BIn, BOut>());
      SNMALLOC_ASSERT(cheri_tag_get(a.unsafe_capptr));

      void* pb = __builtin_cheri_bounds_set_exact(a.unsafe_capptr, size);
      return CapPtr<T, BOut>(static_cast<T*>(pb));
    }

    /**
     * Transfer the address of r onto the bounds provided by a.
     */
    template<
      typename T,
      SNMALLOC_CONCEPT(capptr_bounds::c) BOut,
      SNMALLOC_CONCEPT(capptr_bounds::c) BIn>
    static SNMALLOC_FAST_PATH CapPtr<T, BOut>
    capptr_rebound(CapPtr<void, BOut> a, CapPtr<T, BIn> r) noexcept
    {
      /*
       * This may seem a little strange, but it does exactly what we want:
       *
       *   * if neither is tagged, it's a little strange, but fine.  This case
       *     applies, notably, in the case of NULL: if we're trying to rebound
       *     a NULL r, we're likely to be doing so with a NULL a, too!
       *
       *   * if r isn't tagged, it means we don't suddenly reconstruct the tag
       */
      SNMALLOC_ASSERT(
        cheri_tag_get(a.unsafe_capptr) == cheri_tag_get(r.unsafe_capptr));

      return CapPtr<T, BOut>(static_cast<T*>(__builtin_cheri_address_set(
        a.unsafe_capptr, reinterpret_cast<ptraddr_t>(r.unsafe_capptr))));
    }

    /**
     * On CHERI, we want to ensure that the tag is still set on anything given
     * to us.  Since we extract addresses (e.g., for amplification) we should
     * also check that the cap is in bounds; however, it's simpler to test that
     * the base is 0.  (That does not obviate the need for checking that we are
     * at the start of an object, since the client may have set bounds to a
     * subset of an allocation.)
     */
    template<typename T>
    static SNMALLOC_FAST_PATH CapPtr<T, CBAllocE>
    capptr_dewild(CapPtr<T, CBAllocEW> p) noexcept
    {
      void* vp = p.unsafe_capptr;

#ifdef CHECK_CLIENT
      if ((!cheri_tag_get(vp)) || (cheri_offset_get(vp) != 0))
        return nullptr;
#endif

      return CapPtr<void, CBAllocE>(vp);
    }
  };
} // namespace snmalloc
