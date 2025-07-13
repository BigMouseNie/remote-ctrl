#pragma once

#include <iostream>

template <typename T>
class Singleton {
public:
	Singleton(const Singleton&) = delete;
	Singleton& operator=(const Singleton&) = delete;

protected:
	Singleton() = default;
	virtual ~Singleton() = default;

public:
	static T* GetInstance() {
		static T instance;
		return &instance;
	}
};
