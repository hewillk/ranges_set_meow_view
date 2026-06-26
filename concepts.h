#pragma once
#include <ranges>

namespace std::ranges::__detail {

template<class Comp, class... Views>
constexpr bool __pairwise_indirect_strict_weak_order =
  (indirect_strict_weak_order<Comp, iterator_t<Views>> && ...);
template<class Comp, class First, class... Views>
  requires(sizeof...(Views) > 0)
constexpr bool __pairwise_indirect_strict_weak_order<Comp, First, Views...> =
  (indirect_strict_weak_order<Comp, iterator_t<First>, iterator_t<Views>> && ...) &&
  __pairwise_indirect_strict_weak_order<Comp, Views...>;

}  // namespace std::ranges::__detail
