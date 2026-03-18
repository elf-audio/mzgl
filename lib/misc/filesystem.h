#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <ostream>

#if defined(_WIN32) || defined(WIN32)

namespace winfs {
	using namespace std::filesystem;

	// Convert a u8string (std::string in C++17, std::u8string in C++20) to std::string
	template <typename T>
	inline std::string u8ToStdString(const T &s) {
		return std::string(reinterpret_cast<const char *>(s.data()), s.size());
	}

	// C++20-safe replacement for std::filesystem::u8path (deprecated in C++20)
	inline std::filesystem::path fromUtf8(const char *s, size_t len) {
#if __cplusplus > 201703L || (defined(_MSVC_LANG) && _MSVC_LANG > 201703L)
		return std::filesystem::path(std::u8string_view(reinterpret_cast<const char8_t *>(s), len));
#else
		return std::filesystem::u8path(s, s + len);
#endif
	}

	inline std::filesystem::path fromUtf8(const std::string &s) { return fromUtf8(s.data(), s.size()); }
	inline std::filesystem::path fromUtf8(const char *s) { return fromUtf8(s, std::char_traits<char>::length(s)); }
	inline std::filesystem::path fromUtf8(std::string_view sv) { return fromUtf8(sv.data(), sv.size()); }

	struct path {
		std::filesystem::path inner;

		path() = default;
		path(const std::filesystem::path &p)
			: inner(p) {}
		path(std::filesystem::path &&p)
			: inner(std::move(p)) {}
		path(const std::string &s)
			: inner(fromUtf8(s)) {}
		path(const char *s)
			: inner(fromUtf8(s)) {}
		path(std::string_view sv)
			: inner(fromUtf8(sv)) {}
		path(const std::filesystem::directory_entry &entry)
			: inner(entry.path()) {}

		path(const path &)			  = default;
		path(path &&)				  = default;
		path &operator=(const path &) = default;
		path &operator=(path &&)	  = default;

		operator const std::filesystem::path &() const { return inner; }
		operator std::filesystem::path &() { return inner; }
		operator std::filesystem::path() const { return inner; }
		operator std::string() const { return u8ToStdString(inner.u8string()); }
		explicit operator std::wstring() const { return inner.wstring(); }
		// NOTE: returns dangling pointer to temporary - pre-existing issue
		explicit operator const char *() const { return u8ToStdString(inner.u8string()).c_str(); }

		path &operator+=(const std::string &str) {
			inner += fromUtf8(str);
			return *this;
		}

		path &operator+=(const char *str) {
			inner += fromUtf8(str);
			return *this;
		}

		path &operator+=(const path &other) {
			inner += other.inner;
			return *this;
		}

		path &operator=(const std::filesystem::path &p) {
			inner = p;
			return *this;
		}

		path &operator=(const std::string &s) {
			inner = fromUtf8(s);
			return *this;
		}

		path &operator=(const std::filesystem::directory_entry &entry) {
			inner = entry.path();
			return *this;
		}

		template <typename Source>
		path &operator=(const Source &source) {
			inner = std::filesystem::path(source);
			return *this;
		}

		path &operator/=(const path &rhs) {
			inner /= rhs.inner;
			return *this;
		}

		path &operator=(const char *s) {
			inner = fromUtf8(s);
			return *this;
		}

		path &operator=(std::string_view sv) {
			inner = fromUtf8(sv);
			return *this;
		}

		std::string string() const { return u8ToStdString(inner.u8string()); }
		std::wstring wstring() const { return inner.wstring(); }
		std::string generic_string() const { return u8ToStdString(inner.generic_u8string()); }
		std::u8string u8string() const { return inner.u8string(); }
		path filename() const { return path(inner.filename()); }
		path parent_path() const { return path(inner.parent_path()); }
		path extension() const { return path(inner.extension()); }
		path stem() const { return path(inner.stem()); }
		bool empty() const { return inner.empty(); }
		auto c_str() const { return inner.wstring().c_str(); }
		bool exists() const { return std::filesystem::exists(inner); }
		bool is_directory() const { return std::filesystem::is_directory(inner); }
		bool is_regular_file() const { return std::filesystem::is_regular_file(inner); }

