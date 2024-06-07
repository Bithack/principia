// Linux screenshot build backend

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <libgen.h>

#include <tms/core/project.h>
#include <tms/core/event.h>
#include <tms/core/tms.h>

#include <tms/backend/opengl.h>

#include "settings.hh"
#include "game.hh"
#include "main.hh"
#include "pkgman.hh"
#include "screenshot_marker.hh"

#include <png.h>

#include "tms/bindings/cpp/cpp.hh"

#define STEP_QUIT       -1

#define STEP_WAIT        0
#define STEP_SNAP        1
#define STEP_SCREENSHOT  2
#define STEP_IDLE        3

#define STATE_IDLE    0
#define STATE_WORKING 1

FILE *_f_out = stdout;

SDL_Window *_window;

int keys[235];
int mouse_down;
static char *_storage_path = 0;
static int pipe_h;

static char *_args[2] = {0,0};
static char buf[1024];
static int  step = STEP_IDLE;
static bool first = true;
static int  ss_id = 0;
static FILE *state_fh = NULL;

static uint32_t snap_step_num = 0;

int screenshot(char *file_name, unsigned int x, unsigned int y, unsigned long width, unsigned long height);
extern "C" int tbackend_init_surface();
extern "C" const char *tbackend_get_storage_path(void);


void
set_state(int state)
{
    state_fh = fopen("principia.state", "w");
    tms_infof("Setting state to %d", state);
    fprintf(state_fh, "%d", state);
    fclose(state_fh);
}

int _pipe_listener(void *p)
{
    ssize_t sz;

    while (1) {
        tms_infof("attempting to open principia.run O_RDONLY");
        while ((pipe_h = open("/tmp/principia.run", O_RDONLY)) == -1) {
            if (errno != EINTR)
                return 1;
        }

        while ((sz = read(pipe_h, buf, 1023)) > 0) {
            tms_infof("read %li bytes", sz);
            step = STEP_WAIT;
            first = true;
            ss_id = 0;

            if (sz > 0) {
                buf[sz] = '\0';
                _args[1] = buf;
                tproject_set_args(2, _args);
            }
        }

        close(pipe_h);
    }

    tms_infof("Pipe listener EXITING");
}

