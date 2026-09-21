#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <ostream>
#include <cstdint>
#include <iterator>
#include <system_error>

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
		friend bool operator<(const path &a, const path &b) { return a.inner < b.inner; }
		friend bool operator<=(const path &a, const path &b) { return a.inner <= b.inner; }
		friend bool operator>(const path &a, const path &b) { return a.inner > b.inner; }
		friend bool operator>=(const path &a, const path &b) { return a.inner >= b.inner; }
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

	inline path temp_directory_path() { return path(std::filesystem::temp_directory_path()); }
	inline path temp_directory_path(std::error_code &ec) {
		return path(std::filesystem::temp_directory_path(ec));
	}

	// Every function that hands a path back must hand back a winfs::path, otherwise
	// the caller is holding a raw std::filesystem::path again and its string() goes
	// through the ANSI code page - which throws std::system_error for any character
	// it can't represent. `auto p = entry.path(); p.filename().string()` on a song
	// called "🎹 my song" crashed Koala at every launch that way (2.0.8 crash
	// reports), so directory entries and iterators are wrapped below too.
	inline path current_path() { return path(std::filesystem::current_path()); }
	inline path current_path(std::error_code &ec) { return path(std::filesystem::current_path(ec)); }
	inline void current_path(const path &p) { std::filesystem::current_path(p.inner); }
	inline void current_path(const path &p, std::error_code &ec) { std::filesystem::current_path(p.inner, ec); }
	inline path absolute(const path &p) { return path(std::filesystem::absolute(p.inner)); }
	inline path absolute(const path &p, std::error_code &ec) { return path(std::filesystem::absolute(p.inner, ec)); }
	inline path canonical(const path &p) { return path(std::filesystem::canonical(p.inner)); }
	inline path canonical(const path &p, std::error_code &ec) {
		return path(std::filesystem::canonical(p.inner, ec));
	}

	class directory_entry {
	public:
		directory_entry() = default;
		directory_entry(const std::filesystem::directory_entry &e)
			: inner(e) {}
		directory_entry(std::filesystem::directory_entry &&e)
			: inner(std::move(e)) {}

		winfs::path path() const { return winfs::path(inner.path()); }
		// lets fs::is_directory(entry), fs::file_size(entry) etc. keep working
		operator const std::filesystem::path &() const { return inner.path(); }

		bool exists() const { return inner.exists(); }
		bool is_directory() const { return inner.is_directory(); }
		bool is_regular_file() const { return inner.is_regular_file(); }
		bool is_symlink() const { return inner.is_symlink(); }
		std::uintmax_t file_size() const { return inner.file_size(); }
		std::filesystem::file_time_type last_write_time() const { return inner.last_write_time(); }
		std::filesystem::file_status status() const { return inner.status(); }
		std::filesystem::file_status symlink_status() const { return inner.symlink_status(); }

		const std::filesystem::directory_entry &std_entry() const { return inner; }

	private:
		std::filesystem::directory_entry inner;
	};

	template <class StdIterator>
	class basic_directory_iterator {
	public:
		using iterator_category = std::input_iterator_tag;
		using value_type		= directory_entry;
		using difference_type	= std::ptrdiff_t;
		using pointer			= const directory_entry *;
		using reference			= const directory_entry &;

		basic_directory_iterator() = default;
		explicit basic_directory_iterator(const winfs::path &p)
			: it(p.inner) {
			refresh();
		}
		basic_directory_iterator(const winfs::path &p, std::error_code &ec)
			: it(p.inner, ec) {
			refresh();
		}
		basic_directory_iterator(const winfs::path &p, std::filesystem::directory_options options)
			: it(p.inner, options) {
			refresh();
		}
		basic_directory_iterator(const winfs::path &p,
								 std::filesystem::directory_options options,
								 std::error_code &ec)
			: it(p.inner, options, ec) {
			refresh();
		}

		reference operator*() const { return current; }
		pointer operator->() const { return &current; }

		basic_directory_iterator &operator++() {
			++it;
			refresh();
			return *this;
		}
		basic_directory_iterator &increment(std::error_code &ec) {
			it.increment(ec);
			refresh();
			return *this;
		}

		friend bool operator==(const basic_directory_iterator &a, const basic_directory_iterator &b) {
			return a.it == b.it;
		}
		friend bool operator!=(const basic_directory_iterator &a, const basic_directory_iterator &b) {
			return a.it != b.it;
		}

		// range-for support, found by ADL like the std ones
		friend basic_directory_iterator begin(basic_directory_iterator iter) { return iter; }
		friend basic_directory_iterator end(const basic_directory_iterator &) { return {}; }

	protected:
		void refresh() { current = (it == StdIterator()) ? directory_entry() : directory_entry(*it); }

		StdIterator it;
		directory_entry current;
	};

	using directory_iterator = basic_directory_iterator<std::filesystem::directory_iterator>;

	class recursive_directory_iterator
		: public basic_directory_iterator<std::filesystem::recursive_directory_iterator> {
	public:
		using basic_directory_iterator::basic_directory_iterator;

		int depth() const { return it.depth(); }
		bool recursion_pending() const { return it.recursion_pending(); }
		std::filesystem::directory_options options() const { return it.options(); }
		void disable_recursion_pending() { it.disable_recursion_pending(); }
		void pop() {
			it.pop();
			refresh();
		}
		void pop(std::error_code &ec) {
			it.pop(ec);
			refresh();
		}

		friend recursive_directory_iterator begin(recursive_directory_iterator iter) { return iter; }
		friend recursive_directory_iterator end(const recursive_directory_iterator &) { return {}; }
	};

	using std::filesystem::directory_options;

	using std::filesystem::copy;
	using std::filesystem::copy_file;
	using std::filesystem::copy_options;
	using std::filesystem::create_directories;
	using std::filesystem::create_directory;
	using std::filesystem::exists;
	using std::filesystem::file_size;
	using std::filesystem::file_time_type;
	using std::filesystem::file_type;
	using std::filesystem::is_directory;
	using std::filesystem::is_regular_file;
	using std::filesystem::is_symlink;
	using std::filesystem::last_write_time;
	using std::filesystem::remove;
	using std::filesystem::remove_all;
	using std::filesystem::rename;
	using std::filesystem::status;

} // namespace winfs

namespace fs = winfs;

#else

namespace fs = std::filesystem;

#endif
