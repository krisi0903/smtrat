#pragma once

#include <map>
#include <set>
#include <vector>

#include "../Common.h"

namespace smtrat {
namespace cad {

class BaseCADConstraints {
public:
	using Callback = std::function<void(const UPoly&, std::size_t)>;
protected:
	struct ConstraintComparator {
		std::size_t complexity(const ConstraintT& c) const {
			return c.maxDegree() * c.variables().size() * c.lhs().size();
		}
		bool operator()(const ConstraintT& lhs, const ConstraintT& rhs) const {
			return complexity(lhs) < complexity(rhs);
		}
	};
	Variables mVariables;
	Callback mAddCallback;
	Callback mRemoveCallback;
	std::set<ConstraintT, ConstraintComparator> mOrderedConstraints;
	void callCallback(const Callback& cb, const ConstraintT& c, std::size_t id) const {
		if (cb) cb(c.lhs().toUnivariatePolynomial(mVariables.front()), id);
	}
public:
	BaseCADConstraints(const Callback& onAdd, const Callback& onRemove): mAddCallback(onAdd), mRemoveCallback(onRemove) {}
	void reset(const Variables& vars) {
		mVariables = vars;
		mOrderedConstraints.clear();
	}
	auto ordered() const {
		return mOrderedConstraints;
	}
};

template<Backtracking BT>
class CADConstraints {};

template<>
class CADConstraints<Backtracking::ORDERED>: public BaseCADConstraints {
private:
	using Super = BaseCADConstraints;
	using Super::Callback;
	std::vector<ConstraintT> mConstraints;
public:
	CADConstraints(const Callback& onAdd, const Callback& onRemove): Super(onAdd, onRemove) {}
	void reset(const Variables& vars) {
		Super::reset(vars);
		mConstraints.clear();
	}
	auto get() const {
		return mConstraints;
	}
	auto begin() const {
		return mConstraints.begin();
	}
	auto end() const {
		return mConstraints.end();
	}
	void add(const ConstraintT& c) {
		assert(!mVariables.empty());
		mOrderedConstraints.insert(c);
		mConstraints.push_back(c);
		callCallback(mAddCallback, c, mConstraints.size()-1);
	}
	void remove(const ConstraintT& c) {
		mOrderedConstraints.erase(c);
		std::stack<ConstraintT> cache;
		// Remove constraints added after c
		while (!mConstraints.empty() && mConstraints.back() != c) {
			callCallback(mRemoveCallback, mConstraints.back(), mConstraints.size()-1);
			cache.push(mConstraints.back());
			mConstraints.pop_back();
		}
		assert(mConstraints.back() == c);
		// Remove c
		callCallback(mRemoveCallback, mConstraints.back(), mConstraints.size()-1);
		mConstraints.pop_back();
		// Add constraints removed before
		while (!cache.empty()) {
			callCallback(mAddCallback, cache.top(), mConstraints.size());
			mConstraints.push_back(cache.top());
			cache.pop();
		}
	}
	friend std::ostream& operator<<(std::ostream& os, const CADConstraints<Backtracking::ORDERED>& cadc) {
		std::size_t id = 0;
		for (const auto& c: cadc.mConstraints) os << "\t" << id++ << ": " << c << std::endl;
		return os;
	}
};

template<>
class CADConstraints<Backtracking::UNORDERED>: public BaseCADConstraints {
public:
	template<typename Key, typename Value>
	struct MapKeyIterator : std::map<Key,Value>::const_iterator {
		using Super = typename std::map<Key,Value>::const_iterator;
		MapKeyIterator(): Super() {};
		MapKeyIterator(Super it_): Super(it_) {};
		Key *operator->() { return (Key*const) &(Super::operator->()->first); }
		Key operator*() { return Super::operator*().first; }
	};
private:
	using Super = BaseCADConstraints;
	using Super::Callback;
	using MapIt = MapKeyIterator<ConstraintT,std::size_t>;
	std::map<ConstraintT, std::size_t> mConstraints;
	IDPool mIDPool;
public:
	CADConstraints(const Callback& onAdd, const Callback& onRemove): Super(onAdd, onRemove) {}
	void reset(const Variables& vars) {
		Super::reset(vars);
		mConstraints.clear();
	}
	auto get() const {
		return mConstraints;
	}
	auto begin() const {
		return MapIt(mConstraints.begin());
	}
	auto end() const {
		return MapIt(mConstraints.end());
	}
	void add(const ConstraintT& c) {
		assert(!mVariables.empty());
		std::size_t id = mIDPool.get();
		auto res = mConstraints.emplace(c, id);
		assert(res.second);
		callCallback(mAddCallback, c, id);
	}
	void remove(const ConstraintT& c) {
		auto it = mConstraints.find(c);
		assert(it != mConstraints.end());
		std::size_t id = it->second;
		callCallback(mRemoveCallback, c, id);
		mConstraints.erase(it);
		mIDPool.free(id);
	}
	friend std::ostream& operator<<(std::ostream& os, const CADConstraints<Backtracking::UNORDERED>& cadc) {
		for (const auto& c: cadc.mConstraints) os << "\t" << c.second << ": " << c.first << std::endl;
		return os;
	}
};

	
}
}