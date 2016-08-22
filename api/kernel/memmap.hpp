// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef KERNEL_MEMMAP_HPP
#define KERNEL_MEMMAP_HPP

// std
#include <map>
#include <sstream>

//IncludeOS
#include <delegate>
#include <common>

/** An actual range of memory. */
using Span = gsl::span<uint8_t>;
static constexpr auto span_max() {
  return std::numeric_limits<std::ptrdiff_t>::max();
}

/** Exception for Fixed_memory_range */
class Memory_range_exception : public std::runtime_error {
  using runtime_error::runtime_error;
};


/** A description of a fixed and occupied memory range.  */
class Fixed_memory_range {

public:

  using In_use_delg = delegate<size_t()>;

  void set_in_use_delg(In_use_delg del) {
    in_use_ = del;
  };

  /** Constructors **/
  Fixed_memory_range(uintptr_t begin, uintptr_t end, const char* name, const std::string descr)
    : name_{name}, description_{descr}
  {

    if (begin > end)
      throw Memory_range_exception("Start is larger than end: " + 
          std::to_string(begin) + " > " + std::to_string(end));
    if (end - begin > span_max())
      throw Memory_range_exception("Maximum range size is " + std::to_string(span_max()));

    range_ = Span((uint8_t*)begin, end - begin);
  }

  Fixed_memory_range(uintptr_t begin, uintptr_t end, const char* name)
    : Fixed_memory_range(begin, end, name, "N/A")
  {}

  Fixed_memory_range(Span&& range, const char* name, const std::string descr)
    : range_{std::move(range)}, name_{name}, description_{descr}
  {}

  Fixed_memory_range(Span&& range, const char* name)
    : Fixed_memory_range(std::move(range), name, "N/A")
  {}

  /** Const getters */
  size_t size() const { return range_.size(); }
  const Span cspan() const { return range_; }
  const char* name() const { return name_; }
  const std::string description() const { return description_; }

  uintptr_t addr_start() const noexcept {
    return reinterpret_cast<uintptr_t>(range_.data());
  }

  uintptr_t addr_end() const noexcept {
    return reinterpret_cast<uintptr_t>(range_.data() + range_.size());
  }

  bool in_range(uintptr_t addr) const {
    return addr >= addr_start() and addr <= addr_end();
  }

  bool overlaps(const Fixed_memory_range& other) const {
    // Other range overlaps with my range
    return in_range(other.addr_start()) or in_range(other.addr_end())
      // Or my range is inside other range
      or (other.in_range(addr_start()) and other.in_range(addr_end()));
  }

  std::string to_string() const {

    std::stringstream out;
    out << name_ << " " << std::hex << addr_start() << " - "
        << addr_end() << " (" << description_
        << ", " << std::dec
        << range_.size() <<  " bytes) ";
    return out.str();
  }

  /** Mutable getters */
  Span span() { return range_; }
  operator Span () { return range_; }

  /** Iterators */
  auto begin() { return range_.begin(); }
  auto end() { return range_.end(); }
  auto cbegin() { return range_.cbegin(); }
  auto cend() { return range_.cend(); }


private:
  Span range_;
  const char* name_;
  const std::string description_;
  In_use_delg in_use_ = [&]{ return range_.size(); };

};

class Memory_map {

public:

  using Map = std::map<uintptr_t, Fixed_memory_range>;

  /**
   * Assign a fixed range of memory to a named purpose.
   * @note : The requested range cannot overlap with an existing range.
   * @note : The return value is the only way to access a mutable version of this range
   **/
  Fixed_memory_range& assign_range (Fixed_memory_range&& rng);

  //
  // Act as a map in safe ways
  //

  operator const Map&(){
    return map_;
  }

  const Map& map() {
    return map_;
  }

  const Map::const_iterator begin() { return map_.cbegin(); };
  const Map::const_iterator end() { return map_.cend(); };
  auto size() { return map_.size(); }
  auto empty() { return map_.empty(); }

private:
  Map map_;

};


#endif
