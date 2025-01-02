#pragma once
#include <string>

/// <summary>
/// The kind of error message that occurred.
/// </summary>
enum class PhotinoErrorKind
{
	NoError = 0,
	GenericError
};

/// <summary>
/// Clears the last error message to occur.
/// </summary>
void ClearErrorMessage() noexcept;

/// <summary>
/// Gets the length of the message of the last error to occur.
/// </summary>
/// <returns>The length of the last error to occur on this thread.</returns>
int GetErrorMessageLength() noexcept;

/// <summary>
/// Gets the message of the last error to occur.
/// </summary>
/// <param name="length">The length of the buffer.</param>
/// <param name="buffer">The buffer to which the message should be written.</param>
void GetErrorMessage(int length, char* buffer) noexcept;

/// <summary>
/// Sets the message of the last error to occur.
/// </summary>
/// <param name="message">The message.</param>
void SetErrorMessage(std::string message) noexcept;