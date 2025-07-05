#pragma once

template <typename T, typename Deleter>
class Scoped
{
public:
	Scoped(T resource, Deleter deleter)
		: _resource(resource), _deleter(deleter), _valid(true)
	{}

	~Scoped()
	{
		if (_valid) {
			_deleter(_resource);
		}
	}

	T get() const { return _resource; }

	// 禁用复制
	Scoped(const Scoped&) = delete;
	Scoped& operator=(const Scoped&) = delete;

	// 支持移动
	Scoped(Scoped&& other) noexcept
		: _resource(other._resource), _deleter(other._deleter), _valid(other._valid)
	{
		other._valid = false;
	}

	Scoped& operator=(Scoped&& other) noexcept {
		if (this != &other) {
			if (_valid) { _deleter(_resource); };
			_resource = other._resource;
			_deleter = other._deleter;
			_valid = other._valid;
			other._valid = false;
		}
		return *this;
	}

private:
	T _resource;
	Deleter _deleter;
	bool _valid;
};