#pragma once

// ghc filesystem has some warnings we want to suppress for a less noisy build
#include "DisableAllWarnings.h"
DISABLE_WARNINGS
#include <ghc/filesystem.hpp>
RESTORE_WARNINGS

#include <string>
#include <ostream>

#if defined(_WIN32) || defined(WIN32)

namespace winfs {
	using namespace ghc::filesystem;

	struct path {
		ghc::filesystem::path inner;

		path() = default;
		path(const ghc::filesystem::path &p)
			: inner(p) {}
		path(ghc::filesystem::path &&p)
			: inner(std::move(p)) {}
		path(const std::string &s)
			: inner(s) {}
		path(const char *s)
			: inner(s) {}
		path(std::string_view sv)
			: inner(sv) {}
		path(const ghc::filesystem::directory_entry &entry)
			: inner(entry.path()) {}

		path(const path &)			  = default;
		path(path &&)				  = default;
		path &operator=(const path &) = default;
		path &operator=(path &&)	  = default;

		operator const ghc::filesystem::path &() const { return inner; }
		operator ghc::filesystem::path &() { return inner; }
		operator ghc::filesystem::path() const { return inner; }
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

		path &operator=(const ghc::filesystem::path &p) {
			inner = p;
			return *this;
		}

		path &operator=(const std::string &s) {
			inner = s;
			return *this;
		}

		path &operator=(const ghc::filesystem::directory_entry &entry) {
			inner = entry.path();
			return *this;
		}

		template <typename Source>
		path &operator=(const Source &source) {
			inner = ghc::filesystem::path(source);
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
		bool exists() const { return ghc::filesystem::exists(inner); }
		bool is_directory() const { return ghc::filesystem::is_directory(inner); }
		bool is_regular_file() const { return ghc::filesystem::is_regular_file(inner); }

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
		friend path operator/(const path &lhs, const ghc::filesystem::path &rhs) { return path(lhs.inner / rhs); }
		friend path operator/(const ghc::filesystem::path &lhs, const path &rhs) { return path(lhs / rhs.inner); }

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

	using ghc::filesystem::absolute;
	using ghc::filesystem::canonical;
	using ghc::filesystem::copy;
	using ghc::filesystem::copy_file;
	using ghc::filesystem::copy_options;
	using ghc::filesystem::create_directories;
	using ghc::filesystem::create_directory;
	using ghc::filesystem::current_path;
	using ghc::filesystem::directory_iterator;
	using ghc::filesystem::exists;
	using ghc::filesystem::file_size;
	using ghc::filesystem::file_time_type;
	using ghc::filesystem::file_type;
	using ghc::filesystem::ifstream;
	using ghc::filesystem::is_directory;
	using ghc::filesystem::is_regular_file;
	using ghc::filesystem::is_symlink;
	using ghc::filesystem::last_write_time;
	using ghc::filesystem::ofstream;
	using ghc::filesystem::recursive_directory_iterator;
	using ghc::filesystem::remove;
	using ghc::filesystem::remove_all;
	using ghc::filesystem::rename;
	using ghc::filesystem::status;
	using ghc::filesystem::u8path;

} // namespace winfs

namespace fs = winfs;

#else

namespace fs = ghc::filesystem;

#endif