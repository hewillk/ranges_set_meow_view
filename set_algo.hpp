#include <ranges>

namespace std::ranges::__detail {
template<class R1, class R2>
concept __set_associable = input_range<R1> && input_range<R2> &&
  indirect_strict_weak_order<ranges::less, iterator_t<R1>, iterator_t<R2>>;

template<class R1, class R2>
concept __set_associable_concatable = __set_associable<R1, R2> && __concatable<R1, R2>;
}  // namespace std::ranges::__detail

namespace std::ranges {
template<view V1, view V2>
  requires __detail::__set_associable<V1, V2>
class set_difference_view {
  V1 base1_ = V1();
  V2 base2_ = V2();

  template<bool Const>
  class iterator {
    friend set_difference_view;

    using Base1 = __detail::__maybe_const_t<Const, V1>;
    using Base2 = __detail::__maybe_const_t<Const, V2>;
    iterator_t<Base1> current1_ = iterator_t<Base1>();
    sentinel_t<Base1> end1_ = sentinel_t<Base1>();
    iterator_t<Base2> current2_ = iterator_t<Base2>();
    sentinel_t<Base2> end2_ = sentinel_t<Base2>();

    constexpr void
    satisfy() {
      while (current1_ != end1_) {
        if (current2_ == end2_)
          return;
        if (*current1_ < *current2_)
          return;
        if (!(*current2_ < *current1_))
          ++current1_;
        ++current2_;
      }
    }

    constexpr explicit iterator(iterator_t<Base1> current1, sentinel_t<Base1> end1,
                                iterator_t<Base2> current2, sentinel_t<Base2> end2)
      : current1_(std::move(current1)),
        end1_(std::move(end1)),
        current2_(std::move(current2)),
        end2_(std::move(end2)) {
      satisfy();
    }

   public:
    using value_type = range_value_t<Base1>;
    using difference_type = range_difference_t<Base1>;
    using iterator_concept =
      conditional_t<forward_range<Base1>, forward_iterator_tag, input_iterator_tag>;

    iterator()
      requires default_initializable<iterator_t<Base1>> && default_initializable<iterator_t<Base2>>
    = default;

    iterator(iterator<!Const> i)
      requires Const && convertible_to<iterator_t<V1>, iterator_t<Base1>> &&
                 convertible_to<sentinel_t<V1>, sentinel_t<Base1>> &&
                 convertible_to<iterator_t<V2>, iterator_t<Base2>> &&
                 convertible_to<sentinel_t<V2>, sentinel_t<Base2>>
      : current1_(std::move(i.current1_)),
        end1_(std::move(i.end1)),
        current2_(std::move(i.current2_)),
        end2_(std::move(i.end2)) { }

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
      requires forward_range<Base1> && forward_range<Base2>
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    friend constexpr bool
    operator==(const iterator& x, const iterator& y)
      requires equality_comparable<iterator_t<Base1>>
    {
      return x.current1_ == y.current1_;
    }

    friend constexpr bool
    operator==(const iterator& x, default_sentinel_t) {
      return x.current1_ == x.end1_;
    }

    friend constexpr decltype(auto)
    iter_move(const iterator& i) noexcept(noexcept(ranges::iter_move(i.current1_))) {
      return ranges::iter_move(i.current1_);
    }
  };

 public:
  set_difference_view()
    requires default_initializable<V1> && default_initializable<V2>
  = default;

  constexpr explicit set_difference_view(V1 base1, V2 base2)
    : base1_(std::move(base1)), base2_(std::move(base2)) { }

  constexpr auto
  begin()
    requires(!__detail::__simple_view<V1> || __detail::__simple_view<V2>)
  {
    return iterator<false>(ranges::begin(base1_), ranges::end(base1_), ranges::begin(base2_),
                           ranges::end(base2_));
  }

  constexpr auto
  begin() const
    requires __detail::__set_associable<const V1, const V2>
  {
    return iterator<true>(ranges::begin(base1_), ranges::end(base1_), ranges::begin(base2_),
                          ranges::end(base2_));
  }

