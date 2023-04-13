#pragma once

#ifndef PHOTINO_DIALOG_H
#define PHOTINO_DIALOG_H

#include "Photino.h"

#ifdef __APPLE__
#include <Cocoa/Cocoa.h>
#endif

enum class DialogResult
{
	Cancel = -1,
	Ok,
	Yes,
	No,
	Abort,
	Retry,
	Ignore,
};

enum class DialogButtons
{
	Ok,
	OkCancel,
	YesNo,
	YesNoCancel,
	RetryCancel,
	AbortRetryIgnore,
};

enum class DialogIcon
{
	Info,
	Warning,
	Error,
	Question,
};

class PhotinoDialog
{
public:
#ifdef _WIN32
	PhotinoDialog(Photino *window);
#else
	PhotinoDialog();
#endif
	~PhotinoDialog();

	AutoString *ShowOpenFile(AutoString title, AutoString defaultPath, bool multiSelect, AutoString *filters, int filterCount, int *resultCount);
	AutoString *ShowOpenFolder(AutoString title, AutoString defaultPath, bool multiSelect, int *resultCount);
	AutoString ShowSaveFile(AutoString title, AutoString defaultPath, AutoString *filters, int filterCount);
	DialogResult ShowMessage(AutoString title, AutoString text, DialogButtons buttons, DialogIcon icon);

protected:
#ifdef __APPLE__
	NSImage *_errorIcon;
	NSImage *_infoIcon;
	NSImage *_questionIcon;
	NSImage *_warningIcon;
#elif _WIN32
	Photino *_window;
#endif
};

#endif
