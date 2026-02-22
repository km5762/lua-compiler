template <class... Ts> struct Overloads : Ts... {
  using Ts::operator()...;
};
