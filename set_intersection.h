#include "concepts.hpp"

namespace std::ranges {
template<class Comp, input_range... Views>
  requires(view<Views> && ...) && (sizeof...(Views) > 0) && is_object_v<Comp> &&
  __detail::__pairwise_indirect_strict_weak_order<Comp, Views...>
class set_intersection_view : public view_interface<set_intersection_view<Comp, Views...>> {
  [[no_unique_address]] __detail::__box<Comp> comp_;
  [[no_unique_address]] tuple<Views...> views_;

  // TODO: iterator_category
  class iterator {
    friend set_intersection_view;

    tuple<iterator_t<Views>...> current_;
    set_intersection_view* parent_ = nullptr;

    constexpr explicit iterator(set_intersection_view* parent, tuple<iterator_t<Views>...> current)
      : parent_(parent), current_(std::move(current)) {
      satisfy();
    }

    constexpr bool
    try_satisfy() {
      auto& first = std::get<0>(current_);
      if (first == ranges::end(std::get<0>(parent_->views_)))
        return true;

      template for (constexpr size_t N : views::iota(1uz, sizeof...(Views))) {
        auto& other = std::get<N>(current_);
        while (true) {
          if (other == ranges::end(std::get<N>(parent_->views_)))
            return true;
          if (std::__invoke(*parent_->comp_, *first, *other)) {
            ++first;
            return false;
          }
          if (std::__invoke(*parent_->comp_, *other, *first))
            ++other;
          else
            break;
        }
      }
      return true;
    }

    constexpr void
    satisfy() {
      while (!try_satisfy())
        ;
    }

   public:
    using iterator_concept =
      conditional_t<(forward_range<Views> && ...), forward_iterator_tag, input_iterator_tag>;
    using value_type = range_value_t<Views...[0]>;
    using difference_type = range_difference_t<Views...[0]>;

    iterator() = default;

    constexpr iterator_t<Views...[0]>
    base() && {
      return std::move(std::get<0>(current_));
    }

    constexpr const iterator_t<Views...[0]>&
    base() const& noexcept {
      return std::get<0>(current_);
    }

    constexpr decltype(auto)
    operator*() const {
      return *std::get<0>(current_);
    }

    constexpr iterator&
    operator++() {
      __detail::__tuple_for_each([](auto& i) { ++i; }, current_);
      satisfy();
      return *this;
    }

    constexpr void
    operator++(int) {
      ++*this;
    }

    constexpr iterator
    operator++(int)
      requires(forward_range<Views> && ...)
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    friend constexpr bool
    operator==(const iterator& x, const iterator& y)
      requires(forward_range<Views> && ...)
    {
      return x.current_ == y.current_;
    }

    friend constexpr bool
    operator==(const iterator& it, default_sentinel_t) {
      template for (constexpr size_t N : views::indices(sizeof...(Views))) {
        if (std::get<N>(it.current_) == ranges::end(std::get<N>(it.parent_->views_)))
          return true;
      }
      return false;
    }

    friend constexpr decltype(auto)
    iter_move(const iterator& i) noexcept(noexcept(ranges::iter_move(std::get<0>(i.current_)))) {
      return ranges::iter_move(std::get<0>(i.current_));
    }
  };

 public:
  set_intersection_view() = default;
  constexpr explicit set_intersection_view(Comp comp, Views... bases)
    : comp_(std::move(comp)), views_(std::move(bases)...) { }

  constexpr Views...[0] base() const&
    requires copy_constructible<Views...[0]>
  {
    return std::get<0>(views_);
  }

  constexpr Views...[0] base() && { return std::move(std::get<0>(views_)); }

  constexpr iterator
  begin() {
    // TODO: cache begin
    return iterator(this, __detail::__tuple_transform(ranges::begin, views_));
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
set_intersection_view(Comp, Rs&&...) -> set_intersection_view<Comp, views::all_t<Rs>...>;

namespace views {
namespace __detail {
template<class Comp, class... Rs>
concept __can_set_intersection_view =
  requires { set_intersection_view(std::declval<Comp>(), std::declval<Rs>()...); };
}  // namespace __detail

struct SetIntersectionBy {
  template<class Comp, class... Rs>
    requires __detail::__can_set_intersection_view<Comp, Rs...>
  constexpr auto
  operator() [[nodiscard]] (Comp&& comp, Rs&&... rs) const {
    return set_intersection_view(std::forward<Comp>(comp), std::forward<Rs>(rs)...);
  }
};

inline constexpr SetIntersectionBy set_intersection_by;

struct SetIntersection {
  template<class... Rs>
    requires __detail::__can_set_intersection_view<ranges::less, Rs...>
  constexpr auto
  operator() [[nodiscard]] (Rs&&... rs) const {
    return set_intersection_view(ranges::less{}, std::forward<Rs>(rs)...);
  }
};

inline constexpr SetIntersection set_intersection;
}  // namespace views

}  // namespace std::ranges