		path &replace_extension(const path &replacement = path()) {
			inner.replace_extension(replacement.inner);
			return *this;
		}

		path replace_extension(const path &replacement = path()) const {
			path result = *this;
			result.inner.replace_extension(replacement.inner);
			return result;
		}

		path &replace_extension(const std::string &replacement) {
			inner.replace_extension(fromUtf8(replacement));
			return *this;
		}

		path replace_extension(const std::string &replacement) const {
			path result = *this;
			result.inner.replace_extension(fromUtf8(replacement));
			return result;
		}

		path &replace_extension(const char *replacement) {
			inner.replace_extension(fromUtf8(replacement));
			return *this;
		}

		path replace_extension(const char *replacement) const {
			path result = *this;
			result.inner.replace_extension(fromUtf8(replacement));
			return result;
		}

		friend path operator/(const path &lhs, const std::string &rhs) { return path(lhs.inner / fromUtf8(rhs)); }
		friend path operator/(const path &lhs, const path &rhs) { return path(lhs.inner / rhs.inner); }
		friend path operator/(const path &lhs, const char *rhs) { return path(lhs.inner / fromUtf8(rhs)); }
		friend path operator/(path &lhs, const std::string &rhs) { return path(lhs.inner / fromUtf8(rhs)); }
		friend path operator/(path &lhs, const path &rhs) { return path(lhs.inner / rhs.inner); }
		friend path operator/(path &lhs, const char *rhs) { return path(lhs.inner / fromUtf8(rhs)); }
		friend path operator/(const path &lhs, const std::filesystem::path &rhs) { return path(lhs.inner / rhs); }
		friend path operator/(const std::filesystem::path &lhs, const path &rhs) { return path(lhs / rhs.inner); }

		friend bool operator==(const path &a, const path &b) { return a.inner == b.inner; }
		friend bool operator!=(const path &a, const path &b) { return a.inner != b.inner; }
		friend bool operator==(const path &lhs, const char *rhs) { return lhs.inner == fromUtf8(rhs); }
		friend bool operator==(const char *lhs, const path &rhs) { return fromUtf8(lhs) == rhs.inner; }
		friend bool operator==(const path &lhs, const std::string &rhs) { return lhs.inner == fromUtf8(rhs); }
		friend bool operator==(const std::string &lhs, const path &rhs) { return fromUtf8(lhs) == rhs.inner; }
		friend bool operator!=(const path &lhs, const char *rhs) { return lhs.inner != fromUtf8(rhs); }
		friend bool operator!=(const char *lhs, const path &rhs) { return fromUtf8(lhs) != rhs.inner; }
		friend bool operator!=(const path &lhs, const std::string &rhs) { return lhs.inner != fromUtf8(rhs); }
		friend bool operator!=(const std::string &lhs, const path &rhs) { return fromUtf8(lhs) != rhs.inner; }

		friend std::ostream &operator<<(std::ostream &os, const path &p) { return os << p.string(); }
	};

	using std::filesystem::absolute;
	using std::filesystem::canonical;
	using std::filesystem::copy;
	using std::filesystem::copy_file;
	using std::filesystem::copy_options;
	using std::filesystem::create_directories;
	using std::filesystem::create_directory;
	using std::filesystem::current_path;
	using std::filesystem::directory_iterator;
	using std::filesystem::exists;
	using std::filesystem::file_size;
	using std::filesystem::file_time_type;
	using std::filesystem::file_type;
	using std::filesystem::is_directory;
	using std::filesystem::is_regular_file;
	using std::filesystem::is_symlink;
	using std::filesystem::last_write_time;
	using std::filesystem::recursive_directory_iterator;
	using std::filesystem::remove;
	using std::filesystem::remove_all;
	using std::filesystem::rename;
	using std::filesystem::status;

} // namespace winfs

namespace fs = winfs;

#else

namespace fs = std::filesystem;

#endif
