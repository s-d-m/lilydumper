#!/bin/bash

set +x

this_app_name='lilydumper'

this_dir="$(readlink -f "$(dirname "${BASH_SOURCE[0]}")")"

# too bad linuxdeployqt provides only an x86_64 AppImage, otherwise this script could also make an AppImage
# for other architectures


tmp_dir="$(mktemp -t -d "DIR_TO_MAKE_AN_APPIMAGE_FOR_${this_app_name^^}.XXXXXX")"
squash_fs_root_tmp_dir="$(mktemp -t -d "DIR_FOR_SQUASH_FS_ROOT.XXXXXX")"

function finish()
{
  rm -rf -- "${tmp_dir}"
  rm -rf -- "${squash_fs_root_tmp_dir}"
}

trap finish EXIT

# avoid downloading linuxdeployqt if it is already in the path
function get_linuxdeployqt() {
    local readonly app_name='linuxdeployqt-continuous-x86_64.AppImage'
    local readonly linuxdeployqt="$(command -v "${app_name}")"
    if [ -n "$linuxdeployqt" ] && [ -x "$linuxdeployqt" ] ; then
	echo "$linuxdeployqt"
    else
	local readonly url_linuxqtdeploy='https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage'

	printf >&2 '\n\n\n[INFO] Could not find linuxdeployqt-continuous-x86_64.AppImage on your system.\n'
	printf >&2 '[INFO] downloading it for you\n'
	printf >&2 '[INFO] pro tipp: to avoid downloading again and again, just download it once from %s, make it executable, and save it a folder that appears in %s\n' "${url_linuxqtdeploy}" "${PATH}"
	printf >&2 '\n\n\n\n\n\n'

	wget --progress=bar:force -O "${tmp_dir}/${app_name}" "${url_linuxqtdeploy}"
	if [ ! -e  "${tmp_dir}/${app_name}" ] ; then
	    echo >&2 'Failed to download "${app_name}" from "${url_linuxqtdeploy}"'
	    exit 2
	fi

	chmod +x  "${tmp_dir}/${app_name}"
	echo  "${tmp_dir}/${app_name}"
    fi
}

function make_appimage()
{
    make -C "${this_dir}"
    make -C "${this_dir}" install DESTDIR="${tmp_dir}"


    local readonly linuxqtdeploy="$(get_linuxdeployqt)"
    local readonly version="$(git rev-parse --short HEAD)"

    unset QTDIR
    unset QT_PLUGIN_PATH
    unset LD_LIBRARY_PATH


    ARCH=x86_64 "${linuxqtdeploy}" "${tmp_dir}/usr/share/applications/${this_app_name}.desktop" -verbose=2 -appimage -bundle-non-qt-libs
    if [ ! -e "${this_dir}/${this_app_name}-x86_64.AppImage" ] ; then
	printf >&2 'Failed to create %s\n' "${this_app_name}-x86_64.AppImage"
	exit 2
    fi

    local readonly dst_appimage="${this_dir}/bin/${this_app_name}-${version}-x86_64.AppImage"

    cp -- "${this_dir}/${this_app_name}-x86_64.AppImage" "${dst_appimage}"
    mv -- "${this_dir}/${this_app_name}-x86_64.AppImage" "${this_dir}/bin/${this_app_name}-x86_64.AppImage"

    printf 'Following libraries are required on the system to run the appimage:\n'
    find "${tmp_dir}" -executable -type f -exec ldd '{}' ';' | grep " => /usr" | cut -d " " -f 2-3 | sort | uniq

    printf 'appimage is available at %s\n' "${dst_appimage}"
}

make_appimage
