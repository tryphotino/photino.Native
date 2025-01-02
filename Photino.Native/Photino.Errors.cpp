#include "Photino.Errors.h"
#include <mutex>
#include <string>

static thread_local std::string _lastError;

void ClearErrorMessage() noexcept
{
	_lastError.clear();
}

int GetErrorMessageLength() noexcept
{
	return _lastError.length();
}

void GetErrorMessage(int length, char* buffer) noexcept
{
	_lastError.copy(buffer, length, 0);
}

void SetErrorMessage(std::string message) noexcept
{
	_lastError = message;
}