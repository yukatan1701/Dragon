#ifndef __DRAGON_BIMAP__
#define __DRAGON_BIMAP__

#include <set>
#include <map>
#include <optional>
#include <initializer_list>

template <typename T1, typename T2>
class Bimap {
public:
  Bimap() {}
  
  Bimap(std::initializer_list<std::pair<T1, T2>> List) {
    for (auto &Pair : List)
      insert(Pair.first, Pair.second);
  }

  bool insert(const T1 &KeyLeft, const T2 &KeyRight) {
    if (mKeysLeft.find(KeyLeft) != mKeysLeft.end() ||
        mKeysRight.find(KeyRight) != mKeysRight.end())
      return false;
    auto *ResLeftPtr = &(*mKeysLeft.insert(KeyLeft).first);
    auto *ResRightPtr = &(*mKeysRight.insert(KeyRight).first);
    mLeftToRight.insert(std::make_pair(ResLeftPtr, ResRightPtr));
    mRightToLeft.insert(std::make_pair(ResRightPtr, ResLeftPtr));
    return true;
  }

  bool hasValue(const T2 &Key) const {
    return mKeysRight.find(Key) != mKeysRight.end();
  }

  bool hasValue(const T1 &Key) const {
    return mKeysLeft.find(Key) != mKeysLeft.end();
  }

  std::optional<T1> getValue(const T2 &Key) const {
    auto Itr = mKeysRight.find(Key);
    if (Itr == mKeysRight.end())
      return std::nullopt;
    auto &RightValue = *Itr;
    auto MapItr = mRightToLeft.find(&RightValue);
    assert(MapItr != mRightToLeft.end() &&
          "Bimap must contain all values from lists!");
    return *(MapItr->second);
  }

  std::optional<T2> getValue(const T1 &Key) const {
    auto Itr = mKeysLeft.find(Key);
    if (Itr == mKeysLeft.end())
      return std::nullopt;
    auto &LeftValue = *Itr;
    auto MapItr = mLeftToRight.find(&LeftValue);
    assert(MapItr != mLeftToRight.end() &&
          "Bimap must contain all values from lists!");
    return *(MapItr->second);
  }
private:
  std::set<T1> mKeysLeft;
  std::set<T2> mKeysRight;
  std::map<const T1 *, const T2 *> mLeftToRight;
  std::map<const T2 *, const T1 *> mRightToLeft; 
};

#endif