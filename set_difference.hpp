#include <ranges>

namespace std::ranges {
template<class Comp, input_range V1, input_range V2>
  requires view<V1> && view<V2> && is_object_v<Comp> &&
  indirect_strict_weak_order<Comp, iterator_t<V1>, iterator_t<V2>>
class set_difference_view : public view_interface<set_difference_view<Comp, V1, V2>> {
  V1 base1_ = V1();             // exposition only
  V2 base2_ = V2();             // exposition only
  __detail::__box<Comp> comp_;  // exposition only

  // [range.set.difference.iterator], class set_difference_view::iterator
  class iterator {
    friend set_difference_view;

    iterator_t<V1> current1_ = iterator_t<V1>();  // exposition only
    iterator_t<V2> current2_ = iterator_t<V2>();  // exposition only
    set_difference_view* parent_ = nullptr;

    constexpr void
    satisfy() {  // exposition only
      while (true) {
        if (current1_ == ranges::end(parent_->base1_))
          return;
        if (current2_ == ranges::end(parent_->base2_))
          return;
        if (std::__invoke(*parent_->comp_, *current1_, *current2_))
          return;
        if (std::__invoke(*parent_->comp_, *current2_, *current1_))
          ++current2_;
        else {
          ++current1_;
          ++current2_;
        }
      }
    }

    constexpr iterator(set_difference_view* parent,
                       iterator_t<V1> current1,  // exposition only
                       iterator_t<V2> current2)
      : parent_(parent), current1_(std::move(current1)), current2_(std::move(current2)) {
      satisfy();
    }

   public:
    // using iterator_category = see below;  // not always present
    using iterator_concept = conditional_t<forward_range<V1> && forward_range<V2>,
                                           forward_iterator_tag, input_iterator_tag>;
    using value_type = range_value_t<V1>;
    using difference_type = range_difference_t<V1>;

    iterator()
      requires default_initializable<iterator_t<V1>> && default_initializable<iterator_t<V2>>
    = default;

    constexpr iterator_t<V1>
    base() && {
      return std::move(current1_);
    }

    constexpr const iterator_t<V1>&
    base() const& noexcept {
      return current1_;
    }

    constexpr decltype(auto)
    operator*() const {
      return *current1_;
    }

    constexpr iterator&
    operator++() {
      ++current1_;
      satisfy();
      return *this;
    }

    constexpr void
    operator++(int) {
      ++*this;
    }

    constexpr iterator
    operator++(int)
      requires forward_range<V1> && forward_range<V2>
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    friend constexpr bool
    operator==(const iterator& x, const iterator& y)
      requires forward_range<V1> && forward_range<V2>
    {
      return x.current1_ == y.current1_ && x.current2_ == y.current2_;
    }

    friend constexpr bool
    operator==(const iterator& x, default_sentinel_t) {
      return x.current1_ == ranges::end(x.parent_->base1_);
    }

    friend constexpr decltype(auto)
    iter_move(const iterator& i) noexcept(noexcept(ranges::iter_move(i.current1_))) {
      return ranges::iter_move(i.current1_);
    }
  };  // exposition only

 public:
  set_difference_view()
    requires default_initializable<V1> && default_initializable<V2>
  = default;

  constexpr explicit set_difference_view(Comp comp, V1 base1, V2 base2)
    : comp_(std::move(comp)), base1_(std::move(base1)), base2_(std::move(base2)) {
    // __glibcxx_assert(ranges::is_sorted(base1_));
    // __glibcxx_assert(ranges::is_sorted(base2_));
  }

  constexpr V1
  base() const&
    requires copy_constructible<V1>
  {
    return base1_;
  }

  constexpr V1
  base() && {
    return std::move(base1_);
  }

  constexpr iterator
  begin() {
    return iterator(this, ranges::begin(base1_), ranges::begin(base2_));
  }

  constexpr default_sentinel_t
  end() const noexcept {
    return default_sentinel;
  }
};

template<class Comp, class R1, class R2>
set_difference_view(Comp, R1&&, R2&&)
  -> set_difference_view<Comp, views::all_t<R1>, views::all_t<R2>>;

namespace views {
namespace __detail {
template<class Comp, class R1, class R2>
concept __can_set_difference_view =
  requires { set_difference_view(std::declval<Comp>(), std::declval<R1>(), std::declval<R2>()); };
}  // namespace __detail

struct SetDifferenceBy {
  template<class Comp, class R1, class R2>
    requires __detail::__can_set_difference_view<Comp, R1, R2>
  constexpr auto
  operator() [[nodiscard]] (Comp&& comp, R1&& r1, R2&& r2) const {
    return set_difference_view(std::forward<Comp>(comp), std::forward<R1>(r1),
                               std::forward<R2>(r2));
  }
};

inline constexpr SetDifferenceBy set_difference_by;

struct SetDifference {
  template<class R1, class R2>
    requires __detail::__can_set_difference_view<ranges::less, R1, R2>
  constexpr auto
  operator() [[nodiscard]] (R1&& r1, R2&& r2) const {
    return set_difference_view(ranges::less{}, std::forward<R1>(r1), std::forward<R2>(r2));
  }
};

inline constexpr SetDifference set_difference;
}  // namespace views

}  // namespace std::ranges
