FROM alpine:3.24.0 AS base

# --- Build

FROM base AS build

RUN apk add --no-cache build-base cmake ninja curl freetype-dev libpng-dev jpeg-dev mesa-dev zlib-dev sdl3-dev

WORKDIR /
RUN curl -sLf https://github.com/bithack/principia/archive/master.tar.gz | tar xzf -
RUN mv principia-master principia

WORKDIR /principia/
RUN cmake . -G Ninja -DSCREENSHOT_BUILD=ON -DUSE_VENDORED_SDL3=ON -DUNITY_BUILD=ON -DSDL_X11=OFF -DSDL_UNIX_CONSOLE_BUILD=ON
RUN ninja

# --- Install

FROM base AS final

RUN apk add --no-cache libgcc libstdc++ mesa-dri-gallium mesa-egl mesa-gl freetype libpng jpeg zlib

COPY --from=build /principia/principia /principia/
COPY --from=build /principia/data/ /principia/data/

ENV SDL_VIDEO_DRIVER=offscreen

ENTRYPOINT ["/principia/principia"]
