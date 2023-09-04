bin_PROGRAMS = principia

CXX = /mingw64/bin/g++
CC  = /mingw64/bin/gcc

GLOBAL_FLAGS := -DGL_GLEXT_PROTOTYPES \
				-DWINVER=0x0501 \
				-DLUA_COMPAT_MODULE \
				-D__STDC_FORMAT_MACROS=1 \
				-DSDL_NO_COMPAT \
				-DUNICODE \
				-DTMS_FAST_MATH \
				-DTMS_BACKEND_WINDOWS \
				-DTMS_BACKEND_PC -DBUILD_LUASOCKET

TMS_PATH = ../src/tms
ROOT = ../

GLOBAL_FLAGS += -I../src/tms/backends/windows/ \
    -I../src/src/ -I../src/ \
    -mwindows -Dsrandom=srand -Drandom=rand -Dmain=SDL_main \
    -static-libgcc -static-libstdc++ \
	-I/mingw64/include/ \
	-I/mingw64/include/freetype2 \
	-I/mingw64/include/SDL2/ \
    -I/mingw64/include/gtk-3.0 \
    -I/mingw64/include/pango-1.0 \
    -I/mingw64/include/glib-2.0 \
    -I/mingw64/lib/glib-2.0/include \
    -I/mingw64/include/harfbuzz \
    -I/mingw64/include/freetype2 \
    -I/mingw64/include/libpng1 \
    -I/mingw64/include/fribidi \
    -I/mingw64/include/cairo \
    -I/mingw64/include/pixman-1 \
    -I/mingw64/include/gdk-pixbuf-2.0 \
    -I/mingw64/include/webp \
    -I/mingw64/include/atk-1.0 \
	-I../src/lua/

principia_CFLAGS = ${GLOBAL_FLAGS} -std=gnu99

principia_CXXFLAGS = ${GLOBAL_FLAGS}
principia_CPPFLAGS = -static-libgcc -static-libstdc++
principia_LDADD = -L`pwd` -lopengl32 -lz \
    -lmingw32 -lws2_32 -lwinmm -luuid -loleaut32 -limm32 -lversion \
    -lole32 -luser32 \
    -lgtk-3 -lgdk-3 -lz -lgdi32 -limm32 -lshell32 -lole32 -luuid -lwinmm -ldwmapi -lsetupapi -lcfgmgr32 -lhid -lwinspool -lcomctl32 -lcomdlg32 -lpangowin32-1.0 -lpangocairo-1.0 -lpango-1.0 -lharfbuzz -latk-1.0 -lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0 -lintl \
    -lcurl -lShlwapi -lpng -ljpeg -lfreetype -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lglew32