  constexpr default_sentinel_t
  end() const noexcept {
    return default_sentinel;
  }
};

template<class R1, class R2>
set_difference_view(R1&&, R2&&) -> set_difference_view<views::all_t<R1>, views::all_t<R2>>;

template<class V1, class V2>
constexpr bool enable_borrowed_range<set_difference_view<V1, V2>> =
  enable_borrowed_range<V1> && enable_borrowed_range<V2>;

namespace views {
struct _SetDifference : __adaptor::_RangeAdaptor<_SetDifference> {
  template<class R1, class R2>
    requires requires(R1&& r1, R2&& r2) {
      set_difference_view(std::forward<R1>(r1), std::forward<R2>(r2));
    }
  constexpr auto
  operator()(R1&& r1, R2&& r2) const {
    return set_difference_view(std::forward<R1>(r1), std::forward<R2>(r2));
  }

  using _RangeAdaptor<_SetDifference>::operator();
  static constexpr int _S_arity = 2;
  template<class R2>
  static constexpr bool _S_has_simple_extra_args = (view<R2> && copy_constructible<R2>);
};
inline constexpr _SetDifference set_difference;
}  // namespace views

}  // namespace std::ranges

namespace std::ranges {
template<view V1, view V2>
  requires __detail::__set_associable<V1, V2>
class set_intersection_view {
  V1 base1_ = V1();
  V2 base2_ = V2();

  template<bool Const>
  class iterator {
    friend set_intersection_view;

    using Base1 = __detail::__maybe_const_t<Const, V1>;
    using Base2 = __detail::__maybe_const_t<Const, V2>;
    iterator_t<Base1> current1_ = iterator_t<Base1>();
    sentinel_t<Base1> end1_ = sentinel_t<Base1>();
    iterator_t<Base2> current2_ = iterator_t<Base2>();
    sentinel_t<Base2> end2_ = sentinel_t<Base2>();

    constexpr void
    satisfy() {
      while (current1_ != end1_ && current2_ != end2_) {
        if (*current1_ < *current2_)
          ++current1_;
        else {
          if (!(*current2_ < *current1_))
            return;
          ++current2_;
        }
      }
    }

    constexpr explicit iterator(iterator_t<Base1> current1, sentinel_t<Base1> end1,
                                iterator_t<Base2> current2, sentinel_t<Base2> end2)
      : current1_(std::move(current1)),
        end1_(std::move(end1)),
        current2_(std::move(current2)),
        end2_(std::move(end2)) {
      satisfy();
    }

   public:
    using value_type = range_value_t<Base1>;
    using difference_type = range_difference_t<Base1>;
    using iterator_concept =
      conditional_t<forward_range<Base1>, forward_iterator_tag, input_iterator_tag>;

    iterator()
      requires default_initializable<iterator_t<Base1>> && default_initializable<iterator_t<Base2>>
    = default;

    iterator(iterator<!Const> i)
      requires Const && convertible_to<iterator_t<V1>, iterator_t<Base1>> &&
                 convertible_to<sentinel_t<V1>, sentinel_t<Base1>> &&
                 convertible_to<iterator_t<V2>, iterator_t<Base2>> &&
                 convertible_to<sentinel_t<V2>, sentinel_t<Base2>>
      : current1_(std::move(i.current1_)),
        end1_(std::move(i.end1)),
        current2_(std::move(i.current2_)),
        end2_(std::move(i.end2)) { }

    constexpr decltype(auto)
    operator*() const {
      return *current1_;
    }

    constexpr iterator&
    operator++() {
      ++current1_;
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
      requires forward_range<Base1> && forward_range<Base2>
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    friend constexpr bool
    operator==(const iterator& x, const iterator& y)
      requires equality_comparable<iterator_t<Base1>>
    {
      return x.current1_ == y.current1_;
    }

    friend constexpr bool
    operator==(const iterator& x, default_sentinel_t) {
      return x.current1_ == x.end1_ || x.current2_ == x.end2_;
    }

    friend constexpr decltype(auto)
    iter_move(const iterator& i) noexcept(noexcept(ranges::iter_move(i.current1_))) {
      return ranges::iter_move(i.current1_);
    }
  };

 public:
  set_intersection_view()
    requires default_initializable<V1> && default_initializable<V2>
  = default;

  constexpr explicit set_intersection_view(V1 base1, V2 base2)
    : base1_(std::move(base1)), base2_(std::move(base2)) { }

  constexpr auto
  begin()
    requires(!__detail::__simple_view<V1> || __detail::__simple_view<V2>)
  {
    return iterator<false>(ranges::begin(base1_), ranges::end(base1_), ranges::begin(base2_),
                           ranges::end(base2_));
  }

