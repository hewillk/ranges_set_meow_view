#include <ranges>

namespace std::ranges {
template<class Comp, input_range V1, input_range V2>
  requires view<V1> && view<V2> && is_object_v<Comp> && is_object_v<Comp> &&
  indirect_strict_weak_order<Comp, iterator_t<V1>, iterator_t<V2>> && __detail::__concatable<V1, V2>
class set_symmetric_difference_view
  : public view_interface<set_symmetric_difference_view<Comp, V1, V2>> {
  V1 base1_ = V1();             // exposition only
  V2 base2_ = V2();             // exposition only
  __detail::__box<Comp> comp_;  // exposition only

  // [range.set.symmetric.difference.iterator], class set_symmetric_difference_view::iterator
  class iterator {
    friend set_symmetric_difference_view;

    iterator_t<V1> current1_ = iterator_t<V1>();  // exposition only
    iterator_t<V2> current2_ = iterator_t<V2>();  // exposition only
    set_symmetric_difference_view* parent_ = nullptr;
    bool use_first_ = false;  // exposition only

    constexpr void
    satisfy() {  // exposition only
      while (true) {
        if (current1_ == ranges::end(parent_->base1_)) {
          use_first_ = false;
          return;
        }
        if (current2_ == ranges::end(parent_->base2_)) {
          use_first_ = true;
          return;
        }
        if (std::__invoke(*parent_->comp_, *current1_, *current2_)) {
          use_first_ = true;
          return;
        }
        if (std::__invoke(*parent_->comp_, *current2_, *current1_)) {
          use_first_ = false;
          return;
        }
        ++current1_;
        ++current2_;
      }
    }

    constexpr iterator(set_symmetric_difference_view* parent,
                       iterator_t<V1> current1,  // exposition only
                       iterator_t<V2> current2)
      : parent_(parent), current1_(std::move(current1)), current2_(std::move(current2)) {
      satisfy();
    }

   public:
    // using iterator_category = see below;  // not always present
    using iterator_concept = conditional_t<forward_range<V1> && forward_range<V2>,
                                           forward_iterator_tag, input_iterator_tag>;
    using value_type = __detail::__concat_value_t<V1, V2>;
    using difference_type = common_type_t<range_difference_t<V1>, range_difference_t<V2>>;

    iterator()
      requires default_initializable<iterator_t<V1>> && default_initializable<iterator_t<V2>>
    = default;

    constexpr __detail::__concat_reference_t<V1, V2>
    operator*() const {
      if (use_first_)
        return *current1_;
      return *current2_;
    }

    constexpr iterator&
    operator++() {
      if (use_first_)
        ++current1_;
      else
        ++current2_;
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
      requires equality_comparable<iterator_t<V1>> && equality_comparable<iterator_t<V2>>
    {
      return x.current1_ == y.current1_ && x.current2_ == y.current2_;
    }

    friend constexpr bool
    operator==(const iterator& x, default_sentinel_t) {
      return x.current1_ == ranges::end(x.parent_->base1_) &&
        x.current2_ == ranges::end(x.parent_->base2_);
    }

    friend constexpr __detail::__concat_rvalue_reference_t<V1, V2>
    iter_move(const iterator& i) noexcept(
      noexcept(ranges::iter_move(i.current1_)) && noexcept(ranges::iter_move(i.current2_)) &&
      is_nothrow_convertible_v<range_rvalue_reference_t<V1>,
                               __detail::__concat_rvalue_reference_t<V1, V2>> &&
      is_nothrow_convertible_v<range_rvalue_reference_t<V2>,
                               __detail::__concat_rvalue_reference_t<V1, V2>>) {
      if (i.use_first_)
        return ranges::iter_move(i.current1_);
      return ranges::iter_move(i.current2_);
    }
  };  // exposition only

 public:
  set_symmetric_difference_view()
    requires default_initializable<V1> && default_initializable<V2>
  = default;

  constexpr explicit set_symmetric_difference_view(Comp comp, V1 base1, V2 base2)
    : comp_(std::move(comp)), base1_(std::move(base1)), base2_(std::move(base2)) {
    // __glibcxx_assert(ranges::is_sorted(base1_));
    // __glibcxx_assert(ranges::is_sorted(base2_));
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
set_symmetric_difference_view(Comp, R1&&, R2&&)
  -> set_symmetric_difference_view<Comp, views::all_t<R1>, views::all_t<R2>>;

namespace views {
namespace __detail {
template<class Comp, class R1, class R2>
concept __can_set_symmetric_difference_view_view = requires {
  set_symmetric_difference_view(std::declval<Comp>(), std::declval<R1>(), std::declval<R2>());
};
}  // namespace __detail

struct SetSymmetricDifferenceBy {
  template<class Comp, class R1, class R2>
    requires __detail::__can_set_symmetric_difference_view_view<Comp, R1, R2>
  constexpr auto
  operator() [[nodiscard]] (Comp&& comp, R1&& r1, R2&& r2) const {
    return set_symmetric_difference_view(std::forward<Comp>(comp), std::forward<R1>(r1),
                                         std::forward<R2>(r2));
  }
};

inline constexpr SetSymmetricDifferenceBy set_symmetric_difference_by;

struct SetSymmetricDifference {
  template<class R1, class R2>
    requires __detail::__can_set_symmetric_difference_view_view<ranges::less, R1, R2>
  constexpr auto
  operator() [[nodiscard]] (R1&& r1, R2&& r2) const {
    return set_symmetric_difference_view(ranges::less{}, std::forward<R1>(r1),
                                         std::forward<R2>(r2));
  }
};

inline constexpr SetSymmetricDifference set_symmetric_difference;
}  // namespace views

}  // namespace std::ranges
