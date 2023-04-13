#include "Photino.Dialog.h"

#include <cwchar>
#include <iostream>
#include <shobjidl.h>
#include <shlwapi.h>
#include <objbase.h>
#include <vector>

class Dll
{
public:
	explicit Dll(std::string const& name);
	~Dll();

	template<typename T> class Proc
	{
	public:
		Proc(Dll const& lib, std::string const& sym)
			: _mProc(static_cast<T*>((void*)GetProcAddress(lib._handle, sym.c_str())))
		{}

		explicit operator bool() const { return _mProc != nullptr; }
		explicit operator T* () const { return _mProc; }

	private:
		T* _mProc;
	};

private:
	HMODULE _handle;
};

inline Dll::Dll(std::string const& name)
	: _handle(LoadLibraryA(name.c_str()))
{}

inline Dll::~Dll()
{
	if (_handle)
		FreeLibrary(_handle);
}

class NewStyleContext
{
public:
	NewStyleContext();
	~NewStyleContext();

private:
	static HANDLE Create();
	ULONG_PTR _cookie = 0;
};

inline NewStyleContext::NewStyleContext()
{
	static HANDLE hctx = Create();

	if (hctx != INVALID_HANDLE_VALUE)
		ActivateActCtx(hctx, &_cookie);
}

inline NewStyleContext::~NewStyleContext()
{
	DeactivateActCtx(0, _cookie);
}

inline HANDLE NewStyleContext::Create()
{
	Dll comdlg32("comdlg32.dll");

	const UINT len = GetSystemDirectoryA(nullptr, 0);
	std::string sysDir(len, '\0');
	GetSystemDirectoryA(const_cast<LPSTR>(sysDir.data()), len);

	const ACTCTXA actCtx =
	{
		sizeof(actCtx),
		ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID,
		"shell32.dll", 0, 0, sysDir.c_str(), (LPCSTR)124, nullptr, nullptr,
	};

	return CreateActCtxA(&actCtx);
}

PhotinoDialog::PhotinoDialog(Photino* window)
{
	_window = window;
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
}

PhotinoDialog::~PhotinoDialog()
{
	CoUninitialize();
}

template<typename T>
T* Create(HRESULT* hResult, AutoString title, AutoString defaultPath)
{
	static_assert(std::is_base_of<IFileDialog, T>::value, "T must inherit from IFileDialog");
	T* pfd = nullptr;
	const CLSID clsid = typeid(T) == typeid(IFileOpenDialog) ? CLSID_FileOpenDialog : typeid(T) == typeid(IFileSaveDialog) ? CLSID_FileSaveDialog : CLSID_FileOpenDialog;
	HRESULT hr = CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
	if (SUCCEEDED(hr)) {
		pfd->SetTitle(title);

		if (defaultPath) {
			IShellItem* psiDefault = nullptr;
			hr = SHCreateItemFromParsingName(defaultPath, nullptr, IID_PPV_ARGS(&psiDefault));
			if (SUCCEEDED(hr)) {
				pfd->SetFolder(psiDefault);
				psiDefault->Release();
			}
		}

		*hResult = hr;
		return pfd;
	}
	return nullptr;
}

void AddFilters(IFileDialog* pfd, wchar_t** filters, const int filterCount)
{
	std::vector<COMDLG_FILTERSPEC> specs;
	for (int i = 0; i < filterCount; i++) {
		auto* filter = new wchar_t[MAX_PATH];
		wcscpy_s(filter, MAX_PATH, filters[i]);

		const wchar_t* filterName = wcstok_s(filter, L"|", &filter);
		const wchar_t* filterPattern = filter;
		COMDLG_FILTERSPEC spec;
		spec.pszName = filterName;
		spec.pszSpec = filterPattern;
		specs.push_back(spec);
	}
	pfd->SetFileTypes(filterCount, specs.data());
}

AutoString* GetResults(IFileOpenDialog* pfd, HRESULT* hr, int* resultCount)
{
	IShellItemArray* psiResults = nullptr;
	*hr = pfd->GetResults(&psiResults);
	if (SUCCEEDED(*hr)) {
		DWORD count = 0;
		psiResults->GetCount(&count);
		if (count > 0) {
			*resultCount = static_cast<int>(count);
			auto** result = new wchar_t* [count];
			for (DWORD i = 0; i < count; ++i) {
				IShellItem* psiItem = nullptr;
				*hr = psiResults->GetItemAt(i, &psiItem);
				if (SUCCEEDED(*hr)) {
					PWSTR pszName = nullptr;
					*hr = psiItem->GetDisplayName(SIGDN_FILESYSPATH, &pszName);
					if (SUCCEEDED(*hr)) {
						const auto len = wcslen(pszName);
						result[i] = new wchar_t[len + 1];
						wcscpy_s(result[i], len + 1, pszName);
						CoTaskMemFree(pszName);
					}
					psiItem->Release();
				}
			}
			psiResults->Release();
			pfd->Release();
			return result;
		}
		psiResults->Release();
	}
	pfd->Release();

	return nullptr;
}

