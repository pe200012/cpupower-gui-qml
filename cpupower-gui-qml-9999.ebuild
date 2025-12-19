# Copyright 2024 Gentoo Authors
# Distributed under the terms of the GNU General Public License v3

EAPI=8

inherit ecm

DESCRIPTION="GUI utility to change CPU frequency and governor (Qt/Kirigami port)"
HOMEPAGE="https://github.com/pe200012/cpupower-gui-qml"

if [[ ${PV} == 9999 ]]; then
	inherit git-r3
	EGIT_REPO_URI="https://github.com/pe200012/cpupower-gui-qml.git"
else
	SRC_URI="https://github.com/pe200012/cpupower-gui-qml/archive/refs/tags/v${PV}.tar.gz -> ${P}.tar.gz"
	KEYWORDS="~amd64 ~x86"
fi

LICENSE="GPL-3+"
SLOT="0"
IUSE="systemd"

RDEPEND="
	>=dev-qt/qtbase-6.5:6[dbus,gui,widgets]
	>=dev-qt/qtdeclarative-6.5:6
	>=kde-frameworks/kcoreaddons-6.0:6
	>=kde-frameworks/kconfig-6.0:6
	>=kde-frameworks/ki18n-6.0:6
	>=kde-frameworks/kirigami-6.0:6
	>=kde-frameworks/kstatusnotifieritem-6.0:6
	sys-auth/polkit
"
DEPEND="${RDEPEND}
	>=kde-frameworks/extra-cmake-modules-5.68:0
"
BDEPEND="
	>=dev-qt/qttools-6.5:6[linguist]
"

src_configure() {
	local mycmakeargs=(
		-DBUILD_TESTING=OFF
	)
	ecm_src_configure
}

src_compile() {
	ecm_src_compile

	# Build the helper service
	local helper_builddir="${BUILD_DIR}/helper"
	mkdir -p "${helper_builddir}" || die
	cd "${helper_builddir}" || die

	local helper_cmakeargs=(
		-DCMAKE_INSTALL_PREFIX="${EPREFIX}/usr"
		-DHELPER_INSTALL_DIR="libexec"
		-DDBUS_SYSTEM_SERVICES_DIR="share/dbus-1/system-services"
		-DDBUS_SYSTEM_CONF_DIR="share/dbus-1/system.d"
		-DPOLKIT_ACTIONS_DIR="share/polkit-1/actions"
	)

	if use systemd; then
		helper_cmakeargs+=( -DSYSTEMD_SYSTEM_UNIT_DIR="lib/systemd/system" )
	fi

	cmake "${helper_cmakeargs[@]}" "${S}/helper" || die "Helper cmake failed"
	emake
}

src_install() {
	ecm_src_install

	# Install the helper service
	local helper_builddir="${BUILD_DIR}/helper"
	cd "${helper_builddir}" || die
	emake DESTDIR="${D}" install

	# Remove systemd service if USE flag not set
	if ! use systemd; then
		rm -f "${ED}/usr/lib/systemd/system/cpupower-gui-helper.service" 2>/dev/null
		rmdir "${ED}/usr/lib/systemd/system" 2>/dev/null
		rmdir "${ED}/usr/lib/systemd" 2>/dev/null
	fi
}

pkg_postinst() {
	ecm_pkg_postinst
	elog ""
	elog "The cpupower-gui-helper D-Bus service has been installed."
	elog "It will be automatically activated via D-Bus when the GUI"
	elog "requests privileged operations."
	elog ""
	if use systemd; then
		elog "A systemd service file has been installed. You can enable"
		elog "persistent helper service with:"
		elog "  systemctl enable cpupower-gui-helper.service"
		elog ""
	fi
	elog "PolicyKit will prompt for authentication when changing CPU"
	elog "frequency settings. Users in the 'wheel' group can authenticate"
	elog "with their own password."
	elog ""
}
