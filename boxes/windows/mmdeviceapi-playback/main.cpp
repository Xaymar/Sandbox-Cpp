#include <cinttypes>
#include <exception>
#include <memory>
#include <stdexcept>

#include <Windows.h>
#include <combaseapi.h>
#include <mmdeviceapi.h>

template<typename I, typename C>
std::shared_ptr<I> CoCreateInstance(DWORD context = CLSCTX_ALL, IUnknown* owner = nullptr)
{
	I*      instance = nullptr;
	HRESULT result   = CoCreateInstance(__uuidof(C), owner, context, __uuidof(I), reinterpret_cast<LPVOID*>(&instance));
	if (FAILED(result)) {
		throw std::runtime_error("CoCreateInstance failed");
	}

	return {instance, [](IUnknown* ptr) {
				if (ptr) {
					ptr->Release();
				}
			}};
}

int32_t main(int32_t argc, const char* argv[])
{
	auto enumerator = CoCreateInstance<IMMDeviceEnumerator, MMDeviceEnumerator>();

	return 0;
}
