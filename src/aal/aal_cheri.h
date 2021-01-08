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
      enum capptr_bounds nbounds,
      enum capptr_bounds obounds,
      typename U = T>
    static SNMALLOC_FAST_PATH CapPtr<T, nbounds>
    capptr_bound(CapPtr<U, obounds> a, size_t size) noexcept
    {
      // Impose constraints on bounds annotations.
      static_assert(
        obounds == CBArena || obounds == CBChunkD || obounds == CBChunk ||
        obounds == CBChunkE);
      static_assert(capptr_is_bounds_refinement<obounds, nbounds>());
      SNMALLOC_ASSERT(cheri_tag_get(a.unsafe_capptr));

      void* pb = __builtin_cheri_bounds_set_exact(a.unsafe_capptr, size);
      return CapPtr<T, nbounds>(static_cast<T*>(pb));
    }

    /**
     * Transfer the address of r onto the bounds provided by a.
     */
    template<typename T, capptr_bounds BOut, capptr_bounds BIn>
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
  };
} // namespace snmalloc
