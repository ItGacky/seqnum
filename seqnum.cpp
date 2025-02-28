#include "seqnum.h"

const WCHAR* TEMPDIR = L"(seqnum)";

const struct {
	const WCHAR* ext;
	const WCHAR* normalized;
} TARGETS[] = {
	{ L".jpg", L".jpg" },
	{ L".jpeg", L".jpg" },
	{ L".jfif", L".jpg" },
	{ L".png", L".png" },
	{ L".gif", L".gif" },
};

static bool isFile(IShellItem* item) {
	SFGAOF attrs;
	return SUCCEEDED(item->GetAttributes(SFGAO_CANMOVE | SFGAO_CANRENAME | SFGAO_HIDDEN | SFGAO_FOLDER | SFGAO_FILESYSTEM, &attrs)) &&
		attrs == (SFGAO_CANMOVE | SFGAO_CANRENAME | SFGAO_FILESYSTEM);
}

static IShellItem* tempdir(const WCHAR* parent, WCHAR path[MAX_PATH]) {
	PathCombineW(path, parent, TEMPDIR);
	if (CreateDirectoryW(path, nullptr)) {
		IShellItem* item;
		if SUCCEEDED(SHCreateItemFromParsingName(path, nullptr, IID_PPV_ARGS(&item))) {
			SetFileAttributesW(path, FILE_ATTRIBUTE_HIDDEN);
			return item;
		} else {
			RemoveDirectoryW(path);
		}
	}
	return nullptr;
}

struct Entry {
	IShellItem* item;
	WCHAR* name;	// CoTaskMem
	const WCHAR* ext;	// static string
};

static const WCHAR* isTarget(const WCHAR* name) {
	if (auto ext = PathFindExtensionW(name)) {
		for (auto& i : TARGETS) {
			if (_wcsicmp(ext, i.ext) == 0)
				return i.normalized;
		}
	}
	return nullptr;
}

static size_t enumEntries(IShellItem* parent, std::vector<Entry>& entries) {
	size_t count = 0;
	IEnumShellItems* e;
	if SUCCEEDED(parent->BindToHandler(nullptr, BHID_EnumItems, IID_PPV_ARGS(&e))) {
		IShellItem* child;
		while (e->Next(1, &child, nullptr) == S_OK) {
			if (isFile(child)) {
				WCHAR* name;
				if SUCCEEDED(child->GetDisplayName(SIGDN_PARENTRELATIVEPARSING, &name)) {
					if (auto ext = isTarget(name)) {
						entries.push_back({ child, name, ext });
						child->AddRef();
						++count;
					}
					else {
						CoTaskMemFree(name);
					}
				}
			}
			child->Release();
		}
		e->Release();
	}
	return count;
}

static int log10ceil(size_t n) {
	int digit;
	for (digit = 0; n > 0; n /= 10) {
		++digit;
	}
	return digit;
}

static void run(int argc, WCHAR** argv) {
	IFileOperation* op;
	if SUCCEEDED(CoCreateInstance(__uuidof(FileOperation), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&op))) {
		for (int i = 0; i < argc; ++i) {
			WCHAR* arg = argv[i];
			WCHAR path[MAX_PATH];
			if (PathIsRelative(arg)) {
				GetCurrentDirectory(MAX_PATH, path);
				PathAppend(path, arg);
			} else {
				wcscpy_s(path, arg);
			}

			IShellItem* parent;
			if SUCCEEDED(SHCreateItemFromParsingName(path, nullptr, IID_PPV_ARGS(&parent))) {
				std::vector<Entry> entries;
				if (size_t count = enumEntries(parent, entries)) {
					// sort entries by name in logical order
					std::sort(entries.begin(), entries.end(), [](const Entry& lhs, const Entry& rhs) {
						if (int r = StrCmpLogicalW(lhs.name, rhs.name))
							return r < 0;
						else
							return wcscmp(lhs.name, rhs.name) < 0;
					});

					WCHAR temppath[MAX_PATH];
					IShellItem* temp = nullptr;
					WCHAR format[] = L"%08d%s";
					format[2] = L'0' + static_cast<WCHAR>(log10ceil(count));
					int i = 1;
					for (const auto& e: entries) {
						WCHAR newname[MAX_PATH];
						wsprintfW(newname, format, i, e.ext);
						if (wcscmp(e.name, newname)) {
							if (!temp) {
								temp = tempdir(path, temppath);
							}
							if (temp) {
								op->MoveItem(e.item, temp, newname, nullptr);
							}
						}
						CoTaskMemFree(e.name);
						e.item->Release();
						++i;
					}
					entries.clear();
					op->PerformOperations();
					
					if (temp) {
						IEnumShellItems* moved;
						if SUCCEEDED(temp->BindToHandler(nullptr, BHID_EnumItems, IID_PPV_ARGS(&moved))) {
							op->MoveItems(moved, parent);
							moved->Release();
						}
						op->PerformOperations();

						temp->Release();
						RemoveDirectoryW(temppath);
					}
				}
				parent->Release();
			}
		}
		op->Release();
	}
}

int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE,
	_In_ LPWSTR args,
	_In_ int nCmdShow
) {
	int argc;
	if (WCHAR** argv = CommandLineToArgvW(args, &argc)) {
		if SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)) {
			run(argc, argv);
			CoUninitialize();
		}
		LocalFree(argv);
	}
	return 0;
}