AutoString* PhotinoDialog::ShowOpenFile(AutoString title, AutoString defaultPath, bool multiSelect, AutoString* filters, int filterCount, int* resultCount)
{
	HRESULT hr;
	auto* pfd = Create<IFileOpenDialog>(&hr, title, defaultPath);

	if (SUCCEEDED(hr)) {
		AddFilters(pfd, filters, filterCount);

		DWORD dwOptions;
		pfd->GetOptions(&dwOptions);
		dwOptions |= FOS_FILEMUSTEXIST | FOS_NOCHANGEDIR;
		if (multiSelect) {
			dwOptions |= FOS_ALLOWMULTISELECT;
		}
		else {
			dwOptions &= ~FOS_ALLOWMULTISELECT;
		}
		pfd->SetOptions(dwOptions);

		hr = pfd->Show(_window->getHwnd());
		if (SUCCEEDED(hr)) {
			return GetResults(pfd, &hr, resultCount);
		}
		pfd->Release();
	}
	return nullptr;
}

AutoString* PhotinoDialog::ShowOpenFolder(AutoString title, AutoString defaultPath, bool multiSelect, int* resultCount)
{
	HRESULT hr;
	auto* pfd = Create<IFileOpenDialog>(&hr, title, defaultPath);

	if (SUCCEEDED(hr)) {
		DWORD dwOptions;
		pfd->GetOptions(&dwOptions);
		dwOptions |= FOS_PICKFOLDERS | FOS_NOCHANGEDIR;
		if (multiSelect) {
			dwOptions |= FOS_ALLOWMULTISELECT;
		}
		else {
			dwOptions &= ~FOS_ALLOWMULTISELECT;
		}
		pfd->SetOptions(dwOptions);

		hr = pfd->Show(_window->getHwnd());
		if (SUCCEEDED(hr)) {
			return GetResults(pfd, &hr, resultCount);
		}
		pfd->Release();
	}
	return nullptr;
}

AutoString PhotinoDialog::ShowSaveFile(AutoString title, AutoString defaultPath, AutoString* filters, int filterCount)
{
	HRESULT hr;
	auto* pfd = Create<IFileSaveDialog>(&hr, title, defaultPath);
	if (SUCCEEDED(hr)) {
		AddFilters(pfd, filters, filterCount);

		DWORD dwOptions;
		pfd->GetOptions(&dwOptions);
		dwOptions |= FOS_NOCHANGEDIR;
		pfd->SetOptions(dwOptions);

		hr = pfd->Show(_window->getHwnd());
		if (SUCCEEDED(hr)) {
			IShellItem* psiResult = nullptr;
			hr = pfd->GetResult(&psiResult);
			if (SUCCEEDED(hr)) {
				wchar_t* result = nullptr;
				PWSTR pszName = nullptr;
				hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszName);
				if (SUCCEEDED(hr)) {
					const auto len = wcslen(pszName);
					result = new wchar_t[len + 1];
					wcscpy_s(result, len + 1, pszName);
					CoTaskMemFree(pszName);
				}
				psiResult->Release();
				pfd->Release();
				return result;
			}
		}
		pfd->Release();
	}
	return nullptr;
}

DialogResult PhotinoDialog::ShowMessage(AutoString title, AutoString text, DialogButtons buttons, DialogIcon icon)
{
	NewStyleContext ctx;

	UINT flags = {};

	switch (icon) {
		case DialogIcon::Info:	   flags |= MB_ICONINFORMATION;	break;
		case DialogIcon::Warning:  flags |= MB_ICONWARNING;	    break;
		case DialogIcon::Error:	   flags |= MB_ICONERROR;	    break;
		case DialogIcon::Question: flags |= MB_ICONQUESTION;    break;
	}

	switch (buttons) {
		case DialogButtons::Ok:               flags |= MB_OK;               break;
		case DialogButtons::OkCancel:         flags |= MB_OKCANCEL;         break;
		case DialogButtons::YesNo:			  flags |= MB_YESNO;			break;
		case DialogButtons::YesNoCancel:      flags |= MB_YESNOCANCEL;	    break;
		case DialogButtons::RetryCancel:	  flags |= MB_RETRYCANCEL;	    break;
		case DialogButtons::AbortRetryIgnore: flags |= MB_ABORTRETRYIGNORE; break;
	}

	const auto result = MessageBoxW(_window->getHwnd(), text, title, flags);

	switch (result) {
		case IDCANCEL: return DialogResult::Cancel;
		case IDOK:     return DialogResult::Ok;
		case IDYES:    return DialogResult::Yes;
		case IDNO:     return DialogResult::No;
		case IDABORT:  return DialogResult::Abort;
		case IDRETRY:  return DialogResult::Retry;
		case IDIGNORE: return DialogResult::Ignore;
		default:	   return DialogResult::Cancel;
	}
}