  constexpr auto
  begin() const
    requires __detail::__set_associable<const V1, const V2>
  {
    return iterator<true>(ranges::begin(base1_), ranges::end(base1_), ranges::begin(base2_),
                          ranges::end(base2_));
  }

  constexpr default_sentinel_t
  end() const noexcept {
    return default_sentinel;
  }
};

template<class R1, class R2>
set_intersection_view(R1&&, R2&&) -> set_intersection_view<views::all_t<R1>, views::all_t<R2>>;

template<class V1, class V2>
constexpr bool enable_borrowed_range<set_intersection_view<V1, V2>> =
  enable_borrowed_range<V1> && enable_borrowed_range<V2>;

namespace views {
struct _SetIntersection : __adaptor::_RangeAdaptor<_SetIntersection> {
  template<class R1, class R2>
    requires requires(R1&& r1, R2&& r2) {
      set_intersection_view(std::forward<R1>(r1), std::forward<R2>(r2));
    }
  constexpr auto
  operator()(R1&& r1, R2&& r2) const {
    return set_intersection_view(std::forward<R1>(r1), std::forward<R2>(r2));
  }

  using _RangeAdaptor<_SetIntersection>::operator();
  static constexpr int _S_arity = 2;
  template<class R2>
  static constexpr bool _S_has_simple_extra_args = (view<R2> && copy_constructible<R2>);
};
inline constexpr _SetIntersection set_intersection;
}  // namespace views

}  // namespace std::ranges

namespace std::ranges {
template<view V1, view V2>
  requires __detail::__set_associable_concatable<V1, V2>
class set_union_view {
  V1 base1_ = V1();
  V2 base2_ = V2();

  template<bool Const>
  class iterator {
    friend set_union_view;

    using Base1 = __detail::__maybe_const_t<Const, V1>;
    using Base2 = __detail::__maybe_const_t<Const, V2>;
    enum class set_state { first, second };
    iterator_t<Base1> current1_ = iterator_t<Base1>();
    sentinel_t<Base1> end1_ = sentinel_t<Base1>();
    iterator_t<Base2> current2_ = iterator_t<Base2>();
    sentinel_t<Base2> end2_ = sentinel_t<Base2>();
    set_state state_ = set_state::first;

    constexpr void
    satisfy() {
      if (current1_ == end1_) {
        state_ = set_state::second;
        return;
      }
      if (current2_ == end2_) {
        state_ = set_state::first;
        return;
      }
      if (*current2_ < *current1_) {
        state_ = set_state::second;
        return;
      }
      if (!(*current1_ < *current2_))
        ++current2_;
      state_ = set_state::first;
    }

    constexpr explicit iterator(iterator_t<Base1> current1, sentinel_t<Base1> end1,
                                iterator_t<Base2> current2, sentinel_t<Base2> end2)
      : current1_(std::move(current1)),
        end1_(std::move(end1)),
        current2_(std::move(current2)),
        end2_(std::move(end2)) {
      satisfy();
    }

   public:
    using value_type = __detail::__concat_value_t<Base1, Base2>;
    using difference_type = common_type_t<range_difference_t<Base1>, range_difference_t<Base2>>;
    using iterator_concept = conditional_t<forward_range<Base1> && forward_range<Base2>,
                                           forward_iterator_tag, input_iterator_tag>;

    iterator()
      requires default_initializable<iterator_t<Base1>> && default_initializable<iterator_t<Base2>>
    = default;

    iterator(iterator<!Const> i)
      requires Const && convertible_to<iterator_t<V1>, iterator_t<Base1>> &&
                 convertible_to<sentinel_t<V1>, sentinel_t<Base1>> &&
                 convertible_to<iterator_t<V2>, iterator_t<Base2>> &&
                 convertible_to<sentinel_t<V2>, sentinel_t<Base2>>
      : current1_(std::move(i.current1_)),
        end1_(std::move(i.end1)),
        current2_(std::move(i.current2_)),
        end2_(std::move(i.end2)),
        state_(i.state_) { }

    constexpr __detail::__concat_reference_t<Base1, Base2>
    operator*() const {
      if (state_ == set_state::first)
        return *current1_;
      return *current2_;
    }

