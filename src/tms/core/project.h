#ifndef _TMS_PROJECT__H_
#define _TMS_PROJECT__H_

/**
 * declaractions for project-side functions,
 * required to be implemented per-project
 **/

#ifdef __cplusplus
extern "C" {
#endif

void tproject_init(void);
void *tproject_initialize(void);
void tproject_init_pipelines(void);
void tproject_set_args(int argc, char **argv);
void tproject_window_size_changed(void);

void tproject_soft_resume();
void tproject_soft_pause();
void tproject_quit();

void tproject_step();
#ifdef __cplusplus
}
#endif

#endif