principia_SOURCES = \
principia.rc \
../src/lua/lapi.c \
../src/lua/lauxlib.c \
../src/lua/lbaselib.c \
../src/lua/lbitlib.c \
../src/lua/lcode.c \
../src/lua/lcorolib.c \
../src/lua/lctype.c \
../src/lua/ldblib.c \
../src/lua/ldebug.c \
../src/lua/ldo.c \
../src/lua/ldump.c \
../src/lua/lfunc.c \
../src/lua/lgc.c \
../src/lua/linit.c \
../src/lua/llex.c \
../src/lua/lmathlib.c \
../src/lua/lmem.c \
../src/lua/lobject.c \
../src/lua/lopcodes.c \
../src/lua/lparser.c \
../src/lua/lstate.c \
../src/lua/lstring.c \
../src/lua/lstrlib.c \
../src/lua/ltable.c \
../src/lua/ltablib.c \
../src/lua/ltm.c \
../src/lua/lundump.c \
../src/lua/lvm.c \
../src/lua/lzio.c \
../src/lua/eris.c \
\
../src/luasocket/auxiliar.c \
../src/luasocket/buffer.c \
../src/luasocket/except.c \
../src/luasocket/inet.c \
../src/luasocket/io.c \
../src/luasocket/luasocket.c \
../src/luasocket/mime.c \
../src/luasocket/options.c \
../src/luasocket/select.c \
../src/luasocket/tcp.c \
../src/luasocket/timeout.c \
../src/luasocket/udp.c \
../src/luasocket/wsocket.c \
\
../src/src/SDL2_rotozoom.c \
\
../src/tms/core/atlas.c \
../src/tms/core/backend.c \
../src/tms/core/batch3d.c \
../src/tms/core/batch.c \
../src/tms/core/builtin-shaders.inc.c \
../src/tms/core/camera.c \
../src/tms/core/ddraw.c \
../src/tms/core/entity.c \
../src/tms/core/event.c \
../src/tms/core/framebuffer.c \
../src/tms/core/gbuffer.c \
../src/tms/core/graph.c \
../src/tms/core/hdrbuffer.c \
../src/tms/core/material.c \
../src/tms/core/mesh.c \
../src/tms/core/meshfactory.c \
../src/tms/core/model.c \
../src/tms/core/pipeline.c \
../src/tms/core/program.c \
../src/tms/core/scene.c \
../src/tms/core/screen.c \
../src/tms/core/shader.c \
../src/tms/core/surface.c \
../src/tms/core/texture.c \
../src/tms/core/tms.c \
../src/tms/core/varray.c \
../src/tms/core/wdg.c \
../src/tms/math/intersect.c \
../src/tms/math/math.c \
../src/tms/util/hash.c \
\
../src/tms/backends/windows/main.cc \
../src/tms/bindings/cpp/cpp.cc \
../src/tms/modules/3ds.c \
\
../src/src/fluid.cc \
../src/src/oilrig.cc \
../src/src/factory.cc \
../src/src/timectrl.cc \
../src/src/ball.cc \
../src/src/beam.cc \
../src/src/breadboard.cc \
../src/src/button.cc \
../src/src/cable.cc \
../src/src/command.cc \
../src/src/composable.cc \
../src/src/cup.cc \
../src/src/cylinder.cc \
../src/src/debugdraw.cc \
../src/src/edevice.cc \
../src/src/entity.cc \
../src/src/game.cc \
../src/src/game-message.cc \
../src/src/game-graph.cc \
../src/src/game-edit.cc \
../src/src/game-gui.cc \
../src/src/adventure.cc \
../src/src/gear.cc \
../src/src/magnet.cc \
../src/src/rack.cc \
../src/src/rail.cc \
../src/src/generator.cc \
../src/src/panel.cc \
../src/src/game-panel-edit.cc \
../src/src/gui.cc \
../src/src/goal.cc \
../src/src/group.cc \
../src/src/i0o1gate.cc \
../src/src/i1o1gate.cc \
../src/src/fxemitter.cc \
../src/src/i2o1gate.cc \
../src/src/backpack.cc \
../src/src/progress.cc \
../src/src/pkgwarp.cc \
../src/src/main.cc \
../src/src/material.cc \
../src/src/object_finder.cc \
../src/src/sincos.cc \
../src/src/eatan2.cc \
../src/src/pointer.cc \
../src/src/model.cc \
../src/src/object_factory.cc \
../src/src/soundmanager.cc \
../src/src/robot.cc \
../src/src/checkpoint.cc \
../src/src/rope.cc \
../src/src/connection.cc \
../src/src/settings.cc \
../src/src/shelf.cc \
../src/src/solver.cc \
../src/src/solver_ingame.cc \
../src/src/stabilizer.cc \
../src/src/sticky.cc \
../src/src/tester.cc \
../src/src/trampoline.cc \
../src/src/wheel.cc \
../src/src/wmotor.cc \
../src/src/motor.cc \
../src/src/lmotor.cc \
../src/src/magconn.cc \
../src/src/switch.cc \
../src/src/ledbuffer.cc \
../src/src/spritebuffer.cc \
../src/src/fluidbuffer.cc \
../src/src/linebuffer.cc \
../src/src/seesaw.cc \
../src/src/pivot.cc \
../src/src/gyroscope.cc \
../src/src/tiltmeter.cc \
../src/src/rocket.cc \
../src/src/ragdoll.cc \
../src/src/pkgman.cc \
../src/src/menu_shared.cc \
../src/src/menu-base.cc \
../src/src/menu_pkg.cc \
../src/src/menu_main.cc \
../src/src/menu_create.cc \
../src/src/menu-play.cc \
../src/src/simplebg.cc \
../src/src/world.cc \
../src/src/terrain.cc \
../src/src/rand.c \
../src/src/crc.cc \
../src/src/chunk.cc \
../src/src/chunk_window.cc \
../src/src/chunk_preloader.cc \
../src/src/gentype.cc \
../src/src/noise.cc \
../src/src/worker.cc \
../src/src/pipeline.cc \
../src/src/splank.cc \
../src/src/clip.cc \
../src/src/gearbox.cc \
../src/src/game-gearbox-edit.cc \
../src/src/ctrlmini.cc \
../src/src/ctrlservo.cc \
../src/src/battery.cc \
../src/src/ctrlfplus.cc \
../src/src/damper.cc \
../src/src/gameman.cc \
../src/src/ctrlpass.cc \
../src/src/box.cc \
../src/src/pixel.cc \
../src/src/tpixel.cc \
../src/src/basepixel.cc \
../src/src/ui.cc \
../src/src/rubberband.cc \
../src/src/loading_screen.cc \
../src/src/corner.cc \
../src/src/scanner.cc \
../src/src/proximitysensor.cc \
../src/src/ysplitter.cc \
../src/src/objectfield.cc \
../src/src/dragfield.cc \
../src/src/emitter.cc \
../src/src/explosive.cc \
../src/src/absorber.cc \
../src/src/conveyor.cc \
../src/src/resistor.cc \
../src/src/angulardamper.cc \
../src/src/impact_sensor.cc \
../src/src/decorations.cc \
../src/src/gravityman.cc \
../src/src/jumper.cc \
../src/src/spikes.cc \
../src/src/screenshot_marker.cc \
../src/src/receiver.cc \
../src/src/transmitter.cc \
../src/src/fan.cc \
../src/src/timer.cc \
../src/src/mini_transmitter.cc \
../src/src/robot_base.cc \
../src/src/spikebot.cc \
../src/src/companion.cc \
../src/src/bomber.cc \
../src/src/minibot.cc \
../src/src/robot_parts.cc \
../src/src/lobber.cc \
../src/src/angularvelmeter.cc \
../src/src/display.cc \
../src/src/velmeter.cc \
../src/src/weight.cc \
../src/src/prompt.cc \
../src/src/egraph.cc \
../src/src/i2o0gate.cc \
../src/src/speaker.cc \
../src/src/escript.cc \
../src/src/item.cc \
../src/src/crane.cc \
../src/src/robotman.cc \
../src/src/scup.cc \
../src/src/faction.cc \
../src/src/mood.cc \
../src/src/repair_station.cc \
../src/src/anchor.cc \
../src/src/ladder.cc \
../src/src/plant.cc \
../src/src/resource.cc \
../src/src/vendor.cc \
../src/src/creature.cc \
../src/src/animal.cc \
../src/src/soundman.cc \
../src/src/polygon.cc \
../src/src/textbuffer.cc \
../src/src/grid.cc \
../src/src/wplug.cc \
../src/src/cursorfield.cc \
../src/src/i1o0gate.cc \
../src/src/iomiscgate.cc \
../src/src/widget_manager.cc \
../src/src/font.cc \
../src/src/text.cc \
../src/src/pscreen.cc \
../src/src/misc.cc \
../src/src/levelman.cc \
../src/src/tiles.cc \
../src/src/activator.cc \
../src/src/treasure_chest.cc \
\
../src/src/Box2D/Particle/b2Particle.cc \
../src/src/Box2D/Particle/b2ParticleGroup.cc \
../src/src/Box2D/Particle/b2ParticleSystem.cc \
../src/src/Box2D/Particle/b2VoronoiDiagram.cc \
../src/src/Box2D/Dynamics/b2Body.cc \
../src/src/Box2D/Dynamics/b2ContactManager.cc \
../src/src/Box2D/Dynamics/b2Fixture.cc \
../src/src/Box2D/Dynamics/b2Island.cc \
../src/src/Box2D/Dynamics/b2WorldCallbacks.cc \
../src/src/Box2D/Dynamics/b2World.cc \
../src/src/Box2D/Collision/b2BroadPhase.cc \
../src/src/Box2D/Collision/b2CollideCircle.cc \
../src/src/Box2D/Collision/b2CollideEdge.cc \
../src/src/Box2D/Collision/b2CollidePolygon.cc \
../src/src/Box2D/Collision/b2Collision.cc \
../src/src/Box2D/Collision/b2Distance.cc \
../src/src/Box2D/Collision/b2DynamicTree.cc \
../src/src/Box2D/Collision/b2TimeOfImpact.cc \
../src/src/Box2D/Collision/Shapes/b2ChainShape.cc \
../src/src/Box2D/Collision/Shapes/b2CircleShape.cc \
../src/src/Box2D/Collision/Shapes/b2EdgeShape.cc \
../src/src/Box2D/Collision/Shapes/b2PolygonShape.cc \
../src/src/Box2D/Common/b2BlockAllocator.cc \
../src/src/Box2D/Common/b2Draw.cc \
../src/src/Box2D/Common/b2Math.cc \
../src/src/Box2D/Common/b2Settings.cc \
../src/src/Box2D/Common/b2StackAllocator.cc \
../src/src/Box2D/Common/b2Timer.cc \
../src/src/Box2D/Dynamics/Contacts/b2ChainAndCircleContact.cc \
../src/src/Box2D/Dynamics/Contacts/b2ChainAndPolygonContact.cc \
../src/src/Box2D/Dynamics/Contacts/b2CircleContact.cc \
../src/src/Box2D/Dynamics/Contacts/b2Contact.cc \
../src/src/Box2D/Dynamics/Contacts/b2ContactSolver.cc \
../src/src/Box2D/Dynamics/Contacts/b2EdgeAndCircleContact.cc \
../src/src/Box2D/Dynamics/Contacts/b2EdgeAndPolygonContact.cc \
../src/src/Box2D/Dynamics/Contacts/b2PolygonAndCircleContact.cc \
../src/src/Box2D/Dynamics/Contacts/b2PolygonContact.cc \
../src/src/Box2D/Dynamics/Joints/b2DistanceJoint.cc \
../src/src/Box2D/Dynamics/Joints/b2FrictionJoint.cc \
../src/src/Box2D/Dynamics/Joints/b2GearJoint.cc \
../src/src/Box2D/Dynamics/Joints/b2Joint.cc \
../src/src/Box2D/Dynamics/Joints/b2MotorJoint.cc \
../src/src/Box2D/Dynamics/Joints/b2MouseJoint.cc \
../src/src/Box2D/Dynamics/Joints/b2PrismaticJoint.cc \
../src/src/Box2D/Dynamics/Joints/b2PulleyJoint.cc \
../src/src/Box2D/Dynamics/Joints/b2RevoluteJoint.cc \
../src/src/Box2D/Dynamics/Joints/b2PivotJoint.cc \
../src/src/Box2D/Dynamics/Joints/b2RopeJoint.cc \
../src/src/Box2D/Dynamics/Joints/b2WeldJoint.cc \
../src/src/Box2D/Dynamics/Joints/b2WheelJoint.cc

.rc.o:
	windres $^ -o $@
%.o : %.rc
	windres $^ -o $@