int
main(int argc, char **argv)
{
    SDL_Event  ev;
    int        done = 0;

    int status = mkfifo("/tmp/principia.run", S_IWUSR | S_IRUSR);
    int skip_pipe = 0;

    if (status == 0) {
        tms_infof("Created fifo");
    } else {
        if (errno != EEXIST) {
            tms_errorf("could not create fifo pipe!");
            skip_pipe = 1;
        }
    }

    if (!skip_pipe) {
        if ((pipe_h = open("/tmp/principia.run", O_WRONLY | O_NONBLOCK)) == -1) {
            if (errno != ENXIO) {
                skip_pipe = 1;
                tms_infof("error: %s", strerror(errno));
            }
        } else {
            if (argc > 1) {
                /* open the fifo for writing instead */
                tms_infof("sending arg: %s", argv[1]);
                set_state(STATE_WORKING);
                write(pipe_h, argv[1], strlen(argv[1]));
            } else {
                tms_infof("principia already running");
            }

            close(pipe_h);
            exit(0);
        }
    }

    if (!skip_pipe) {
        tms_infof("Starting fifo listener thread");
        SDL_CreateThread(_pipe_listener, "_pipe_listener", 0);
    }

    char* exedir = SDL_GetBasePath();
    tms_infof("chdirring to %s", exedir);
    (void)chdir(exedir);

    mkdir(tbackend_get_storage_path(), S_IRWXU | S_IRWXG | S_IRWXO);

    tms_infof("Initializing SDL...");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_DisplayMode mode;
    SDL_GetCurrentDisplayMode(0, &mode);

    tproject_set_args(argc, argv);
    tms_init();

    if (_tms.screen == 0) {
        tms_fatalf("context has no initial screen, bailing out");
    }

    /* XXX: Check for a special signal which quits principia smoothly? i.e. after the current screenshots have been taken */

    do {
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_QUIT:
                    _tms.state = TMS_STATE_QUITTING;
                    break;
            }
        }

        switch (step) {
            case STEP_IDLE:
                if (P.loaded) {
                    /* The delay is handled in the main menu instead. */
                }
                break;

            case STEP_WAIT:
                if (!G) {
                    //tms_infof("Game is not yet initialized..");
                    break;
                }

                if (!W) {
                    //tms_infof("World is not yet initialized...");
                    break;
                }

                if (_tms.screen != &G->super) {
                    //tms_infof("Waiting for screen to be set to game...");
                    break;
                }

                if (P.num_actions > 0) {
                    //tms_infof("Waiting for main actions to complete.. %d remaining", P.num_actions);
                    break;
                }

                if (W->step_count < 1) {
                    tms_infof("Waiting for step count to be >= 1. Currently: %d", W->step_count);
                    break;
                }

                G->set_follow_object(0, false, false);

                if (W->level.flag_active(LVL_CHUNKED_LEVEL_LOADING)) {
                    bool all_chunks_loaded = true;

                    const std::map<chunk_pos, level_chunk*> &active_chunks = W->cwindow->preloader.get_active_chunks();

                    /* This never fails, but I'll leave it for now! */
                    for (std::map<chunk_pos, level_chunk*>::const_iterator it = active_chunks.begin();
                            it != active_chunks.end(); ++it) {
                        if (it->second->slot != -1 && it->second->load_phase != 2) {
                            tms_infof("Chunk %p load phase: %d", it->second, it->second->load_phase);
                            all_chunks_loaded = false;
                            break;
                        }
                    }

                    if (!all_chunks_loaded) {
                        tms_infof("Waiting for chunks to fully load...")
                        break;
                    }

                    tms_infof("Num loaded chunks: %d", (int)active_chunks.size());
                }

                set_state(STATE_WORKING);
                step = STEP_SNAP;

                break;

            case STEP_SNAP: {
                P.focused = true;

                tms_infof("STEP_SNAP");
                size_t sz = W->cam_markers.size();
                if (sz > 0) {
                    if (first) {
                        for (G->cam_iterator = W->cam_markers.begin(); G->cam_iterator != W->cam_markers.end(); G->cam_iterator++) {
                            G->cam_iterator->second->hide();
                        }
                    }

                    if (G->cam_iterator == W->cam_markers.end()) {
                        if (first)
                            G->cam_iterator = W->cam_markers.begin();
                        else {
                            step = STEP_QUIT;
                        }
                    } else {
                        if (ss_id == 15) {
                            tms_infof("Aborting, only 15 cam markers allowed.");
                            step = STEP_QUIT;
                        } else {
                            tms_infof("Snapping to camera %p...", G->cam_iterator->second);
                            G->snap_to_camera(G->cam_iterator->second);
                            G->cam_iterator->second->hide();
                            G->cam_iterator++;

                            snap_step_num = G->state.step_num;
                            step = STEP_SCREENSHOT;
                        }
                    }
                } else {
                    /* snap to "saved position" */
                    if (first) {
                        tms_infof("Snapping to saved pos... (%.2f/%.2f %.2f)",
                                W->level.sandbox_cam_x,
                                W->level.sandbox_cam_y,
                                W->level.sandbox_cam_zoom
                                );
                        G->cam->_position.x = W->level.sandbox_cam_x;
                        G->cam->_position.y = W->level.sandbox_cam_y;
                        G->cam->_position.z = W->level.sandbox_cam_zoom;

                        snap_step_num = G->state.step_num;
                        step = STEP_SCREENSHOT;
                    } else {
                        step = STEP_QUIT;
                    }
                }
                first = false;
            } break;

            case STEP_QUIT:
                tms_infof("We are returning to main menu!");
                remove("principia.state");
                step = STEP_IDLE;
                P.focused = false;
                P.add_action(ACTION_GOTO_MAINMENU, 0);
                break;
        }

        tms_step();
        tms_begin_frame();
        tms_render();

        if (step == STEP_SCREENSHOT) {
            if (W->level.flag_active(LVL_CHUNKED_LEVEL_LOADING)) {
                int32_t step_diff = G->state.step_num - snap_step_num;
                if (step_diff < 1) {
                    tms_infof("Step diff is %d, waiting...", step_diff);
                    break;
                }
                bool all_chunks_loaded = true;

                const std::map<chunk_pos, level_chunk*> &active_chunks = W->cwindow->preloader.get_active_chunks();
                /* This never fails, but I'll leave it for now! */
                for (std::map<chunk_pos, level_chunk*>::const_iterator it = active_chunks.begin();
                        it != active_chunks.end(); ++it) {
                    if (it->second->slot != -1 && it->second->load_phase != 2) {
                        tms_infof("Chunk %p load phase: %d", it->second, it->second->load_phase);
                        all_chunks_loaded = false;
                        break;
                    }
                }

                if (!all_chunks_loaded) {
                    tms_infof("Waiting for chunks to fully load...");
                    break;
                }
            }

            char filename[256];
            snprintf(filename, 256, "ss-%d.png", ss_id++);

            tms_infof("Saving screenshot to %s...", filename);

            int r = screenshot(filename, 0, 0, 1280, 720);
            if (r != 0) {
                tms_infof("ERROR!");
            }
            step = STEP_SNAP;
        }

        SDL_GL_SwapWindow(_window);
        tms_end_frame();

        SDL_Delay(1);

    } while (_tms.state != TMS_STATE_QUITTING);

    remove("principia.state");

    return 0;
}

