{ stdenv
, lib
, alsa-lib
, automake
, autoconf
, curl
, fribidi
, freetype
, glew
, gtk3
, gtksourceview4
, libdatrie
, libGL
, libjpeg
, libpng
, libselinux
, libsepol
, libthai
, m4
, pango
, pcre
, pkgconf
, pkg-config
, SDL2
, SDL2_gfx
, SDL2_image
, SDL2_mixer
, SDL2_ttf
, systemd
, util-linux
, valgrind
, version ? "git"
, xorg
}:

stdenv.mkDerivation {
  pname = "principia";
  inherit version;
  src = ../.;

  nativeBuildInputs = [
    automake
    autoconf
    pkg-config
    # pkgconf
  ];

  buildInputs = [
    alsa-lib
    curl
    fribidi
    glew
    gtk3
    libdatrie
    libGL
    libjpeg
    libpng
    libselinux
    libsepol
    libthai
    m4
    pango
    pcre
    SDL2
    SDL2_gfx
    SDL2_image
    SDL2_mixer
    SDL2_ttf
    systemd
    util-linux
    valgrind
    xorg.libXScrnSaver
    xorg.libXxf86vm
    xorg.libXdmcp
  ];

  preConfigure = ''
    for p in $SDL2_PATH; do
      NIX_CFLAGS_COMPILE="$NIX_CFLAGS_COMPILE -isystem $p"
    done
  '';

  configurePhase = ''
    runHook preConfigure

    cd build-linux 
    ./autogen.sh 
    ./configure
  '';

  buildPhase = ''
    patchShebangs go
    ./go --clean --release --silent
  '';

  installPhase = ''
    mkdir -p $out/share/applications
    mkdir -p $out/share/principia
    mkdir -p $out/share/pixmaps
    mkdir -p $out/bin

    cp -r --dereference data-pc data-shared $out/share/principia/
    install -Dm755 principia $out/share/principia/
    install -Dm644 principia.desktop principia-url-handler.desktop $out/share/applications/
    install -Dm644 principia.png $out/share/pixmaps/

    ln -s $out/share/principia/principia "$out/bin/principia"
    chmod 755 $out/bin/principia
  '';

  meta = with lib; {
    description = "Physics-based sandbox building game";
    license = licenses.bsd3;
    homepage = "https://github.com/Bithack/principia";
    platforms = platforms.linux;
  };

}
