#pragma once

template<typename BaseClass>
struct Inherit : BaseClass
{
  using Base = BaseClass;
};
