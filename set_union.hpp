#include "concepts.hpp"

namespace std::ranges {
template<class Comp, input_range... Views>
  requires(view<Views> && ...) && (sizeof...(Views) > 0) && is_object_v<Comp> &&
  __detail::__pairwise_indirect_strict_weak_order<Comp, Views...> &&
  __detail::__concatable<Views...>
class set_union_view : public view_interface<set_union_view<Comp, Views...>> {
  [[no_unique_address]] __detail::__box<Comp> comp_;
  [[no_unique_address]] tuple<Views...> views_;

  // TODO: iterator_category
  template<bool Const>
  class iterator {
    friend set_union_view;

    tuple<iterator_t<__detail::__maybe_const_t<Const, Views>>...> current_;
    __detail::__maybe_const_t<Const, set_union_view>* parent_ = nullptr;
    // or constexpr static no_active = sizeof...(Views);
    size_t active_idx_ = -1;

    constexpr explicit iterator(
      __detail::__maybe_const_t<Const, set_union_view>* parent,
      tuple<iterator_t<__detail::__maybe_const_t<Const, Views>>...> current)
      : parent_(parent), current_(std::move(current)) {
      satisfy();
    }

    constexpr iterator(iterator<!Const> i)
      requires Const && (convertible_to<iterator_t<Views>, iterator_t<const Views>> && ...)
      : current_(i.current_), parent_(i.parent_), active_idx_(i.active_idx_) { }

    template<class F>
    constexpr decltype(auto)
    visit_active(F&& f) {
      template for (constexpr size_t N : views::indices(sizeof...(Views))) {
        if (N == active_idx_)
          return std::forward<F>(f).template operator()<N>();
      }
      unreachable();
    }

    template<class F>
    constexpr decltype(auto)
    visit_active(F&& f) const {
      template for (constexpr size_t N : views::indices(sizeof...(Views))) {
        if (N == active_idx_)
          return std::forward<F>(f).template operator()<N>();
      }
      unreachable();
    }

    constexpr void
    satisfy() {
      active_idx_ = -1;
      template for (constexpr size_t N : views::indices(sizeof...(Views))) {
        if (std::get<N>(current_) == ranges::end(std::get<N>(parent_->views_)))
          continue;
        if (active_idx_ == -1)
          active_idx_ = N;
        else if (visit_active([&]<size_t ActiveIdx> {
                   return std::__invoke(*parent_->comp_, *std::get<N>(current_),
                                        *std::get<ActiveIdx>(current_));
                 }))
          active_idx_ = N;
      }
    }

   public:
    using iterator_concept =
      conditional_t<(forward_range<__detail::__maybe_const_t<Const, Views>> && ...),
                    forward_iterator_tag, input_iterator_tag>;
    using value_type = __detail::__concat_value_t<__detail::__maybe_const_t<Const, Views>...>;
    using difference_type =
      common_type_t<range_difference_t<__detail::__maybe_const_t<Const, Views>>...>;

    iterator() = default;

    constexpr __detail::__concat_reference_t<__detail::__maybe_const_t<Const, Views>...>
    operator*() const {
      return visit_active(
        [&]<size_t ActiveIdx> -> decltype(auto) { return *std::get<ActiveIdx>(current_); });
    }

    constexpr iterator&
    operator++() {
      visit_active([&]<size_t ActiveIdx> {
        template for (constexpr size_t N : views::indices(sizeof...(Views))) {
          if constexpr (N != ActiveIdx) {
            auto& other = std::get<N>(current_);
            if (other == ranges::end(std::get<N>(parent_->views_)))
              continue;
            if (!std::__invoke(*parent_->comp_, *other, *std::get<ActiveIdx>(current_)) &&
                !std::__invoke(*parent_->comp_, *std::get<ActiveIdx>(current_), *other))
              ++other;
          }
        }
        ++get<ActiveIdx>(current_);
      });
      satisfy();
      return *this;
    }

    constexpr void
    operator++(int) {
      ++*this;
    }

    constexpr iterator
    operator++(int)
      requires(forward_range<__detail::__maybe_const_t<Const, Views>> && ...)
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    friend constexpr bool
    operator==(const iterator& x, const iterator& y)
      requires(forward_range<__detail::__maybe_const_t<Const, Views>> && ...)
    {
      return x.current_ == y.current_ && x.active_idx_ == y.active_idx_;
    }

    friend constexpr bool
    operator==(const iterator& it, default_sentinel_t) {
      template for (constexpr size_t N : views::indices(sizeof...(Views))) {
        if (std::get<N>(it.current_) != ranges::end(std::get<N>(it.parent_->views_)))
          return false;
      }
      return it.active_idx_ == -1;
    }

    friend constexpr __detail::__concat_rvalue_reference_t<
      __detail::__maybe_const_t<Const, Views>...>
    iter_move(const iterator& i) {
      return i.visit_active([&]<size_t ActiveIdx> -> decltype(auto) {
        return ranges::iter_move(std::get<ActiveIdx>(i.current_));
      });
    }
  };

 public:
  set_union_view() = default;
  constexpr explicit set_union_view(Comp comp, Views... bases)
    : comp_(std::move(comp)), views_(std::move(bases)...) { }

  constexpr iterator<false>
  begin() {
    return iterator<false>(this, __detail::__tuple_transform(ranges::begin, views_));
  }

  constexpr iterator<true>
  begin() const
    requires(range<const Views> && ...) &&
    __detail::__pairwise_indirect_strict_weak_order<const Comp, const Views...> &&
    __detail::__concatable<const Views...>
  {
    return iterator<true>(this, __detail::__tuple_transform(ranges::begin, views_));
  }

  constexpr auto
  end() const noexcept {
    return default_sentinel;
  }

  // constexpr auto
  // reserve_hint()
  //   requires approximately_sized_range<Views...[0]>
  // {
  //   return ranges::reserve_hint(std::get<0>(views_));
  // }

  // constexpr auto
  // reserve_hint() const
  //   requires approximately_sized_range<const Views...[0]>
  // {
  //   return ranges::reserve_hint(std::get<0>(views_));
  // }
};

template<class Comp, class... Rs>
set_union_view(Comp, Rs&&...) -> set_union_view<Comp, views::all_t<Rs>...>;

namespace views {
namespace __detail {
template<class Comp, class... Rs>
concept __can_set_union_view =
  requires { set_union_view(std::declval<Comp>(), std::declval<Rs>()...); };
}  // namespace __detail

struct SetUnionBy {
  template<class Comp, class... Rs>
    requires __detail::__can_set_union_view<Comp, Rs...>
  constexpr auto
  operator() [[nodiscard]] (Comp&& comp, Rs&&... rs) const {
    return set_union_view(std::forward<Comp>(comp), std::forward<Rs>(rs)...);
  }
};

inline constexpr SetUnionBy set_union_by;

struct SetUnion {
  template<class... Rs>
    requires __detail::__can_set_union_view<ranges::less, Rs...>
  constexpr auto
  operator() [[nodiscard]] (Rs&&... rs) const {
    return set_union_view(ranges::less{}, std::forward<Rs>(rs)...);
  }
};

inline constexpr SetUnion set_union;
}  // namespace views

}  // namespace std::ranges
