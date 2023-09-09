#pragma once

#include <Windows.h>

class SRWReadLockGuard
{
public:
	SRWReadLockGuard() = delete;
	SRWReadLockGuard(PSRWLOCK lock) : lock_(lock)
	{
		AcquireSRWLockShared(lock_);
	}

	~SRWReadLockGuard()
	{
		ReleaseSRWLockShared(lock_);
	}

private:
	PSRWLOCK lock_ = nullptr;
};

class SRWWriteLockGuard
{
public:
	SRWWriteLockGuard() = delete;
	SRWWriteLockGuard(PSRWLOCK lock) : lock_(lock)
	{
		AcquireSRWLockExclusive(lock_);
	}

	~SRWWriteLockGuard()
	{
		ReleaseSRWLockExclusive(lock_);
	}

private:
	PSRWLOCK lock_ = nullptr;
};