    constexpr iterator&
    operator++() {
      if (state_ == set_state::first)
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
      requires forward_range<Base1> && forward_range<Base2>
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    friend constexpr bool
    operator==(const iterator& x, const iterator& y)
      requires equality_comparable<iterator_t<Base1>> && equality_comparable<iterator_t<Base2>>
    {
      return x.current1_ == y.current1_ && x.current2_ == y.current2_;
    }

    friend constexpr bool
    operator==(const iterator& x, default_sentinel_t) {
      return x.current1_ == x.end1_ && x.current2_ == x.end2_;
    }

    friend constexpr __detail::__concat_rvalue_reference_t<Base1, Base2>
    iter_move(const iterator& i) {
      if (i.state_ == set_state::first)
        return ranges::iter_move(i.current1_);
      return ranges::iter_move(i.current2_);
    }
  };

 public:
  set_union_view()
    requires default_initializable<V1> && default_initializable<V2>
  = default;

  constexpr explicit set_union_view(V1 base1, V2 base2)
    : base1_(std::move(base1)), base2_(std::move(base2)) { }

  constexpr auto
  begin()
    requires(!__detail::__simple_view<V1> || __detail::__simple_view<V2>)
  {
    return iterator<false>(ranges::begin(base1_), ranges::end(base1_), ranges::begin(base2_),
                           ranges::end(base2_));
  }

  constexpr auto
  begin() const
    requires __detail::__set_associable_concatable<const V1, const V2>
  {
    return iterator<true>(ranges::begin(base1_), ranges::end(base1_), ranges::begin(base2_),
                          ranges::end(base2_));
  }

  constexpr default_sentinel_t
  end() const noexcept {
    return default_sentinel;
  }
};

template<class R1, class R2>
set_union_view(R1&&, R2&&) -> set_union_view<views::all_t<R1>, views::all_t<R2>>;

template<class V1, class V2>
constexpr bool enable_borrowed_range<set_union_view<V1, V2>> =
  enable_borrowed_range<V1> && enable_borrowed_range<V2>;

namespace views {
struct _SetUnion : __adaptor::_RangeAdaptor<_SetUnion> {
  template<class R1, class R2>
    requires requires(R1&& r1, R2&& r2) {
      set_union_view(std::forward<R1>(r1), std::forward<R2>(r2));
    }
  constexpr auto
  operator()(R1&& r1, R2&& r2) const {
    return set_union_view(std::forward<R1>(r1), std::forward<R2>(r2));
  }

  using _RangeAdaptor<_SetUnion>::operator();
  static constexpr int _S_arity = 2;
  template<class R2>
  static constexpr bool _S_has_simple_extra_args = (view<R2> && copy_constructible<R2>);
};
inline constexpr _SetUnion set_union;
}  // namespace views

}  // namespace std::ranges

namespace std::ranges {
template<view V1, view V2>
  requires __detail::__set_associable_concatable<V1, V2>
class set_symmetric_difference_view {
  V1 base1_ = V1();
  V2 base2_ = V2();

  template<bool Const>
  class iterator {
    friend set_symmetric_difference_view;

    using Base1 = __detail::__maybe_const_t<Const, V1>;
    using Base2 = __detail::__maybe_const_t<Const, V2>;
    enum class set_state { first, second, only_first, only_second };
    iterator_t<Base1> current1_ = iterator_t<Base1>();
    sentinel_t<Base1> end1_ = sentinel_t<Base1>();
    iterator_t<Base2> current2_ = iterator_t<Base2>();
    sentinel_t<Base2> end2_ = sentinel_t<Base2>();
    set_state state_ = set_state::first;

    constexpr void
    satisfy() {
      while (true) {
        if (current1_ == end1_) {
          state_ = set_state::only_second;
          return;
        }
        if (current2_ == end2_) {
          state_ = set_state::only_first;
          return;
        }
        if (*current1_ < *current2_) {
          state_ = set_state::first;
          return;
        }
        if (*current2_ < *current1_) {
          state_ = set_state::second;
          return;
        }
        ++current1_;
        ++current2_;
      }
    }

    constexpr explicit iterator(iterator_t<Base1> current1, sentinel_t<Base1> end1,
                                iterator_t<Base2> current2, sentinel_t<Base2> end2)
      : current1_(std::move(current1)),
        end1_(std::move(end1)),
        current2_(std::move(current2)),
        end2_(std::move(end2)) {
      satisfy();
    }

   public:
    using value_type = __detail::__concat_value_t<Base1, Base2>;
    using difference_type = common_type_t<range_difference_t<Base1>, range_difference_t<Base2>>;
    using iterator_concept = conditional_t<forward_range<Base1> && forward_range<Base2>,
                                           forward_iterator_tag, input_iterator_tag>;