int
tbackend_init_surface()
{
    _tms.window_width = 1280;
    _tms.window_height = 720;

    _tms.xppcm = 108.f/2.54f * 1.5f;
    _tms.yppcm = 107.f/2.54f * 1.5f;

    uint32_t flags = 0;

    flags |= SDL_WINDOW_OPENGL;
    flags |= SDL_WINDOW_SHOWN;

    tms_infof("Creating window...");
    _window = SDL_CreateWindow("Principia Screenshotter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _tms.window_width, _tms.window_height, flags);

    if (_window == NULL) {
        tms_infof("ERROR: %s", SDL_GetError());
        exit(1);
    }

    _tms._window = _window;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

    SDL_GL_CreateContext(_window);

    SDL_GL_SetSwapInterval(0);

    return T_OK;
}

const char *tbackend_get_storage_path(void)
{
    return "storage";
}

int
screenshot(char *path, unsigned int x, unsigned int y, unsigned long width, unsigned long height)
{
    FILE *fp;
    unsigned long i;
    png_structp png_ptr;
    png_infop info_ptr;
    png_colorp palette;
    png_byte *image;
    png_bytep *row_pointers;

    fp = fopen(path, "wb");
    if (fp == NULL)
        return -1;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (png_ptr == NULL)
    {
        fclose(fp);
        return -1;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
        fclose(fp);
        png_destroy_write_struct(&png_ptr, NULL);
        return -1;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        fclose(fp);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return -1;
    }

    png_init_io(png_ptr, fp);

    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    palette = (png_colorp)png_malloc(png_ptr, PNG_MAX_PALETTE_LENGTH * sizeof (png_color));
    png_set_PLTE(png_ptr, info_ptr, palette, PNG_MAX_PALETTE_LENGTH);

    png_write_info(png_ptr, info_ptr);

    png_set_packing(png_ptr);

    image = (png_byte *)malloc(width * height * 3 * sizeof(png_byte));
    if(image == NULL)
    {
        fclose(fp);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return -1;
    }

    row_pointers = (png_bytep *)malloc(height * sizeof(png_bytep));
    if(row_pointers == NULL)
    {
        fclose(fp);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        free(image);
        image = NULL;
        return -1;
    }

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid *)image);

    for (i = 0; i < height; i++)
    {
        row_pointers[i] = (png_bytep)image + (height - i - 1) * width * 3;
    }

    png_write_image(png_ptr, row_pointers);

    png_write_end(png_ptr, info_ptr);

    png_free(png_ptr, palette);
    palette = NULL;

    png_destroy_write_struct(&png_ptr, &info_ptr);

    free(row_pointers);
    row_pointers = NULL;

    free(image);
    image = NULL;

    fclose(fp);

    return 0;
}
