#include "netUtil.h"
#include "tests.h"

TEST_CASE("pathToFileUrl builds a URL a browser will open", "[netUtil]") {
#ifdef _WIN32
	REQUIRE(pathToFileUrl("C:\\Program Files\\Koala\\data/www/index.html")
			== "file:///C:/Program%20Files/Koala/data/www/index.html");
	REQUIRE(pathToFileUrl("C:/Users/me/koala") == "file:///C:/Users/me/koala");
#else
	REQUIRE(pathToFileUrl("/Users/marek/test files") == "file:///Users/marek/test%20files");
	REQUIRE(pathToFileUrl("/Applications/Koala.app/Contents/Resources/data/www/index.html")
			== "file:///Applications/Koala.app/Contents/Resources/data/www/index.html");
	// non-ascii bytes come out as two hex digits each
	REQUIRE(pathToFileUrl("/tmp/caf\xc3\xa9") == "file:///tmp/caf%C3%A9");
	// '#' and '?' would otherwise be read as fragment / query
	REQUIRE(pathToFileUrl("/tmp/a#b?c") == "file:///tmp/a%23b%3Fc");
#endif
}