    iterator()
      requires default_initializable<iterator_t<Base1>> && default_initializable<iterator_t<Base2>>
    = default;

    iterator(iterator<!Const> i)
      requires Const && convertible_to<iterator_t<V1>, iterator_t<Base1>> &&
                 convertible_to<sentinel_t<V1>, sentinel_t<Base1>> &&
                 convertible_to<iterator_t<V2>, iterator_t<Base2>> &&
                 convertible_to<sentinel_t<V2>, sentinel_t<Base2>>
      : current1_(std::move(i.current1_)),
        end1_(std::move(i.end1)),
        current2_(std::move(i.current2_)),
        end2_(std::move(i.end2)),
        state_(i.state_) { }

    constexpr __detail::__concat_reference_t<Base1, Base2>
    operator*() const {
      if (state_ == set_state::first || state_ == set_state::only_first)
        return *current1_;
      return *current2_;
    }

    constexpr iterator&
    operator++() {
      if (state_ == set_state::only_first)
        ++current1_;
      else if (state_ == set_state::only_second)
        ++current2_;
      else if (state_ == set_state::first) {
        ++current1_;
        satisfy();
      } else {
        ++current2_;
        satisfy();
      }
      return *this;
    }

    constexpr void
    operator++(int) {
      ++*this;
    }

    constexpr iterator
    operator++(int)
      requires forward_range<Base1> && forward_range<Base2>
    {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    friend constexpr bool
    operator==(const iterator& x, const iterator& y)
      requires equality_comparable<iterator_t<Base1>> && equality_comparable<iterator_t<Base2>>
    {
      return x.current1_ == y.current1_ && x.current2_ == y.current2_;
    }

    friend constexpr bool
    operator==(const iterator& x, default_sentinel_t) {
      return x.current1_ == x.end1_ && x.current2_ == x.end2_;
    }

    friend constexpr __detail::__concat_rvalue_reference_t<Base1, Base2>
    iter_move(const iterator& i) {
      if (i.state_ == set_state::first || i.state_ == set_state::only_first)
        return ranges::iter_move(i.current1_);
      return ranges::iter_move(i.current2_);
    }
  };

 public:
  set_symmetric_difference_view()
    requires default_initializable<V1> && default_initializable<V2>
  = default;

  constexpr explicit set_symmetric_difference_view(V1 base1, V2 base2)
    : base1_(std::move(base1)), base2_(std::move(base2)) { }

  constexpr auto
  begin()
    requires(!__detail::__simple_view<V1> || __detail::__simple_view<V2>)
  {
    return iterator<false>(ranges::begin(base1_), ranges::end(base1_), ranges::begin(base2_),
                           ranges::end(base2_));
  }

  constexpr auto
  begin() const
    requires __detail::__set_associable_concatable<const V1, const V2>
  {
    return iterator<true>(ranges::begin(base1_), ranges::end(base1_), ranges::begin(base2_),
                          ranges::end(base2_));
  }

  constexpr default_sentinel_t
  end() const noexcept {
    return default_sentinel;
  }
};

template<class R1, class R2>
set_symmetric_difference_view(R1&&, R2&&)
  -> set_symmetric_difference_view<views::all_t<R1>, views::all_t<R2>>;

template<class V1, class V2>
constexpr bool enable_borrowed_range<set_symmetric_difference_view<V1, V2>> =
  enable_borrowed_range<V1> && enable_borrowed_range<V2>;

namespace views {
struct _SetSymmetricDifference : __adaptor::_RangeAdaptor<_SetSymmetricDifference> {
  template<class R1, class R2>
    requires requires(R1&& r1, R2&& r2) {
      set_symmetric_difference_view(std::forward<R1>(r1), std::forward<R2>(r2));
    }
  constexpr auto
  operator()(R1&& r1, R2&& r2) const {
    return set_symmetric_difference_view(std::forward<R1>(r1), std::forward<R2>(r2));
  }

  using _RangeAdaptor<_SetSymmetricDifference>::operator();
  static constexpr int _S_arity = 2;
  template<class R2>
  static constexpr bool _S_has_simple_extra_args = (view<R2> && copy_constructible<R2>);
};
inline constexpr _SetSymmetricDifference set_symmetric_difference;
}  // namespace views

}  // namespace std::ranges
