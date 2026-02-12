#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <ostream>

#if defined(_WIN32) || defined(WIN32)

namespace winfs {
	using namespace std::filesystem;

	struct path {
		std::filesystem::path inner;

		path() = default;
		path(const std::filesystem::path &p)
			: inner(p) {}
		path(std::filesystem::path &&p)
			: inner(std::move(p)) {}
		path(const std::string &s)
			: inner(s) {}
		path(const char *s)
			: inner(s) {}
		path(std::string_view sv)
			: inner(sv) {}
		path(const std::filesystem::directory_entry &entry)
			: inner(entry.path()) {}

		path(const path &)			  = default;
		path(path &&)				  = default;
		path &operator=(const path &) = default;
		path &operator=(path &&)	  = default;

		operator const std::filesystem::path &() const { return inner; }
		operator std::filesystem::path &() { return inner; }
		operator std::filesystem::path() const { return inner; }
		operator std::string() const { return inner.string(); }
		explicit operator std::wstring() const { return inner.wstring(); }
		explicit operator const char *() const { return inner.string().c_str(); }

		path &operator+=(const std::string &str) {
			inner += str;
			return *this;
		}

		path &operator+=(const char *str) {
			inner += str;
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
			inner = s;
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
			inner = s;
			return *this;
		}

		path &operator=(std::string_view sv) {
			inner = sv;
			return *this;
		}

		std::string string() const { return inner.string(); }
		std::wstring wstring() const { return inner.wstring(); }
		std::string generic_string() const { return inner.generic_string(); }
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
			inner.replace_extension(replacement);
			return *this;
		}

		path replace_extension(const std::string &replacement) const {
			path result = *this;
			result.inner.replace_extension(replacement);
			return result;
		}

		path &replace_extension(const char *replacement) {
			inner.replace_extension(replacement);
			return *this;
		}

		path replace_extension(const char *replacement) const {
			path result = *this;
			result.inner.replace_extension(replacement);
			return result;
		}

		friend path operator/(const path &lhs, const std::string &rhs) { return path(lhs.inner / rhs); }
		friend path operator/(const path &lhs, const path &rhs) { return path(lhs.inner / rhs.inner); }
		friend path operator/(const path &lhs, const char *rhs) { return path(lhs.inner / rhs); }
		friend path operator/(path &lhs, const std::string &rhs) { return path(lhs.inner / rhs); }
		friend path operator/(path &lhs, const path &rhs) { return path(lhs.inner / rhs.inner); }
		friend path operator/(path &lhs, const char *rhs) { return path(lhs.inner / rhs); }
		friend path operator/(const path &lhs, const std::filesystem::path &rhs) { return path(lhs.inner / rhs); }
		friend path operator/(const std::filesystem::path &lhs, const path &rhs) { return path(lhs / rhs.inner); }

		friend bool operator==(const path &a, const path &b) { return a.inner == b.inner; }
		friend bool operator!=(const path &a, const path &b) { return a.inner != b.inner; }
		friend bool operator==(const path &lhs, const char *rhs) { return lhs.inner == rhs; }
		friend bool operator==(const char *lhs, const path &rhs) { return lhs == rhs.inner; }
		friend bool operator==(const path &lhs, const std::string &rhs) { return lhs.inner == rhs; }
		friend bool operator==(const std::string &lhs, const path &rhs) { return lhs == rhs.inner; }
		friend bool operator!=(const path &lhs, const char *rhs) { return lhs.inner != rhs; }
		friend bool operator!=(const char *lhs, const path &rhs) { return lhs != rhs.inner; }
		friend bool operator!=(const path &lhs, const std::string &rhs) { return lhs.inner != rhs; }
		friend bool operator!=(const std::string &lhs, const path &rhs) { return lhs != rhs.inner; }

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
