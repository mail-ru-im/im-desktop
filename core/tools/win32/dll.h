#pragma once

#define CORE_TOOLS_WIN32_NS_BEGIN namespace core { namespace tools { namespace win32 {
#define CORE_TOOLS_WIN32_NS_END } } }

CORE_TOOLS_WIN32_NS_BEGIN

template<class T>
class imported_proc : boost::noncopyable
{
public:
	typedef std::shared_ptr<imported_proc> sptr;

	imported_proc(const HMODULE _module_handle, const T _proc_address);

	virtual ~imported_proc();

	T get() const;

private:
	const HMODULE module_handle_;

	const T proc_address_;

};

template<class T>
imported_proc<T>::imported_proc(const HMODULE _module_handle, const T _proc_address)
	: module_handle_(_module_handle)
	, proc_address_(_proc_address)
{
	assert(module_handle_);
	assert(proc_address_);
}

template<class T>
imported_proc<T>::~imported_proc()
{
	assert(module_handle_);

	::FreeLibrary(module_handle_);
}

template<class T>
T imported_proc<T>::get() const
{
	assert(proc_address_);

	return proc_address_;
}

template<class T>
typename imported_proc<T>::sptr import_proc(const std::wstring &dll_path, const std::string &proc_name)
{
	assert(!dll_path.empty());
	assert(!proc_name.empty());

	const auto handle = ::LoadLibraryEx(dll_path.c_str(), nullptr, 0);
	if (!handle)
	{
		return imported_proc<T>::sptr();
	}

	const auto proc = (T)::GetProcAddress(handle, proc_name.c_str());
	if (!proc)
	{
		::FreeLibrary(handle);

		return imported_proc<T>::sptr();
	}

	return imported_proc<T>::sptr(
		new imported_proc<T>(handle, proc)
	);
}

CORE_TOOLS_WIN32_NS_END
