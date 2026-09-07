#!/usr/bin/env bash
#
# Package the 05_Plugin example into a macOS installer .pkg containing whatever
# formats have been built:
#   "mzgl Gain.vst3"       -> /Library/Audio/Plug-Ins/VST3
#   "mzgl Gain.component"  -> /Library/Audio/Plug-Ins/Components   (AUv2)
#   "mzgl Gain.app"        -> /Applications                        (AUv3 container app)
#
# Usage: package.sh [--sign "<Developer ID Application: ...>"] [--notarize <keychain-profile>]
#
# Without --sign the bundles stay ad-hoc signed and the pkg is unsigned (fine
# for your own machine, won't pass Gatekeeper elsewhere). With --sign the
# bundles are re-signed with hardened runtime + timestamp and the pkg is signed
# with the matching "Developer ID Installer" cert if one is in the keychain.
# --notarize submits the pkg with `xcrun notarytool` (profile created via
# `xcrun notarytool store-credentials <name>`) and staples the ticket.
#
# Build the plugins first:
#   cmake -Bbuild -GNinja && cmake --build build --target 05_PluginVST3 05_PluginAUv2
#   cmake -Bbuild-xcode -GXcode && cmake --build build-xcode --target 05_PluginAUv3App --config Release
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
EXAMPLES="$(cd "$HERE/../.." && pwd)"
BIN="$EXAMPLES/bin"
PRODUCT="mzgl Gain"
PKG_ID_BASE="com.elf-audio.mzgl.Gain"

SIGN_ID=""
NOTARIZE_PROFILE=""
while [[ $# -gt 0 ]]; do
	case "$1" in
		--sign) SIGN_ID="$2"; shift 2 ;;
		--notarize) NOTARIZE_PROFILE="$2"; shift 2 ;;
		-h|--help) sed -n '2,20p' "$0"; exit 0 ;;
		*) echo "unknown option: $1" >&2; exit 1 ;;
	esac
done

VST3="$BIN/$PRODUCT.vst3"
AUV2="$BIN/$PRODUCT.component"
# The Xcode build puts the container app under build-xcode/05_Plugin/<config>/.
APP="$(ls -d "$EXAMPLES"/build-xcode/05_Plugin/*/"$PRODUCT.app" 2>/dev/null | head -1 || true)"

VERSION="$(/usr/libexec/PlistBuddy -c 'Print CFBundleShortVersionString' "$VST3/Contents/Info.plist" 2>/dev/null \
	|| /usr/libexec/PlistBuddy -c 'Print CFBundleShortVersionString' "$AUV2/Contents/Info.plist" 2>/dev/null \
	|| echo 0.0.0)"
OUT="$BIN/mzgl-gain-$VERSION.pkg"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

sign_bundle() {
	[[ -z "$SIGN_ID" ]] && return 0
	echo "signing $(basename "$1")"
	codesign --force --deep --sign "$SIGN_ID" --timestamp --options runtime "$1"
	codesign --verify --deep --strict "$1"
}

# component pkg for one bundle: stage it at its install path, mark it non-relocatable
component_pkg() {
	local bundle="$1" install_dir="$2" pkg_id="$3" out="$4"
	local staging="$WORK/stage-$(basename "$out" .pkg)"
	mkdir -p "$staging$install_dir"
	cp -R "$bundle" "$staging$install_dir/"
	local plist="$out.plist"
	pkgbuild --analyze --root "$staging" "$plist" >/dev/null
	# newer pkgbuild only writes the key when it is true; add it if missing
	/usr/libexec/PlistBuddy -c 'Set :0:BundleIsRelocatable false' "$plist" 2>/dev/null \
		|| /usr/libexec/PlistBuddy -c 'Add :0:BundleIsRelocatable bool false' "$plist"
	pkgbuild --root "$staging" --component-plist "$plist" --identifier "$pkg_id" \
		--version "$VERSION" --install-location / "$out" >/dev/null
}

choices=""
refs=""
add_choice() { # id title description pkg_id pkg_file
	choices+="        <line choice=\"$1\"/>
"
	refs+="    <choice id=\"$1\" title=\"$2\" description=\"$3\" selected=\"true\"><pkg-ref id=\"$4\"/></choice>
    <pkg-ref id=\"$4\" version=\"$VERSION\">$5</pkg-ref>
"
}

found=0
if [[ -d "$VST3" ]]; then
	sign_bundle "$VST3"
	component_pkg "$VST3" /Library/Audio/Plug-Ins/VST3 "$PKG_ID_BASE.VST3" "$WORK/vst3.pkg"
	add_choice vst3 "$PRODUCT (VST3)" "Installs to /Library/Audio/Plug-Ins/VST3" "$PKG_ID_BASE.VST3" vst3.pkg
	found=1
fi
if [[ -d "$AUV2" ]]; then
	sign_bundle "$AUV2"
	component_pkg "$AUV2" /Library/Audio/Plug-Ins/Components "$PKG_ID_BASE.AUv2" "$WORK/auv2.pkg"
	add_choice auv2 "$PRODUCT (Audio Unit)" "Installs to /Library/Audio/Plug-Ins/Components" "$PKG_ID_BASE.AUv2" auv2.pkg
	found=1
fi
if [[ -n "$APP" && -d "$APP" ]]; then
	sign_bundle "$APP"
	component_pkg "$APP" /Applications "$PKG_ID_BASE" "$WORK/app.pkg"
	add_choice auv3 "$PRODUCT (AUv3 app)" "Installs the AUv3 container app to /Applications" "$PKG_ID_BASE" app.pkg
	found=1
fi
if [[ $found -eq 0 ]]; then
	echo "nothing to package: build 05_PluginVST3 / 05_PluginAUv2 / 05_PluginAUv3App first" >&2
	exit 1
fi

cat > "$WORK/distribution.xml" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>$PRODUCT $VERSION</title>
    <options customize="always" require-scripts="false" hostArchitectures="arm64,x86_64"/>
    <domains enable_anywhere="false" enable_currentUserHome="false" enable_localSystem="true"/>
    <choices-outline>
$choices    </choices-outline>
$refs</installer-gui-script>
XML

rm -f "$OUT"
productbuild --distribution "$WORK/distribution.xml" --package-path "$WORK" "$OUT" >/dev/null

if [[ -n "$SIGN_ID" ]]; then
	if security find-identity -v 2>/dev/null | grep -q "Developer ID Installer"; then
		installer_id="$(security find-identity -v | grep "Developer ID Installer" | head -1 | awk '{print $2}')"
		productsign --sign "$installer_id" "$OUT" "$OUT.signed" >/dev/null
		mv "$OUT.signed" "$OUT"
		echo "signed pkg with $installer_id"
	else
		echo "note: no 'Developer ID Installer' cert found - pkg left unsigned"
	fi
fi

if [[ -n "$NOTARIZE_PROFILE" ]]; then
	echo "notarizing..."
	xcrun notarytool submit "$OUT" --keychain-profile "$NOTARIZE_PROFILE" --wait
	xcrun stapler staple "$OUT"
fi

echo "-> $OUT"
