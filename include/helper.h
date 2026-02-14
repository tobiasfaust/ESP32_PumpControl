#include <vector>
#include <algorithm>

// ---------------------------------------------------------------------------
// vector helper: push_back_unique
// Adds value to vector only if it does not already exist (using operator==)
// If replaceIfExists is true, replaces the existing value.
// Returns true if inserted, false if value already present.
// Usage:
//   std::vector<int> v;
//   push_back_unique(v, 42);            // free function
//   push_back_unique(v, 42, true);      // replace existing value if found
// ---------------------------------------------------------------------------
template<typename T, typename Alloc>
inline bool push_back_unique(std::vector<T, Alloc>& vec, const T& value, const bool replaceIfExists = false) {
  if (std::find(vec.begin(), vec.end(), value) == vec.end()) {
    vec.push_back(value);
    return true;
  }
  if (replaceIfExists) {
    auto it = std::find(vec.begin(), vec.end(), value);
    if (it != vec.end()) {
      *it = value; // replace existing value
      return true;
    }
  }
  return false;
}

// Pointer overload: pass a pointer to a std::vector. Returns false
// if pointer is null or value already exists; true if inserted.
// Usage:
//   std::vector<int>* vp = &v;
//   push_back_unique(vp, 7);
template<typename T, typename Alloc>
inline bool push_back_unique(std::vector<T, Alloc>* vecPtr, const T& value, const bool replaceIfExists = false) {
  if (!vecPtr) return false; // null: nothing to do
  // Forward to reference overload to avoid code duplication
  return push_back_unique(*vecPtr, value, replaceIfExists);
}